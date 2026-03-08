/*
 * # Computational Expressions in C
 *
 * ## x-prim.c -- Implementation - Primitives
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
/*
 * # Includes
 */
#include "x-prim.h"
#include "x-base.h"
#include "x-eval.h"
#include "x-type/list.h"
#include "x-type/prim.h"
#include "x-type/symbol.h"

/*
 * # Helpers
 */
x_obj_t *x_prim_eval_arg(x_obj_t *p_base, x_obj_t *p_arg)
{
	x_satom_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_arg });
	x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { wrap }, { NULL });

	return x_eval(p_base, (x_obj_t *)args);
}

x_obj_t *x_prim_evlis(x_obj_t *p_base, x_obj_t *p_args)
{
	if (x_obj_isnil(p_base, p_args)) {
		return p_base;
	}

	return x_mklist(p_base,
		x_prim_eval_arg(p_base, x_firstobj(p_args)),
		x_prim_evlis(p_base, x_restobj(p_args)));
}

x_obj_t *x_prim_multiple_extend(x_obj_t *p_base, x_obj_t *p_env,
	x_obj_t *p_params, x_obj_t *p_vals)
{
	/* Variadic: single symbol binds to entire remaining arg list. */
	if ( ! x_obj_isnil(p_base, p_params)
		&& x_obj_type_issymbol(p_base, p_params)) {
		return x_mkspair(p_base,
			x_mkspair(p_base, p_params, p_vals), p_env);
	}

	/* Base case: no more params. */
	if (x_obj_isnil(p_base, p_params)) {
		return p_env;
	}

	/* Recursive case: bind first param to first val, continue. */
	return x_prim_multiple_extend(p_base,
		x_mkspair(p_base,
			x_mkspair(p_base, x_firstobj(p_params), x_firstobj(p_vals)),
			p_env),
		x_restobj(p_params),
		x_restobj(p_vals));
}

/*
 * # Registration
 */
void x_prim_bind(x_obj_t *p_base, x_char_t *name, x_prim_fn fn)
{
	x_obj_t *p_sym = x_make_symbol(p_base, X_OBJ_FLAG_NONE, name),
		*p_prim = x_mkprim(p_base, fn),
		*p_pair = x_mkspair(p_base, p_sym, p_prim);

	x_base_env_alist_extend(p_base, p_pair);
}

x_obj_t *x_prim_register(x_obj_t *p_base, x_obj_t *p_args)
{
	/* Bind t as a self-evaluating truth symbol. */
	{
		x_obj_t *p_t = x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE),
			*p_pair = x_mkspair(p_base, p_t, p_t);
		x_base_env_alist_extend(p_base, p_pair);
	}

	x_prim_core_register(p_base, p_args);
	x_prim_arith_register(p_base, p_args);
	x_prim_pred_register(p_base, p_args);
	x_prim_string_register(p_base, p_args);
	x_prim_io_register(p_base, p_args);
	x_prim_type_register(p_base, p_args);
	x_prim_ffi_register(p_base, p_args);

	return p_base;
}
