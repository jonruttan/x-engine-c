/*
 * # Computational Expressions in C
 *
 * ## x-sexp/symbol.c -- Implementation - SExp - Symbol
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
#include "x-sexp/symbol.h"
#include "x-base.h"
#include "x-token.h"
#include "x-type/buffer.h"
#include "x-type/str.h"
#include "x-type/symbol.h"

x_satom_t x_sexp_symbol_analyse_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_symbol_analyse }),
	x_sexp_symbol_read_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_symbol_read }),
	x_sexp_symbol_write_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_symbol_write });

x_obj_t *x_sexp_symbol_analyse(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_score = x_token_read_arg_score(p_args);

	if (x_obj_isnil(p_base, x_token_delimit(p_base, p_args))) {
		return p_args;
	}

	if (x_bufferlen(p_buffer) == 0) {
		return p_base;
	}

	x_firstint(p_score) = -x_bufferlen(p_buffer);
	x_restobj(p_score) = x_sexp_symbol_read_prim;

	return p_score;
}

x_obj_t *x_sexp_symbol_read(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_str = x_mkstrown(p_base, x_lib_strndup(x_bufferval(p_buffer), x_bufferlen(p_buffer)));
	x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_str }, { NULL });

	return x_type_symbol_make(p_base, (x_obj_t *)args);
}

x_obj_t *x_sexp_symbol_write(x_obj_t *p_base, x_obj_t *p_args)
{
	int fd = x_base_isset(p_base) ? x_atomint(x_base_field_fileout(p_base)) : STDOUT_FILENO;
	x_char_t *s = x_firststr(x_firstobj(p_args));

	x_sys_write(fd, s, x_lib_strlen(s));

	return x_firstobj(p_args);
}
