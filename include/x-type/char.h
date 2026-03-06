#ifndef X_TYPE_CHAR_H
#define X_TYPE_CHAR_H

/*
 * # Computational Expressions in C
 *
 * ## x-type/char.h -- Header - Type - Character
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

#ifndef X_TYPE_CHAR_NAME
#define X_TYPE_CHAR_NAME	"CHARACTER"
#endif /* X_TYPE_CHAR_NAME */

/*
 * # Macros
 */
#define x_obj_type_ischar(B,X)	x_obj_is_type((B), (X), X_TYPE_CHAR_NAME)

#define x_charval(X)			x_firstchar((X))

#define x_mkchar(B, C)			x_make_char((B), X_OBJ_FLAG_NONE, (C))
#define x_mkfchar(B, F, C)		x_make_char((B), (F), (C))

/*
 * # Data Structures
 */
x_obj_t *x_make_char(x_obj_t *p_base, x_obj_flag_t flags, x_char_t c);

x_obj_t *x_type_char_struct(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_char_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_char_make(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_char_analyse1(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_char_analyse2(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_char_read(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_char_write(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_char_write(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_CHAR_H */
