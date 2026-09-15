/** @file x-prim/base.c
 *  @brief Sandbox-base primitives -- make-base, base-eval (setjmp cross-base
 *         eval), base-bind, make-token-base, base-make-type, and the two
 *         binders that name their environment: def-global and def-in.
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
#include "x-prim.h"
#include "x-alist.h"
#include "x-eval.h"
#include "x-heap.h"
#include "x-type.h"
#include <setjmp.h>
#include "x-token.h"
#include "x-type/buffer.h"
#include "x-type/char.h"
#include "x-type/err.h"
#include "x-type/comment.h"
#include "x-type/int.h"
#include "x-type/list.h"
#include "x-type/operative.h"
#include "x-type/prim.h"
#include "x-type/procedure.h"
#include "x-type/ptr.h"
#include "x-type/str.h"
#include "x-type/symbol.h"
#include "x-type/whitespace.h"

/**
 * @brief Create a type on a target base (cross-base type registration).
 *
 * x-lang form: @code (base-make-type base name handlers) @endcode
 *
 * Like make-type, but registers the type on @p p_target rather than the
 * calling base. Pins the type struct as SHARED so the calling base's GC
 * will not sweep the name atom or the handler closures the target now
 * refers to from its own heap chain.
 *
 * @param p_base  Calling execution context (used for handler closure allocation).
 * @param p_args  Unevaluated: (self target-base name-string handlers-alist).
 * @return The type name atom.
 * @note Sets X_OBJ_FLAG_SHARED on the target base and on the type struct
 *       to prevent cross-base GC.
 * @see x_prim_make_type
 */
static x_obj_t *x_prim_base_make_type(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_target, *p_name_str, *p_handlers;
	x_char_t *name;
	x_obj_t *p_name_atom, *p_type;

	x_eargs(p_base, p_args, 4, NULL, &p_target, &p_name_str, &p_handlers);
	name = x_lib_strndup(x_strval(p_name_str),
		x_lib_strlen(x_strval(p_name_str)));
	p_name_atom = x_obj_make(p_base, x_type_atom_obj,
		X_OBJ_FLAG_OWN, X_OBJ_LENGTH_ATOM, name);

	/* Build type using calling base; register on target. */
	p_type = x_prim_type_build_struct(p_base, p_name_atom, p_handlers);
	x_eval_type_alist_extend(p_target, p_type);

	/* Pin what this registration built on the CALLING base and then handed
	 * to the target: the name atom, the type struct around it, and every
	 * handler closure.  Those objects sit on the calling base's heap chain,
	 * so the calling base's sweep decides their fate -- while the only thing
	 * that still refers to them, the target's type alist, sits on the
	 * TARGET's chain, out of that sweep's reach.
	 *
	 * Pin from p_type, not from the target's tree root.  Marking from
	 * x_atomobj(p_target) marked NOTHING: x_heap_tree_mark stops at any
	 * object that already carries the flags it is setting, and a base's tree
	 * root is born SHARED (x-expr's x_base_make allocates every skeleton
	 * node with X_OBJ_FLAG_SHARED), so the walk short-circuited on its own
	 * first node.  p_type is an ordinary X_OBJ_FLAG_NONE pair tree
	 * (x_type_struct_make), so the walk descends it and reaches the name
	 * atom and the handlers -- which is what the pin was always for.
	 *
	 * With the pin doing nothing, the registration survived exactly one
	 * collect and died on the next (x-lang#599).  The calling base's mark
	 * walk reaches the target base through whatever binding holds it and
	 * descends the target's tree, so the first collect marked these objects
	 * by that route and retained them.  But that walk also set the mark bit
	 * on the target's OWN skeleton cells, which live on the target's chain
	 * and so are never visited by the calling base's sweep -- the bit is
	 * never cleared.  On the second collect the walk hit those still-marked
	 * cells, took them for already-done and stopped, leaving the name atom
	 * unmarked and unpinned; the sweep freed it, and the next read
	 * dereferenced it in x_alist_assoc. */
	x_obj_flags(p_target) |= X_OBJ_FLAG_SHARED;
	x_heap_tree_mark(p_base, p_type, X_OBJ_FLAG_SHARED);

	return p_name_atom;
}

