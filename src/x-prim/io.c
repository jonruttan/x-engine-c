/** @file io.c
 *  @brief I/O primitives: read, write, display, string conversion, heap/GC, system, REPL.
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2026 Jon Ruttan
 *  @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
/*
 * # Includes
 */
#include "x-prim.h"
#include "x-eval.h"
#include "x-eval.h"
#include "x-heap.h"
#include "x-token.h"
#include "x-type.h"
#include "x-type/buffer.h"
#include "x-type/char.h"
#include "x-type/int.h"
#include "x-type/prim.h"
#include "x-type/procedure.h"
#include "x-type/str.h"
#include "x-type/symbol.h"
#include "x-obj/prim.h"

/** Write a string's bytes to the current output: the OUT port instruction.
 *  x-lang: (write-str s)
 *  @param p_base  Base (execution context).
 *  @param p_args  Unevaluated argument list (s).
 *  @return NULL.
 *  @note The byte door the pure-X printer (lib/x/boot/printer.x) bottoms
 *        out at.  Emits through x_eval_write_str, so it respects the
 *        write-buffer capture stack exactly as the C renderers did.
 */
static x_obj_t *x_prim_write_str(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_s;
	x_satom_t data = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .s = NULL }),
		sz = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .i = 0 });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { data }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { sz }, { NULL })
	};

	x_eargs(p_base, p_args, 2, NULL, &p_s);
	x_atomstr(data) = x_strval(p_s);
	x_atomint(sz) = (x_int_t)x_lib_strlen(x_strval(p_s));
	x_eval_write_str(p_base, (x_obj_t *)args);

	return NULL;
}

/** Read one s-expression from stdin.
 *  x-lang: (read)
 *  @param p_base  Base (execution context).
 *  @param p_args  Unused.
 *  @return Parsed s-expression, or NULL on EOF.
 *  @see x_prim_read_char
 */
static x_obj_t *x_prim_read_expr(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_firstobj(x_base_field_buffer(p_base));
	x_spair_t read_args[1];
	(void)p_args;
	read_args[0][X_OBJ_META_TYPE].p = NULL;
	read_args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)read_args) = p_buffer;
	x_restobj((x_obj_t *)read_args) = p_base;

	return x_token_read(p_base, (x_obj_t *)read_args);
}

/** Read one character from stdin.
 *  x-lang: (read-char)
 *  @param p_base  Base (execution context).
 *  @param p_args  Unused.
 *  @return Character object, or NULL on EOF.
 *  @see x_prim_read_expr
 */
static x_obj_t *x_prim_read_char(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_firstobj(x_base_field_buffer(p_base));
	x_spair_t buf_args[1];
	(void)p_args;
	buf_args[0][X_OBJ_META_TYPE].p = NULL;
	buf_args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)buf_args) = p_buffer;
	x_restobj((x_obj_t *)buf_args) = p_base;

	p_buffer = x_type_buffer_read(p_base, (x_obj_t *)buf_args);

	if (x_obj_isnil(p_base, p_buffer)) {
		return NULL;
	}

	/* Byte-level read: a CHARACTER holding the raw byte (0-255). */
	return x_mkchar(p_base, (unsigned char)x_bufferlastchar(p_buffer));
}

#ifdef X_SYS_CLOCK
/** Return CPU microseconds since process start.
 *  x-lang: (clock)
 *  @param p_base  Base (execution context).
 *  @param p_args  Unused.
 *  @return Integer with microseconds elapsed.
 *  @note Only available when X_SYS_CLOCK is defined.
 */
static x_obj_t *x_prim_clock(x_obj_t *p_base, x_obj_t *p_args)
{
	(void)p_args;
	return x_mkint(p_base, x_sys_clock());
}
#endif /* X_SYS_CLOCK */

