#ifndef X_TYPE_COMMENT_H
#define X_TYPE_COMMENT_H

/*
 * # Computational Expressions in C
 *
 * ## x-type/comment.h -- Header - Type - Comment
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

#define X_TYPE_COMMENT_NAME		"COMMENT"

/*
 * # Macros
 */
#define x_obj_type_iscomment(B,X)	x_obj_is_type((B), (X), X_TYPE_COMMENT_NAME)

#define x_commentval(X)				x_firststr((X))
#define x_commentlen(X)				x_lib_strlen(x_strval((X)))

/*
 * # Data Structures
 */
x_obj_t *x_type_comment_struct(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_comment_register(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_COMMENT_H */