/**
 * @brief Give a base the reader buffer the tokenizer reads through.
 *
 * @details Both base constructors need this and each used to spell it out,
 *          which is how make-tok came to be missing it entirely: an empty
 *          input happened to work, because nothing was ever read, and the
 *          first character dereferenced a buffer that was never made.
 *
 *          Attaching rather than allocating-and-returning keeps each caller's
 *          existing order intact -- make-base registers its types before this
 *          runs, and that ordering is not something to disturb while fixing a
 *          crash.
 *
 * @param p_new  The base to attach a reader buffer to.
 */
static void x_base_attach_read_buffer(x_obj_t *p_new)
{
	x_char_t *buffer = (x_char_t *)x_sys_malloc(X_READ_BUF_SIZE);
	x_obj_t *p_buffer = x_mkbuffer(p_new, buffer);

	x_base_field_buffer(p_new) = x_mkspair(p_new, X_OBJ_FLAG_NONE,
		p_buffer, x_base_field_buffer(p_new));
}


/**
 * @brief Create a bare base suitable for tokenization only.
 *
 * x-lang form: @code (make-token-base) @endcode
 *
 * Allocates a minimal base with no types or primitives registered,
 * inheriting only the boolean singletons (t/f) from the calling base.
 * Used for custom tokenizer type registration on an isolated base.
 *
 * @param p_base  Base (execution context) (boolean singletons are inherited).
 * @param p_args  Unused.
 * @return New bare base object.
 * @see x_prim_make_base
 */
static x_obj_t *x_prim_make_token_base(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_new = x_eval_make(NULL, NULL);
	(void)p_args;

	/* Inherit the boolean singletons from the calling base, WRITING THROUGH
	 * THE CELL.  true/false/sigint are cells (x-eval-layout.h), and
	 * x_eval_make's own parented path assigns x_firstobj(field) for exactly
	 * that reason; it is skipped here because this base is made parentless.
	 *
	 * Assigning the FIELD instead -- which is what this did -- replaced each
	 * cell with the singleton it should have contained, so every later
	 * x_firstobj() on it read the singleton's first slot as a cell: garbage,
	 * and a segfault the moment the tokenizer consulted a truth value. */
	x_firstobj(x_eval_field_true(p_new))   = x_firstobj(x_eval_field_true(p_base));
	x_firstobj(x_eval_field_false(p_new))  = x_firstobj(x_eval_field_false(p_base));
	x_firstobj(x_eval_field_sigint(p_new)) = x_firstobj(x_eval_field_sigint(p_base));

	/* The reader reads THROUGH the base's buffer, so a tokenizer base needs
	 * one exactly as make-base does -- and this is the constructor that was
	 * missing it.  Without it an empty input happens to work, because nothing
	 * is ever read, and the first character dereferences a buffer that was
	 * never made. */
	x_base_attach_read_buffer(p_new);

	return p_new;
}

/**
 * @brief Create a fully initialized sandboxed interpreter base.
 *
 * x-lang form: @code (make-base) @endcode
 *
 * Allocates a new base, registers all built-in types (prim, operative,
 * procedure, symbol, list, int, str, char, whitespace, comment), sets up
 * a read buffer, and registers all C primitives. The result is a complete
 * interpreter context that can be evaluated into via base-eval.
 *
 * @param p_base  Base (execution context) (unused beyond allocation).
 * @param p_args  Unused.
 * @return Fully bootstrapped base object.
 * @see x_prim_base_eval
 */
static x_obj_t *x_prim_make_base(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_new_base;
	(void)p_args;

	p_new_base = x_eval_make(NULL, NULL);

	/* Register types. */
	x_type_prim_register(p_new_base, p_new_base);
	x_type_operative_register(p_new_base, p_new_base);
	x_type_procedure_register(p_new_base, p_new_base);
	x_type_symbol_register(p_new_base, p_new_base);
	x_type_list_register(p_new_base, p_new_base);
	x_type_int_register(p_new_base, p_new_base);
	x_type_str_register(p_new_base, p_new_base);
	x_type_char_register(p_new_base, p_new_base);
	x_type_err_register(p_new_base, p_new_base);
	x_type_whitespace_register(p_new_base, p_new_base);
	x_type_comment_register(p_new_base, p_new_base);

	x_base_attach_read_buffer(p_new_base);

	/* Register primitives. */
	x_prim_register(p_new_base, p_new_base);

	return p_new_base;
}

