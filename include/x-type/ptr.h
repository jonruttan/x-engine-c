#ifndef X_TYPE_PTR_H
#define X_TYPE_PTR_H

/*
 * # Computational Expressions in C
 *
 * ## x-type/ptr.h -- Header - Type - Pointer
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

#define X_TYPE_PTR_NAME		"POINTER"

/*
 * # Macros
 */
#define x_obj_type_isptr(B,X)	x_obj_is_type((B), (X), X_TYPE_PTR_NAME)

#define x_ptrval(X)				x_firstptr((X))

#define x_mkptr(B, P)			x_make_ptr((B), X_OBJ_FLAG_NONE, (P))
#define x_mkfptr(B, F, P)		x_make_ptr((B), (F), (P))
#define x_mkptrown(B, P)		x_make_ptr((B), X_OBJ_FLAG_OWN, (P))
#define x_mkfptrown(B, F, P)	x_make_ptr((B), X_OBJ_FLAG_OWN | (F), (P))

/*
 * # Data Structures
 */
x_obj_t *x_make_ptr(x_obj_t *p_base, x_obj_flag_t flags, void *p);

x_obj_t *x_type_ptr_struct(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_ptr_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_ptr_make(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_PTR_H */
