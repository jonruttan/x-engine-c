/**
 * @file atom.c
 * @brief Atom type implementation -- opaque pointer wrapper.
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2023 Jon Ruttan
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
#include "x-type/atom.h"
#include "x-type/int.h"
#include "x-obj.h"
#include "x-interp.h"

x_satom_t x_type_atom_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_ATOM_SYMBOL }),
	x_type_atom_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_atom_make }),
	x_type_atom_write_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_atom_write }),
	x_type_atom_struct_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_atom_struct });

/**
 * Allocate a heap atom wrapping an opaque pointer.
 *
 * Builds a stack-based argument list and delegates to x_type_atom_make.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param flags   x_obj_flag_t -- Object flags (e.g. X_OBJ_FLAG_NONE)
 * @param p       void* -- Raw pointer to wrap
 * @return x_obj_t* -- New heap-allocated atom object
 */
x_obj_t *x_make_atom(x_obj_t *p_base, x_obj_flag_t flags, void *p)
{
	x_satom_t o_atom = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .v = p }),
		o_flags = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = flags });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_atom }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_flags }, { NULL })
	};

	return x_type_atom_make(p_base, (x_obj_t *)args);
}

/**
 * Build the atom type descriptor struct.
 *
 * Populates name, make, and write callbacks for the atom type.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Unused
 * @return x_obj_t* -- Type descriptor pair list
 */
x_obj_t *x_type_atom_struct(x_obj_t *p_base, x_obj_t *p_args)
{
	struct x_type_t type = {
		.p_name = x_type_atom_name,
		.p_make = x_type_atom_make_prim,
		.p_write = x_type_atom_write_prim
	};

	return x_type_struct_make(p_base, type);
}

/**
 * Register or retrieve the atom type on the base context.
 *
 * Calls x_type_struct_get with the atom name and struct builder.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Unused
 * @return x_obj_t* -- Registered type object
 */
x_obj_t *x_type_atom_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_atom_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_atom_struct_prim }, { NULL })
	};

	return x_type_struct_get(p_base, (x_obj_t *)args);
}

/**
 * Type-system make callback for atom objects.
 *
 * Extracts the pointer value and optional flags from p_args,
 * then allocates a heap object via x_obj_make.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- (atom-value . (flags | nil))
 * @return x_obj_t* -- New heap-allocated atom object
 */
x_obj_t *x_type_atom_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = x_type_atom_register(p_base, p_base),
		*p_atom = x_0(p_args);
	x_obj_flag_t flags = x_obj_isnil(p_base, x_1(p_args))
		? 0 : x_firstint(x_01(p_args));

	return x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_ATOM, x_atomval(p_atom));
}

/**
 * Type-system write callback -- prints "#<atom>" to the output.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- (atom-object . ...)
 * @return x_obj_t* -- The atom object (first arg)
 */
x_obj_t *x_type_atom_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_satom_t str = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .s = (x_char_t *)X_TYPE_ATOM_WRITE_STR });
	x_spair_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { str }, { NULL });

	x_interp_write_str(p_base, (x_obj_t *)&wrap);

	return x_firstobj(p_args);
}
