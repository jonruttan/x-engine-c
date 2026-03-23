/*
 * x-obj/jit.c -- Non-inline wrappers for JIT-compiled code
 *
 * These thin wrappers expose macro/inline operations as real functions
 * that JIT code can call via dlsym + BLR.
 */
#include "x-obj.h"
#include "x-prim.h"
#include "x-type/buffer.h"
#include "x-type/int.h"
#include "x-type/pair.h"
#include "x-type/prim.h"
#include "x-type/ptr.h"

/* jit_mkint: allocate an integer atom */
x_obj_t *jit_mkint(x_obj_t *p_base, long value)
{
	return x_mkint(p_base, (x_int_t)value);
}

/* jit_mkpair: allocate a pair (cons) */
x_obj_t *jit_mkpair(x_obj_t *p_base, x_obj_t *a, x_obj_t *b)
{
	return x_mkspair(p_base, a, b);
}

/* jit_firstobj: car */
x_obj_t *jit_firstobj(x_obj_t *p)
{
	return x_firstobj(p);
}

/* jit_restobj: cdr */
x_obj_t *jit_restobj(x_obj_t *p)
{
	return x_restobj(p);
}

/* jit_atomint: unbox integer */
long jit_atomint(x_obj_t *p)
{
	return (long)x_atomint(p);
}

/* jit_eval_arg: evaluate an expression */
x_obj_t *jit_eval_arg(x_obj_t *p_base, x_obj_t *p_expr)
{
	return x_eval_arg(p_base, p_expr);
}

/* jit_build_args: build x-lang arg list from raw integers.
 * Receives p_base, nargs, then nargs raw longs.
 * Returns (nil arg0-atom arg1-atom ...) — with nil as self placeholder. */
x_obj_t *jit_build_args(x_obj_t *p_base, long nargs,
	long a0, long a1, long a2, long a3)
{
	x_obj_t *p_list = NULL;
	long args[4];
	int i;

	args[0] = a0; args[1] = a1; args[2] = a2; args[3] = a3;

	/* Build list right-to-left */
	for (i = (int)nargs - 1; i >= 0; i--) {
		x_obj_t *p_atom = x_mkint(p_base, (x_int_t)args[i]);
		p_list = x_mkspair(p_base, p_atom, p_list);
	}

	/* Prepend nil as self placeholder */
	p_list = x_mkspair(p_base, NULL, p_list);

	return p_list;
}

/* jit_score_set: (score-set score sign buffer) -> score
 * Sets score integer to sign * bufferlen(buffer), returns score. */
x_obj_t *jit_score_set(x_obj_t *score, long sign, x_obj_t *buffer)
{
	x_firstint(score) = (x_int_t)(sign * (long)x_bufferlen(buffer));
	return score;
}

/* jit_buffer_unread: (buffer-unread buffer) -> buffer
 * Decrements the buffer read pointer, returns buffer. */
x_obj_t *jit_buffer_unread(x_obj_t *buffer)
{
	x_bufferread(buffer)--;
	return buffer;
}

/* jit_buffer_len: (buffer-len buffer) -> raw length */
long jit_buffer_len(x_obj_t *buffer)
{
	return (long)x_bufferlen(buffer);
}

/* jit_make_prim: (jit-make-prim fn-addr) -> proper prim callable.
 * Registered as an x-lang primitive so the return value flows through
 * dispatch and is usable as a normal x-lang value. */
x_obj_t *jit_make_prim(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_addr = x_eval_arg(p_base, x_01(p_args));

	return x_make_prim(p_base, X_OBJ_FLAG_NONE,
		(x_callable_fn)x_ptrval(p_addr));
}
