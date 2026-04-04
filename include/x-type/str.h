#ifndef X_TYPE_STR_H
#define X_TYPE_STR_H

/**
 * @file str.h
 * @brief String type for the x-lang type system.
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

#define X_TYPE_STR_NAME		"STRING"		/**< Type-system symbol name */

/*
 * # Macros
 */
/** Test whether object X is a string on base B. */
#define x_obj_type_isstr(B,X)	x_obj_is_type((B), (X), X_TYPE_STR_NAME)

/** Extract the C string pointer from a string object. */
#define x_strval(X)				x_firststr((X))
/** Compute the length of a string object. */
#define x_strlen(X)				x_lib_strlen(x_strval((X)))

/** Make a string with default flags (non-owning). */
#define x_mkstr(B, S)			x_make_str((B), X_OBJ_FLAG_NONE, (S))
/** Make a string with explicit flags F. */
#define x_mkfstr(B, F, S)		x_make_str((B), (F), (S))
/** Make a string that owns its buffer (will be freed by GC). */
#define x_mkstrown(B, S)		x_make_str((B), X_OBJ_FLAG_OWN, (S))
/** Make an owning string with additional flags F. */
#define x_mkfstrown(B, F, S)	x_make_str((B), X_OBJ_FLAG_OWN | (F), (S))

/*
 * # Data Structures
 */
/** Allocate a heap string object wrapping C string s. */
x_obj_t *x_make_str(x_obj_t *p_base, x_obj_flag_t flags, x_char_t *s);
/** Build the string type descriptor struct. */
x_obj_t *x_type_str_struct(x_obj_t *p_base, x_obj_t *p_args);
/** Register (or retrieve) the string type struct on p_base. */
x_obj_t *x_type_str_register(x_obj_t *p_base, x_obj_t *p_args);
/** Type-system make callback for string. */
x_obj_t *x_type_str_make(x_obj_t *p_base, x_obj_t *p_args);
/** Type-system length callback for string. */
x_obj_t *x_type_str_length(x_obj_t *p_base, x_obj_t *p_args);
/** Tokenizer read callback for string literals. */
x_obj_t *x_type_str_read(x_obj_t *p_base, x_obj_t *p_args);
/** Type-system write callback for string (quoted output). */
x_obj_t *x_type_str_write(x_obj_t *p_base, x_obj_t *p_args);
/** Type-system call callback -- index/slice into the string. */
x_obj_t *x_type_str_call(x_obj_t *p_base, x_obj_t *p_args);
/** String procedure callback. */
x_obj_t *x_type_str_proc(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_STR_H */
