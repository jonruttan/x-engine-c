/*
 * # Computational Expressions in C
 *
 * ## x-eval.c -- Implementation - Evaluator
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2021 Jon Ruttan
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
#include "x-eval.h"
#include "x-obj.h"
#include "x-type.h"
#include "x-type/prim.h"

x_obj_t *x_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_exp;
	x_spair_t prim_args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL });

eval_start:
	p_exp = x_firstobj(x_eval_arg_exp(p_args));

	if (x_obj_isnil(p_base, p_exp)) {
		return p_base;
	}

	/* Differentiate simple from complex types. */
	if (x_obj_isnil(p_base, x_obj_type(x_obj_type(p_exp)))) {
		return p_exp;
	}

	x_firstobj((x_obj_t *)prim_args) = x_type_field_eval(x_obj_type(p_exp));

	if ( ! x_obj_isnil(p_base, x_firstobj((x_obj_t *)prim_args))) {
		x_restobj((x_obj_t *)prim_args) = p_args;
		p_exp = x_type_prim_call(p_base, (x_obj_t *)prim_args);

		if (p_exp == p_args) {
			goto eval_start;
		}
	}

	return p_exp;
}
