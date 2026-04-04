#ifndef X_TYPE_COMMENT_H
#define X_TYPE_COMMENT_H

/**
 * @file x-type/comment.h
 * @brief Comment token type for the tokenizer.
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2022 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
#include "x-type.h"

#define X_TYPE_COMMENT_NAME		"COMMENT" /**< Type name string. */

/** @name Type predicates */
/** @{ */
#define x_obj_type_iscomment(B,X)	x_obj_is_type((B), (X), X_TYPE_COMMENT_NAME) /**< Test if object is a comment. */
/** @} */

/** @name Field accessors */
/** @{ */
#define x_commentval(X)				x_firststr((X))                 /**< Comment text as C string. */
#define x_commentlen(X)				x_lib_strlen(x_strval((X)))      /**< Length of comment text. */
/** @} */

/** Build the COMMENT type struct descriptor. */
x_obj_t *x_type_comment_struct(x_obj_t *p_base, x_obj_t *p_args);
/** Register (or retrieve) the COMMENT type struct on p_base. */
x_obj_t *x_type_comment_register(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_COMMENT_H */
