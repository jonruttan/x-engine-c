/*
 * # Computational Expressions in C
 *
 * ## x-sexp/list.c -- Implementation - SExp - List
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
#include "x-type/list.h"
#include "x-sexp/list.h"
#include "x-sexp/whitespace.h"

x_satom_t x_sexp_list_analyse_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_list_analyse }),
	x_sexp_list_delimit_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_list_delimit }),
	x_sexp_list_read_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_list_read }),
	x_sexp_list_write_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_list_write });

x_obj_t *x_sexp_list_analyse(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	if (x_lib_strchr(X_SEXP_LIST_CHARS_STR, x_bufferlastchar(p_buffer))) {
		return p_buffer;
	}

	return p_base;
}

x_obj_t *x_sexp_list_delimit(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	if (x_lib_strchr(X_SEXP_LIST_CHARS_STR, x_bufferlastchar(p_buffer))) {
		x_bufferread(p_buffer)--;
		return p_buffer;
	}

	return p_base;
}

x_obj_t *x_sexp_list_read(x_obj_t *p_base, x_obj_t *p_args)
{
/*	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	switch (x_bufferlastchar(p_buffer)) {
		case X_SEXP_LIST_PRE_STR:
			break;

		case X_SEXP_LIST_POST_STR:
			break;

		case X_SEXP_LIST_DOT_STR:
			break;

	}*/

	return p_base;
}
/*
 * Writes a written representation of _args_ list to output.
 *
 * @function x_sexp_list_write
 * @param {x_obj_t *} p_base A pointer to the p_base of the object structure.
 * @param {x_obj_t *} p_args A pointer to the list to be written.
 * @returns {x_obj_t *} The object pointer passed as _args_.
 */
x_obj_t *x_sexp_list_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args);
	x_satom_t data_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .s = NULL }),
		size_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = 1 });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { data_obj }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { size_obj }, { NULL })
	};

	x_atomstr(data_obj) = X_SEXP_LIST_PRE_STR;
	x_base_write(p_base, (x_obj_t *)args);

	for (;;) {
		if ( ! x_obj_isnil(p_base, x_car(p_obj))) {
			x_sexp_write(p_base, x_car(p_obj));
		}

		p_obj = x_cdr(p_obj);

		if (x_obj_isnil(p_base, p_obj)) {
			x_atomstr(data_obj) = X_SEXP_LIST_POST_STR;
			x_base_write(p_base, (x_obj_t *)args);

			break;
		}

		if ( ! x_obj_type_islist(p_base, p_obj)) {
			x_atomstr(data_obj) = X_SEXP_WHITESPACE_SPACE_STR X_SEXP_LIST_DOT_STR X_SEXP_WHITESPACE_SPACE_STR;
			x_atomint(size_obj) = 3;
			x_base_write(p_base, (x_obj_t *)args);

			x_sexp_write(p_base, p_obj);

			x_atomstr(data_obj) = X_SEXP_LIST_POST_STR;
			x_atomint(size_obj) = 1;
			x_base_write(p_base, (x_obj_t *)args);

			break;
		}

		x_atomstr(data_obj) = X_SEXP_WHITESPACE_SPACE_STR;
		x_base_write(p_base, (x_obj_t *)args);
	}

	return p_obj;
}
