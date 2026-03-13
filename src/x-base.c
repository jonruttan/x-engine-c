/*
 * # Computational Expressions in C
 *
 * ## x-base.c -- Implementation - Base
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
#include "x-base.h"
#include "x-type.h"
#include "x-alist.h"
#include "x-eval.h"
#include "x-type/ptr.h"
#include "x-type/str.h"
#include <setjmp.h>

#include "x-token.h"

#define nil			NULL
#define pair(X,Y)	(x_mkspair(p_base, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, (X)))

x_obj_t *x_base_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_parent = p_base;

	p_base = x_obj_make(p_base, x_type_base_obj, X_OBJ_FLAG_NONE,
		X_OBJ_LENGTH_ATOM, NULL);
	x_atomobj(p_base) = pair(
		/* type-alist */
		nil,
		pair(
			/* files: '(filein-stack fileout fileerr) */
			pair(pair(atom(STDIN_FILENO), nil),
			pair(atom(STDOUT_FILENO),
			pair(atom(STDERR_FILENO),
			nil))),
		pair(
			/* env-state: '(alist eval-list (buffer-stack) cache error tco-expr tco-env) */
			pair(nil,
			pair(nil,
			pair(pair(nil, nil),
			pair(nil,
			pair(nil,
			pair(nil,
			pair(nil,
			nil))))))),
		pair(
			/* true symbol (inherit from parent) */
			p_parent ? x_base_field_true(p_parent) : nil,
		pair(
			/* line counter */
			atom(1),
		pair(
			/* profile: '(allocs evals tco) */
			pair(atom(0), pair(atom(0), pair(atom(0), nil))),
		pair(
			/* hooks: '(type-name units length error) */
			pair(atom(x_type_prim_type_name),
			pair(atom(x_type_prim_units),
			pair(atom(x_type_prim_length),
			pair(atom(x_base_error),
			nil)))),
		nil)))))));

	return p_base;
}

#undef nil
#undef pair
#undef atom

void x_base_error(x_obj_t *p_base, x_char_t *message, x_obj_t *p_obj)
{
	int fd;
	x_char_t *symbol = NULL;

	/* Extract symbol string from object if possible. */
	if (p_obj != NULL && x_obj_type_issatom(p_obj)) {
		symbol = x_atomstr(p_obj);
	}

	/* If an error handler is installed, build error string and longjmp. */
	if (x_base_isset(p_base)
		&& ! x_obj_isnil(p_base, x_base_field_error_handler(p_base))) {
		x_obj_t *p_handler = x_base_field_error_handler(p_base);
		x_int_t line = x_atomint(x_base_field_line(p_base));
		x_char_t line_buf[24], *line_str;
		size_t msg_len = x_lib_strlen(message);
		size_t sym_len = symbol ? x_lib_strlen(symbol) : 0;
		size_t line_len, total;
		x_char_t *combined, *p;

		line_str = x_lib_inttostr(line, line_buf, 10);
		line_len = x_lib_strlen(line_str);
		/* "message ['symbol] (line N)\0" */
		total = msg_len + (symbol ? 2 + sym_len : 0) + 7 + line_len + 1 + 1;
		combined = (x_char_t *)x_sys_malloc(total);
		p = combined;

		x_lib_memcpy(p, message, msg_len); p += msg_len;
		if (symbol) {
			*p++ = ' '; *p++ = '\'';
			x_lib_memcpy(p, symbol, sym_len); p += sym_len;
		}
		x_lib_memcpy(p, " (line ", 7); p += 7;
		x_lib_memcpy(p, line_str, line_len); p += line_len;
		*p++ = ')'; *p = '\0';

		x_error_handler_error(p_handler) = x_mkstrown(p_base, combined);
		x_base_field_env_alist(p_base)
			= x_error_handler_saved_env(p_handler);
		longjmp(*(jmp_buf *)x_error_handler_jmp(p_handler), 1);
	}

	fd = x_base_isset(p_base) ? x_atomint(x_base_field_fileerr(p_base)) : STDERR_FILENO;

	x_error(fd, message, symbol);
}

x_obj_t *x_base_type_alist_extend(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_args }, { NULL });

	if ( ! x_base_isset(p_base)) {
		return NULL;
	}

	x_restobj((x_obj_t *)args) = x_base_field_type_alist(p_base);

	return x_base_field_type_alist(p_base) = x_alist_extend(p_base, (x_obj_t *)args);
}

x_obj_t *x_base_type_alist_assoc(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_firstobj(p_args) }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL })
	};

	if ( ! x_base_isset(p_base)) {
		return NULL;
	}

	x_firstobj((x_obj_t *)args[1]) = x_base_field_type_alist(p_base);

	return x_alist_assoc(p_base, (x_obj_t *)args);
}

x_obj_t *x_base_env_alist_extend(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_args }, { NULL });

	if ( ! x_base_isset(p_base)) {
		return NULL;
	}

	x_restobj((x_obj_t *)args) = x_base_field_env_alist(p_base);

	return x_base_field_env_alist(p_base) = x_alist_extend(p_base, (x_obj_t *)args);
}

x_obj_t *x_base_load(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_base_field_buffer(p_base);
	x_obj_t *p_exp, *p_result = NULL;
	x_satom_t exp_wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL });
	x_spair_t eval_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { exp_wrap }, { NULL })
	};
	x_spair_t read_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_buffer }, { p_base })
	};

	for (;;) {
		p_exp = x_token_read(p_base, (x_obj_t *)read_args);
		if (x_obj_isnil(p_base, p_exp)) break;

		x_firstobj((x_obj_t *)exp_wrap) = p_exp;
		p_result = x_eval(p_base, (x_obj_t *)eval_args);
	}

	return p_result;
}

x_obj_t *x_base_read(x_obj_t *p_base, x_obj_t *p_args)
{
	int fd = x_base_isset(p_base) ? x_atomint(x_base_field_filein(p_base)) : STDIN_FILENO;
	x_obj_t *p_atom = x_firstobj(p_args);
	x_int_t size = x_atomint(x_firstobj(x_restobj(p_args)));

	if (x_sys_read(fd, &x_atomchar(p_atom), size) == size) {
		return p_atom;
	}

	return NULL;
}

x_obj_t *x_base_write(x_obj_t *p_base, x_obj_t *p_args)
{
	int fd = x_base_isset(p_base) ? x_atomint(x_base_field_fileout(p_base)) : STDOUT_FILENO;
	x_obj_t *p_atom = x_firstobj(p_args);
	x_int_t size = x_atomint(x_firstobj(x_restobj(p_args)));

	if (x_sys_write(fd, x_atomstr(p_atom), size) == size) {
		return p_atom;
	}

	return NULL;
}
