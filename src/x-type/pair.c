/*
 * # Computational Expressions in C
 *
 * ## x-type/pair.c -- Implementation - Type - Pair
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
#include "x-type/pair.h"
#include "x-sexp/pair.h"

x_satom_t x_type_pair_name = x_obj_set(x_type_pair_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_PAIR_NAME }),
	x_type_pair_length_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_pair_length }),
	x_type_pair_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_pair_make }),
	x_type_pair_struct_prim = x_obj_set(x_type_pair_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_pair_struct });

x_obj_t *x_make_pair(x_obj_t *p_base, x_obj_flag_t flags, void *p1, void *p2)
{
	x_satom_t o_flags = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = flags });
	x_spair_t o_pair = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .v = p1 }, { .v = p2 }),
		args[2] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_pair }, { (x_obj_t *)(args + 1) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_flags }, { NULL })
		};

	return x_type_pair_make(p_base, (x_obj_t *)args);
}

x_obj_t *x_type_pair_length(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args);
	x_int_t len = 0;

	while (! x_obj_isnil(p_base, p_obj)) {
		len++;
		p_obj = x_restobj(p_obj);
	}

	return x_mksatom(p_base, len);
}

x_obj_t *x_type_pair_struct(x_obj_t *p_base, x_obj_t *p_args)
{
	struct x_type_t type = {
		.p_name = x_type_pair_name,
		.p_make = x_type_pair_make_prim,
		.p_length = x_type_pair_length_prim,
		.p_write = x_sexp_pair_write_prim
	};

	return x_type_struct_make(p_base, type);
}

x_obj_t *x_type_pair_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_pair_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_pair_struct_prim }, { NULL })
	};

	return x_type_struct_get(p_base, (x_obj_t *)args);
}

x_obj_t *x_type_pair_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = x_type_pair_register(p_base, p_base),
		*p_pair = x_0(p_args);
	x_obj_flag_t flags = x_obj_isnil(p_base, x_1(p_args))
		? 0 : x_firstint(x_01(p_args));

	return x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_PAIR, x_0(p_pair), x_1(p_pair));
}
