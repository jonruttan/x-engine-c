#ifndef X_SEXP_CHAR_H
#define X_SEXP_CHAR_H

/**
 * @file char.h
 * @brief S-expression analyser, reader, and writer for character literals.
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2023 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */

#include "x-obj.h"

#ifndef X_SEXP_CHAR_PRE_STR
#define X_SEXP_CHAR_PRE_STR		"#\\" /**< Character literal prefix. */
#endif /* X_SEXP_CHAR_PRE */

/** @name Analyser state primitives (spair -- composable state machines). */
/** @{ */
extern x_spair_t x_sexp_char_analyse1_prim,
	x_sexp_char_analyse2_prim,
	x_sexp_char_analyse3_prim;
/** @} */

/** @name Read / write / display primitives (satom -- type-internal). */
/** @{ */
extern x_satom_t x_sexp_char_read_prim,
	x_sexp_char_write_prim,
	x_sexp_char_display_prim;
/** @} */

/** Analyse state 1: match first char of @c #\\ prefix. */
x_obj_t *x_sexp_char_analyse1(x_obj_t *p_base, x_obj_t *p_args);
/** Analyse state 2: match second char of @c #\\ prefix. */
x_obj_t *x_sexp_char_analyse2(x_obj_t *p_base, x_obj_t *p_args);
/** Analyse state 3: classify single character vs. named character start. */
x_obj_t *x_sexp_char_analyse3(x_obj_t *p_base, x_obj_t *p_args);
/** Read a character literal from the token buffer. */
x_obj_t *x_sexp_char_read(x_obj_t *p_base, x_obj_t *p_args);
/** Write the external representation @c #\\c or @c #\\name. */
/** Display a character (raw byte, no prefix). */

#endif /* X_SEXP_CHAR_H */