/**
 * @brief Evaluate an expression in a target base's environment.
 *
 * x-lang form: @code (base-eval base expr) @endcode
 *
 * Pushes a setjmp-based error handler onto the target base's error handler
 * stack, evaluates @p p_expr in the target, then pops the handler. If an
 * error occurs in the target, it is caught, the handler is popped, the
 * environment is restored, and the error is propagated to the calling
 * base's error handler (or printed if none exists).
 *
 * @param p_base  Calling execution context.
 * @param p_args  Unevaluated: (self target-base expr).
 * @return Result of evaluating @p expr in the target base, or NULL on error.
 * @note Uses setjmp/longjmp for error propagation across bases.
 */
static x_obj_t *x_prim_base_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	jmp_buf jmp;
	x_obj_t *p_target, *p_expr;
	x_obj_t *p_handler, *p_result;
	x_obj_t *p_err, *p_parent;

	x_eargs(p_base, p_args, 3, NULL, &p_target, &p_expr);

	/* Build handler pair tree, SAME shape as x_prim_guard's (#253):
	 * (jmp-ptr . ((saved-env . saved-boundary) . (error-value . line))).
	 * The x_error_handler_* accessors read saved-env as x_001 -- one
	 * level below the (env . boundary) cell -- so the env must be
	 * wrapped in that cell.  The old build put the bare env where the
	 * (env . boundary) cell belongs, so recovery restored first(env),
	 * degrading the child's env-alist head on every caught error until
	 * a lookup walked a non-pair and segfaulted (0x18). */
	p_handler = x_mkspair(p_target, X_OBJ_FLAG_NONE,
		x_mkptr(p_target, &jmp),
		x_mkspair(p_target, X_OBJ_FLAG_NONE,
			x_mkspair(p_target, X_OBJ_FLAG_NONE,
				x_firstobj(x_eval_field_env_alist(p_target)),
				x_eval_field_env_local_boundary(p_target)),
			x_mkspair(p_target, X_OBJ_FLAG_NONE, NULL, NULL)));

	/* Push handler onto error_handler_stack */
	x_eval_field_error_handler(p_target) = x_mkspair(p_target, X_OBJ_FLAG_NONE,
		p_handler, x_eval_field_error_handler(p_target));

	if (setjmp(jmp) == 0) {
		p_result = x_eval_arg(p_target, p_expr);
	} else {
		p_err = x_error_handler_error(p_handler);

		/* Error caught from target: pop handler, restore env and
		 * boundary, propagate. */
		x_eval_field_error_handler(p_target)
			= x_restobj(x_eval_field_error_handler(p_target));
		x_firstobj(x_eval_field_env_alist(p_target))
			= x_error_handler_saved_env(p_handler);
		x_eval_field_env_local_boundary(p_target)
			= x_error_handler_saved_boundary(p_handler);

		if ( ! x_obj_isnil(p_base, x_firstobj(x_eval_field_error_handler(p_base)))) {
			p_parent = x_firstobj(x_eval_field_error_handler(p_base));

			x_error_handler_error(p_parent) = p_err;
			x_error_handler_line(p_parent) = x_error_handler_line(p_handler);
			x_firstobj(x_eval_field_env_alist(p_base))
				= x_error_handler_saved_env(p_parent);
			longjmp(*(jmp_buf *)x_error_handler_jmp(p_parent), 1);
		}

		x_obj_error(p_base, "error", p_err);

		return NULL;
	}

	/* Pop error_handler_stack */
	x_eval_field_error_handler(p_target)
		= x_restobj(x_eval_field_error_handler(p_target));

	return p_result;
}

/**
 * @brief Bind a name-value pair in a target base's environment.
 *
 * x-lang form: @code (base-bind base name value) @endcode
 *
 * Creates a (name . value) pair and prepends it to the target base's
 * environment alist, making it visible to subsequent evaluations.
 *
 * @param p_base  Calling execution context.
 * @param p_args  Unevaluated: (self target-base name value).
 * @return The bound value.
 */
