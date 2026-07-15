#ifndef X_SEXP_STR_H
#define X_SEXP_STR_H

/**
 * @file str.h
 * @brief S-expression analyser and reader for string literals.
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

#ifndef X_SEXP_STR_PRE_STR
#define X_SEXP_STR_PRE_STR				"\"" /**< String open delimiter. */
#endif /* X_SEXP_STR_PRE_STR */

#ifndef X_SEXP_STR_PRE_STR_LEN
#define X_SEXP_STR_PRE_STR_LEN			1 /**< Length of string open delimiter. */
#endif /* X_SEXP_STR_PRE_STR_LEN */

#ifndef X_SEXP_STR_POST_STR
#define X_SEXP_STR_POST_STR				X_SEXP_STR_PRE_STR /**< String close delimiter. */
#endif /* X_SEXP_STR_POST_STR */

#ifndef X_SEXP_STR_POST_STR_LEN
#define X_SEXP_STR_POST_STR_LEN			X_SEXP_STR_PRE_STR_LEN /**< Length of string close delimiter. */
#endif /* X_SEXP_STR_POST_STR_LEN */

#ifndef X_SEXP_STR_CHARS_STR
#define X_SEXP_STR_CHARS_STR			X_SEXP_STR_PRE_STR /**< Characters that delimit other tokens. */
#endif /* X_SEXP_STR_CHARS_STR */

/** @name Analyser state primitives (spair -- composable state machines). */
/** @{ */
extern x_spair_t x_sexp_str_analyse1_prim,
	x_sexp_str_analyse2_prim,
	x_sexp_str_analyse3_prim;
/** @} */

/** @name Read primitives (satom -- type-internal). */
/** @{ */
extern x_satom_t x_sexp_str_read_prim;
/** @} */

/** Analyse state 1: match the opening quote. */
x_obj_t *x_sexp_str_analyse1(x_obj_t *p_base, x_obj_t *p_args);
/** Analyse state 2: consume body characters, detect escape or close. */
x_obj_t *x_sexp_str_analyse2(x_obj_t *p_base, x_obj_t *p_args);
/** Analyse state 3: escape state -- consume one character after backslash. */
x_obj_t *x_sexp_str_analyse3(x_obj_t *p_base, x_obj_t *p_args);
/** Read a string literal from the token buffer (with unescape). */
x_obj_t *x_sexp_str_read(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_SEXP_STR_H */
