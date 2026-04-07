/**
 * @file x-obj/jit.c
 * @brief Non-inline wrappers for JIT-compiled code.
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

/**
 * Allocate an integer atom for JIT code.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param value   long     -- Integer value
 * @return New integer object
 */
x_obj_t *jit_mkint(x_obj_t *p_base, long value)
{
	return x_mkint(p_base, (x_int_t)value);
}

/**
 * Allocate a pair for JIT code.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param a       x_obj_t* -- First element
 * @param b       x_obj_t* -- Rest element
 * @return New pair object
 */
x_obj_t *jit_mkpair(x_obj_t *p_base, x_obj_t *a, x_obj_t *b)
{
	return x_mkspair(p_base, X_OBJ_FLAG_NONE, a, b);
}

/**
 * Extract the first element of a pair.
 *
 * @param p  x_obj_t* -- Pair object
 * @return First element
 */
x_obj_t *jit_firstobj(x_obj_t *p)
{
	return x_firstobj(p);
}

/**
 * Extract the rest element of a pair.
 *
 * @param p  x_obj_t* -- Pair object
 * @return Rest element
 */
x_obj_t *jit_restobj(x_obj_t *p)
{
	return x_restobj(p);
}

/**
 * Unbox an integer atom to a raw long.
 *
 * @param p  x_obj_t* -- Integer atom
 * @return Raw long value
 */
long jit_atomint(x_obj_t *p)
{
	return (long)x_atomint(p);
}

/**
 * Evaluate an expression for JIT code.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_expr  x_obj_t* -- Expression to evaluate
 * @return Evaluation result
 */
x_obj_t *jit_eval_arg(x_obj_t *p_base, x_obj_t *p_expr)
{
	return x_eval_arg(p_base, p_expr);
}

/**
 * Build an x-lang argument list from up to 4 raw integers.
 *
 * Constructs a proper list with nil as self placeholder followed by
 * boxed integer atoms for each argument.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param nargs   long     -- Number of arguments (0..4)
 * @param a0      long     -- First raw argument
 * @param a1      long     -- Second raw argument
 * @param a2      long     -- Third raw argument
 * @param a3      long     -- Fourth raw argument
 * @return (nil arg0-atom arg1-atom ...) list
 */
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
		p_list = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_atom, p_list);
	}

	/* Prepend nil as self placeholder */
	p_list = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, p_list);

	return p_list;
}

/**
 * Set a tokenizer score to sign * buffer-length.
 *
 * @param score   x_obj_t* -- Score integer atom (mutated in place)
 * @param sign    long     -- Sign multiplier (+1 or -1)
 * @param buffer  x_obj_t* -- Token buffer object
 * @return The score atom
 */
x_obj_t *jit_score_set(x_obj_t *score, long sign, x_obj_t *buffer)
{
	x_firstint(score) = (x_int_t)(sign * (long)x_bufferlen(buffer));
	return score;
}

/**
 * Decrement the buffer read pointer (unread one character).
 *
 * @param buffer  x_obj_t* -- Token buffer object
 * @return The buffer (for chaining)
 */
x_obj_t *jit_buffer_unread(x_obj_t *buffer)
{
	x_bufferread(buffer)--;
	return buffer;
}

/**
 * Return the current buffer length as a raw long.
 *
 * @param buffer  x_obj_t* -- Token buffer object
 * @return Buffer length
 */
long jit_buffer_len(x_obj_t *buffer)
{
	return (long)x_bufferlen(buffer);
}

/**
 * Create a proper x-lang prim callable from a JIT function address.
 *
 * Registered as an x-lang primitive so the return value flows through
 * dispatch and is usable as a normal x-lang value.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- (self fn-addr-ptr)
 * @return New prim object wrapping the JIT function
 */
x_obj_t *jit_make_prim(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_addr = x_eval_arg(p_base, x_01(p_args));

	return x_make_prim(p_base, X_OBJ_FLAG_NONE,
		(x_fn_t)x_ptrval(p_addr));
}
