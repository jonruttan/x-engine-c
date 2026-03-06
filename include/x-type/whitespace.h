#ifndef X_TYPE_WHITESPACE_H
#define X_TYPE_WHITESPACE_H

/*
 * # Computational Expressions in C
 *
 * ## x-type/whitespace.h -- Header - Type - Whitespace
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2022 Jon Ruttan
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

#define X_TYPE_WHITESPACE_NAME		"WHITESPACE"

/*
 * # Macros
 */
#define x_obj_type_iswhitespace(B,X)	x_obj_is_type((B), (X), X_TYPE_WHITESPACE_NAME)

#define x_whitespaceval(X)				x_firststr((X))
#define x_whitespacelen(X)				x_lib_strlen(x_strval((X)))

/*
 * # Data Structures
 */
x_obj_t *x_type_whitespace_struct(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_whitespace_register(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_WHITESPACE_H */
