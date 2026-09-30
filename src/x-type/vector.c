/** @file x-type/vector.c
 *  @brief Vector type -- construction and registration.
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2026 Jon Ruttan
 *  @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */

#include <stdarg.h>

#include "x-type/vector.h"
#include "x-type/int.h"
#include "x-eval.h"

x_satom_t x_type_vector_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_VECTOR_NAME }),
	x_type_vector_length_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_vector_length }),
	x_type_vector_struct_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_vector_struct });

/** The type's unit count: a bare count, so every unit is a reference. */
static x_satom_t x_type_vector_units_obj =
	x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = X_TYPE_VECTOR_UNITS });

/**
 * Allocate a VECTOR object holding the objects that follow.
 *
 * The length is an INTEGER object of its own, as the length of a vector
 * x-lang makes is, so the two are read the same way.
 *
 * @param p_base  Base (execution context).
 * @param flags   Object flags.
 * @param length  The number of elements.
 * @param ...     The elements, @p length of them, each an `x_obj_t *`.
 * @return Newly allocated VECTOR object.
 */
x_obj_t *x_make_vector(x_obj_t *p_base, x_obj_flag_t flags, x_int_t length, ...)
{
	x_obj_t *p_type = x_type_vector_register(p_base, p_base),
		*p_obj;
	x_int_t i;
	va_list ap;

	p_obj = x_obj_alloc(p_base, p_type, flags, (size_t)x_vector_units(length));

	/* Every unit holds an object or nil before the length is allocated:
	 * the elements are stored first, and the length unit is nil until its
	 * object exists. */
	x_vectorlengthobj(p_obj) = NULL;

	va_start(ap, length);

	for (i = 0; i < length; i++) {
		x_vectorobj(p_obj, i) = va_arg(ap, x_obj_t *);
	}

	va_end(ap);

	x_vectorlengthobj(p_obj) = x_mkint(p_base, length);

	return p_obj;
}

/**
 * Type-system length handler for VECTOR objects.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Argument list: (vector . ...).
 * @return The vector's length object.
 */
x_obj_t *x_type_vector_length(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_vectorlengthobj(x_firstobj(p_args));
}

/**
 * Build the VECTOR type struct descriptor.
 *
 * Populates the name, the unit count and the length handler for the type
 * system. With no save handler of its own the type takes the default,
 * which writes the units the count declares: the length, then the
 * elements, each a reference.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unused.
 * @return Type struct pair-tree for VECTOR.
 */
x_obj_t *x_type_vector_struct(x_obj_t *p_base, x_obj_t *p_args)
{
	struct x_type_t type = {
		.p_name = x_type_vector_name,
		.p_units = (x_obj_t *)&x_type_vector_units_obj,
		.p_length = x_type_vector_length_prim
	};

	return x_type_struct_make(p_base, type);
}

/**
 * Register (or retrieve) the VECTOR type in the type alist.
 *
 * Calls x_type_struct_get() with the VECTOR name and struct constructor,
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unused.
 * @return The registered VECTOR type object.
 */
x_obj_t *x_type_vector_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type;
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_vector_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_vector_struct_prim }, { NULL })
	};

	p_type = x_type_struct_get(p_base, (x_obj_t *)args);

	return p_type;
}
