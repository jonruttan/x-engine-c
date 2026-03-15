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
		/* type-alist (stack-wrapped) */
		pair(nil, nil),
		pair(
			/* files: '(filein-stack fileout-stack fileerr-stack write-buf-stack) */
			pair(pair(atom(STDIN_FILENO), nil),
			pair(pair(atom(STDOUT_FILENO), nil),
			pair(pair(atom(STDERR_FILENO), nil),
			pair(pair(nil, nil),
			nil)))),
		pair(
			/* env-state: all stack-wrapped */
			pair(pair(nil, nil),
			pair(pair(nil, nil),
			pair(pair(nil, nil),
			pair(pair(nil, nil),
			pair(pair(nil, nil),
			pair(pair(nil, nil),
			pair(pair(nil, nil),
			nil))))))),
		pair(
			/* true object (stack-wrapped) */
			pair(p_parent ? x_base_field_true(p_parent) : nil, nil),
		pair(
			/* false object (stack-wrapped) */
			pair(p_parent ? x_base_field_false(p_parent) : nil, nil),
		pair(
			/* line counter (stack-wrapped) */
			pair(atom(1), nil),
		pair(
			/* profile: all stack-wrapped */
			pair(pair(atom(0), nil), pair(pair(atom(0), nil), pair(pair(atom(0), nil), nil))),
		pair(
			/* hooks: all stack-wrapped */
			pair(pair(atom(x_type_prim_type_name), nil),
			pair(pair(atom(x_type_prim_units), nil),
			pair(pair(atom(x_type_prim_length), nil),
			pair(pair(atom(x_base_error), nil),
			nil)))),
		pair(
			/* save-stack */
			nil,
		pair(
			/* obj-meta-extra (stack-wrapped) */
			pair(atom(0), nil),
		nil))))))))));

	/* Set x-obj hooks for the type system. */
	x_obj_hook_type_name = x_type_prim_type_name;
	x_obj_hook_units = x_type_prim_units;
	x_obj_hook_length = x_type_prim_length;
	x_obj_hook_error = x_base_error;

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
	x_obj_t *p_entry;
	x_spair_t args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL })
	};

	if ( ! x_base_isset(p_base)) {
		return NULL;
	}

	/* Wrap type struct as (name . type_struct) for alist keying */
	p_entry = x_mkspair(p_base, x_type_field_name(p_args), p_args);
	x_firstobj((x_obj_t *)args) = p_entry;
	x_restobj((x_obj_t *)args) = x_base_field_type_alist(p_base);

	return x_base_field_type_alist(p_base) = x_alist_extend(p_base, (x_obj_t *)args);
}

x_obj_t *x_base_type_alist_assoc(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_result;
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_firstobj(p_args) }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL })
	};

	if ( ! x_base_isset(p_base)) {
		return NULL;
	}

	x_firstobj((x_obj_t *)args[1]) = x_base_field_type_alist(p_base);

	p_result = x_alist_assoc(p_base, (x_obj_t *)args);

	/* Unwrap (name . type_struct) entry to return bare type struct */
	return x_obj_isnil(p_base, p_result) ? NULL : x_restobj(p_result);
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

x_obj_t *x_base_filein_push(x_obj_t *p_base, x_int_t fd)
{
	x_base_field_filein_stack(p_base) = x_mkspair(p_base,
		x_mksatom(p_base, fd), x_base_field_filein_stack(p_base));
	return x_base_field_filein(p_base);
}

x_obj_t *x_base_filein_pop(x_obj_t *p_base)
{
	x_obj_t *p_top = x_base_field_filein(p_base);
	x_base_field_filein_stack(p_base) =
		x_restobj(x_base_field_filein_stack(p_base));
	return p_top;
}

x_obj_t *x_base_buffer_push(x_obj_t *p_base, x_obj_t *p_buffer)
{
	x_base_field_buffer_stack(p_base) = x_mkspair(p_base,
		p_buffer, x_base_field_buffer_stack(p_base));
	return p_buffer;
}

x_obj_t *x_base_buffer_pop(x_obj_t *p_base)
{
	x_obj_t *p_top = x_base_field_buffer(p_base);
	x_base_field_buffer_stack(p_base) =
		x_restobj(x_base_field_buffer_stack(p_base));
	return p_top;
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

x_obj_t *x_base_write_str(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_atom = x_firstobj(p_args);
	x_char_t *str = x_atomstr(p_atom);
	x_int_t len = x_obj_isnil(p_base, x_restobj(p_args))
		? (x_int_t)x_lib_strlen(str)
		: x_atomint(x_firstobj(x_restobj(p_args)));
	x_satom_t data = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .s = str }),
		sz = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .i = len });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE,
			{ data }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { sz }, { NULL })
	};

	return x_base_write(p_base, (x_obj_t *)args);
}

x_obj_t *x_base_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_atom = x_firstobj(p_args);
	x_int_t size = x_atomint(x_firstobj(x_restobj(p_args)));

	if (x_base_isset(p_base)) {
		x_obj_t *p_buf = x_base_field_write_buf(p_base);

		if ( ! x_obj_isnil(p_base, p_buf)) {
			x_char_t *dst = (x_char_t *)x_ptrval(p_buf);
			x_int_t pos = x_atomint(x_restobj(p_buf));

			x_lib_memcpy(dst + pos, x_atomstr(p_atom), size);
			x_atomint(x_restobj(p_buf)) = pos + size;
			return p_atom;
		}
	}

	{
		int fd = x_base_isset(p_base)
			? x_atomint(x_base_field_fileout(p_base)) : STDOUT_FILENO;

		if (x_sys_write(fd, x_atomstr(p_atom), size) == size) {
			return p_atom;
		}
	}

	return NULL;
}