/** Mark phase: trace live objects from every root (GC phase 1).
 *
 *  Four passes: (1) fire mark hooks, (2) tree-mark from the base data
 *  tree, (3) root-chain walk -- the off-chain stack objects frames
 *  registered via x_heap_root_push, (4) tree-mark each registered GC
 *  root.  The hook/root lists live in x-expr's heap-group; x-expr
 *  stores but cannot dispatch (no callable mechanism at that layer), so
 *  the walk + invoke happens here.
 *
 *  Hooks MUST fire before the marking passes: everything a hook
 *  allocates is born unmarked, and the sweep that follows this phase
 *  frees every unmarked object.  Firing hooks first means a hook
 *  allocation that escaped into reachable state -- a (heap-mark-root!)
 *  spine cell, a set! into a global -- is marked by the later passes
 *  and survives, while the hook's transient garbage is correctly
 *  swept.  It also makes a root registered mid-collect count in the
 *  same cycle (pass 4 runs after the hooks push it).  With hooks last,
 *  the sweep freed the escaped cells and left reachable dangling
 *  pointers -- a use-after-free on the next collect (usually silent:
 *  the chunk is recycled and the mark walk traverses a reinterpreted
 *  object; ASan catches it only when the chunk stays unreused).
 *
 *  @note A sweep must run with no allocation between it and this mark:
 *        a transient cell allocated *after* the mark is unmarked, so an
 *        intervening sweep frees it while the evaluator is still
 *        traversing it (see x_prim_heap_collect).
 *  @see x_heap_sweep_phase
 */
static void x_heap_mark_phase(x_obj_t *p_base)
{
	x_obj_t *p_roots;
	x_obj_t *p_hooks;
	x_spair_t hook_args[1];

	if (x_base_isset(p_base)) {
		p_hooks = x_firstobj(x_base_field_heap_mark_hooks(p_base));

		hook_args[0][X_OBJ_META_TYPE].p = NULL;
		hook_args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;

		while ( ! x_obj_isnil(p_base, p_hooks)) {
			x_firstobj((x_obj_t *)hook_args) = x_firstobj(p_hooks);
			x_restobj((x_obj_t *)hook_args) = NULL;
			/* Drain any deferred tail call: a procedure hook that
			 * returns a non-nil tail leaves the env extended and
			 * tco_expr/tco_env set, expecting an outer trampoline.
			 * There is none here, so run it -- otherwise the env
			 * frame the hook left live is freed by the sweep and
			 * the next eval dereferences it. */
			x_eval_tco_trampoline(p_base,
				x_obj_prim_call(p_base, (x_obj_t *)hook_args));
			p_hooks = x_restobj(p_hooks);
		}
	}

	x_heap_tree_mark(p_base, x_atomobj(p_base), X_OBJ_FLAG_MARK);
	x_heap_root_chain_mark(p_base, X_OBJ_FLAG_MARK);

	if (x_base_isset(p_base)) {
		p_roots = x_firstobj(x_base_field_heap_mark_roots(p_base));

		while ( ! x_obj_isnil(p_base, p_roots)) {
			x_heap_tree_mark(p_base, x_firstobj(p_roots),
				X_OBJ_FLAG_MARK);
			p_roots = x_restobj(p_roots);
		}
	}
}

/** Sweep phase: fire free hooks, then reclaim unmarked objects (GC phase
 *  2).  x_heap_sweep also clears the mark flag on retained objects, readying
 *  them for the next cycle.
 *  @see x_heap_mark_phase
 */
static void x_heap_sweep_phase(x_obj_t *p_base)
{
	x_obj_t *p_hooks;
	x_spair_t hook_args[1];

	if (x_base_isset(p_base)) {
		p_hooks = x_firstobj(x_base_field_heap_free_hooks(p_base));

		hook_args[0][X_OBJ_META_TYPE].p = NULL;
		hook_args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;

		while ( ! x_obj_isnil(p_base, p_hooks)) {
			x_firstobj((x_obj_t *)hook_args) = x_firstobj(p_hooks);
			x_restobj((x_obj_t *)hook_args) = NULL;
			/* Drain deferred tail call (see x_heap_mark_phase). */
			x_eval_tco_trampoline(p_base,
				x_obj_prim_call(p_base, (x_obj_t *)hook_args));
			p_hooks = x_restobj(p_hooks);
		}
	}

	x_heap_sweep(p_base, x_obj_heap(p_base), X_OBJ_FLAG_MARK);
}

