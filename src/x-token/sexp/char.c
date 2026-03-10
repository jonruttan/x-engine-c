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
#include "x-token.h"
#include "x-type/buffer.h"
#include "x-type/char.h"
#include "x-type/int.h"
#include "x-token/sexp/char.h"

/* Forward declaration for named-character state */
static x_obj_t *x_sexp_char_analyse4(x_obj_t *p_base, x_obj_t *p_args);

x_satom_t x_sexp_char_analyse1_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_char_analyse1 }),
 	x_sexp_char_analyse2_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_char_analyse2 }),
 	x_sexp_char_analyse3_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_char_analyse3 }),
 	x_sexp_char_read_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_char_read }),
 	x_sexp_char_write_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_char_write });

/* analyse4 prim: named-character state — reads lowercase letters */
static x_satom_t x_sexp_char_analyse4_prim =
	x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_char_analyse4 });

static int is_lower(x_char_t c)
{
	return c >= 'a' && c <= 'z';
}

x_obj_t *x_sexp_char_analyse1(x_obj_t *p_base, x_obj_t *p_args)
{
	if (X_SEXP_CHAR_PRE_STR[0] != x_bufferlastchar(x_token_read_arg_buffer(p_args))) {
		return NULL;
	}

	return x_sexp_char_analyse2_prim;
}

x_obj_t *x_sexp_char_analyse2(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	if (X_SEXP_CHAR_PRE_STR[1] != x_bufferlastchar(p_buffer)) {
		return NULL;
	}

	return x_sexp_char_analyse3_prim;
}

x_obj_t *x_sexp_char_analyse3(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_score = x_token_read_arg_score(p_args);

	/* Lowercase letter: may be start of a named character */
	if (is_lower(x_bufferlastchar(p_buffer))) {
		return x_sexp_char_analyse4_prim;
	}

	/* Non-letter: single character literal, score immediately */
	x_firstint(p_score) = x_bufferlen(p_buffer);
	x_restobj(p_score) = x_sexp_char_read_prim;
	return p_score;
}

/* analyse4: named-character state — keep reading lowercase letters */
static x_obj_t *x_sexp_char_analyse4(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_score = x_token_read_arg_score(p_args);

	if (is_lower(x_bufferlastchar(p_buffer))) {
		return x_sexp_char_analyse4_prim;
	}

	/* Non-letter delimiter: un-read it and score */
	x_bufferread(p_buffer)--;

	x_firstint(p_score) = x_bufferlen(p_buffer);
	x_restobj(p_score) = x_sexp_char_read_prim;
	return p_score;
}

x_obj_t *x_sexp_char_read(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);
	x_int_t len = x_bufferlen(p_buffer);
	x_obj_t *p_type, *p_data, *p_entry;
	x_char_t *buf_name, *sym_name;
	x_int_t name_len;

	/* Single character: #\c (length 3) */
	if (len == 3) {
		return x_mkchar(p_base, *(x_bufferval(p_buffer) + 2));
	}

	/* Named character: lookup in type data alist */
	buf_name = x_bufferval(p_buffer) + 2;
	name_len = len - 2;
	p_type = x_type_char_register(p_base, p_base);
	p_data = x_type_field_data(p_type);

	while ( ! x_obj_isnil(p_base, p_data)) {
		p_entry = x_firstobj(p_data);
		sym_name = x_atomstr(x_firstobj(p_entry));

		if (name_len == (x_int_t)x_lib_strlen(sym_name)
			&& 0 == x_lib_strncmp(buf_name, sym_name, name_len)) {
			return x_mkchar(p_base, (x_char_t)x_intval(x_restobj(p_entry)));
		}

		p_data = x_restobj(p_data);
	}

	x_obj_error(p_base, "read: unknown character name", NULL);

	return NULL;
}

x_obj_t *x_sexp_char_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_ret, *p_type, *p_data, *p_entry;
	x_char_t ch = x_atomchar(x_firstobj(p_args));
	x_char_t *sym_name;
	int fd = x_base_isset(p_base)
		? x_atomint(x_base_field_fileout(p_base)) : STDOUT_FILENO;
	x_satom_t buffer = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .s = &x_atomchar(x_firstobj(p_args)) }),
		size = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .i = 1 });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE,
			{ buffer }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { size }, { NULL })
	};

	/* Named characters: reverse-lookup in type data alist */
	if (x_base_isset(p_base)) {
		p_type = x_type_char_register(p_base, p_base);
		p_data = x_type_field_data(p_type);

		while ( ! x_obj_isnil(p_base, p_data)) {
			p_entry = x_firstobj(p_data);

			if ((x_char_t)x_intval(x_restobj(p_entry)) == ch) {
				sym_name = x_atomstr(x_firstobj(p_entry));
				x_sys_write(fd, sym_name, x_lib_strlen(sym_name));
				return x_firstobj(p_args);
			}

			p_data = x_restobj(p_data);
		}
	}

	/* Single character: write raw byte */
	p_ret = x_base_write(p_base, (x_obj_t *)args);

	if ( ! x_obj_isnil(p_base, p_ret)) {
		return x_firstobj(p_args);
	}

	return NULL;
}
