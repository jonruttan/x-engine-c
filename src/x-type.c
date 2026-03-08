/*
 * # Computational Expressions in C
 *
 * ## x-type.c -- Implementation - Type
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
#include "x-type.h"
#include "x-base.h"
#include "x-obj.h"
#include "x-type/prim.h"

#define nil			p_base
#define pair(X,Y)	(x_mkspair(p_base, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, (X)))

x_obj_t *x_type_struct_make(x_obj_t *p_base, struct x_type_t type)
{
	x_obj_t *p_type =
		/* name */
		pair(type.p_name,
		/* data */
		pair(type.p_data,
		/* Heap: '(make free clone units length) */
		pair(pair(type.p_make,
			pair(type.p_free,
			pair(type.p_clone,
			pair(type.p_units,
			pair(type.p_length,
			nil))))),
		/* Proc: '(call eval convert) */
		pair(pair(type.p_call,
			pair(type.p_eval,
			pair(type.p_convert,
			nil))),
		/* IO: '(analyse delimit write) */
		pair(pair(type.p_analyse,
			pair(type.p_delimit,
			pair(type.p_write,
			nil))),
		nil)))));

	return p_type;
}

#undef nil
#undef pair
#undef atom

x_obj_t *x_type_struct_get(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = NULL;

	if (x_base_isset(p_base)) {
		p_type = x_base_type_alist_assoc(p_base, p_args);
	}

	/* TODO: GC on exit, with and w/o GC structures. */
	if (x_obj_isnil(p_base, p_type)) {
		p_type = x_type_prim_call(p_base, x_restobj(p_args));

		if (x_base_isset(p_base)) {
			x_base_type_alist_extend(p_base, p_type);
		}
	}

	return p_type;
}

x_obj_t *x_type_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args),
		*p_fn = x_type_field_write(x_obj_type(p_obj));
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_fn }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_obj }, { NULL })
	};

	if ( ! x_obj_isnil(p_base, p_fn)) {
		/* Use prim_apply (no TCO) so all body forms execute
		 * before returning to C. prim_call sets the last form
		 * as a TCO expr that nobody would process here. */
		return x_type_prim_apply(p_base, (x_obj_t *)args);
	}

	return p_base;
}

