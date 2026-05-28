/**
 * @file symbol.c
 * @brief S-expression analyser, reader, writer, and display for symbols.
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

#include "x-token/sexp/symbol.h"
#include "x-interp.h"
#include "x-token.h"
#include "x-type/buffer.h"
#include "x-type/str.h"
#include "x-type/symbol.h"

x_satom_t x_sexp_symbol_analyse_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_symbol_analyse }),
	x_sexp_symbol_read_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_symbol_read }),
	x_sexp_symbol_write_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_symbol_write }),
	x_sexp_symbol_display_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_symbol_display });

/**
 * Analyse: match any non-delimiter sequence as a symbol.
 *
 * Symbols are the fallback token type.  If no delimiter is found, the
 * analyser keeps reading.  When a delimiter terminates the sequence,
 * the buffer length is scored as a negative value so that more
 * specific types (which score positively) always win ties.
 *
 * @param p_base  Execution context.
 * @param p_args  Read-args containing the token buffer and score.
 * @return The args (keep reading), score (on delimiter), or NULL (empty).
 */
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

/**
 * Read a symbol from the token buffer.
 *
 * Copies the buffer contents into a new string, then interns it as a
 * symbol through @c x_type_symbol_make.
 *
 * @param p_base  Execution context.
 * @param p_args  Read-args containing the token buffer.
 * @return An interned symbol object.
 */
x_obj_t *x_sexp_symbol_read(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_str = x_mkstrown(p_base, x_lib_strndup(x_bufferval(p_buffer), x_bufferlen(p_buffer)));
	x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_str }, { NULL });

	return x_type_symbol_make(p_base, (x_obj_t *)args);
}

/**
 * Display a symbol as its bare name (no wrapping).
 *
 * @param p_base  Execution context.
 * @param p_args  Pair whose first element is the symbol to display.
 * @return The symbol object.
 */
x_obj_t *x_sexp_symbol_display(x_obj_t *p_base, x_obj_t *p_args)
{
	x_satom_t str = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .s = x_firststr(x_firstobj(p_args)) });
	x_spair_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { str }, { NULL });

	x_interp_write_str(p_base, (x_obj_t *)&wrap);

	return x_firstobj(p_args);
}

/**
 * Write the external representation of a symbol.
 *
 * Outputs the symbol in @c (lit\ name) form so that the written
 * representation is readable and unambiguous.
 *
 * @param p_base  Execution context.
 * @param p_args  Pair whose first element is the symbol to write.
 * @return The symbol object.
 */
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

	x_interp_write_str(p_base, (x_obj_t *)&wrap_pre);
	x_interp_write_str(p_base, (x_obj_t *)&wrap_str);
	x_interp_write_str(p_base, (x_obj_t *)&wrap_suf);

	return x_firstobj(p_args);
}
