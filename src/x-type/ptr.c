/*
 * # Computational Expressions in C
 *
 * ## x-type/ptr.c -- Implementation - Type - Pointer
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
#include "x-type/ptr.h"

x_satom_t x_type_ptr_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_PTR_NAME }),
	x_type_ptr_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_ptr_make }),
	x_type_ptr_struct_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_ptr_struct });

x_obj_t *x_make_ptr(x_obj_t *p_base, x_obj_flag_t flags, void *p)
{
	x_satom_t o_ptr = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .v = p }),
		o_flags = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = flags });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_ptr }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_flags }, { NULL })
	};

	return x_type_ptr_make(p_base, (x_obj_t *)args);
}

x_obj_t *x_type_ptr_struct(x_obj_t *p_base, x_obj_t *p_args)
{
	struct x_type_t type = {
		.p_name = x_type_ptr_name,
		.p_make = x_type_ptr_make_prim
	};

	return x_type_struct_make(p_base, type);
}

x_obj_t *x_type_ptr_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_ptr_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_ptr_struct_prim }, { NULL })
	};

	return x_type_struct_get(p_base, (x_obj_t *)args);
}

x_obj_t *x_type_ptr_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = x_type_ptr_register(p_base, p_base),
		*p_atom = x_0(p_args);
	x_obj_flag_t flags = x_obj_isnil(p_base, x_1(p_args))
		? 0 : x_firstint(x_01(p_args));

	return x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_ATOM, x_ptrval(p_atom));
}
