/** @file x-cli.c
 *  @brief Command-line interface, file inclusion, and interpreter entry point
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2024 Jon Ruttan
 *  @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
/*
 * # Includes
 */
#include "x-base-typesystem.h"
#include "x-heap.h"
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
/**
 * Raw syscall interface. x-lang: (syscall num arg ...)
 *
 * Accepts up to 7 arguments (syscall number + 6 parameters). Each
 * argument may be an integer or string; strings are passed as their
 * C pointer. Evaluates arguments and dispatches via the platform
 * syscall() function.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Unevaluated variadic args
 * @return x_obj_t* -- Integer syscall return value
 */
static x_obj_t *x_prim_syscall(x_obj_t *p_base, x_obj_t *p_args)
{
	long i = 0, p[7];
	x_obj_t *arg;

	p_args = x_1(p_args); /* variadic: skip self, walk rest */
	p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = p[6] = 0;

	while (!x_obj_isnil(p_base, p_args) && i < 7) {
		arg = x_eval_arg(p_base, x_firstobj(p_args));
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
/**
 * Load and evaluate a file. x-lang: (include path)
 *
 * Opens the file at path, pushes a new input state (fd, line counter,
 * read buffer) onto the base stacks, evaluates all expressions via
 * x_base_load, then pops and restores the previous input state.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Unevaluated args; path is a string
 * @return x_obj_t* -- Result of the last expression in the file
 *
 * @note Emits timing info to stderr when X_PROFILE is defined.
 */
static x_obj_t *x_prim_include(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_path, *p_buffer, *p_result;
	int fd;
	x_eargs(p_base, p_args, 2, NULL, &p_path);
	fd = x_sys_open(x_strval(p_path), 0 /* O_RDONLY */);
	x_char_t *buf;
#ifdef X_PROFILE
	x_int_t t0, t1;
	int err_fd;
	x_char_t tbuf[24];
#endif

	if (fd < 0) {
		x_obj_error(p_base, "include: cannot open", p_path);
		return NULL;
	}

	/* Push line counter for included file. */
	x_base_field_line(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mksatom(p_base, X_OBJ_FLAG_NONE, 1), x_base_field_line(p_base));

	/* Push new input state. */
	x_base_field_filein(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mksatom(p_base, X_OBJ_FLAG_NONE, fd), x_base_field_filein(p_base));

	buf = (x_char_t *)x_sys_malloc(X_CLI_BUFFER_SIZE);
	p_buffer = x_mkbufferown(p_base, buf);
	x_base_field_buffer(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		p_buffer, x_base_field_buffer(p_base));

	/* Load all expressions. */
#ifdef X_PROFILE
	t0 = x_sys_clock();
#endif
	p_result = x_base_load(p_base, p_base);
#ifdef X_PROFILE
	t1 = x_sys_clock();
	err_fd = x_atomint(x_firstobj(x_base_field_fileerr(p_base)));
	x_sys_write(err_fd, "[include] ", 10);
	x_sys_write(err_fd, x_strval(p_path),
		x_lib_strlen(x_strval(p_path)));
	x_sys_write(err_fd, ": ", 2);
	x_lib_inttostr(t1 - t0, tbuf, 10);
	x_sys_write(err_fd, tbuf, x_lib_strlen(tbuf));
	x_sys_write(err_fd, "us\n", 3);
#endif

	/* Pop and close, restore line counter. */
	x_base_field_filein(p_base)
		= x_restobj(x_base_field_filein(p_base));
	x_base_field_buffer(p_base)
		= x_restobj(x_base_field_buffer(p_base));
	x_sys_close(fd);
	x_base_field_line(p_base)
		= x_restobj(x_base_field_line(p_base));

	return p_result;
}
#endif /* X_INCLUDE */

#endif /* ! TESTS -- CLI-only helpers above */

/**
 * Initialize the interpreter.
 *
 * Creates the base object, registers all built-in types and primitives,
 * sets up the read buffer, and optionally binds the syscall and include
 * primitives (when X_SYSCALL / X_INCLUDE are defined).
 *
 * @param p_base   x_obj_t* -- Ignored (always creates a fresh base)
 * @param buffer   x_char_t* -- Pre-allocated read buffer
 * @return x_obj_t* -- Newly created base object
 */
x_obj_t * init(x_obj_t *p_base, x_char_t *buffer)
{
	x_obj_t *p_buffer;

	/* Create base object. */
	p_base = x_base_ts_make(NULL, NULL);

	/* Enable 1 metadata slot per object for source line tracking.
	 * Must be set before the first buffer is created so the buffer
	 * itself gets X_OBJ_FLAG_META (needed for newline counting). */
	x_atomint(x_firstobj(x_base_field_obj_meta_extra(p_base))) = 1;

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
	x_base_field_buffer(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		p_buffer, x_base_field_buffer(p_base));

	/* Register primitives. */
	x_prim_register(p_base, p_base);

#ifdef X_SYSCALL
	/* Register syscall primitive. */
	x_callable_bind(p_base, "syscall", x_prim_syscall);
#endif

#ifdef X_INCLUDE
	/* Register include primitive. */
	x_callable_bind(p_base, "include", x_prim_include);
#endif

	return p_base;
}

#ifndef TESTS

/**
 * CLI entry point.
 *
 * Initializes the interpreter, records the stack base for conservative
 * GC scanning, binds command-line arguments as the x-lang `args` list,
 * binds `x-machine` and `x-version` platform constants, then enters
 * the REPL.
 *
 * @param argc  int -- Argument count
 * @param argv  char*[] -- Argument vector
 * @return int -- Exit status (0 on success, 1 on init failure)
 */
int main(int argc, char *argv[])
{
	x_obj_t *p_base;
	x_char_t buffer[X_CLI_BUFFER_SIZE];
	x_obj_t *p_list = NULL;
	int i;

	(void)0; /* stack base set after init creates base */

	x_callcc_init();

	p_base = init(NULL, buffer);

	/* Record stack base for conservative GC stack scanning. */
	x_atomint(x_firstobj(x_base_field_stack_base(p_base)))
		= (x_int_t)(void *)&p_base;

	if (p_base == NULL) {
		x_error(STDERR_FILENO, "Error: ", "Initialization");
		return 1;
	}

	/* Bind args as a list of command-line argument strings. */
	for (i = argc - 1; i >= 0; i--) {
		p_list = x_mklist(p_base, x_mkstr(p_base, (x_char_t *)argv[i]), p_list);
	}
	x_value_bind(p_base, "args", p_list);

	/* Bind platform constants. */
	x_value_bind(p_base, "x-machine",
		x_mkstr(p_base, (x_char_t *)X_MACHINE));
	x_value_bind(p_base, "x-version",
		x_mkstr(p_base, (x_char_t *)X_VERSION));

	/* REPL. */
	x_prim_repl(p_base, NULL);

	return 0;
}


#endif /* TESTS */
