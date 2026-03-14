/*
 * # Computational Expressions in C
 *
 * ## x-sexp/int.c -- Implementation - SExp - Int
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2021 Jon Ruttan
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
#include <ctype.h>
#include "x-token/sexp/int.h"
#include "x-token.h"
#include "x-type/char.h"
#include "x-type/buffer.h"
#include "x-type/int.h"
#include "x-token/sexp/int.h"

x_satom_t x_sexp_int_analyse_sign_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_int_analyse_sign }),
	x_sexp_int_analyse_prefix_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_int_analyse_prefix }),
	x_sexp_int_analyse_base_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_int_analyse_base }),
	x_sexp_int_analyse_digits_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_int_analyse_digits }),
	x_sexp_int_analyse_xdigits_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_int_analyse_xdigits }),
	x_sexp_int_read_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_int_read }),
	x_sexp_int_write_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_int_write });

x_obj_t *x_sexp_int_analyse_digits(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_score = x_token_read_arg_score(p_args);

	if (isdigit(x_bufferlastchar(p_buffer))) {
		return x_sexp_int_analyse_digits_prim;
	}

	/* Reposition the read pointer to before the non-digit character. */
	x_bufferread(p_buffer)--;

	if (x_bufferlen(p_buffer) < 1) {
		return NULL;
	}

	x_firstint(p_score) = x_bufferlen(p_buffer);
	x_restobj(p_score) = x_sexp_int_read_prim;
	return p_score;
}

x_obj_t *x_sexp_int_analyse_xdigits(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_score = x_token_read_arg_score(p_args);

	if (isxdigit(x_bufferlastchar(p_buffer))) {
		return x_sexp_int_analyse_xdigits_prim;
	}

	/* Reposition the read pointer to before the non-xdigit character. */
	x_bufferread(p_buffer)--;

	if (x_bufferlen(p_buffer) < 1) {
		return NULL;
	}

	x_firstint(p_score) = x_bufferlen(p_buffer);
	x_restobj(p_score) = x_sexp_int_read_prim;
	return p_score;
}

x_obj_t *x_sexp_int_analyse_base(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	if (x_lib_strchr("Xx", x_bufferlastchar(p_buffer))) {
		return x_sexp_int_analyse_xdigits_prim;
	}

	return x_sexp_int_analyse_digits(p_base, p_args);
}

x_obj_t *x_sexp_int_analyse_prefix(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	if ('0' == x_bufferlastchar(p_buffer)) {
		return x_sexp_int_analyse_base_prim;
	}

	if ( ! isdigit(x_bufferlastchar(p_buffer))) {
		x_bufferread(p_buffer)--;
		return NULL;
	}

	return x_sexp_int_analyse_digits(p_base, p_args);
}

x_obj_t *x_sexp_int_analyse_sign(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	if (x_lib_strchr("+-", x_bufferlastchar(p_buffer)) != NULL) {
		return x_sexp_int_analyse_prefix_prim;
	}

	return x_sexp_int_analyse_prefix(p_base, p_args);
}

x_obj_t *x_sexp_int_read(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_firstobj(p_args), *p_int;

	if (x_bufferlen(p_buffer) < 1) {
		return NULL;
	}

	/* Not required, final non-numeric char will act as delimiter. */
	/* *x_bufferead(p_buffer) = '\0'; */

	p_int = x_mkint(p_base, x_lib_strtoint(x_bufferval(p_buffer), NULL, 0));

	return p_int;
}

x_obj_t *x_sexp_int_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_char_t s[22];
	x_obj_t *p_ret;
	x_satom_t str = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = s });
	x_spair_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { str }, { NULL });

	x_lib_inttostr(x_intval(x_firstobj(p_args)), s, 10);

	p_ret = x_base_write_str(p_base, (x_obj_t *)&wrap);

	if ( ! x_obj_isnil(p_base, p_ret)) {
		return x_firstobj(p_args);
	}

	return NULL;
}
