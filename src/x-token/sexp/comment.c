/**
 * @file comment.c
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

#include "x-token/sexp/comment.h"
#include "x-eval.h"
#include "x-token.h"
#include "x-type/buffer.h"
#include "x-type/char.h"

x_satom_t x_sexp_comment_analyse1_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_comment_analyse1 }),
	x_sexp_comment_analyse2_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_comment_analyse2 }),
	x_sexp_comment_delimit_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_comment_delimit });

/**
 * Analyse state 1: detect the comment start delimiter.
 *
 * Compares the buffer contents against @ref X_SEXP_COMMENT_PRE_STR.
 *
 * @param p_base  Execution context.
 * @param p_args  Read-args containing the token buffer.
 * @return The analyse2 primitive on match, or NULL.
 */
x_obj_t *x_sexp_comment_analyse1(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	if (0 == x_lib_strncmp(X_SEXP_COMMENT_PRE_STR, x_bufferval(p_buffer), X_SEXP_COMMENT_PRE_STR_LEN)) {
		return x_sexp_comment_analyse2_prim;
	}

	return NULL;
}

/**
 * Analyse state 2: consume characters until end-of-line.
 *
 * Continues reading until @ref X_SEXP_COMMENT_POST_STR is found,
 * then scores the full comment length.
 *
 * @param p_base  Execution context.
 * @param p_args  Read-args containing the token buffer and score.
 * @return Score on end-of-line match, or self to keep reading.
 */
x_obj_t *x_sexp_comment_analyse2(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_score = x_token_read_arg_score(p_args);

	if (0 == x_lib_strncmp(X_SEXP_COMMENT_POST_STR, x_bufferread(p_buffer) - X_SEXP_COMMENT_PRE_STR_LEN, X_SEXP_COMMENT_POST_STR_LEN)) {
		x_firstint(p_score) = x_bufferlen(p_buffer);
		return p_score;
	}

	return x_sexp_comment_analyse2_prim;
}

/**
 * Delimiter callback for comment characters.
 *
 * If the last character in the buffer is a comment-start character,
 * backs up the read pointer so the comment is not consumed by the
 * current token, and returns the buffer to signal delimiting.
 *
 * @param p_base  Execution context.
 * @param p_args  Read-args containing the token buffer.
 * @return The buffer on delimiter match, or NULL.
 */
x_obj_t *x_sexp_comment_delimit(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	if (x_lib_strchr(X_SEXP_COMMENT_CHARS_STR, x_bufferlastchar(p_buffer))) {
		x_bufferread(p_buffer)--;
		return p_buffer;
	}

	return NULL;
}
