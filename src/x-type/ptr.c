/** @file x-type/ptr.c
 *  @brief Opaque pointer type -- construction and registration.
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2021 Jon Ruttan
 *  @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */

#include "x-type/ptr.h"
#include "x-eval.h"

x_satom_t x_type_ptr_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_PTR_NAME }),
	x_type_ptr_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_ptr_make }),
	x_type_ptr_struct_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_ptr_struct });

/**
 * Allocate a POINTER object wrapping a raw void pointer.
 *
 * Packs @p p and @p flags into stack-allocated args and delegates to
 * x_type_ptr_make() for type-system allocation.
 *
 * @param p_base  Base (execution context).
 * @param flags   Object flags (e.g. @c X_OBJ_FLAG_OWN).
 * @param p       Raw pointer to wrap.
 * @return Newly allocated POINTER object.
 */
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

/**
 * Build the POINTER type struct descriptor.
 *
 * Populates name and make hooks for the type system.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unused.
 * @return Type struct pair-tree for POINTER.
 */
x_obj_t *x_type_ptr_struct(x_obj_t *p_base, x_obj_t *p_args)
{
	struct x_type_t type = {
		.p_name = x_type_ptr_name,
		.p_make = x_type_ptr_make_prim
	};

	return x_type_struct_make(p_base, type);
}

/**
 * Register (or retrieve) the POINTER type in the type alist.
 *
 * Calls x_type_struct_get() with the POINTER name and struct constructor.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unused.
 * @return The registered POINTER type object.
 */
x_obj_t *x_type_ptr_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_ptr_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_ptr_struct_prim }, { NULL })
	};

	return x_type_struct_get(p_base, (x_obj_t *)args);
}

/**
 * Type-system make handler for POINTER objects.
 *
 * Extracts the raw pointer from @c p_args[0] and optional flags from
 * @c p_args[1], then allocates via x_obj_make().
 *
 * @param p_base  Base (execution context).
 * @param p_args  Argument list: (pointer-atom [flags-atom]).
 * @return Newly allocated POINTER object.
 */
x_obj_t *x_type_ptr_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = x_type_ptr_register(p_base, p_base),
		*p_atom = x_0(p_args);
	x_obj_flag_t flags = x_obj_isnil(p_base, x_1(p_args))
		? 0 : x_firstint(x_01(p_args));

	return x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_ATOM, x_ptrval(p_atom));
}
