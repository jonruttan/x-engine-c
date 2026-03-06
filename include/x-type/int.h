#ifndef X_TYPE_INT_H
#define X_TYPE_INT_H

/*
 * # Computational Expressions in C
 *
 * ## x-type/int.h -- Header - Type - Integer
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

#ifndef X_TYPE_INT_NAME
#define X_TYPE_INT_NAME		"INTEGER"
#endif /* X_TYPE_INT_NAME */

/*
 * # Macros
 */
#define x_obj_type_isint(B,X)	x_obj_is_type((B), (X), X_TYPE_INT_NAME)

#define x_intval(X)				x_firstint((X))

#define x_mkint(B, I)			x_make_int((B), X_OBJ_FLAG_NONE, (I))
#define x_mkfint(B, F, I)		x_make_int((B), (F), (I))

/*
 * # Data Structures
 */
x_obj_t *x_make_int(x_obj_t *p_base, x_obj_flag_t flags, x_int_t i);

x_obj_t *x_type_int_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_int_struct(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_int_make(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_INT_H */
