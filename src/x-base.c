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
#include "x-alist.h"
#include "x-eval.h"
#include "x-type/ptr.h"

#include "x-token.h"

#define nil			NULL
#define pair(X,Y)	(x_mkspair(p_base, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, (X)))

x_obj_t *x_base_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_parent = p_base;

	p_base = x_obj_make(p_base, x_type_base_obj, X_OBJ_FLAG_BASE,
		X_OBJ_LENGTH_ATOM, p_base);
	x_atomobj(p_base) = pair(
		nil,
		pair(
			pair(pair(atom(STDIN_FILENO), nil),
			pair(atom(STDOUT_FILENO),
			pair(atom(STDERR_FILENO),
			nil))),
		pair(
			pair(nil,
			pair(nil,
			pair(pair(nil, nil),
			pair(nil,
			pair(nil,
			pair(nil,
			pair(nil,
			nil))))))),
		nil)));

	/* Append p_true and line counter slots after base is rooted (GC-safe).
	 * Inherit parent's cached t symbol for child bases. */
	x_restobj(x_restobj(x_restobj(x_firstobj(p_base)))) =
		pair(p_parent ? x_base_field_true(p_parent) : nil,
		pair(atom(1),
#ifdef X_PROFILE
		pair(pair(atom(0), pair(atom(0), pair(atom(0), nil))),
#endif
		nil
#ifdef X_PROFILE
		)
#endif
		));

	return p_base;
}

#undef nil
#undef pair
#undef atom

/*
 * Output an error message to *stderr*, then **exit**.
 */
void x_obj_error(x_obj_t *p_base, x_char_t *message, x_char_t *symbol)
{
	int fd;

	/* If an error handler is installed, longjmp to it. */
	if (x_base_isset(p_base)
		&& ! x_obj_isnil(p_base, x_base_field_error_handler(p_base))) {
		x_error_handler_t *handler =
			(x_error_handler_t *)x_ptrval(x_base_field_error_handler(p_base));
		x_int_t line = x_atomint(x_base_field_line(p_base));
		size_t msg_len = x_lib_strlen(message);
		/* Build combined message: "message ['symbol] (line N)" */
		/* Max line number digits: 20. Format: " (line " + digits + ")" = 28 */
		size_t sym_len = symbol ? x_lib_strlen(symbol) : 0;
		size_t total = msg_len + (symbol ? 2 + sym_len : 0) + 28;
		x_char_t *combined = (x_char_t *)x_sys_malloc(total);
		size_t pos = 0;

		x_lib_memcpy(combined, message, msg_len);
		pos = msg_len;
		if (symbol) {
			combined[pos++] = ' ';
			combined[pos++] = '\'';
			x_lib_memcpy(combined + pos, symbol, sym_len);
			pos += sym_len;
		}
		/* Append line number. */
		{
			x_char_t line_buf[24];
			x_int_t n = line, len = 0, i;
			if (n == 0) {
				line_buf[len++] = '0';
			} else {
				while (n > 0) {
					line_buf[len++] = '0' + (n % 10);
					n /= 10;
				}
			}
			combined[pos++] = ' ';
			combined[pos++] = '(';
			combined[pos++] = 'l';
			combined[pos++] = 'i';
			combined[pos++] = 'n';
			combined[pos++] = 'e';
			combined[pos++] = ' ';
			for (i = len - 1; i >= 0; i--)
				combined[pos++] = line_buf[i];
			combined[pos++] = ')';
		}
		combined[pos] = '\0';
		handler->error_msg = combined;
		handler->error_msg_owned = 1;

		x_base_field_env_alist(p_base) = handler->p_saved_env;
		longjmp(handler->jmp, 1);
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
