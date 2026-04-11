/** @file signal.c
 *  @brief SIGINT signal handling primitives.
 *
 *  One static atom whose .i field is the SIGINT flag (unavoidable:
 *  POSIX handlers receive only the signal number, not an application
 *  context).  Bound as %sigint-flag so x-lang can read/clear it
 *  directly with first-int / set-first-int!.
 *
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
#include "x-prim.h"
#include "x-base-typesystem.h"

#include <signal.h>

/** Static atom whose .i field is the SIGINT flag. */
x_satom_t x_sigint_flag = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .i = 0 });

static void x_sigint_handler(int sig)
{
	(void)sig;
	x_atomint(x_sigint_flag) = 1;
}

/** Install the SIGINT handler.
 *  x-lang: (sigint-install)
 */
static x_obj_t *x_prim_sigint_install(x_obj_t *p_base, x_obj_t *p_args)
{
	struct sigaction sa;
	(void)p_args;

	sa.sa_handler = x_sigint_handler;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, NULL);

	return NULL;
}

/** Restore default SIGINT handling.
 *  x-lang: (sigint-restore)
 */
static x_obj_t *x_prim_sigint_restore(x_obj_t *p_base, x_obj_t *p_args)
{
	(void)p_args;
	signal(SIGINT, SIG_DFL);

	return NULL;
}

/** Register signal primitives and bind %sigint-flag. */
x_obj_t *x_prim_signal_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_callable_entry_t entries[] = {
		{ "sigint-install", x_prim_sigint_install },
		{ "sigint-restore", x_prim_sigint_restore }
	};

	(void)p_args;
	x_callable_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	x_value_bind(p_base, "%sigint-flag", (x_obj_t *)&x_sigint_flag);

	return p_base;
}