/** Sweep unmarked objects from the heap (GC phase 2, low-level).
 *  x-lang: (heap-sweep)
 *
 *  LOW-LEVEL / UNSAFE on its own.  A sweep frees every object not marked
 *  by a *preceding* mark, so calling (heap-sweep) without an immediately
 *  preceding (heap-mark) -- or with any allocation in between -- frees
 *  live data, including the eval-list cell the evaluator is mid-traversal
 *  on.  Use (heap-collect) for a safe, atomic mark+sweep.  This primitive
 *  is exposed only for instrumentation that controls the phases manually
 *  with no intervening allocation.
 *
 *  @param p_base  Base (execution context).
 *  @param p_args  Unused.
 *  @return NULL.
 *  @see x_prim_heap_collect, x_prim_heap_mark, x_prim_heap_count
 */
static x_obj_t *x_prim_heap_sweep(x_obj_t *p_base, x_obj_t *p_args)
{
	(void)p_args;
#ifdef X_PROFILE
	if (x_base_isset(p_base))
		x_atomint(x_firstobj(x_eval_field_profile_gc_runs(p_base)))++;
#endif

	x_heap_sweep_phase(p_base);

	return NULL;
}

/** Count the number of objects currently on the heap.
 *  x-lang: (heap-count)
 *  @param p_base  Base (execution context).
 *  @param p_args  Unused.
 *  @return Integer with the heap object count.
 *  @see x_prim_heap_collect, x_prim_heap_mark, x_prim_heap_sweep
 */
static x_obj_t *x_prim_heap_count(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p = x_obj_heap(p_base);
	long count = 0;
	(void)p_args;

	while (p) {
		count++;
		p = x_obj_heap(p);
	}

	return x_mkint(p_base, count);
}

/** Set the allocation ceiling (the runaway-memory guard).
 *  x-lang: (alloc-limit! n)  -- n > 0 arms the guard; 0 disables (the
 *  default; negative input is treated as 0).
 *
 *  When armed, x_obj_alloc reports an error through the standard error path
 *  and stops the process rather than allocate past n objects; once tripped
 *  the limit latches, so an intercepting guard handler cannot spin it (see
 *  x_obj_alloc).  The trip-message text is stored here at arm time --
 *  x-expr's mechanism layer holds no prose.  Configuration is in-language
 *  (the interpreter reads no environment): the spec runner feeds
 *  (alloc-limit! n) ahead of each library load, so a runaway ./x stops
 *  itself instead of exhausting system memory.  A development guard against
 *  runaway allocation, not a sandbox -- code can re-set or disable it.
 *
 *  @param p_base  Base (execution context).
 *  @param p_args  Unevaluated (n); x_eargs evaluates it.
 *  @return NULL.
 *  @note Fexpr: args unevaluated; x_eargs evaluates n.
 */
static x_obj_t *x_prim_alloc_limit(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_val;
	x_int_t n;

	x_eargs(p_base, p_args, 2, NULL, &p_val);
	n = x_intval(p_val);
	/* Negative cell values are reserved for the allocator's tripped latch. */
	x_atomint(x_firstobj(x_base_field_alloc_limit(p_base))) = n < 0 ? 0 : n;
	/* The trip message lives with the policy, not the mechanism: x-expr
	 * holds no prose, so arming supplies what the allocator reports. */
	x_firstobj(x_base_field_alloc_error(p_base)) =
		x_mkstr(p_base, (x_char_t *)"allocation limit exceeded");

	return NULL;
}

/** Mark all reachable objects on the heap (GC phase 1, low-level).
 *  x-lang: (heap-mark)
 *
 *  LOW-LEVEL.  Marking alone is harmless (it frees nothing), but a mark
 *  is only useful paired with a sweep that runs with no allocation in
 *  between.  Use (heap-collect) for a safe, atomic mark+sweep.
 *
 *  @param p_base  Base (execution context).
 *  @param p_args  Unused.
 *  @return NULL.
 *  @see x_prim_heap_collect, x_prim_heap_sweep, x_prim_heap_count
 */
static x_obj_t *x_prim_heap_mark(x_obj_t *p_base, x_obj_t *p_args)
{
	(void)p_args;
	x_heap_mark_phase(p_base);

	return NULL;
}

