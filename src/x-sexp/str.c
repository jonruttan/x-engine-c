/*
 * # Computational Expressions in C
 *
 * ## x-type.c -- Implementation - Type - String
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
#include "x-sexp/str.h"
#include "x-base.h"
#include "x-token.h"
#include "x-type/str.h"
#include "x-type/buffer.h"

x_satom_t x_sexp_str_analyse1_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_str_analyse1 }),
	x_sexp_str_analyse2_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_str_analyse2 }),
	x_sexp_str_read_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_str_read }),
	x_sexp_str_write_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_str_write });

x_obj_t *x_sexp_str_analyse1(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	if (0 == x_lib_strncmp(X_SEXP_STR_PRE_STR, x_bufferval(p_buffer), X_SEXP_STR_PRE_STR_LEN)) {
		return x_sexp_str_analyse2_prim;
	}

	return p_base;
}

x_obj_t *x_sexp_str_analyse2(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_score = x_token_read_arg_score(p_args);

	if (0 == x_lib_strncmp(X_SEXP_STR_POST_STR, x_bufferread(p_buffer) - X_SEXP_STR_PRE_STR_LEN, X_SEXP_STR_POST_STR_LEN)) {
		x_firstint(p_score) = x_bufferlen(p_buffer);
		x_restobj(p_score) = x_sexp_str_read_prim;
		return p_score;
	}

	return x_sexp_str_analyse2_prim;
}

x_obj_t *x_sexp_str_read(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	return x_mkstrown(
		p_base,
		x_lib_strndup(
			x_bufferval(p_buffer) + X_SEXP_STR_PRE_STR_LEN,
			x_bufferlen(p_buffer) - X_SEXP_STR_PRE_STR_LEN - X_SEXP_STR_POST_STR_LEN
		)
	);
}

x_obj_t *x_sexp_str_write(x_obj_t *p_base, x_obj_t *p_args)
{
	int fd = x_base_isset(p_base) ? x_atomint(x_base_field_fileout(p_base)) : STDOUT_FILENO;
	x_char_t *s = x_firststr(x_firstobj(p_args));

	x_sys_write(fd, X_SEXP_STR_PRE_STR, X_SEXP_STR_PRE_STR_LEN);
	x_sys_write(fd, s, x_lib_strlen(s));
	x_sys_write(fd, X_SEXP_STR_POST_STR, X_SEXP_STR_POST_STR_LEN);

	return x_firstobj(p_args);
}