static x_obj_t *x_prim_base_bind(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_target, *p_name, *p_val;
	x_obj_t *p_pair;

	x_eargs(p_base, p_args, 4, NULL, &p_target, &p_name, &p_val);

	p_pair = x_mkspair(p_target, X_OBJ_FLAG_NONE, p_name, p_val);
	x_eval_env_alist_extend(p_target, p_pair);

	return p_val;
}


/**
 * @brief Bind a name in the base's GLOBAL environment: the body shared by
 *        def-global and by def-in's top-level path.
 *
 * @details Redefinition updates the existing BST entry in place; a fresh
 *          name is inserted into the BST.  Globals resolve through the BST
 *          (GH #47), so the insert is what makes the binding findable
 *          afterwards.
 *
 *          The env alist is extended ALWAYS, and the boundary advanced only
 *          at top level.  Skipping the extension inside a frame left the
 *          binding in the BST but not on the spine, and anything that walks
 *          the env alist rather than resolving through the BST could not see
 *          it -- syntax-rules' hygiene lookup is one such walker, and a macro
 *          expanding to a lambda bound its parameter to a stale entry.  A
 *          half-present binding is worse than either alternative.
 *
 *          The boundary is the part that must not move under a frame: it
 *          marks where globals end, and the spine it would point into unwinds
 *          when the frame pops.
 *
 * @param p_base  Base (execution context).
 * @param p_name  The symbol to bind.
 * @param p_val   The value, already evaluated.
 * @return The bound value.
 * @see x_prim_define_global, x_prim_define_in
 */
static x_obj_t *x_define_global(x_obj_t *p_base, x_obj_t *p_name, x_obj_t *p_val)
{
	x_obj_t *p_pair, *p_entry;

	p_entry = x_alist_bst_lookup(p_base,
		x_eval_field_env_global_tree(p_base), p_name);
	if ( ! x_obj_isnil(p_base, p_entry)) {
		x_restobj(p_entry) = p_val;
		return p_val;
	}

	p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_name, p_val);

	x_eval_env_alist_extend(p_base, p_pair);

	if (x_base_isset(p_base)
		&& x_obj_isnil(p_base, x_eval_field_save_stack(p_base))) {
		x_eval_field_env_local_boundary(p_base)
			= x_firstobj(x_eval_field_env_alist(p_base));
	}

	x_eval_field_env_global_tree(p_base) = x_alist_bst_insert(
		p_base, x_eval_field_env_global_tree(p_base), p_pair);

	return p_val;
}


/**
 * @brief Bind a name in the base's GLOBAL environment, whatever the frame depth.
 *
 * x-lang form: @code ((prim-ref 'base 'def-global) name value) @endcode
 *
 * @details `def` chooses global-versus-local by the LIVE FRAME: top-level
 *          iff the env head is not a FRAME-marked cell (x_prim_define).  It
 *          chose by save-stack depth before, which made a def in a closure's
 *          TAIL position global -- the frame is popped before a deferred
 *          tail runs -- so temporaries def'd inside an if/do tail leaked
 *          into the base.
 *
 *          Either way an OPERATIVE cannot define for its caller by plain def.
 *          Every surface language on x (Scheme's `define`, Kernel's `$define!`)
 *          works around it by putting its eval in tail position so TCO pops the
 *          operative's frame first.  That is an accident of frame depth: one
 *          extra wrapper frame and the binding silently lands nowhere -- not
 *          shadowed, gone -- and a definition in BODY position never worked at
 *          all.  See x-lang#527.
 *
 *          This takes the global path unconditionally.  The door that binds
 *          in the frame the caller names is def-in, below.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self name value); both are evaluated.
 * @return The bound value.
 * @see x_prim_define  -- the depth-sensitive form this complements
 * @see x_prim_define_in
 */
static x_obj_t *x_prim_define_global(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_name, *p_val;

	x_eargs(p_base, p_args, 3, NULL, &p_name, &p_val);

	return x_define_global(p_base, p_name, p_val);
}