/** Run a full, atomic garbage collection cycle (mark then sweep).
 *  x-lang: (heap-collect)
 *
 *  This is the safe GC entry point.  Mark and sweep run back-to-back in
 *  one C call with no x-lang-level evaluation -- and therefore no
 *  allocation -- between them.  That matters because eval-list scratch
 *  cells (and the env/ctrl/extras half of the base tree) are allocated
 *  X_OBJ_FLAG_NONE and survive a sweep only by being marked.  The mark
 *  phase here runs while the (heap-collect) call's own eval-list frame is
 *  live, so that frame is marked and survives the immediately following
 *  sweep.  Splitting mark and sweep into two separate evaluations (e.g.
 *  (begin (heap-mark) (heap-sweep))) reintroduces an allocation between
 *  them and frees the in-flight frame -- hence (heap-collect), not the
 *  raw phases, is the supported API.
 *
 *  @param p_base  Base (execution context).
 *  @param p_args  Unused.
 *  @return NULL.
 *  @note Increments the GC run counter when X_PROFILE is defined.
 *  @see x_prim_heap_mark, x_prim_heap_sweep, x_prim_heap_count
 */
static x_obj_t *x_prim_heap_collect(x_obj_t *p_base, x_obj_t *p_args)
{
	(void)p_args;
#ifdef X_PROFILE
	if (x_base_isset(p_base))
		x_atomint(x_firstobj(x_eval_field_profile_gc_runs(p_base)))++;
#endif

	x_heap_mark_phase(p_base);
	x_heap_sweep_phase(p_base);

	return NULL;
}

/** Recursively mark an object and all reachable objects as SYSTEM (GC-immune).
 *  x-lang: (gc-pin! obj)
 *  @param p_base  Base (execution context).
 *  @param p_args  Unevaluated argument list (obj).
 *  @return The marked object.
 *  @note Fexpr: args unevaluated; x_eargs evaluates obj.
 *  @note Uses X_OBJ_FLAG_SHARED to make objects immune to GC sweep.
 *  @see x_prim_heap_mark, x_prim_heap_sweep
 */
static x_obj_t *x_prim_system_mark(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj;
	x_eargs(p_base, p_args, 2, NULL, &p_obj);

	/* Reuse the mark traversal with SYSTEM flag */
	x_heap_tree_mark(p_base, p_obj, X_OBJ_FLAG_SHARED);

	return p_obj;
}

/** Register a callable to run during the GC mark phase.
 *  x-lang: (heap-mark-hook! hook)
 *  @param p_base  Base (execution context).
 *  @param p_args  Unevaluated (hook).
 *  @return NULL.
 *  @note Storage in x-expr's heap-group (one canonical location).
 *  @see x_heap_mark_hook_add
 */
static x_obj_t *x_prim_heap_mark_hook(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_hook;
	x_eargs(p_base, p_args, 2, NULL, &p_hook);
	x_heap_mark_hook_add(p_base, p_hook);
	return NULL;
}

/** Register a callable to run during the GC sweep phase.
 *  x-lang: (heap-free-hook! hook)
 *  @param p_base  Base (execution context).
 *  @param p_args  Unevaluated (hook).
 *  @return NULL.
 *  @see x_heap_free_hook_add
 */
static x_obj_t *x_prim_heap_free_hook(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_hook;
	x_eargs(p_base, p_args, 2, NULL, &p_hook);
	x_heap_free_hook_add(p_base, p_hook);
	return NULL;
}

/** Register an object to mark on every collection (extra GC root).
 *  x-lang: (heap-mark-root! obj)
 *  @param p_base  Base (execution context).
 *  @param p_args  Unevaluated (obj).
 *  @return NULL.
 *  @see x_heap_mark_root_add
 */
static x_obj_t *x_prim_heap_mark_root(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj;
	x_eargs(p_base, p_args, 2, NULL, &p_obj);
	x_heap_mark_root_add(p_base, p_obj);
	return NULL;
}

/** Minimal read-eval loop: reads and evaluates expressions until EOF.
 *  @param p_base  Base (execution context).
 *  @param p_args  Unused.
 *  @return NULL on EOF.
 *  @note No output, no prompt, no hooks. Used for C-level bootstrapping;
 *        the x-lang REPL operative in x-core.x provides the full experience.
 *  @note Clears shadows after each evaluation.
 */
