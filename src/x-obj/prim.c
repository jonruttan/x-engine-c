/*
 * # Computational Expressions in C
 *
 * ## x-obj.c -- Implementation - Objects - Primitives
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
#include "x-obj.h"
#include "x-type.h"
#include "x-type/procedure.h"


/*
 * # Object Functions
 */
x_obj_t *x_obj_prim_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_make, *p_obj;

	/* TODO: Move argument checks to Lisp layer. */
	if (x_obj_isnil(p_base, p_args) || x_obj_isnil(p_base, (p_obj = x_firstobj(p_args)))) {
		return NULL;
	}

	if (x_obj_isnil(p_base, x_obj_type(p_obj))) {
		return NULL;
	}

	if (x_obj_type_issatom(p_obj) || x_obj_type_issatom(x_obj_type(p_obj))) {
		return x_obj_make(p_base, x_obj_type(p_obj), x_atomint(x_firstobj(x_restobj(p_args))), X_OBJ_LENGTH_ATOM, x_atomint(x_firstobj(x_restobj(x_restobj(p_args)))));
	}

	if (x_obj_type_isspair(p_obj)) {
		return x_obj_make(p_base, x_obj_type(p_obj), x_atomint(x_firstobj(x_restobj(p_args))), X_OBJ_LENGTH_PAIR, x_atomint(x_firstobj(x_restobj(x_restobj(p_args)))), x_atomint(x_firstobj(x_restobj(x_restobj(x_restobj(p_args))))));
	}

	p_make = x_type_field_make(x_obj_type(p_obj));

	if (x_obj_isnil(p_base, p_make) || x_obj_isnil(p_base, x_atomobj(p_make))) {
		return NULL;
	}

	return (*x_atomfn(p_make))(p_base, x_mkspair(p_base, p_obj, NULL));
}

x_obj_t *x_obj_prim_call(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_call, *p_obj;

	/* TODO: Move argument checks to Lisp layer. */
	if (x_obj_isnil(p_base, p_args) || x_obj_isnil(p_base, (p_obj = x_firstobj(p_args)))) {
		return NULL;
	}

	if (x_obj_isnil(p_base, x_obj_type(p_obj))) {
		return NULL;
	}

	p_call = x_type_field_call(x_obj_type(p_obj));

	if (x_obj_isnil(p_base, p_call)) {
		return NULL;
	}

	if (x_obj_type_isprocedure(p_base, p_call)) {
		x_spair_t closure_args = x_obj_set(NULL, X_OBJ_FLAG_NONE,
			{ p_call }, { p_args });
		return x_type_procedure_call(p_base, (x_obj_t *)&closure_args);
	}

	if (x_obj_isnil(p_base, x_atomobj(p_call))) {
		return NULL;
	}

	return (*x_atomfn(p_call))(p_base, p_args);
}
