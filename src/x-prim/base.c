/** @file x-prim/base.c
 *  @brief Sandbox-base primitives -- make-base, base-eval (setjmp cross-base
 *         eval), base-bind, make-token-base, base-make-type.
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
#include "x-eval.h"
#include "x-heap.h"
#include "x-type.h"
#include <setjmp.h>
#include "x-token.h"
#include "x-type/buffer.h"
#include "x-type/char.h"
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
 * calling base. Marks the target base tree as SHARED so the calling
 * base's GC will not sweep handler closures referenced across bases.
 *
 * @param p_base  Calling execution context (used for handler closure allocation).
 * @param p_args  Unevaluated: (self target-base name-string handlers-alist).
 * @return The type name atom.
 * @note Sets X_OBJ_FLAG_SHARED on the target base to prevent cross-base GC.
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

	/* Mark target base and its tree with SHARED so calling base's GC
	 * won't sweep handler closures referenced cross-base. */
	x_obj_flags(p_target) |= X_OBJ_FLAG_SHARED;
	x_heap_tree_mark(p_base, x_atomobj(p_target), X_OBJ_FLAG_SHARED);

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


/** Register the sandbox base primitives. */
x_obj_t *x_prim_base_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_prim_entry_t entries[] = {
		{ "base-make-type",    x_prim_base_make_type,    "base",   "make-type"     },
		{ "make-token-base",   x_prim_make_token_base,   "base",   "make-tok"      },
		{ "make-base",         x_prim_make_base,         "base",   "make"          },
		{ "base-eval",         x_prim_base_eval,         "base",   "eval"          },
		{ "base-bind",         x_prim_base_bind,         "base",   "bind"          }
	};

	x_prims_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
