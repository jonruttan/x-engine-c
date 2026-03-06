/*
 * # Computational Expressions in C
 *
 * ## x-cli.c -- Implementation - CLI
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
 * Compile with:
 *
 *     gcc -DX_MACHINE="\"$(gcc -dumpmachine)\"" -o x src/*.c
 *
 */
/*
 * # Includes
 */
#define _GNU_SOURCE			/* For *syscall* */
#include <stdlib.h>
#include <unistd.h>
/*#include "x-eval.h"*/

/*
 * # Primitives
 */
/*x_obj_t *x_prim_syscall(x_obj_t *p_base, x_obj_t *args) {
	long i=0, p[6] = { 0, 0, 0, 0, 0, 0 };

	for (; args != nil; args = x_cdr(args))
		switch (x_obj_type(x_car(args))) {
		case X_INTEGER:
				p[i++] = x_intval(x_car(args));
				break;

		case X_STRING:
				p[i++] = (long)x_strval(x_car(args));
				break;

		default:
			break;
		}

	return x_mkint(p_base, syscall(p[0], p[1], p[2], p[3], p[4], p[5]));
}
*/
#ifndef TESTS

/*
 * Main Driver
 */
int main(int argc, char *argv[], char *envp[]) {
/*	x_obj_t *p_base = x_init(STDIN_FILENO, STDOUT_FILENO), *args = x_mkvector(p_base, argc);
	int i;

	x_extend_top(p_base, x_intern(p_base, "syscall"), x_primop(p_base, x_prim_syscall));
	x_extend_top(p_base, x_intern(p_base, "args"), args);

	for (i = 0; i < argc; i++) {
		x_vectorval(args, i) = x_mkstr(p_base, argv[i]);
	}

	for (;;) {
		x_obj_t *exp = x_eval(p_base, x_readobj(p_base), x_vectorval(p_base, X_I_EXPR));
		x_eval(p_base, x_proccall(p_base, "write", exp), x_vectorval(p_base, X_I_EXPR));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
		x_write(STDOUT_FILENO, "\n", 1);
#pragma GCC diagnostic pop
		x_eval(p_base, x_proccall(p_base, "gc", nil), x_vectorval(p_base, X_I_EXPR));
	}
	x_fini(p_base);

	return X_EXIT_SUCCESS;
*/
	return 0;
}

#endif
