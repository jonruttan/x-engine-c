#ifndef X_SEXP_WHITESPACE_H
#define X_SEXP_WHITESPACE_H

/**
 * @file whitespace.h
 * @brief S-expression analyser and delimiter for whitespace tokens.
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

#ifndef X_SEXP_WHITESPACE_SPACE_STR
#define X_SEXP_WHITESPACE_SPACE_STR			" " /**< Space character string. */
#endif /* X_SEXP_WHITESPACE_SPACE_STR */

#ifndef X_SEXP_WHITESPACE_NEWLINE_STR
#define X_SEXP_WHITESPACE_NEWLINE_STR		"\n" /**< Newline character string. */
#endif /* X_SEXP_WHITESPACE_NEWLINE_STR */

#ifndef X_SEXP_WHITESPACE_TAB_STR
#define X_SEXP_WHITESPACE_TAB_STR			"\t" /**< Tab character string. */
#endif /* X_SEXP_WHITESPACE_TAB_STR */

#ifndef X_SEXP_WHITESPACE_CHARS_STR
#define X_SEXP_WHITESPACE_CHARS_STR			"\t\n\v\f\r " /**< All recognised whitespace characters. */
#endif /* X_SEXP_WHITESPACE_CHARS_STR */

/** @name Analyser / delimiter primitives (satom). */
/** @{ */
extern x_satom_t x_sexp_whitespace_analyse1_prim,
	x_sexp_whitespace_analyse2_prim,
	x_sexp_whitespace_delimit_prim;
/** @} */

/** Analyse state 1: detect a whitespace character. */
x_obj_t *x_sexp_whitespace_analyse1(x_obj_t *p_base, x_obj_t *p_args);
/** Analyse state 2: consume additional whitespace, score on non-whitespace. */
x_obj_t *x_sexp_whitespace_analyse2(x_obj_t *p_base, x_obj_t *p_args);
/** Delimiter callback: back up read pointer on whitespace characters. */
x_obj_t *x_sexp_whitespace_delimit(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_SEXP_WHITESPACE_H */
