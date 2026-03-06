/*
 * # Computational Expressions in C
 *
 * ## x-env.c -- Implementation - Environment
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2022 Jon Ruttan
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
#include "x-type/symbol.h"
#include "x-type/str.h"
#include "x-alist.h"

x_obj_t *x_env_assoc(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_firstobj(p_args) }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_base_field_env_alist(p_base) }, { NULL })
	};
	x_obj_t *p_sym = x_alist_assoc(p_base, (x_obj_t *)args);

	if (x_obj_isnil(p_base, p_sym)) {
		/* TODO: Implement type name. */
		x_obj_error(p_base, "Unbound "X_TYPE_SYMBOL_NAME, x_symbolval(x_firstobj(p_args)));

		return p_base;
	}

	return x_restobj(p_sym);
}
