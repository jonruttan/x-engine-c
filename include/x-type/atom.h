#ifndef X_TYPE_ATOM_H
#define X_TYPE_ATOM_H

/*
 * # Computational Expressions in C
 *
 * ## x-obj.h -- Header - Type - Atom
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

#ifndef X_TYPE_ATOM_NAME
#define X_TYPE_ATOM_NAME		"ATOM"
#endif /* X_TYPE_ATOM_NAME */

/*
 * # Macros
 */
#define x_obj_type_isatom(B,X)		x_obj_is_type((B), (X), X_TYPE_ATOM_NAME)

#define x_atomval(X)				x_atomptr(X)

#define x_mkatom(B,P)				x_make_atom((B), X_OBJ_FLAG_NONE, (P))
#define x_mkfatom(B,F,P)			x_make_atom((B), (F), (P))

/*
 * # Data Structures
 */
extern x_satom_t x_type_atom_name,
	x_type_atom_make_prim,
	x_type_atom_call_prim,
	x_type_atom_struct_prim;

x_obj_t *x_make_atom(x_obj_t *p_base, x_obj_flag_t flags, void *p);

x_obj_t *x_type_atom_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_atom_struct(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_atom_make(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_atom_call(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_ATOM_H */
