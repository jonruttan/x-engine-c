/*
 * # Computational Expressions in C
 *
 * ## comment.c -- Implementation - SExp - Comment
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
#include "x-token/sexp/comment.h"
#include "x-base.h"
#include "x-token.h"
#include "x-type/buffer.h"
#include "x-type/char.h"

x_satom_t x_sexp_comment_analyse1_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_comment_analyse1 }),
	x_sexp_comment_analyse2_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_comment_analyse2 }),
	x_sexp_comment_delimit_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_comment_delimit }),
	x_sexp_comment_read_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_comment_read });

x_obj_t *x_sexp_comment_analyse1(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	if (0 == x_lib_strncmp(X_SEXP_COMMENT_PRE_STR, x_bufferval(p_buffer), X_SEXP_COMMENT_PRE_STR_LEN)) {
		return x_sexp_comment_analyse2_prim;
	}

	return p_base;
}

x_obj_t *x_sexp_comment_analyse2(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_score = x_token_read_arg_score(p_args);

	if (0 == x_lib_strncmp(X_SEXP_COMMENT_POST_STR, x_bufferread(p_buffer) - X_SEXP_COMMENT_PRE_STR_LEN, X_SEXP_COMMENT_POST_STR_LEN)) {
		x_firstint(p_score) = x_bufferlen(p_buffer);
		x_restobj(p_score) = x_sexp_comment_read_prim;
		return p_score;
	}

	return x_sexp_comment_analyse2_prim;
}

x_obj_t *x_sexp_comment_delimit(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	if (x_lib_strchr(X_SEXP_COMMENT_CHARS_STR, x_bufferlastchar(p_buffer))) {
		x_bufferread(p_buffer)--;
		return p_buffer;
	}

	return p_base;
}

x_obj_t *x_sexp_comment_read(x_obj_t *p_base, x_obj_t *p_args)
{
	/* Discard the buffer. */
	return p_args;
}
