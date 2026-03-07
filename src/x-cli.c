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
#include "x-eval.h"
#include "x-prim.h"
#include "x-sexp.h"
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

#ifndef TESTS

#define X_CLI_BUFFER_SIZE 256

int main(int argc, char *argv[])
{
	x_obj_t *p_base, *p_buffer, *p_read_args, *p_exp, *p_result;
	x_char_t buffer[X_CLI_BUFFER_SIZE];
	x_satom_t exp_wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL });
	x_spair_t eval_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { exp_wrap }, { NULL })
	},
	write_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL })
	};
	(void)argc;
	(void)argv;

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
	x_base_field_buffer(p_base) = p_buffer;
	p_read_args = x_mkspair(p_base, p_buffer, p_base);

	/* Register primitives. */
	x_prim_register(p_base, p_base);

	/* REPL loop. */
	for (;;) {
		x_sys_write(STDOUT_FILENO, "> ", 2);

		/* Read. */
		p_exp = x_sexp_read(p_base, p_read_args);

		if (x_obj_isnil(p_base, p_exp)) {
			x_sys_write(STDOUT_FILENO, "\n", 1);
			break;
		}

		/* Eval. */
		x_firstobj((x_obj_t *)exp_wrap) = p_exp;
		p_result = x_eval(p_base, (x_obj_t *)eval_args);

		/* Print. */
		if ( ! x_obj_isnil(p_base, p_result)) {
			x_firstobj((x_obj_t *)write_args) = p_result;
			x_sexp_write(p_base, (x_obj_t *)write_args);
		}

		x_sys_write(STDOUT_FILENO, "\n", 1);
	}

	return 0;
}

#endif
