/*
 * # Computational Expressions in C
 *
 * ## x-type/iter.c -- Implementation - Type - Iter
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
#include "x-type/iter.h"
#include "x-obj.h"
#include "x-base.h"
#include "x-type/prim.h"

x_satom_t x_type_iter_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_ITER_NAME }),
	x_type_iter_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_iter_make }),
	x_type_iter_write_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_iter_write }),
	x_type_iter_struct_prim = x_obj_set(x_type_pair_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_iter_struct });

x_obj_t *x_make_iter(x_obj_t *p_base, x_obj_flag_t flags, void *p1, void *p2)
{
	x_satom_t o_flags = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = flags });
	x_spair_t o_iter = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .v = p1 }, { .v = p2 }),
		args[2] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_iter }, { (x_obj_t *)(args + 1) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_flags }, { NULL })
		};

	return x_type_iter_make(p_base, (x_obj_t *)args);
}

x_obj_t *x_type_iter_struct(x_obj_t *p_base, x_obj_t *p_args)
{
	struct x_type_t type = {
		.p_name = x_type_iter_name,
		.p_make = x_type_iter_make_prim,
		.p_write = x_type_iter_write_prim
	};

	return x_type_struct_make(p_base, type);
}

x_obj_t *x_type_iter_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_iter_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_iter_struct_prim }, { NULL })
	};

	return x_type_struct_get(p_base, (x_obj_t *)args);
}

x_obj_t *x_type_iter_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = x_type_iter_register(p_base, p_base);
	x_obj_flag_t flags = x_obj_isnil(p_base, x_1(p_args))
		? 0 : x_firstint(x_01(p_args));

	return x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_PAIR, x_00(p_args), x_10(p_args));
}

x_obj_t *x_type_iter_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_satom_t str = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .s = (x_char_t *)X_TYPE_ITER_WRITE_STR });
	x_spair_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { str }, { NULL });

	x_base_write_str(p_base, (x_obj_t *)&wrap);

	return x_firstobj(p_args);
}

x_obj_t *x_type_iter_isempty(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_iterempty(p_base, x_firstobj(p_args)) ? p_base : p_args;
}

x_obj_t *x_type_iter_next(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_iter = x_firstobj(p_args);
	x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_iterprim(p_iter) }, { p_args });

	return x_callable_call(p_base, (x_obj_t *)args);
}
