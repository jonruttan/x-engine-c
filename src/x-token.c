/*
 * # Computational Expressions in C
 *
 * ## x-token.c -- Implementation - Token
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
#include <ctype.h>
#include "x-base.h"
#include "x-obj.h"
#include "x-token.h"
#include "x-type.h"
#include "x-type/buffer.h"
#include "x-type/char.h"
#include "x-type/int.h"
#include "x-type/iter.h"
#include "x-type/prim.h"
#include "x-type/str.h"
#include "x-type/list.h"

/*
 * # Tokenization Functions
 */

/* VESTIGIAL, being replaced by token_read */
x_obj_t *x_token_get(x_obj_t *p_base, x_obj_t *p_args)
{
	int fd = x_base_isset(p_base) ? x_atomint(x_base_field_fileout(p_base)) : STDOUT_FILENO;
	x_obj_t *p_obj, *p_char = x_mkchar(p_base, '\0');
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_base_field_buffer(p_base) }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_char }, { NULL })
	};

	/* If there's a cached token return it */
	if ( ! x_obj_isnil(p_base, x_base_field_token_cache(p_base))) {
		p_obj = x_base_field_token_cache(p_base);
		x_base_field_token_cache(p_base) = p_base;

		return p_obj;
	}

	for (;;) {
		/* Reset the token buffer */
		x_type_buffer_reset(p_base, (x_obj_t *)args);

		if (x_obj_isnil(p_base, x_type_buffer_read(p_base, (x_obj_t *)args))) {
			x_obj_debug(p_base, "EOF (l.%d)", __LINE__);
			x_sys_exit(X_SYS_EXIT_FAILURE);

			return p_base;
		}

		/* Handle comment */
		if (';' == *x_bufferval(x_base_field_buffer(p_base))) {
			while ('\n' != *x_bufferval(x_base_field_buffer(p_base))) {
				/* Reset the token buffer */
				x_type_buffer_reset(p_base, (x_obj_t *)args);

				if (x_obj_isnil(p_base, x_type_buffer_read(p_base, (x_obj_t *)args))) {
					x_obj_debug(p_base, "EOF (l.%d)", __LINE__);
					x_sys_exit(X_SYS_EXIT_FAILURE);

					return p_base;
				}
			}
		}

		if (isgraph(*x_bufferval(x_base_field_buffer(p_base)))) {
			break;
		}
	}

	for (;;) {
		if (x_lib_strchr("()\'\"`,@ \f\n\r\t\v", x_bufferlastchar(x_base_field_buffer(p_base)))) {
			x_type_buffer_append(p_base, (x_obj_t *)args);

			return x_base_field_buffer(p_base);
		}

		if (x_obj_isnil(p_base, x_type_buffer_read(p_base, (x_obj_t *)args))) {
			x_obj_debug(p_base, "EOF (l.%d)", __LINE__);
			x_sys_exit(X_SYS_EXIT_FAILURE);

			return p_base;
		}
	}

	x_obj_debug(p_base, "Unhandled (l.%d)", __LINE__);
	x_sys_exit(X_SYS_EXIT_FAILURE);

	return p_base;

	/* When a delimiter is found, convert the buffer to an object */
	if (x_lib_strchr("()\'\"`,@", x_charval(p_char))) {
		x_charval(p_char) = 0;

		return x_base_field_buffer(p_base);
	}

	for (;;) {
		if ((x_charval(p_char) = x_sys_read_char(fd)) == X_SYS_EOF) {
			x_obj_debug(p_base, "EOF (l.%d)", __LINE__);
			x_sys_exit(0);
		}

		if (x_lib_strchr("()\'\"`,@", x_charval(p_char)) || isspace(x_charval(p_char))) {

			return x_base_field_buffer(p_base);
		}

		x_type_buffer_append(p_base, (x_obj_t *)args);
	}

	x_charval(p_char) = 0;

	return p_base;
}


#define prim_arg_prim			x_0((x_obj_t *)prim_args)
#define prim_arg_buffer			x_01((x_obj_t *)prim_args)

x_obj_t *x_token_delimit(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_firstobj(p_args),
		*p_type = x_01(p_args),
		*p_types = x_base_field_type_alist(p_base);
	x_spair_t prim_args[1] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { p_args }),
		};

	/* Cycle through of all of the types. */
	while ( ! x_obj_isnil(p_base, p_types)) {
		prim_arg_prim = x_type_field_delimit(x_firstobj(p_types));

		if (p_type != x_firstobj(p_types)
			&& ! x_obj_isnil(p_base, prim_arg_prim)
			&& x_type_prim_call(p_base, (x_obj_t *)prim_args) == p_buffer
		) {
			return p_buffer;
		}

		p_types = x_restobj(p_types);
	}

	return p_base;
}

