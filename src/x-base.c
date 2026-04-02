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
#include "x-base-typesystem.h"
#include "x-type.h"
#include "x-alist.h"
#include "x-eval.h"
#include "x-type/ptr.h"
#include "x-type/str.h"
#include <setjmp.h>

#include "x-token.h"

#define nil			NULL
#define pair(X,Y)	(x_mkspair(p_base, X_OBJ_FLAG_NONE, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, X_OBJ_FLAG_NONE, (X)))

static x_satom_t x_type_prim_type_name_hook =
	x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = x_type_prim_type_name });
static x_satom_t x_type_prim_units_hook =
	x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = x_type_prim_units });
static x_satom_t x_type_prim_length_hook =
	x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = x_type_prim_length });
static x_satom_t x_base_error_hook =
	x_obj_set(NULL, X_OBJ_FLAG_NONE, { .v = (void *)x_base_error });
static x_satom_t x_type_heap_mark_hook =
	x_obj_set(NULL, X_OBJ_FLAG_NONE, { .v = (void *)x_type_heap_mark });
static x_satom_t x_type_heap_free_hook =
	x_obj_set(NULL, X_OBJ_FLAG_NONE, { .v = (void *)x_type_heap_free });

x_obj_t *x_base_ts_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_parent = p_base;
	struct x_base_t base_cfg;

	base_cfg.filein = STDIN_FILENO;
	base_cfg.fileout = STDOUT_FILENO;
	base_cfg.fileerr = STDERR_FILENO;
	base_cfg.p_hook_type_name = (x_obj_t *)x_type_prim_type_name_hook;
	base_cfg.p_hook_units = (x_obj_t *)x_type_prim_units_hook;
	base_cfg.p_hook_length = (x_obj_t *)x_type_prim_length_hook;
	base_cfg.p_hook_error = (x_obj_t *)x_base_error_hook;
	base_cfg.obj_meta_extra = 0;
	base_cfg.p_heap_mark = (x_obj_t *)x_type_heap_mark_hook;
	base_cfg.p_heap_free = (x_obj_t *)x_type_heap_free_hook;
	base_cfg.p_stack_base = nil;

	p_base = x_base_make(p_base, base_cfg);

	/* Set base type (x-expr uses NULL). */
	x_obj_type(p_base) = x_type_base_obj;

	/* Replace x-expr's p_base sentinel in type-alist slot with a real stack cell. */
	x_firstobj(x_firstobj(x_base_field_io_group(p_base))) = pair(nil, nil);

	/* Fill env+ctrl (x-expr leaves nil). */
	x_base_hot(p_base) = pair(
		/* env-group */
		pair(
			/* env-alist */
			pair(nil, nil),
			/* env-aux: (local-boundary . (global-tree . shadow-list)) */
			pair(nil, pair(nil, nil))),
		/* ctrl-group */
		pair(
			/* ctrl-head: (save-stack . error-handler) */
			pair(nil, pair(nil, nil)),
			/* tco: (tco-expr . tco-env) */
			pair(pair(nil, nil), pair(nil, nil))));

	/* Fill io-state (x-expr leaves nil). */
	x_base_io_state(p_base) = pair(
		/* line */
		pair(atom(1), nil),
		/* booleans: (true . false) */
		pair(
			pair(p_parent ? x_firstobj(x_base_field_true(p_parent)) : nil, nil),
			pair(p_parent ? x_firstobj(x_base_field_false(p_parent)) : nil, nil)));

	/* Extend profile (x-expr has 1 counter; we add 9 more). */
	x_restobj(x_base_field_profile(p_base)) =
		pair(pair(atom(0), nil), pair(pair(atom(0), nil),
		pair(pair(atom(0), nil), pair(pair(atom(0), nil),
		pair(pair(atom(0), nil), pair(pair(atom(0), nil),
		pair(pair(atom(0), nil), pair(pair(atom(0), nil),
		pair(pair(atom(0), nil),
		nil)))))))));

	/* Fill x-project-extras (x-expr leaves nil after heap-group). */
	x_base_extras(p_base) = pair(
		/* eval-list */
		pair(nil, nil),
		pair(
			/* token-cache */
			pair(nil, nil),
			pair(
				/* mark-hooks */
				pair(nil, nil),
				pair(
					/* free-hooks */
					pair(nil, nil),
					/* mark-roots */
					pair(nil, nil)))));

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
		&& ! x_obj_isnil(p_base, x_firstobj(x_base_field_error_handler(p_base)))) {
		x_obj_t *p_handler = x_firstobj(x_base_field_error_handler(p_base));
		x_int_t line = x_atomint(x_firstobj(x_base_field_line(p_base)));
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
		x_firstobj(x_base_field_env_alist(p_base))
			= x_error_handler_saved_env(p_handler);
		x_base_field_env_local_boundary(p_base)
			= x_error_handler_saved_boundary(p_handler);
		longjmp(*(jmp_buf *)x_error_handler_jmp(p_handler), 1);
	}

	fd = x_base_isset(p_base) ? x_atomint(x_firstobj(x_base_field_fileerr(p_base))) : STDERR_FILENO;

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
	p_entry = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_type_field_name(p_args), p_args);
	x_firstobj((x_obj_t *)args) = p_entry;
	x_restobj((x_obj_t *)args) = x_firstobj(x_base_field_type_alist(p_base));

	return x_firstobj(x_base_field_type_alist(p_base)) = x_alist_extend(p_base, (x_obj_t *)args);
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

	x_firstobj((x_obj_t *)args[1]) = x_firstobj(x_base_field_type_alist(p_base));

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

	x_restobj((x_obj_t *)args) = x_firstobj(x_base_field_env_alist(p_base));

	return x_firstobj(x_base_field_env_alist(p_base)) = x_alist_extend(p_base, (x_obj_t *)args);
}

x_obj_t *x_base_filein_push(x_obj_t *p_base, x_int_t fd)
{
	x_base_field_filein(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mksatom(p_base, X_OBJ_FLAG_NONE, fd), x_base_field_filein(p_base));
	return x_firstobj(x_base_field_filein(p_base));
}

x_obj_t *x_base_filein_pop(x_obj_t *p_base)
{
	x_obj_t *p_top = x_firstobj(x_base_field_filein(p_base));
	x_base_field_filein(p_base) =
		x_restobj(x_base_field_filein(p_base));
	return p_top;
}

x_obj_t *x_base_buffer_push(x_obj_t *p_base, x_obj_t *p_buffer)
{
	x_base_field_buffer(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		p_buffer, x_base_field_buffer(p_base));
	return p_buffer;
}

x_obj_t *x_base_buffer_pop(x_obj_t *p_base)
{
	x_obj_t *p_top = x_firstobj(x_base_field_buffer(p_base));
	x_base_field_buffer(p_base) =
		x_restobj(x_base_field_buffer(p_base));
	return p_top;
}

x_obj_t *x_base_load(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_firstobj(x_base_field_buffer(p_base));
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