/**
 * @brief Bind a name in a GIVEN environment: the frame at its head, or the
 *        global tree when the head is not a frame cell.
 *
 * x-lang form: @code ((prim-ref 'base 'def-in) env name value) @endcode
 *
 * @details An operative cannot define for its caller with a plain def.  The
 *          def extends the operative's own frame, and the restore that ends
 *          the operative puts the caller's head back, so the cell is gone; a
 *          tail-eval'd def runs after that restore and grows the caller's
 *          chain in front of a head the caller's own saved compound still
 *          points at, so the next restore drops it.  At top level the same
 *          def went into the BST, which is why the pattern looked like it
 *          worked, and why every definer written as an operative -- x-lang's
 *          doc, def-class, def-record, a lang's define -- binds nothing
 *          inside a frame.  See x-engine-c#46 and x-lang#527.
 *
 *          This binds where the CALLER says.  @p env is an environment head
 *          as an operative receives it in its env parameter.  When that head
 *          is a FRAME-marked cell the binding goes into the frame.  An
 *          existing binding for the name in the leading frame run -- the
 *          region symbol lookup walks first -- is updated in place, the way a
 *          top-level redefinition updates its BST entry: a cell spliced
 *          behind the head could never be seen past a same-named cell in
 *          front of it.  Otherwise a FRAME-marked cell is spliced AFTER the
 *          head, and after rather than before for three reasons.  Every
 *          saved restore compound holds a pointer to a head cell and stays
 *          valid.  A closure that captured the frame earlier sees the new
 *          binding, because lookup walks from the head through the rests,
 *          which is what mutually recursive definitions in one frame need.
 *          And the frame run stays contiguous, so lookup's step 1 and the
 *          loader's strip both see one region.
 *
 *          When the head is not a frame cell -- nil, a global cell, or not a
 *          spine pair at all -- the environment is the global chain, and
 *          the binding takes the path def-global takes.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self env name value); all three are evaluated.
 * @return The bound value.
 * @see x_prim_define_global  -- the global-only door this generalises
 * @see x_prim_define         -- the frame-or-global decision this makes explicit
 * @see x_type_symbol_eval    -- the frame run this searches
 */
static x_obj_t *x_prim_define_in(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_env, *p_name, *p_val, *p_walk, *p_pair, *p_cell;

	x_eargs(p_base, p_args, 4, NULL, &p_env, &p_name, &p_val);

	/* A frame head is a spine pair carrying the FRAME mark.  The mark is a
	 * flag bit, and flag bits mean different things on other types, so
	 * the type is asked first: anything that is not a spine pair is not
	 * an environment, and the binding goes global rather than into it. */
	if (x_obj_isnil(p_base, p_env)
		|| ! x_obj_type_isspair(p_env)
		|| ! (x_obj_flags(p_env) & X_OBJ_FLAG_FRAME)) {
		return x_define_global(p_base, p_name, p_val);
	}

	p_walk = p_env;
	while ( ! x_obj_isnil(p_base, p_walk)
		&& (x_obj_flags(p_walk) & X_OBJ_FLAG_FRAME)) {
		if (x_firstobj(x_firstobj(p_walk)) == p_name) {
			x_restobj(x_firstobj(p_walk)) = p_val;
			return p_val;
		}
		p_walk = x_restobj(p_walk);
	}

	p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_name, p_val);
	p_cell = x_mkspair(p_base, X_OBJ_FLAG_FRAME, p_pair, x_restobj(p_env));
	x_restobj(p_env) = p_cell;

	return p_val;
}

/** Register the sandbox base primitives. */
x_obj_t *x_prim_base_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_prim_entry_t entries[] = {
		{ "base-make-type",    x_prim_base_make_type,    "base",   "make-type"     },
		{ "make-token-base",   x_prim_make_token_base,   "base",   "make-tok"      },
		{ "make-base",         x_prim_make_base,         "base",   "make"          },
		{ "base-eval",         x_prim_base_eval,         "base",   "eval"          },
		{ "base-bind",         x_prim_base_bind,         "base",   "bind"          },
		{ "base-def-global",   x_prim_define_global,     "base",   "def-global"    },
		{ "base-def-in",       x_prim_define_in,         "base",   "def-in"        }
	};

	x_prims_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
