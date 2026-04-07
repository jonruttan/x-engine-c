/** @file io.c
 *  @brief I/O primitives: read, write, display, string conversion, heap/GC, system, REPL.
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2024 Jon Ruttan
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
#include "x-base-typesystem.h"
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

/** Output an object as an s-expression to stdout.
 *  x-lang: (write obj)
 *  @param p_base  Execution context.
 *  @param p_args  Unevaluated argument list (obj).
 *  @return NULL.
 *  @note Fexpr: args unevaluated; x_eargs evaluates obj.
 *  @see x_prim_display, x_prim_write_to_string
 */
static x_obj_t *x_prim_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_val;
	x_spair_t write_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL })
	};

	x_eargs(p_base, p_args, 2, NULL, &p_val);
	x_firstobj((x_obj_t *)write_args) = p_val;
	x_token_write(p_base, (x_obj_t *)write_args);

	return NULL;
}

/** Output an object in human-readable form via the type system.
 *  x-lang: (display obj)
 *  @param p_base  Execution context.
 *  @param p_args  Unevaluated argument list (obj).
 *  @return NULL.
 *  @note Fexpr: args unevaluated; x_eargs evaluates obj.
 *  @note Unlike write, display omits quoting (e.g. strings without quotes).
 *  @see x_prim_write, x_prim_display_to_string
 */
static x_obj_t *x_prim_display(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_val;
	x_spair_t display_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL })
	};

	x_eargs(p_base, p_args, 2, NULL, &p_val);
	x_firstobj((x_obj_t *)display_args) = p_val;
	x_token_display(p_base, (x_obj_t *)display_args);

	return NULL;
}

/** Read one s-expression from stdin.
 *  x-lang: (read)
 *  @param p_base  Execution context.
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
 *  @param p_base  Execution context.
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

	return x_mkchar(p_base, x_bufferlastchar(p_buffer));
}

/** Capture output of a dispatch function (write or display) into a string.
 *  @param p_base    Execution context.
 *  @param p_args    Unevaluated argument list (obj).
 *  @param dispatch  Output function to redirect (x_token_write or x_token_display).
 *  @return New string containing the captured output.
 *  @note Pushes a temporary write buffer onto write_buf_stack, invokes
 *        dispatch, then pops and converts the buffer to a string.
 *  @note Returns "()" for nil values.
 */
static x_obj_t *x_prim_to_string(x_obj_t *p_base, x_obj_t *p_args,
	x_obj_t *(*dispatch)(x_obj_t *, x_obj_t *))
{
	x_obj_t *p_val, *p_result;
	x_char_t *buf;
	x_satom_t buf_pos = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = 0 });
	x_spair_t buf_obj[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE,
			{ .v = NULL }, { (x_obj_t *)&buf_pos })
	};
	x_spair_t dispatch_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL })
	};

	p_val = x_eval_arg(p_base, x_firstobj(p_args));

	if (x_obj_isnil(p_base, p_val)) {
		return x_mkstrown(p_base, x_lib_strndup((x_char_t *)"()", 2));
	}

	buf = (x_char_t *)x_sys_malloc(65536);
	if (buf == NULL) return NULL;
	x_first((x_obj_t *)buf_obj).v = buf;

	/* Push write-buffer onto write_buf_stack */
	x_base_field_write_buf(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		(x_obj_t *)buf_obj, x_base_field_write_buf(p_base));

	x_firstobj((x_obj_t *)dispatch_args) = p_val;
	dispatch(p_base, (x_obj_t *)dispatch_args);

	/* Pop write_buf_stack */
	x_base_field_write_buf(p_base)
		= x_restobj(x_base_field_write_buf(p_base));
	buf[x_atomint(buf_pos)] = '\0';

	p_result = x_mkstrown(p_base,
		x_lib_strndup(buf, x_atomint(buf_pos)));
	x_sys_free(buf);
	return p_result;
}

/** Convert an object to its write (s-expression) string representation.
 *  x-lang: (write-to-str obj)
 *  @param p_base  Execution context.
 *  @param p_args  Unevaluated argument list (obj).
 *  @return String containing the write representation.
 *  @note Fexpr: args unevaluated; delegates to x_prim_to_string.
 *  @see x_prim_display_to_string, x_prim_to_string
 */
x_obj_t *x_prim_write_to_string(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_prim_to_string(p_base, x_1(p_args), x_token_write);
}

/** Convert an object to its display (human-readable) string representation.
 *  x-lang: (display-to-str obj)
 *  @param p_base  Execution context.
 *  @param p_args  Unevaluated argument list (obj).
 *  @return String containing the display representation.
 *  @note Fexpr: args unevaluated; delegates to x_prim_to_string.
 *  @see x_prim_write_to_string, x_prim_to_string
 */
