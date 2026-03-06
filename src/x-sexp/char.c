/*
 * # Computational Expressions in C
 *
 * ## x-sexp/char.c -- Implementation - SExp - Character
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
#include "x-type/buffer.h"
#include "x-type/char.h"
#include "x-sexp/char.h"

x_satom_t x_sexp_char_analyse1_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_char_analyse1 }),
 	x_sexp_char_analyse2_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_char_analyse2 }),
 	x_sexp_char_read_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_char_read }),
 	x_sexp_char_write_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_char_write });

x_obj_t *x_sexp_char_analyse1(x_obj_t *p_base, x_obj_t *p_args)
{
	if (X_SEXP_CHAR_PRE_STR[0] != x_bufferlastchar(x_token_read_arg_buffer(p_args))) {
		return p_base;
	}

	return x_sexp_char_analyse2_prim;
}

x_obj_t *x_sexp_char_analyse2(x_obj_t *p_base, x_obj_t *p_args)
{
	if (X_SEXP_CHAR_PRE_STR[1] != x_bufferlastchar(x_token_read_arg_buffer(p_args))) {
		return  p_base;
	}

	return x_token_read_arg_buffer(p_args);
}

x_obj_t *x_sexp_char_read(x_obj_t *p_base, x_obj_t *p_args)
{
	x_satom_t int_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = 1 });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_mkchar(p_base, '\0') }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { int_obj }, { NULL })
	};

	x_type_buffer_reset(p_base, p_args);

	return x_base_read(p_base, (x_obj_t *)args);
}

x_obj_t *x_sexp_char_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_ret;
	x_satom_t buffer = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = &x_atomchar(x_firstobj(p_args)) }),
		size = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .i = 1 });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { buffer }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { size }, { NULL })
	};

	p_ret = x_base_write(p_base, (x_obj_t *)args);

	if ( ! x_obj_isnil(p_base, p_ret)) {
		return x_firstobj(p_args);
	}

	return p_base;
}
