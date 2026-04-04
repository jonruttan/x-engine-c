#ifndef X_SEXP_LIST_H
#define X_SEXP_LIST_H

/**
 * @file list.h
 * @brief S-expression analyser, reader, writer, and display for list literals.
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2024 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */

#include "x-obj.h"

#ifndef X_SEXP_LIST_PRE_STR
#define X_SEXP_LIST_PRE_STR			"(" /**< List open delimiter. */
#endif /* X_SEXP_LIST_PRE_STR */

#ifndef X_SEXP_LIST_DOT_STR
#define X_SEXP_LIST_DOT_STR			"." /**< Dotted-pair separator. */
#endif /* X_SEXP_LIST_DOT_STR */

#ifndef X_SEXP_LIST_POST_STR
#define X_SEXP_LIST_POST_STR		")" /**< List close delimiter. */
#endif /* X_SEXP_LIST_POST_STR */

#ifndef X_SEXP_LIST_CHARS_STR
#define X_SEXP_LIST_CHARS_STR		X_SEXP_LIST_PRE_STR X_SEXP_LIST_DOT_STR X_SEXP_LIST_POST_STR /**< All list-related delimiter characters. */
#endif /* X_SEXP_LIST_CHARS_STR */

/** @name Analyser / reader / writer / display primitives (satom). */
/** @{ */
extern x_satom_t x_sexp_list_analyse_prim,
	x_sexp_list_delimit_prim,
	x_sexp_list_read_prim,
	x_sexp_list_write_prim,
	x_sexp_list_display_prim;
/** @} */

/** Analyse: score on any list delimiter character. */
x_obj_t *x_sexp_list_analyse(x_obj_t *p_base, x_obj_t *args);
/** Delimiter callback: back up read pointer on list characters. */
x_obj_t *x_sexp_list_delimit(x_obj_t *p_base, x_obj_t *args);
/** Read a list (or dotted pair) from the token stream. */
x_obj_t *x_sexp_list_read(x_obj_t *p_base, x_obj_t *args);
/** Write the external representation of a list. */
x_obj_t *x_sexp_list_write(x_obj_t *p_base, x_obj_t *args);
/** Display a list (human-readable, no quoting). */
x_obj_t *x_sexp_list_display(x_obj_t *p_base, x_obj_t *args);

#endif /* X_SEXP_LIST_H */
