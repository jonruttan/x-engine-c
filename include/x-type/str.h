#ifndef X_TYPE_STR_H
#define X_TYPE_STR_H

/*
 * # Computational Expressions in C
 *
 * ## x-type/str.h -- Header - Type - String
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

#define X_TYPE_STR_NAME		"STRING"

/*
 * # Macros
 */
#define x_obj_type_isstr(B,X)	x_obj_is_type((B), (X), X_TYPE_STR_NAME)

#define x_strval(X)				x_firststr((X))
#define x_strlen(X)				x_lib_strlen(x_strval((X)))

#define x_mkstr(B, S)			x_make_str((B), X_OBJ_FLAG_NONE, (S))
#define x_mkfstr(B, F, S)		x_make_str((B), (F), (S))
#define x_mkstrown(B, S)		x_make_str((B), X_OBJ_FLAG_OWN, (S))
#define x_mkfstrown(B, F, S)	x_make_str((B), X_OBJ_FLAG_OWN | (F), (S))

/*
 * # Data Structures
 */
x_obj_t *x_make_str(x_obj_t *p_base, x_obj_flag_t flags, x_char_t *s);
x_obj_t *x_type_str_struct(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_str_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_str_make(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_str_length(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_str_read(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_str_write(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_str_call(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_str_proc(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_STR_H */
