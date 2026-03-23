/*
 * # Computational Expressions in C
 *
 * ## x-syntax/closure.c -- Syntax - Closure & Operative Creation
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2024 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 *
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
#include "x-prim.h"
#include "x-base.h"
#include "x-type/operative.h"
#include "x-type/procedure.h"

/* fn: (fn params body...) -> create closure */
static x_obj_t *x_prim_closure(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_params, *p_body,
		*p_env = x_base_field_env_alist(p_base),
		*p_bst = x_base_field_env_global_tree(p_base);
	x_args(p_args, 2, NULL, &p_params);
	p_body = x_11(p_args);

	return x_mkproc(p_base, p_params, p_body, p_env, p_bst);
}

/* op: (op formals env-param body...) -> create operative (user-level fexpr) */
static x_obj_t *x_prim_operative(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_params, *p_envparam, *p_body,
		*p_env = x_base_field_env_alist(p_base);
	x_args(p_args, 3, NULL, &p_params, &p_envparam);
	p_body = x_111(p_args);

	return x_mkop(p_base, p_params, p_envparam, p_body, p_env);
}

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
