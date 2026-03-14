#ifndef X_SEXP_COMMENT_H
#define X_SEXP_COMMENT_H

/*
 * # Computational Expressions in C
 *
 * ## x-sexp/comment.h -- Header - SExp - Comment
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
#include "x-obj.h"

#ifndef X_SEXP_COMMENT_PRE_STR
#define X_SEXP_COMMENT_PRE_STR				";"
#endif /* X_SEXP_COMMENT_PRE_STR */

#ifndef X_SEXP_COMMENT_PRE_STR_LEN
#define X_SEXP_COMMENT_PRE_STR_LEN			1
#endif /* X_SEXP_COMMENT_PRE_STR_LEN */

#ifndef X_SEXP_COMMENT_POST_STR
#define X_SEXP_COMMENT_POST_STR				"\n"
#endif /* X_SEXP_COMMENT_POST_STR */

#ifndef X_SEXP_COMMENT_POST_STR_LEN
#define X_SEXP_COMMENT_POST_STR_LEN			1
#endif /* X_SEXP_COMMENT_POST_STR_LEN */

#ifndef X_SEXP_COMMENT_CHARS_STR
#define X_SEXP_COMMENT_CHARS_STR		X_SEXP_COMMENT_PRE_STR
#endif /* X_SEXP_COMMENT_CHARS_STR */

/*
 * # Data Structures
 */
extern x_satom_t x_sexp_comment_analyse1_prim,
	x_sexp_comment_analyse2_prim,
	x_sexp_comment_delimit_prim;

x_obj_t *x_sexp_comment_analyse1(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_sexp_comment_analyse2(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_sexp_comment_delimit(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_SEXP_COMMENT_H */
