#ifndef X_SEXP_LIST_H
#define X_SEXP_LIST_H

/*
 * # Computational Expressions in C
 *
 * ## x-sexp/list.h -- Header - SExp - List
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2024 Jon Ruttan
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
#include "x-obj.h"

#ifndef X_SEXP_LIST_PRE_STR
#define X_SEXP_LIST_PRE_STR			"("
#endif /* X_SEXP_LIST_PRE_STR */

#ifndef X_SEXP_LIST_DOT_STR
#define X_SEXP_LIST_DOT_STR			"."
#endif /* X_SEXP_LIST_DOT_STR */

#ifndef X_SEXP_LIST_POST_STR
#define X_SEXP_LIST_POST_STR		")"
#endif /* X_SEXP_LIST_POST_STR */

#ifndef X_SEXP_LIST_CHARS_STR
#define X_SEXP_LIST_CHARS_STR		X_SEXP_LIST_PRE_STR X_SEXP_LIST_DOT_STR X_SEXP_LIST_POST_STR
#endif /* X_SEXP_LIST_CHARS_STR */

/*
 * # Data Structures
 */
 extern x_satom_t x_sexp_list_analyse_prim,
	x_sexp_list_delimit_prim,
	x_sexp_list_read_prim,
	x_sexp_list_write_prim;

x_obj_t *x_sexp_list_analyse(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_sexp_list_delimit(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_sexp_list_read(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_sexp_list_write(x_obj_t *p_base, x_obj_t *args);

#endif /* X_SEXP_LIST_H */
