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
#include "x-token/sexp/symbol.h"
#include "x-base.h"
#include "x-token.h"
#include "x-type/buffer.h"
#include "x-type/str.h"
#include "x-type/symbol.h"

x_satom_t x_sexp_symbol_analyse_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_symbol_analyse }),
	x_sexp_symbol_read_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_symbol_read }),
	x_sexp_symbol_write_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_symbol_write }),
	x_sexp_symbol_display_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_symbol_display });

x_obj_t *x_sexp_symbol_analyse(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_score = x_token_read_arg_score(p_args);

	if (x_obj_isnil(p_base, x_token_delimit(p_base, p_args))) {
		return p_args;
	}

	if (x_bufferlen(p_buffer) == 0) {
		return NULL;
	}

	x_firstint(p_score) = -x_bufferlen(p_buffer);

	return p_score;
}

x_obj_t *x_sexp_symbol_read(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_str = x_mkstrown(p_base, x_lib_strndup(x_bufferval(p_buffer), x_bufferlen(p_buffer)));
	x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_str }, { NULL });

	return x_type_symbol_make(p_base, (x_obj_t *)args);
}

x_obj_t *x_sexp_symbol_display(x_obj_t *p_base, x_obj_t *p_args)
{
	x_satom_t str = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .s = x_firststr(x_firstobj(p_args)) });
	x_spair_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { str }, { NULL });

	x_base_write_str(p_base, (x_obj_t *)&wrap);

	return x_firstobj(p_args);
}

x_obj_t *x_sexp_symbol_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_satom_t prefix = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .s = (x_char_t *)"(lit " }),
		str = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .s = x_firststr(x_firstobj(p_args)) }),
		suffix = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .s = (x_char_t *)")" });
	x_spair_t wrap_pre = x_obj_set(NULL, X_OBJ_FLAG_NONE, { prefix }, { NULL }),
		wrap_str = x_obj_set(NULL, X_OBJ_FLAG_NONE, { str }, { NULL }),
		wrap_suf = x_obj_set(NULL, X_OBJ_FLAG_NONE, { suffix }, { NULL });

	x_base_write_str(p_base, (x_obj_t *)&wrap_pre);
	x_base_write_str(p_base, (x_obj_t *)&wrap_str);
	x_base_write_str(p_base, (x_obj_t *)&wrap_suf);

	return x_firstobj(p_args);
}
