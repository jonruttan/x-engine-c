#ifndef X_SEXP_WHITESPACE_H
#define X_SEXP_WHITESPACE_H

/*
 * # Computational Expressions in C
 *
 * ## x-sexp/whitespace.h -- Header - SExp - Whitespace
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2023 Jon Ruttan
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
#include "x-sexp.h"

#ifndef X_SEXP_WHITESPACE_SPACE_STR
#define X_SEXP_WHITESPACE_SPACE_STR			" "
#endif /* X_SEXP_WHITESPACE_SPACE_STR */

#ifndef X_SEXP_WHITESPACE_NEWLINE_STR
#define X_SEXP_WHITESPACE_NEWLINE_STR		"\n"
#endif /* X_SEXP_WHITESPACE_NEWLINE_STR */

#ifndef X_SEXP_WHITESPACE_TAB_STR
#define X_SEXP_WHITESPACE_TAB_STR			"\t"
#endif /* X_SEXP_WHITESPACE_TAB_STR */

#ifndef X_SEXP_WHITESPACE_CHARS_STR
#define X_SEXP_WHITESPACE_CHARS_STR			"\t\n\v\f\r "
#endif /* X_SEXP_WHITESPACE_CHARS_STR */

/*
 * # Data Structures
 */
extern x_satom_t x_sexp_whitespace_analyse1_prim,
	x_sexp_whitespace_analyse2_prim,
	x_sexp_whitespace_delimit_prim,
	x_sexp_whitespace_read_prim;

x_obj_t *x_sexp_whitespace_analyse1(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_sexp_whitespace_analyse2(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_sexp_whitespace_delimit(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_sexp_whitespace_read(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_SEXP_WHITESPACE_H */
