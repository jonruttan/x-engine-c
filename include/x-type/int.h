#ifndef X_TYPE_INT_H
#define X_TYPE_INT_H

/**
 * @file int.h
 * @brief Integer type for the x-lang type system.
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
#include "x-type.h"

#ifndef X_TYPE_INT_NAME
#define X_TYPE_INT_NAME		"INTEGER"	/**< Type-system symbol name */
#endif /* X_TYPE_INT_NAME */

/*
 * # Macros
 */
/** Test whether object X is an integer on base B. */
#define x_obj_type_isint(B,X)	x_obj_is_type((B), (X), X_TYPE_INT_NAME)

/** Extract the integer value from an integer object. */
#define x_intval(X)				x_firstint((X))

/** Make an integer with default flags. */
#define x_mkint(B, I)			x_make_int((B), X_OBJ_FLAG_NONE, (I))
/** Make an integer with explicit flags F. */
#define x_mkfint(B, F, I)		x_make_int((B), (F), (I))

/*
 * # Data Structures
 */
/** Allocate a heap integer object with value i. */
x_obj_t *x_make_int(x_obj_t *p_base, x_obj_flag_t flags, x_int_t i);

/** Register (or retrieve) the integer type struct on p_base. */
x_obj_t *x_type_int_register(x_obj_t *p_base, x_obj_t *p_args);
/** Build the integer type descriptor struct. */
x_obj_t *x_type_int_struct(x_obj_t *p_base, x_obj_t *p_args);
/** Type-system make callback for integer. */
x_obj_t *x_type_int_make(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_INT_H */