x_obj_t *x_prim_repl(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_exp;
	x_obj_t **p_cell = x_heap_root_slot(p_base);
	x_spair_t read_state[1];
	x_spair_t root = x_obj_set((x_obj_t *)x_type_pair_obj, X_OBJ_FLAG_NONE,
		{ NULL }, { NULL });
	read_state[0][X_OBJ_META_TYPE].p = NULL;
	read_state[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)read_state) = NULL;
	x_restobj((x_obj_t *)read_state) = NULL;

	x_heap_root_push(p_cell, root);

	for (;;) {
		p_exp = x_prim_read_expr(p_base, (x_obj_t *)read_state);
		if (x_obj_isnil(p_base, p_exp))
			break;
		/* The freshly read form is this frame's only reference. */
		x_firstobj((x_obj_t *)root) = p_exp;
		x_eval_arg(p_base, p_exp);
		x_prim_clear_shadows(p_base);
	}

	x_heap_root_pop(p_cell);

	return NULL;
}

/**
 * Read one expression, after resetting the source-line counter to 0.
 *
 * The REPL uses this instead of plain read so that forms typed at the prompt
 * are tagged with lines relative to THIS input rather than the cumulative
 * boot+session stream.  The reset must happen here, inside the read primitive:
 * eval updates the line counter from each form's source-line metadata
 * (x-eval.c) when it evaluates the (repl-read) call, so an earlier reset would
 * be clobbered before the read runs.  (include pushes its own counter, so file
 * lines are unaffected.)
 *
 * x-lang form: @code (repl-read) @endcode
 *
 * @param p_base  Base (execution context).
 * @param p_args  Passed through to read.
 * @return The expression read, or nil at end of input.
 */
static x_obj_t *x_prim_repl_read(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_firstobj(x_base_field_buffer(p_base));

	/* The tokenizer tags each form's source line from the buffer's line
	 * metadata (x-token.c), which eval then copies into the counter that
	 * error-line reports.  Reset it to 0 so forms typed here are numbered
	 * relative to this input, not the cumulative boot+session stream. */
	if (p_buffer != NULL && (x_obj_flags(p_buffer) & X_OBJ_FLAG_META)) {
		x_obj_meta_i(p_buffer, 0).i = 0;
	}

	return x_prim_read_expr(p_base, p_args);
}

/** Register I/O primitives into the environment.
 *
 *  Binds: write-str, read, read-char, heap-mark, heap-sweep, heap-count,
 *  gc-pin!, repl-read.  Conditionally binds clock (when X_SYS_CLOCK defined).
 *  (The printers -- write, display, write-to-str, display-to-str -- and
 *  error-line are pure x-lang now: boot/printer.x renders over the
 *  (io write-str) OUT door; boot/reflect.x walks the error handler.)
 *
 *  @param p_base  Base (execution context).
 *  @param p_args  Unused.
 *  @return The base object.
 */
x_obj_t *x_prim_io_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_prim_entry_t entries[] = {
		{ "write-str",       x_prim_write_str,         "io",   "write-str"      },
		{ "read",            x_prim_read_expr,         "io",   "read"           },
		{ "read-char",       x_prim_read_char,         "io",   "read-char"      },
		{ "heap-collect",    x_prim_heap_collect,      "heap", "collect"        },
		{ "heap-mark",       x_prim_heap_mark,         "heap", "mark"           },
		{ "heap-sweep",      x_prim_heap_sweep,        "heap", "sweep"          },
		{ "heap-count",      x_prim_heap_count,        "heap", "count"          },
		{ "alloc-limit!",    x_prim_alloc_limit,       "alloc", "limit!"        },
		{ "heap-mark-hook!", x_prim_heap_mark_hook,    "heap", "mark-hook!"     },
		{ "heap-free-hook!", x_prim_heap_free_hook,    "heap", "free-hook!"     },
		{ "heap-mark-root!", x_prim_heap_mark_root,    "heap", "mark-root!"     },
		{ "gc-pin!",         x_prim_system_mark,       "heap", "pin!"           },
		{ "repl-read",       x_prim_repl_read,         "io",   "repl-read"      }
	};
#ifdef X_SYS_CLOCK
	static const x_prim_entry_t clock_entry[] = {
		{ "clock", x_prim_clock, "sys", "clock" }
	};
#endif /* X_SYS_CLOCK */

	x_prims_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));
#ifdef X_SYS_CLOCK
	x_prims_bind_table(p_base, clock_entry,
		sizeof(clock_entry) / sizeof(clock_entry[0]));
#endif /* X_SYS_CLOCK */

	return p_base;
}
