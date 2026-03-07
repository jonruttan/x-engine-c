/*
 * # Computational Expressions in C
 *
 * ## x-type.c -- Implementation - Type - Primitive
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
#include "x-type/prim.h"
#include "x-base.h"

x_satom_t x_type_prim_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_PRIM_NAME }),
	x_type_prim_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_prim_make }),
	x_type_prim_call_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_prim_call }),
	x_type_prim_write_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_prim_write }),
	x_type_prim_struct_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_prim_struct });

x_obj_t *x_make_prim(x_obj_t *p_base, x_obj_flag_t flags, x_prim_fn fn)
{
	x_satom_t prim = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = fn }),
		flags_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = flags });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { prim }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { flags_obj }, { NULL })
	};

	return x_type_prim_make(p_base, (x_obj_t *)args);
}

x_obj_t *x_type_prim_struct(x_obj_t *p_base, x_obj_t *p_obj)
{
	struct x_type_t type = {
		.p_name = x_type_prim_name,
		.p_make = x_type_prim_make_prim,
		.p_call = x_type_prim_call_prim,
		.p_write = x_type_prim_write_prim
	};

	return x_type_struct_make(p_base, type);
}

x_obj_t *x_type_prim_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_prim_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_prim_struct_prim }, { NULL })
	};

	return x_type_struct_get(p_base, (x_obj_t *)args);
}

x_obj_t *x_type_prim_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = x_type_prim_register(p_base, p_base),
		*p_prim = x_0(p_args);
	x_obj_flag_t flags = x_obj_isnil(p_base, x_1(p_args))
		? 0 : x_firstint(x_01(p_args));

	return x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_ATOM, x_atomfn(p_prim));
}

x_obj_t *x_type_prim_call(x_obj_t *p_base, x_obj_t *p_args)
{
	return (*x_primval(x_firstobj(p_args)))(p_base, x_restobj(p_args));
}

x_obj_t *x_type_prim_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_satom_t buffer = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_PRIM_WRITE_STR }),
		size = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .i = X_TYPE_PRIM_WRITE_LEN });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { buffer }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { size }, { NULL })
	};

	x_base_write(p_base, (x_obj_t *)args);

	return x_firstobj(p_args);
}
