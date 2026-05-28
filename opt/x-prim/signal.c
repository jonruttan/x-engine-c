/** @file signal.c
 *  @brief SIGINT signal handling primitives.
 *
 *  A static atom's .i field is the SIGINT flag (unavoidable: POSIX handlers
 *  receive only the signal number, not an application context).  Its pointer
 *  is published into the base (x_interp_field_sigint) so x_eval can poll it via
 *  p_base rather than naming this module's global.  Bound as %sigint-flag so
 *  x-lang can read/clear it with first-int / set-first-int!.
 *
 *  This is the signal-support module: it is compiled and linked only when
 *  X_SIGNAL is enabled (the Makefile drops it from the build otherwise, and
 *  the x-lang library falls back to inert no-ops).
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
#include "x-interp.h"

#include <signal.h>

/** Static atom whose .i field is the SIGINT flag.  Kept in static (non-heap)
 *  storage so the handler can write it without risk of a GC relocation moving
 *  it mid-store. */
static x_satom_t x_sigint_flag = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .i = 0 });

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
	x_callable_bind_table(p_base, entries, sizeof(entries) / sizeof(entries[0]));

	/* Publish the flag pointer onto the base so x_eval can poll it without
	 * naming this module's global, and bind it for x-lang (%sigint-flag). */
	x_firstobj(x_interp_field_sigint(p_base)) = (x_obj_t *)&x_sigint_flag;
	x_value_bind(p_base, "%sigint-flag", (x_obj_t *)&x_sigint_flag);

	return p_base;
}