static x_obj_t *x_prim_display_to_string(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_prim_to_string(p_base, x_1(p_args), x_token_display);
}

#ifdef X_CLOCK
/** Return CPU microseconds since process start.
 *  x-lang: (clock)
 *  @param p_base  Execution context.
 *  @param p_args  Unused.
 *  @return Integer with microseconds elapsed.
 *  @note Only available when X_CLOCK is defined.
 */
static x_obj_t *x_prim_clock(x_obj_t *p_base, x_obj_t *p_args)
{
	(void)p_args;
	return x_mkint(p_base, x_sys_clock());
}
#endif /* X_CLOCK */

/** Sweep unmarked objects from the heap (garbage collection phase 2).
 *  x-lang: (heap-sweep)
 *  @param p_base  Execution context.
 *  @param p_args  Unused.
 *  @return NULL.
 *  @note Calls registered free hooks before sweeping.
 *  @note Increments GC run counter when X_PROFILE is defined.
 *  @see x_prim_heap_mark, x_prim_heap_count
 */
static x_obj_t *x_prim_heap_sweep(x_obj_t *p_base, x_obj_t *p_args)
{
	(void)p_args;
#ifdef X_PROFILE
	if (x_base_isset(p_base))
		x_atomint(x_firstobj(x_base_field_profile_gc_runs(p_base)))++;
#endif

	/* Call free hooks before sweep */
	if (x_base_isset(p_base)) {
		x_obj_t *p_hooks = x_firstobj(x_base_field_heap_free_hooks(p_base));
		x_spair_t hook_args[1];

		hook_args[0][X_OBJ_META_TYPE].p = NULL;
		hook_args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;

		while ( ! x_obj_isnil(p_base, p_hooks)) {
			x_firstobj((x_obj_t *)hook_args) = x_firstobj(p_hooks);
			x_restobj((x_obj_t *)hook_args) = NULL;
			x_obj_prim_call(p_base, (x_obj_t *)hook_args);
			p_hooks = x_restobj(p_hooks);
		}
	}

	x_heap_sweep(p_base, x_obj_heap(p_base), X_OBJ_FLAG_HEAP);

	return NULL;
}

/** Count the number of objects currently on the heap.
 *  x-lang: (heap-count)
 *  @param p_base  Execution context.
 *  @param p_args  Unused.
 *  @return Integer with the heap object count.
 *  @see x_prim_heap_mark, x_prim_heap_sweep
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

/** Mark all reachable objects on the heap (garbage collection phase 1).
 *  x-lang: (heap-mark)
 *  @param p_base  Execution context.
 *  @param p_args  Unused.
 *  @return NULL.
 *  @note Performs three marking passes: tree mark from base, conservative
 *        C stack scan, and registered GC root traversal.
 *  @note Calls registered mark hooks after root marking.
 *  @see x_prim_heap_sweep, x_prim_heap_count
 */
static x_obj_t *x_prim_heap_mark(x_obj_t *p_base, x_obj_t *p_args)
{
	(void)p_args;
	/* Normal mark: trace from base data tree */
	x_heap_tree_mark(p_base, x_atomobj(p_base), X_OBJ_FLAG_HEAP);

	/* Conservative stack scan: mark objects referenced from C stack */
	x_heap_callstack_mark(p_base, X_OBJ_FLAG_HEAP);

	/* Mark all registered GC roots */
	if (x_base_isset(p_base)) {
		x_obj_t *p_roots = x_firstobj(x_base_field_heap_mark_roots(p_base));

		while ( ! x_obj_isnil(p_base, p_roots)) {
			x_heap_tree_mark(p_base, x_firstobj(p_roots),
				X_OBJ_FLAG_HEAP);
			p_roots = x_restobj(p_roots);
		}
	}

	/* Call mark hooks (each is a callable) */
	if (x_base_isset(p_base)) {
		x_obj_t *p_hooks = x_firstobj(x_base_field_heap_mark_hooks(p_base));
		x_spair_t hook_args[1];

		hook_args[0][X_OBJ_META_TYPE].p = NULL;
		hook_args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;

		while ( ! x_obj_isnil(p_base, p_hooks)) {
			x_firstobj((x_obj_t *)hook_args) = x_firstobj(p_hooks);
			x_restobj((x_obj_t *)hook_args) = NULL;
			x_obj_prim_call(p_base, (x_obj_t *)hook_args);
			p_hooks = x_restobj(p_hooks);
		}
	}

	return NULL;
}

