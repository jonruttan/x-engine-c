/*
 * # Computational Expressions in C
 *
 * ## whitespace.c -- Implementation - SExp - Whitespace
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
#include "x-base.h"
#include "x-token.h"
#include "x-type/buffer.h"
#include "x-type/char.h"

x_satom_t x_sexp_whitespace_analyse1_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_whitespace_analyse1 }),
	x_sexp_whitespace_analyse2_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_whitespace_analyse2 }),
	x_sexp_whitespace_delimit_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_whitespace_delimit }),
	x_sexp_whitespace_read_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_whitespace_read });

x_obj_t *x_sexp_whitespace_analyse1(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	if (x_lib_strchr(X_SEXP_WHITESPACE_CHARS_STR, x_bufferlastchar(p_buffer))) {
		return x_sexp_whitespace_analyse2_prim;
	}

	return p_base;
}

x_obj_t *x_sexp_whitespace_analyse2(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_score = x_token_read_arg_score(p_args);

	if (x_obj_isnil(p_base, x_sexp_whitespace_analyse1(p_base, p_args))) {
		x_intval(p_score) = x_bufferlen(p_buffer) - 1;
		x_restobj(p_score) = x_sexp_whitespace_read_prim;

		return p_score;
	}

	return x_sexp_whitespace_analyse2_prim;
}

x_obj_t *x_sexp_whitespace_delimit(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	if (x_obj_isnil(p_base, x_sexp_whitespace_analyse1(p_base, p_args))) {
		return p_base;
	}

	x_bufferread(p_buffer)--;

	return p_buffer;
}

x_obj_t *x_sexp_whitespace_read(x_obj_t *p_base, x_obj_t *p_args)
{
	/* Discard the buffer. */
	return p_args;
}
