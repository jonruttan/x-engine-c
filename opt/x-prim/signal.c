/** @file signal.c
 *  @brief Signal handling primitives: SIGINT, and any signal recorded.
 *
 *  signal catch and signal take record any other signal's arrival in a static
 *  flag and report it; what an arrival means is left to x-lang.
 *
 *  A static atom's .i field is the SIGINT flag (unavoidable: POSIX handlers
 *  receive only the signal number, not an application context).  Its pointer
 *  is published into the base (x_eval_field_sigint) so x_eval can poll it via
 *  p_base rather than naming this module's global.  Bound as %sigint-flag so
 *  x-lang can read/clear it with first-int / set-first-int!.
 *
 *  This is the signal-support module: it is compiled and linked only when
 *  X_SIGNAL is enabled (the Makefile drops it from the build otherwise, and
 *  the x-lang library falls back to inert no-ops).
 *
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2026 Jon Ruttan
 *  @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
/* sigaction(2)/sigemptyset(3) are POSIX, not ISO C.  This module builds under
 * -ansi, which defines __STRICT_ANSI__; on glibc that makes <features.h> hide
 * every non-ISO-C declaration unless a feature-test macro asks for them, so
 * without this `struct sigaction` is an unknown type.  Must precede all
 * #includes.  (No effect on macOS, whose headers stay at __DARWIN_C_FULL
 * because -ansi does not define _ANSI_SOURCE.) */
#define _GNU_SOURCE

#include "x-prim.h"
#include "x-eval.h"
#include "x-type/int.h"

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
	/* NO SA_RESTART, deliberately: ctrl-c at a blocked REPL read works
	 * BY interrupting the read (EINTR -> EOF latch -> clean exit or
	 * cancel, see lib/x/repl/loop.x).  Restarting the read would leave
	 * the flag unseen until the next eval poll, deadening ctrl-c at an
	 * idle prompt (x-lang#170). */
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

/** One flag a signal number: set by the handler when that signal arrives,
 *  cleared by signal-take.  Static, so the handler writes no heap object. */
static volatile sig_atomic_t x_signal_arrived[NSIG];

static void x_signal_record(int sig)
{
	x_signal_arrived[sig] = 1;
}

/** Whether @p sig names a signal this module can record. */
static int x_signal_valid(x_int_t sig)
{
	return sig > 0 && sig < NSIG;
}

/** Catch a signal: from now on its arrival is recorded, for signal-take to
 *  report, and nothing else is done.  What the arrival means is the caller's
 *  to decide.
 *  x-lang: (signal catch sig)
 *
 *  @param p_base  Base (execution context).
 *  @param p_args  The signal number.
 *  @return 0, or -1 for a number that names no signal or a signal that cannot
 *          be caught.
 */
static x_obj_t *x_prim_signal_catch(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_sig;
	struct sigaction sa;
	x_int_t sig;

	x_eargs(p_base, p_args, 2, NULL, &p_sig);
	sig = x_intval(p_sig);
	if ( ! x_signal_valid(sig)) {
		return x_mkint(p_base, -1);
	}

	sa.sa_handler = x_signal_record;
	/* No SA_RESTART, as for SIGINT: a read or poll waiting for input is
	 * interrupted, so the caller sees the arrival without waiting for the
	 * next byte. */
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	x_signal_arrived[sig] = 0;
	if (sigaction((int)sig, &sa, NULL) != 0) {
		return x_mkint(p_base, -1);
	}

	return x_mkint(p_base, 0);
}

/** Whether a caught signal has arrived since the last take, clearing the
 *  record.  Two arrivals between takes are one, as the kernel's own pending
 *  set has them.
 *  x-lang: (signal take sig)
 *
 *  @param p_base  Base (execution context).
 *  @param p_args  The signal number.
 *  @return 1 if it arrived, else 0 (also for a number that names no signal).
 */
static x_obj_t *x_prim_signal_take(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_sig;
	x_int_t sig;

	x_eargs(p_base, p_args, 2, NULL, &p_sig);
	sig = x_intval(p_sig);
	if ( ! x_signal_valid(sig) || ! x_signal_arrived[sig]) {
		return x_mkint(p_base, 0);
	}
	x_signal_arrived[sig] = 0;

	return x_mkint(p_base, 1);
}

/** Register signal primitives and bind %sigint-flag. */
x_obj_t *x_prim_signal_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_callable_entry_t entries[] = {
		{ "sigint-install", x_prim_sigint_install },
		{ "sigint-restore", x_prim_sigint_restore }
	};
	static const x_prim_entry_t recorded[] = {
		{ "signal-catch", x_prim_signal_catch, "signal", "catch" },
		{ "signal-take",  x_prim_signal_take,  "signal", "take"  }
	};

	(void)p_args;
	x_callable_bind_table(p_base, entries, sizeof(entries) / sizeof(entries[0]));
	x_prims_bind_table(p_base, recorded, sizeof(recorded) / sizeof(recorded[0]));

	/* Publish the flag pointer onto the base so x_eval can poll it without
	 * naming this module's global, and bind it for x-lang (%sigint-flag). */
	x_firstobj(x_eval_field_sigint(p_base)) = (x_obj_t *)&x_sigint_flag;
	x_value_bind(p_base, "%sigint-flag", (x_obj_t *)&x_sigint_flag);

	return p_base;
}
