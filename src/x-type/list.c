/*
 * # Computational Expressions in C
 *
 * ## x-type/list.c -- Implementation - Type - List
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
#include "x-type/list.h"
#include "x-type/iter.h"
#include "x-sexp/list.h"

x_satom_t x_type_list_name = x_obj_set(x_type_pair_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_LIST_NAME }),
	x_type_list_struct_prim = x_obj_set(x_type_pair_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_list_struct }),
	x_type_list_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_list_make }),
	x_type_list_eval_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_list_eval }),
	x_type_list_iter_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_type_list_iter });

x_obj_t *x_make_list(x_obj_t *p_base, x_obj_flag_t flags, void *p1, void *p2)
{
	x_satom_t o_flags = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = flags });
	x_spair_t o_list = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .v = p1 }, { .v = p2 }),
		args[2] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_list }, { (x_obj_t *)(args + 1) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_flags }, { NULL })
		};

	return x_type_list_make(p_base, (x_obj_t *)args);
}

x_obj_t *x_type_list_struct(x_obj_t *p_base, x_obj_t *p_args)
{
	struct x_type_t type = {
		.p_name = x_type_list_name,
		.p_make = x_type_list_make_prim,
		.p_eval = x_type_list_eval_prim,
		.p_analyse = x_sexp_list_analyse_prim,
		.p_delimit = x_sexp_list_delimit_prim,
		.p_write = x_sexp_list_write_prim
	};

	return x_type_struct_make(p_base, type);
}

x_obj_t *x_type_list_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_list_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_list_struct_prim }, { NULL })
	};

	return x_type_struct_get(p_base, (x_obj_t *)args);
}

x_obj_t *x_type_list_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = x_type_list_register(p_base, p_base),
		*p_list = x_0(p_args);
	x_obj_flag_t flags = x_obj_isnil(p_base, x_1(p_args))
		? 0 : x_firstint(x_01(p_args));

	return x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_PAIR, x_0(p_list), x_1(p_list));
}

x_obj_t *x_type_list_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_exp = x_firstobj(x_eval_arg_exp(p_args));
	x_spair_t prim_args = x_obj_set(NULL, X_OBJ_FLAG_NONE,
			{ x_type_field_call(x_obj_type(x_firstobj(p_exp))) }, { p_exp });

	if (x_obj_isnil(p_base, x_firstobj((x_obj_t *)prim_args))) {
		return p_exp;
	}

	return x_type_prim_call(p_base, (x_obj_t *)prim_args);
}

x_obj_t *x_type_list_iter(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_iter = x_firstobj(p_args),
		*p_obj = x_firstobj(x_iterval(p_iter));

	if ( ! x_obj_isnil(p_base, x_iterval(p_iter))) {
		x_iterval(p_iter) = x_restobj(x_iterval(p_iter));
	}

	return p_obj;
}
