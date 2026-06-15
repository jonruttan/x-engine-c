/** @file closure.c
 *  @brief Syntax - Closure and Operative Creation (fn, op)
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2026 Jon Ruttan
 *  @license MIT No Attribution (MIT-0)
 */

/*     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
#include "x-prim.h"
#include "x-eval.h"
#include "x-type/operative.h"
#include "x-type/procedure.h"

/**
 * Closure creation form. x-lang: (fn params body ...)
 *
 * Creates a procedure (closure) that captures the current environment and
 * BST index (fexpr -- params and body are not evaluated).  The resulting
 * closure does NOT have X_OBJ_FLAG_WRAP; only wrap applicatives do.
 *
 * @param p_base  Execution context.
 * @param p_args  Unevaluated argument list; expects (caller params body ...).
 * @return A new procedure object.
 * @see x_prim_operative
 * @see x_mkproc
 */
static x_obj_t *x_prim_closure(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_params, *p_body,
		*p_env = x_firstobj(x_eval_field_env_alist(p_base)),
		*p_bst = x_eval_field_env_global_tree(p_base);
	x_args(p_args, 2, NULL, &p_params);
	p_body = x_11(p_args);

	return x_mkproc(p_base, p_params, p_body, p_env, p_bst);
}

/**
 * Operative creation form. x-lang: (op formals env-param body ...)
 *
 * Creates a user-level fexpr (operative) that receives unevaluated
 * arguments and an explicit environment parameter (fexpr -- formals,
 * env-param, and body are not evaluated).
 *
 * @param p_base  Execution context.
 * @param p_args  Unevaluated argument list; expects (caller formals env-param body ...).
 * @return A new operative object.
 * @see x_prim_closure
 * @see x_mkop
 */
static x_obj_t *x_prim_operative(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_params, *p_envparam, *p_body,
		*p_env = x_firstobj(x_eval_field_env_alist(p_base));
	x_args(p_args, 3, NULL, &p_params, &p_envparam);
	p_body = x_111(p_args);

	return x_mkop(p_base, p_params, p_envparam, p_body, p_env);
}

/**
 * Register closure and operative syntax primitives.
 *
 * Binds: fn, op.
 *
 * @param p_base  Execution context.
 * @param p_args  Unused.
 * @return p_base.
 */
x_obj_t *x_syntax_closure_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_callable_entry_t entries[] = {
		{ "fn", x_prim_closure },
		{ "op", x_prim_operative }
	};

	x_callable_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