/** Recursively mark an object and all reachable objects as SYSTEM (GC-immune).
 *  x-lang: (gc-pin! obj)
 *  @param p_base  Execution context.
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

/** Call each zero-arg function sequentially with no allocations between.
 *  x-lang: (applicative f1 f2 ...)
 *  @param p_base  Execution context.
 *  @param p_args  Pre-evaluated argument list of callables.
 *  @return Result of the last callable invoked.
 *  @note Registered as a wrapped combiner (applicative), so args are
 *        pre-evaluated before this function is called.
 *  @note Roots p_args during iteration to protect from GC.
 */
static x_obj_t *x_prim_atomic(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_result = NULL;
	x_spair_t call_args[1];
	p_args = x_1(p_args);

	call_args[0][X_OBJ_META_TYPE].p = NULL;
	call_args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;

	/* Root p_args so mark+sweep inside the loop doesn't free them */
	x_obj_push_field(p_base, &x_base_field_eval_list(p_base), p_args, X_OBJ_FLAG_NONE);

	while ( ! x_obj_isnil(p_base, p_args)) {
		x_firstobj((x_obj_t *)call_args) = x_firstobj(p_args);
		x_restobj((x_obj_t *)call_args) = NULL;
		p_result = x_obj_prim_call(p_base, (x_obj_t *)call_args);
		p_args = x_restobj(p_args);
	}

	x_obj_pop_field(p_base, &x_base_field_eval_list(p_base));

	return p_result;
}

/** Minimal read-eval loop: reads and evaluates expressions until EOF.
 *  @param p_base  Execution context.
 *  @param p_args  Unused.
 *  @return NULL on EOF.
 *  @note No output, no prompt, no hooks. Used for C-level bootstrapping;
 *        the x-lang REPL operative in x-core.x provides the full experience.
 *  @note Clears shadows after each evaluation.
 */
x_obj_t *x_prim_repl(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_exp;
	x_spair_t read_state[1];
	read_state[0][X_OBJ_META_TYPE].p = NULL;
	read_state[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)read_state) = NULL;
	x_restobj((x_obj_t *)read_state) = NULL;

	for (;;) {
		p_exp = x_prim_read_expr(p_base, (x_obj_t *)read_state);
		if (x_obj_isnil(p_base, p_exp))
			break;
		x_eval_arg(p_base, p_exp);
		x_prim_clear_shadows(p_base);
	}

	return NULL;
}

/**
 * Return the source line at which the most recent error was signaled.
 *
 * Reads the line number saved in the current error handler's line slot
 * (set by x_base_error before longjmp).  Returns 0 outside a guard handler.
 *
 * x-lang form: @code (error-line) @endcode
 *
 * @param p_base  Execution context.
 * @param p_args  Unused.
 * @return Integer line number.
 */
static x_obj_t *x_prim_error_line(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_handler;
	(void)p_args;

	p_handler = x_firstobj(x_base_field_error_handler(p_base));
	if (x_obj_isnil(p_base, p_handler))
		return x_mkint(p_base, 0);

	return x_mkint(p_base, (x_int_t)x_error_handler_line(p_handler));
}

/** Register I/O primitives into the environment.
 *
 *  Binds: write, display, read, read-char, write-to-str, display-to-str,
 *  heap-mark, heap-sweep, heap-count, gc-pin!, error-line.
 *  Conditionally binds clock (when X_CLOCK defined).
 *  Also binds "applicative" as a wrapped combiner for atomic execution.
 *
 *  @param p_base  Execution context.
 *  @param p_args  Unused.
 *  @return The base object.
 */
x_obj_t *x_prim_io_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_callable_entry_t entries[] = {
		{ "write", x_prim_write },
		{ "display", x_prim_display },
		{ "read", x_prim_read_expr },
		{ "read-char", x_prim_read_char },
		{ "write-to-str", x_prim_write_to_string },
		{ "display-to-str", x_prim_display_to_string },
		{ "heap-mark", x_prim_heap_mark },
		{ "heap-sweep", x_prim_heap_sweep },
		{ "heap-count", x_prim_heap_count },
		{ "gc-pin!", x_prim_system_mark },
		{ "error-line", x_prim_error_line }
	};
#ifdef X_CLOCK
	static const x_callable_entry_t clock_entry[] = {
		{ "clock", x_prim_clock }
	};
#endif /* X_CLOCK */

	x_callable_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));
#ifdef X_CLOCK
	x_callable_bind_table(p_base, clock_entry,
		sizeof(clock_entry) / sizeof(clock_entry[0]));
#endif /* X_CLOCK */

	/* applicative: wrapped so args are pre-evaluated */
	{
		x_obj_t *p_sym = x_make_symbol(p_base, X_OBJ_FLAG_NONE,
			"applicative"),
			*p_prim = x_mkprim(p_base, x_prim_atomic),
			*p_wrapped = x_mkwrap(p_base, p_prim),
			*p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_sym, p_wrapped);
		x_base_env_alist_extend(p_base, p_pair);
	}

	return p_base;
}
