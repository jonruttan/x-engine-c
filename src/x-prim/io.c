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
#include "x-heap.h"
#include "x-token.h"
#include "x-type/buffer.h"
#include "x-type/char.h"
#include "x-type/int.h"
#include "x-type/prim.h"
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

/* peek-char: (peek-char) -> peek at next character without consuming */
static x_obj_t *x_prim_peek_char(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_base_field_buffer(p_base);
	x_char_t ch;
	x_spair_t buf_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_buffer }, { p_base })
	};

	p_buffer = x_type_buffer_read(p_base, (x_obj_t *)buf_args);

	if (x_obj_isnil(p_base, p_buffer)) {
		return NULL;
	}

	ch = x_bufferlastchar(p_buffer);
	x_bufferread(p_buffer) -= 1;

	return x_mkchar(p_base, ch);
}

/* write-to-string: (write-to-string obj) -> string representation */
x_obj_t *x_prim_write_to_string(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_val;
	int pipefd[2];
	int saved_fd;
	x_char_t *result;
	x_int_t total, n;
	x_spair_t write_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL })
	};

	p_val = x_prim_eval_arg(p_base, x_firstobj(p_args));

	if (x_obj_isnil(p_base, p_val)) {
		return x_mkstrown(p_base, x_lib_strndup((x_char_t *)"", 0));
	}

	if (pipe(pipefd) < 0) {
		return NULL;
	}

	/* Swap fileout fd to pipe write end */
	saved_fd = x_atomint(x_base_field_fileout(p_base));
	x_atomint(x_base_field_fileout(p_base)) = pipefd[1];

	x_firstobj((x_obj_t *)write_args) = p_val;
	x_token_write(p_base, (x_obj_t *)write_args);

	/* Restore and close write end */
	x_atomint(x_base_field_fileout(p_base)) = saved_fd;
	x_sys_close(pipefd[1]);

	/* Read result from pipe */
	total = 0;
	result = (x_char_t *)x_sys_malloc(256);
	while ((n = x_sys_read(pipefd[0], result + total, 255 - total)) > 0) {
		total += n;
	}
	result[total] = '\0';
	x_sys_close(pipefd[0]);

	return x_mkstrown(p_base, result);
}

#ifdef X_CLOCK
/* clock: (clock) -> CPU microseconds since process start */
static x_obj_t *x_prim_clock(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_mkint(p_base, x_sys_clock());
}
#endif /* X_CLOCK */

#ifdef X_PROFILE
/* heap-sweep: (heap-sweep) -> sweep unmarked objects from heap */
static x_obj_t *x_prim_heap_sweep(x_obj_t *p_base, x_obj_t *p_args)
{
	x_heap_sweep(p_base, x_obj_heap(p_base), X_OBJ_FLAG_HEAP);

	return NULL;
}

/* heap-count: (heap-count) -> count objects on heap */
static x_obj_t *x_prim_heap_count(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p = x_obj_heap(p_base);
	long count = 0;

	while (p) {
		count++;
		p = x_obj_heap(p);
	}

	return x_mkint(p_base, count);
}
#endif /* X_PROFILE */

/* heap-mark: (heap-mark) -> mark reachable objects on heap */
static x_obj_t *x_prim_heap_mark(x_obj_t *p_base, x_obj_t *p_args)
{
	x_heap_mark(p_base, p_base, X_OBJ_FLAG_HEAP);

	return NULL;
}

/* current-line: (current-line) -> current input line number */
static x_obj_t *x_prim_current_line(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_mkint(p_base, x_atomint(x_base_field_line(p_base)));
}

/* repl: minimal read-eval loop until EOF (no output, no hooks) */
x_obj_t *x_prim_repl(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_exp;

	for (;;) {
		p_exp = x_prim_read_expr(p_base, NULL);
		if (x_obj_isnil(p_base, p_exp))
			break;
		x_prim_eval_arg(p_base, p_exp);
	}

	return NULL;
}

x_obj_t *x_prim_io_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_prim_bind(p_base, "write", x_prim_write);
	x_prim_bind(p_base, "display", x_prim_display);
	x_prim_bind(p_base, "newline", x_prim_newline);
	x_prim_bind(p_base, "read", x_prim_read_expr);
	x_prim_bind(p_base, "read-char", x_prim_read_char);
	x_prim_bind(p_base, "peek-char", x_prim_peek_char);
	x_prim_bind(p_base, "write-to-string", x_prim_write_to_string);
#ifdef X_CLOCK
	x_prim_bind(p_base, "clock", x_prim_clock);
#endif /* X_CLOCK */
	x_prim_bind(p_base, "heap-mark", x_prim_heap_mark);
#ifdef X_PROFILE
	x_prim_bind(p_base, "heap-sweep", x_prim_heap_sweep);
	x_prim_bind(p_base, "heap-count", x_prim_heap_count);
#endif /* X_PROFILE */
	x_prim_bind(p_base, "current-line", x_prim_current_line);

	return p_base;
}
