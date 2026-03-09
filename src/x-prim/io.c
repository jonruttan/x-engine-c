/*
 * # Computational Expressions in C
 *
 * ## x-prim/io.c -- Implementation - Primitives - I/O
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2024 Jon Ruttan
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
#include "x-prim.h"
#include "x-base.h"
#include "x-eval.h"
#include "x-gc.h"
#include "x-token.h"
#include "x-type/buffer.h"
#include "x-type/char.h"
#include "x-type/int.h"
#include "x-type/str.h"

/* write: (write obj) -> output s-expression to stdout */
static x_obj_t *x_prim_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_val = x_prim_eval_arg(p_base, x_firstobj(p_args));
	x_spair_t write_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_val }, { NULL })
	};

	x_token_write(p_base, (x_obj_t *)write_args);

	return NULL;
}

/* display: (display obj) -> output human-readable (strings unquoted) */
static x_obj_t *x_prim_display(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_val = x_prim_eval_arg(p_base, x_firstobj(p_args));

	if (x_obj_type_isstr(p_base, p_val)) {
		int fd = x_base_isset(p_base)
			? x_atomint(x_base_field_fileout(p_base)) : STDOUT_FILENO;
		x_char_t *s = x_strval(p_val);

		x_sys_write(fd, s, x_lib_strlen(s));
	} else {
		x_spair_t write_args[1] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_val }, { NULL })
		};

		x_token_write(p_base, (x_obj_t *)write_args);
	}

	return NULL;
}

/* newline: (newline) -> output newline character */
static x_obj_t *x_prim_newline(x_obj_t *p_base, x_obj_t *p_args)
{
	int fd = x_base_isset(p_base)
		? x_atomint(x_base_field_fileout(p_base)) : STDOUT_FILENO;

	x_sys_write(fd, "\n", 1);

	return NULL;
}

/* read: (read) -> read one s-expression from stdin */
static x_obj_t *x_prim_read_expr(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_base_field_buffer(p_base);
	x_spair_t read_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_buffer }, { p_base })
	};

	return x_token_read(p_base, (x_obj_t *)read_args);
}

/* read-char: (read-char) -> read one character from stdin */
static x_obj_t *x_prim_read_char(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_base_field_buffer(p_base);
	x_spair_t buf_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_buffer }, { p_base })
	};

	p_buffer = x_type_buffer_read(p_base, (x_obj_t *)buf_args);

	if (x_obj_isnil(p_base, p_buffer)) {
		return NULL;
	}

	return x_mkchar(p_base, x_bufferlastchar(p_buffer));
}

#ifdef X_CLOCK
/* clock: (clock) -> CPU microseconds since process start */
static x_obj_t *x_prim_clock(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_mkint(p_base, x_sys_clock());
}
#endif /* X_CLOCK */

/* gc: (gc) -> trigger garbage collection (mark reachable objects) */
static x_obj_t *x_prim_gc(x_obj_t *p_base, x_obj_t *p_args)
{
	x_gc_mark(p_base, p_base, X_OBJ_FLAG_GC);

	return NULL;
}

/* repl: (repl) -> read-eval-print loop until EOF */
x_obj_t *x_prim_repl(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_base_field_buffer(p_base);
	x_obj_t *p_exp, *p_result = NULL;
	int fd = x_base_isset(p_base)
		? x_atomint(x_base_field_fileout(p_base)) : STDOUT_FILENO;
	x_satom_t exp_wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL });
	x_spair_t eval_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { exp_wrap }, { NULL })
	};
	x_spair_t read_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_buffer }, { p_base })
	};
	x_spair_t write_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL })
	};

	for (;;) {
		x_sys_write(fd, "> ", 2);

		p_exp = x_token_read(p_base, (x_obj_t *)read_args);

		if (x_obj_isnil(p_base, p_exp)) {
			x_sys_write(fd, "\n", 1);
			break;
		}

		x_firstobj((x_obj_t *)exp_wrap) = p_exp;
		p_result = x_eval(p_base, (x_obj_t *)eval_args);

		if ( ! x_obj_isnil(p_base, p_result)) {
			x_firstobj((x_obj_t *)write_args) = p_result;
			x_token_write(p_base, (x_obj_t *)write_args);
		}

		x_sys_write(fd, "\n", 1);
	}

	return p_result;
}

x_obj_t *x_prim_io_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_prim_bind(p_base, "write", x_prim_write);
	x_prim_bind(p_base, "display", x_prim_display);
	x_prim_bind(p_base, "newline", x_prim_newline);
	x_prim_bind(p_base, "read", x_prim_read_expr);
	x_prim_bind(p_base, "read-char", x_prim_read_char);
#ifdef X_CLOCK
	x_prim_bind(p_base, "clock", x_prim_clock);
#endif /* X_CLOCK */
	x_prim_bind(p_base, "gc", x_prim_gc);
	x_prim_bind(p_base, "repl", x_prim_repl);

	return p_base;
}
