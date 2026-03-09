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

/* repl: (repl) -> read-eval-print loop until EOF
 *
 * Parses two x-lang forms from a source string:
 *   1. Setup: saves %do (before libraries can redefine do),
 *      defines temp variables %repl-exp and %repl-result
 *   2. Step: one REPL iteration (prompt, read, eval, print)
 * The C code loops evaluating the step form directly (no closure),
 * so def bindings from (eval exp) persist across iterations. */
x_obj_t *x_prim_repl(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_char_t repl_src[] =
		"(do (def %do do) (def %repl-exp ()) (def %repl-result ()))"
		" (%do (display \"" X_REPL_PROMPT "\")"
		" (set %repl-exp (read))"
		" (if (null? %repl-exp) ()"
		"  (%do (set %repl-result (eval %repl-exp))"
		"   (if (null? %repl-result) () (write %repl-result))"
		"   (newline) t)))";
	x_int_t len = sizeof(repl_src) - 1;
	x_char_t *buf = (x_char_t *)x_lib_strndup(repl_src, len);
	x_obj_t *p_buffer, *p_setup, *p_step, *p_result;

	p_buffer = x_mkfbufferown(p_base, X_OBJ_FLAG_RO, buf);
	x_bufferwrite(p_buffer) = x_bufferval(p_buffer) + len;

	{
		x_spair_t read_args[1] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_buffer }, { p_base })
		};

		/* Parse and eval setup (def temp vars). */
		p_setup = x_token_read(p_base, (x_obj_t *)read_args);

		{
			x_satom_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_setup });
			x_spair_t eval_args[1] = {
				x_obj_set(NULL, X_OBJ_FLAG_NONE, { wrap }, { NULL })
			};

			x_eval(p_base, (x_obj_t *)eval_args);
		}

		/* Parse step form (reused each iteration). */
		p_step = x_token_read(p_base, (x_obj_t *)read_args);
	}

	/* REPL loop: eval step form until it returns nil (EOF). */
	p_result = NULL;

	for (;;) {
		x_satom_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_step });
		x_spair_t eval_args[1] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { wrap }, { NULL })
		};

		p_result = x_eval(p_base, (x_obj_t *)eval_args);

		if (x_obj_isnil(p_base, p_result)) {
			break;
		}
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
