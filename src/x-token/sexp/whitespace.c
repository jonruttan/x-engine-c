/**
 * @file whitespace.c
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

#include "x-token/sexp/whitespace.h"
#include "x-eval.h"
#include "x-token.h"
#include "x-type/buffer.h"
#include "x-type/char.h"

x_satom_t x_sexp_whitespace_analyse1_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_whitespace_analyse1 }),
	x_sexp_whitespace_analyse2_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_whitespace_analyse2 }),
	x_sexp_whitespace_delimit_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_whitespace_delimit });

/**
 * Analyse state 1: detect a whitespace character.
 *
 * Checks the last character in the buffer against
 * @ref X_SEXP_WHITESPACE_CHARS_STR.
 *
 * @param p_base  Execution context.
 * @param p_args  Read-args containing the token buffer.
 * @return The analyse2 primitive on match, or NULL.
 */
x_obj_t *x_sexp_whitespace_analyse1(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	if (x_lib_strchr(X_SEXP_WHITESPACE_CHARS_STR, x_bufferlastchar(p_buffer))) {
		return x_sexp_whitespace_analyse2_prim;
	}

	return NULL;
}

/**
 * Analyse state 2: consume additional whitespace and score.
 *
 * Re-invokes analyse1 to check for more whitespace.  When a
 * non-whitespace character is found, scores @c bufferlen-1 (the @c -1
 * accounts for the extra non-whitespace character that was read to
 * terminate the run).
 *
 * @param p_base  Execution context.
 * @param p_args  Read-args containing the token buffer and score.
 * @return Self to keep reading, or score on non-whitespace.
 */
x_obj_t *x_sexp_whitespace_analyse2(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_score = x_token_read_arg_score(p_args);

	if (x_obj_isnil(p_base, x_sexp_whitespace_analyse1(p_base, p_args))) {
		x_firstint(p_score) = x_bufferlen(p_buffer) - 1;

		return p_score;
	}

	return x_sexp_whitespace_analyse2_prim;
}

/**
 * Delimiter callback for whitespace characters.
 *
 * If the current character is whitespace, backs up the read pointer
 * so the whitespace is not consumed by the current token.
 *
 * @param p_base  Execution context.
 * @param p_args  Read-args containing the token buffer.
 * @return The buffer on whitespace match, or NULL.
 */
x_obj_t *x_sexp_whitespace_delimit(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	if (x_obj_isnil(p_base, x_sexp_whitespace_analyse1(p_base, p_args))) {
		return NULL;
	}

	x_bufferread(p_buffer)--;

	return p_buffer;
}
