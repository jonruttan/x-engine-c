#ifndef X_TYPE_CHAR_H
#define X_TYPE_CHAR_H

/**
 * @file char.h
 * @brief Character type for the x-lang type system.
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

#ifndef X_TYPE_CHAR_NAME
#define X_TYPE_CHAR_NAME	"CHARACTER"	/**< Type-system symbol name */
#endif /* X_TYPE_CHAR_NAME */

/*
 * # Macros
 */
/** Test whether object X is a character on base B. */
#define x_obj_type_ischar(B,X)	x_obj_is_type((B), (X), X_TYPE_CHAR_NAME)

/** Extract the character value from a character object. */
#define x_charval(X)			x_firstchar((X))

/** Make a character with default flags. */
#define x_mkchar(B, C)			x_make_char((B), X_OBJ_FLAG_NONE, (C))
/** Make a character with explicit flags F. */
#define x_mkfchar(B, F, C)		x_make_char((B), (F), (C))

/*
 * # Data Structures
 */
/** Allocate a heap character object with value c. */
x_obj_t *x_make_char(x_obj_t *p_base, x_obj_flag_t flags, x_char_t c);

/** Build the character type descriptor struct. */
x_obj_t *x_type_char_struct(x_obj_t *p_base, x_obj_t *p_args);
/** Register (or retrieve) the character type struct on p_base. */
x_obj_t *x_type_char_register(x_obj_t *p_base, x_obj_t *p_args);
/** Type-system make callback for character. */
x_obj_t *x_type_char_make(x_obj_t *p_base, x_obj_t *p_args);
/** Tokenizer analyse callback (phase 1) for character literals. */
x_obj_t *x_type_char_analyse1(x_obj_t *p_base, x_obj_t *p_args);
/** Tokenizer analyse callback (phase 2) for character literals. */
x_obj_t *x_type_char_analyse2(x_obj_t *p_base, x_obj_t *p_args);
/** Tokenizer read callback for character literals. */
x_obj_t *x_type_char_read(x_obj_t *p_base, x_obj_t *p_args);
/** Type-system write callback for character objects. */
x_obj_t *x_type_char_write(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_char_write(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_CHAR_H */
