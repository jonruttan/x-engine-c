#ifndef X_PRIM_H
#define X_PRIM_H

/**
 * @file x-prim.h
 * @brief Primitive function infrastructure and registration.
 *
 * Declares the callable binding mechanism used to register C primitives
 * into the x-lang environment, argument unpacking helpers, body/TCO
 * evaluation entry points, shadow-list management, and the per-module
 * register functions.
 *
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2024 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 */

/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */

#include "x-obj.h"
#include <stdarg.h>

/** Evaluate a single argument expression. */
x_obj_t *x_eval_arg(x_obj_t *p_base, x_obj_t *p_arg);

/**
 * @defgroup arg_helpers Argument Unpacking Helpers
 * @brief Variadic helpers for extracting arguments from x-lang arg lists.
 * @{
 */

/**
 * Unpack @p count elements from an args list into output pointers.
 *
 * NULL pointers skip that position (like @c _ in pattern matching).
 * @code
 *   x_args(p_args, 3, NULL, &a, &b);  // skip self, extract 2
 * @endcode
 *
 * @param p_args  Argument list (pair chain).
 * @param count   Number of positions to unpack.
 * @param ...     Pointers to @c x_obj_t* slots (or NULL to skip).
 */
static void __attribute__((unused)) x_args(x_obj_t *p_args, int count, ...)
{
	va_list ap;
	int i;

	va_start(ap, count);
	for (i = 0; i < count; i++) {
		x_obj_t **slot = va_arg(ap, x_obj_t **);
		if (slot != NULL)
			*slot = x_firstobj(p_args);
		p_args = x_restobj(p_args);
	}
	va_end(ap);
}

/**
 * Unpack and evaluate @p count elements from an args list.
 *
 * NULL pointers skip that position without evaluating.
 * @code
 *   x_eargs(p_base, p_args, 3, NULL, &a, &b);  // skip self, eval+extract 2
 * @endcode
 *
 * @param p_base  Base/execution context.
 * @param p_args  Argument list (pair chain).
 * @param count   Number of positions to unpack.
 * @param ...     Pointers to @c x_obj_t* slots (or NULL to skip).
 */
static void __attribute__((unused)) x_eargs(x_obj_t *p_base, x_obj_t *p_args, int count, ...)
{
	va_list ap;
	int i;

	va_start(ap, count);
	for (i = 0; i < count; i++) {
		x_obj_t **slot = va_arg(ap, x_obj_t **);
		if (p_args == NULL) { if (slot) *slot = NULL; continue; }
		if (slot != NULL)
			*slot = x_eval_arg(p_base, x_firstobj(p_args));
		p_args = x_restobj(p_args);
	}
	va_end(ap);
}

/** @} */ /* end arg_helpers */

/** @name Evaluation Entry Points
 * @{ */

/** Evaluate an argument list, returning a list of results. */
x_obj_t *x_eval_list(x_obj_t *p_base, x_obj_t *p_args);

/** Extend an environment by binding params to vals. */
x_obj_t *x_env_extend(x_obj_t *p_base, x_obj_t *p_env,
	x_obj_t *p_params, x_obj_t *p_vals);

/** Evaluate a body (sequence of expressions), returning the last result. */
x_obj_t *x_eval_body(x_obj_t *p_base, x_obj_t *p_body);

/** Evaluate a body with TCO, setting up a trampoline for the tail call. */
x_obj_t *x_eval_body_tco(x_obj_t *p_base, x_obj_t *p_body);

/** Simplified TCO body evaluation for non-wrapping contexts. */
x_obj_t *x_eval_body_tco_simple(x_obj_t *p_base, x_obj_t *p_body);

/** Execute the TCO trampoline loop until a non-TCO result is produced. */
x_obj_t *x_eval_tco_trampoline(x_obj_t *p_base, x_obj_t *p_result);

/** @} */

/**
 * @defgroup callable_bind Callable Binding
 * @brief Bind C functions into the x-lang environment.
 * @{
 */

/** Name/function pair for bulk registration via x_callable_bind_table(). */
typedef struct {
	x_char_t *name;                        /**< Symbol name to bind. */
	x_fn_t fn;                             /**< C primitive function pointer. */
} x_callable_entry_t;

/** Bind a single C function as a named callable in the environment. */
void x_callable_bind(x_obj_t *p_base, x_char_t *name, x_fn_t fn);

/** Bind an array of name/function entries into the environment. */
void x_callable_bind_table(x_obj_t *p_base, const x_callable_entry_t *table, int count);

/** @} */

/** @name Module Registration Functions
 * @{ */
/** Register core primitives (eval, if, do, let, fn, op, apply, guard, etc.). */
x_obj_t *x_prim_core_register(x_obj_t *p_base, x_obj_t *p_args);

/** Register arithmetic primitives (+, -, *, /, modulo, etc.). */
x_obj_t *x_prim_arith_register(x_obj_t *p_base, x_obj_t *p_args);

/** Register predicate primitives (eq?, pair?, atom?, null?, etc.). */
x_obj_t *x_prim_pred_register(x_obj_t *p_base, x_obj_t *p_args);

/** Register string primitives (string-length, substring, etc.). */
x_obj_t *x_prim_string_register(x_obj_t *p_base, x_obj_t *p_args);

/** Register I/O primitives (read, write, display, load, etc.). */
x_obj_t *x_prim_io_register(x_obj_t *p_base, x_obj_t *p_args);

/** Minimal C read-eval loop (no output, no hooks). */
x_obj_t *x_prim_repl(x_obj_t *p_base, x_obj_t *p_args);

/** Write an object to a string representation. */
x_obj_t *x_prim_write_to_string(x_obj_t *p_base, x_obj_t *p_args);

/** Register custom type primitives (make-type, type accessors, etc.). */
x_obj_t *x_prim_type_register(x_obj_t *p_base, x_obj_t *p_args);

/** Register FFI primitives (ffi-call, ffi-lib, etc.). */
x_obj_t *x_prim_ffi_register(x_obj_t *p_base, x_obj_t *p_args);

/** Register call/cc continuation primitives. */
x_obj_t *x_prim_callcc_register(x_obj_t *p_base, x_obj_t *p_args);

/** Initialize the call/cc subsystem. */
void x_callcc_init(void);
/** @} */

/** @name Shadow List Management
 * @{ */
/** Clear all shadow-list entries from the environment. */
void x_prim_clear_shadows(x_obj_t *p_base);

/** Clear shadow-list entries back to a saved checkpoint. */
void x_prim_clear_shadows_to(x_obj_t *p_base, x_obj_t *p_old);
/** @} */

/** Register all primitive modules into the base environment. */
x_obj_t *x_prim_register(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_PRIM_H */