x_obj_t *x_type_alist_iter(x_obj_t *p_base, x_obj_t *p_args);
x_satom_t x_type_alist_iter_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_type_alist_iter });

x_obj_t *x_type_alist_iter(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_type_list_iter(p_base, p_args);

	if (x_obj_isnil(p_base, p_obj)) {
		return p_obj;
	}

	return x_type_field_analyse(p_obj);
}

#ifdef DEBUG
#define dprintf printf
#else
#define dprintf(fmt, ...)
#endif /* DEBUG */


x_obj_t *x_token_analyse(x_obj_t *p_base, x_obj_t *p_args)
{
	x_int_t i_best;
	x_obj_t *p_buffer = x_firstobj(p_args), *p_read, *p_analyse, *p_obj;
	x_satom_t chr = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .c = '\0' } );
	x_spair_t
		score = x_obj_set(NULL, X_OBJ_FLAG_NONE, {}),
		type_iter = x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_alist_iter_prim }, { x_base_field_type_alist(p_base) }),
		iter_args[2] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { type_iter }, { (x_obj_t *)(iter_args + 1) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL }),
		},
		buffer_args[3] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_buffer }, { (x_obj_t *)(buffer_args + 1) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { score }, { (x_obj_t *)(buffer_args + 2) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL }),
		},
		prim_args[1] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { (x_obj_t *)(buffer_args) }),
		},
		read_args[2] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_buffer }, { (x_obj_t *)(read_args + 1) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { chr }, { NULL }),
		};
	x_obj_t *p_score = (x_obj_t *)score;

	/* Cycle through of all of the types. */
	p_read = p_base;
	i_best = 0;

	dprintf("Building:\n");
	while ( ! x_iterempty(p_base, (x_obj_t *)type_iter)) {
		p_analyse = x_type_iter_next(p_base, (x_obj_t *)iter_args);

		if ( ! x_obj_isnil(p_base, p_analyse)) {

			while (1) {

				/* TODO: Handle error. */
				x_type_buffer_read_text(p_base, (x_obj_t *)read_args);

				prim_arg_prim = p_analyse;
				p_obj = x_type_prim_call(p_base, (x_obj_t *)prim_args);

				/* Not recognized. */
				if (x_obj_isnil(p_base, p_obj)) {
					x_bufferread(p_buffer) = x_bufferval(p_buffer);
					break;
				}

				if (p_obj == (x_obj_t *)buffer_args) {
					continue;
				}

				if (p_obj == p_score) {
					/* If this is the longest match, mark this type as best. */
					if (x_firstint(p_score) >= i_best || (i_best < 1 && x_firstint(p_score) <= i_best)) {
						dprintf("Best %ld.\n", x_firstint(p_score));
						i_best = x_firstint(p_score);
						p_read = x_restobj(p_score);
					}

					x_bufferread(p_buffer) = x_bufferval(p_buffer);
					dprintf("Reset\n");
					break;
				}

				/* Replace Analyser. */
				p_analyse = p_obj;
			}
		}
	}

	dprintf("Processing:\n");

	x_bufferread(p_buffer) = x_bufferread(p_buffer) + x_lib_abs(i_best);

	return p_read;
}

x_obj_t *x_token_read(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_firstobj(p_args), *p_read, *p_obj;
	x_spair_t buffer_args[3] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_buffer }, { (x_obj_t *)(buffer_args + 1) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL }),
		},
		prim_args[1] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { (x_obj_t *)(buffer_args) }),
		};

x_token_read_reset:
	p_read = x_token_analyse(p_base, p_args);

	/* If a type was matched, call the Reader. */
	if ( ! x_obj_isnil(p_base, p_read)) {
		prim_arg_prim = p_read;

		p_obj = x_type_prim_call(p_base, (x_obj_t *)prim_args);

		/* TODO: Handle error response. */
		if (x_obj_isnil(p_base, p_obj)) {
			x_obj_debug(p_base, "EOF (l.%d)", __LINE__);
			x_sys_exit(X_SYS_EXIT_FAILURE);
			return p_base;
		}

		x_type_buffer_retain(p_base, (x_obj_t *)buffer_args);

		/* Not a token, fetch another. */
		if (p_obj == (x_obj_t *)buffer_args) {
			goto x_token_read_reset;
		}

		return p_obj;
	}

	dprintf("No match.\n");

	return p_base;
}
