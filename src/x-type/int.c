/**
 * @file int.c
 * @brief Integer type implementation.
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2021 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
/*
 * # Includes
 */
#include <ctype.h>
#include "x-type/int.h"
#include "x-token/sexp/int.h"

x_satom_t x_type_int_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_INT_NAME }),
	x_type_int_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_int_make }),
	x_type_int_struct_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_int_struct });

/**
 * Allocate a heap integer object with a given value.
 *
 * Builds a stack-based argument list and delegates to x_type_int_make.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param flags   x_obj_flag_t -- Object flags
 * @param i       x_int_t -- Integer value
 * @return x_obj_t* -- New heap-allocated integer object
 */
x_obj_t *x_make_int(x_obj_t *p_base, x_obj_flag_t flags, x_int_t i)
{
	x_satom_t o_int = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = i }),
		o_flags = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = flags });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_int }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_flags }, { NULL })
	};

	return x_type_int_make(p_base, (x_obj_t *)args);
}

/**
 * Build the integer type descriptor struct.
 *
 * Populates name, make, analyse, read, and write callbacks.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_obj   x_obj_t* -- Unused
 * @return x_obj_t* -- Type descriptor pair list
 */
x_obj_t *x_type_int_struct(x_obj_t *p_base, x_obj_t *p_obj)
{
	struct x_type_t type = {
		.p_name = x_type_int_name,
		.p_make = x_type_int_make_prim,
		.p_analyse = x_sexp_int_analyse_sign_prim,
		.p_read = x_sexp_int_read_prim,
		.p_write = x_sexp_int_write_prim
	};

	return x_type_struct_make(p_base, type);
}

/**
 * Register or retrieve the integer type on the base context.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Unused
 * @return x_obj_t* -- Registered type object
 */
x_obj_t *x_type_int_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_int_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_int_struct_prim }, { NULL })
	};

	return x_type_struct_get(p_base, (x_obj_t *)args);
}

/**
 * Type-system make callback for integer objects.
 *
 * Extracts the integer value and optional flags from p_args,
 * then allocates a heap object via x_obj_make.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- (int-value . (flags | nil))
 * @return x_obj_t* -- New heap-allocated integer object
 */
x_obj_t *x_type_int_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = x_type_int_register(p_base, p_base),
		*p_int = x_0(p_args);
	x_obj_flag_t flags = x_obj_isnil(p_base, x_1(p_args))
		? 0 : x_firstint(x_01(p_args));

	return x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_ATOM, x_intval(p_int));
}
