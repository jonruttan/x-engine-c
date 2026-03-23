/*
 * x-obj/jit.c -- Non-inline wrappers for JIT-compiled code
 *
 * These thin wrappers expose macro/inline operations as real functions
 * that JIT code can call via dlsym + BLR.
 */
#include "x-obj.h"
#include "x-prim.h"
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
	return x_prim_eval_arg(p_base, p_expr);
}

/* jit_make_prim: (jit-make-prim fn-addr) -> proper prim callable.
 * Registered as an x-lang primitive so the return value flows through
 * dispatch and is usable as a normal x-lang value. */
x_obj_t *jit_make_prim(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_addr = x_prim_eval_arg(p_base, x_01(p_args));

	return x_make_prim(p_base, X_OBJ_FLAG_NONE,
		(x_prim_fn)x_ptrval(p_addr));
}
