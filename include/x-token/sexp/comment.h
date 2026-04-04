#ifndef X_SEXP_COMMENT_H
#define X_SEXP_COMMENT_H

/**
 * @file comment.h
 * @brief S-expression analyser and delimiter for line comments.
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

#ifndef X_SEXP_COMMENT_PRE_STR
#define X_SEXP_COMMENT_PRE_STR				";" /**< Comment start delimiter. */
#endif /* X_SEXP_COMMENT_PRE_STR */

#ifndef X_SEXP_COMMENT_PRE_STR_LEN
#define X_SEXP_COMMENT_PRE_STR_LEN			1 /**< Length of comment start delimiter. */
#endif /* X_SEXP_COMMENT_PRE_STR_LEN */

#ifndef X_SEXP_COMMENT_POST_STR
#define X_SEXP_COMMENT_POST_STR				"\n" /**< Comment end delimiter. */
#endif /* X_SEXP_COMMENT_POST_STR */

#ifndef X_SEXP_COMMENT_POST_STR_LEN
#define X_SEXP_COMMENT_POST_STR_LEN			1 /**< Length of comment end delimiter. */
#endif /* X_SEXP_COMMENT_POST_STR_LEN */

#ifndef X_SEXP_COMMENT_CHARS_STR
#define X_SEXP_COMMENT_CHARS_STR		X_SEXP_COMMENT_PRE_STR /**< Characters that delimit other tokens. */
#endif /* X_SEXP_COMMENT_CHARS_STR */

/** @name Analyser / delimiter primitives (satom). */
/** @{ */
extern x_satom_t x_sexp_comment_analyse1_prim,
	x_sexp_comment_analyse2_prim,
	x_sexp_comment_delimit_prim;
/** @} */

/** Analyse state 1: detect the comment start delimiter. */
x_obj_t *x_sexp_comment_analyse1(x_obj_t *p_base, x_obj_t *p_args);
/** Analyse state 2: consume characters until end-of-line. */
x_obj_t *x_sexp_comment_analyse2(x_obj_t *p_base, x_obj_t *p_args);
/** Delimiter callback: back up read pointer on comment-start char. */
x_obj_t *x_sexp_comment_delimit(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_SEXP_COMMENT_H */
