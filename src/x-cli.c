/*
 * # Computational Expressions in C
 *
 * ## x-cli.c -- Implementation - CLI
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
#include "x-base.h"
#include "x-prim.h"
#include "x-type/buffer.h"
#include "x-type/char.h"
#include "x-type/comment.h"
#include "x-type/int.h"
#include "x-type/list.h"
#include "x-type/operative.h"
#include "x-type/prim.h"
#include "x-type/procedure.h"
#include "x-type/str.h"
#include "x-type/symbol.h"
#include "x-type/whitespace.h"

#ifdef X_SYSCALL
#include <unistd.h>
#include <sys/syscall.h>
#endif

#define X_CLI_BUFFER_SIZE 65536

#ifndef TESTS

#ifdef X_SYSCALL
static x_obj_t *x_prim_syscall(x_obj_t *p_base, x_obj_t *p_args)
{
	long i = 0, p[7];
	x_obj_t *arg;

	p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = p[6] = 0;

	while (!x_obj_isnil(p_base, p_args) && i < 7) {
		arg = x_prim_eval_arg(p_base, x_firstobj(p_args));
		if (x_obj_type_isint(p_base, arg))
			p[i++] = x_intval(arg);
		else if (x_obj_type_isstr(p_base, arg))
			p[i++] = (long)x_strval(arg);
		p_args = x_restobj(p_args);
	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	return x_mkint(p_base,
		syscall(p[0], p[1], p[2], p[3], p[4], p[5], p[6]));
#pragma GCC diagnostic pop
}
#endif

#ifdef X_INCLUDE
static x_obj_t *x_prim_include(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_path = x_prim_eval_arg(p_base, x_firstobj(p_args));
	int fd = x_sys_open(x_strval(p_path), 0 /* O_RDONLY */);
	x_obj_t *p_buffer, *p_result;
	x_char_t *buf;

	if (fd < 0) {
		x_obj_error(p_base, "include: cannot open", p_path);
		return NULL;
	}

	/* Push line counter for included file. */
	x_base_field_line_stack(p_base) = x_mkspair(p_base,
		x_mksatom(p_base, 1), x_base_field_line_stack(p_base));

	/* Push new input state. */
	x_base_field_filein_stack(p_base) = x_mkspair(p_base,
		x_mksatom(p_base, fd), x_base_field_filein_stack(p_base));

	buf = (x_char_t *)x_sys_malloc(X_CLI_BUFFER_SIZE);
	p_buffer = x_mkbufferown(p_base, buf);
	x_base_field_buffer_stack(p_base) = x_mkspair(p_base,
		p_buffer, x_base_field_buffer_stack(p_base));

	/* Load all expressions. */
	p_result = x_base_load(p_base, p_base);

	/* Pop and close, restore line counter. */
	x_base_field_filein_stack(p_base)
		= x_restobj(x_base_field_filein_stack(p_base));
	x_base_field_buffer_stack(p_base)
		= x_restobj(x_base_field_buffer_stack(p_base));
	x_sys_close(fd);
	x_base_field_line_stack(p_base)
		= x_restobj(x_base_field_line_stack(p_base));

	return p_result;
}
#endif /* X_INCLUDE */

#endif /* ! TESTS -- CLI-only helpers above */

x_obj_t * init(x_obj_t *p_base, x_char_t *buffer)
{
	x_obj_t *p_buffer;

	/* Create base object. */
	p_base = x_base_make(NULL, NULL);

	/* Register types for parsing. */
	x_type_prim_register(p_base, p_base);
	x_type_operative_register(p_base, p_base);
	x_type_procedure_register(p_base, p_base);
	x_type_symbol_register(p_base, p_base);
	x_type_list_register(p_base, p_base);
	x_type_int_register(p_base, p_base);
	x_type_str_register(p_base, p_base);
	x_type_char_register(p_base, p_base);
	x_type_whitespace_register(p_base, p_base);
	x_type_comment_register(p_base, p_base);

	/* Set up read buffer. */
	p_buffer = x_mkbuffer(p_base, buffer);
	x_base_field_buffer_stack(p_base) = x_mkspair(p_base,
		p_buffer, x_base_field_buffer_stack(p_base));

	/* Register primitives. */
	x_prim_register(p_base, p_base);

#ifdef X_SYSCALL
	/* Register syscall primitive. */
	{
		x_obj_t *p_sym = x_make_symbol(p_base, X_OBJ_FLAG_NONE,
			(x_char_t *)"syscall");
		x_obj_t *p_prim = x_mkprim(p_base, x_prim_syscall);
		x_obj_t *p_pair = x_mkspair(p_base, p_sym, p_prim);
		x_base_env_alist_extend(p_base, p_pair);
	}
#endif

#ifdef X_INCLUDE
	/* Register include primitive. */
	x_prim_bind(p_base, "include", x_prim_include);
#endif

	return p_base;
}

#ifndef TESTS

int main(int argc, char *argv[])
{
	x_obj_t *p_base;
	x_char_t buffer[X_CLI_BUFFER_SIZE];
	x_obj_t *p_sym, *p_list = NULL, *p_pair;
	int i;

	x_callcc_init();

	p_base = init(NULL, buffer);

	if (p_base == NULL) {
		x_error(STDERR_FILENO, "Error: ", "Initialization");
		return 1;
	}

	/* Bind args as a list of command-line argument strings. */
	p_sym = x_make_symbol(p_base, X_OBJ_FLAG_NONE, (x_char_t *)"args");

	for (i = argc - 1; i >= 0; i--) {
		p_list = x_mklist(p_base, x_mkstr(p_base, (x_char_t *)argv[i]), p_list);
	}

	p_pair = x_mkspair(p_base, p_sym, p_list);
	x_base_env_alist_extend(p_base, p_pair);

	/* REPL. */
	x_prim_repl(p_base, NULL);

	return 0;
}


#endif /* TESTS */
