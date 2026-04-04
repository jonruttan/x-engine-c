#ifndef X_TYPE_WHITESPACE_H
#define X_TYPE_WHITESPACE_H

/**
 * @file x-type/whitespace.h
 * @brief Whitespace token type for the tokenizer.
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

#define X_TYPE_WHITESPACE_NAME		"WHITESPACE" /**< Type name string. */

/** @name Type predicates */
/** @{ */
#define x_obj_type_iswhitespace(B,X)	x_obj_is_type((B), (X), X_TYPE_WHITESPACE_NAME) /**< Test if object is whitespace. */
/** @} */

/** @name Field accessors */
/** @{ */
#define x_whitespaceval(X)				x_firststr((X))                 /**< Whitespace text as C string. */
#define x_whitespacelen(X)				x_lib_strlen(x_strval((X)))      /**< Length of whitespace text. */
/** @} */

/** Build the WHITESPACE type struct descriptor. */
x_obj_t *x_type_whitespace_struct(x_obj_t *p_base, x_obj_t *p_args);
/** Register (or retrieve) the WHITESPACE type struct on p_base. */
x_obj_t *x_type_whitespace_register(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_WHITESPACE_H */
