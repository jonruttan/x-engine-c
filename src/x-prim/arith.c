/*
 * # Computational Expressions in C
 *
 * ## x-prim/arith.c -- Implementation - Primitives - Arithmetic
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
#include "x-prim.h"
#include "x-type/int.h"

/* +: (+ a ...) -> variadic addition, identity 0 */
static x_obj_t *x_prim_sum(x_obj_t *p_base, x_obj_t *p_args)
{
	x_int_t result = 0;

	for (; ! x_obj_isnil(p_base, p_args); p_args = x_restobj(p_args))
		result += x_intval(x_prim_eval_arg(p_base, x_firstobj(p_args)));

	return x_mkint(p_base, result);
}

/* -: (- a ...) -> variadic subtraction; (- a) negates */
static x_obj_t *x_prim_sub(x_obj_t *p_base, x_obj_t *p_args)
{
	x_int_t result;

	if (x_obj_isnil(p_base, p_args))
		return x_mkint(p_base, 0);

	result = x_intval(x_prim_eval_arg(p_base, x_firstobj(p_args)));
	p_args = x_restobj(p_args);

	if (x_obj_isnil(p_base, p_args))
		return x_mkint(p_base, -result);

	for (; ! x_obj_isnil(p_base, p_args); p_args = x_restobj(p_args))
		result -= x_intval(x_prim_eval_arg(p_base, x_firstobj(p_args)));

	return x_mkint(p_base, result);
}

/* *: (* a ...) -> variadic multiplication, identity 1 */
static x_obj_t *x_prim_prod(x_obj_t *p_base, x_obj_t *p_args)
{
	x_int_t result = 1;

	for (; ! x_obj_isnil(p_base, p_args); p_args = x_restobj(p_args))
		result *= x_intval(x_prim_eval_arg(p_base, x_firstobj(p_args)));

	return x_mkint(p_base, result);
}

/* /: (/ a ...) -> variadic integer division */
static x_obj_t *x_prim_div(x_obj_t *p_base, x_obj_t *p_args)
{
	x_int_t result;

	if (x_obj_isnil(p_base, p_args))
		return x_mkint(p_base, 1);

	result = x_intval(x_prim_eval_arg(p_base, x_firstobj(p_args)));
	p_args = x_restobj(p_args);

	for (; ! x_obj_isnil(p_base, p_args); p_args = x_restobj(p_args))
		result /= x_intval(x_prim_eval_arg(p_base, x_firstobj(p_args)));

	return x_mkint(p_base, result);
}

/* %: (% a ...) -> variadic integer modulo */
static x_obj_t *x_prim_mod(x_obj_t *p_base, x_obj_t *p_args)
{
	x_int_t result;

	if (x_obj_isnil(p_base, p_args))
		return x_mkint(p_base, 0);

	result = x_intval(x_prim_eval_arg(p_base, x_firstobj(p_args)));
	p_args = x_restobj(p_args);

	for (; ! x_obj_isnil(p_base, p_args); p_args = x_restobj(p_args))
		result %= x_intval(x_prim_eval_arg(p_base, x_firstobj(p_args)));

	return x_mkint(p_base, result);
}

/* ~: (~ n) -> bitwise NOT */
static x_obj_t *x_prim_bitnot(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_mkint(p_base, ~x_intval(a));
}

/* &: (& a b) -> bitwise AND */
static x_obj_t *x_prim_bitand(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return x_mkint(p_base, x_intval(a) & x_intval(b));
}

/* |: (| a b) -> bitwise OR */
static x_obj_t *x_prim_bitor(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return x_mkint(p_base, x_intval(a) | x_intval(b));
}

/* ^: (^ a b) -> bitwise XOR */
static x_obj_t *x_prim_bitxor(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return x_mkint(p_base, x_intval(a) ^ x_intval(b));
}

/* <<: (<< a b) -> shift left */
static x_obj_t *x_prim_shl(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return x_mkint(p_base, x_intval(a) << x_intval(b));
}

/* >>: (>> a b) -> shift right */
static x_obj_t *x_prim_shr(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return x_mkint(p_base, x_intval(a) >> x_intval(b));
}

x_obj_t *x_prim_arith_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_prim_bind(p_base, "+", x_prim_sum);
	x_prim_bind(p_base, "-", x_prim_sub);
	x_prim_bind(p_base, "*", x_prim_prod);
	x_prim_bind(p_base, "/", x_prim_div);
	x_prim_bind(p_base, "%", x_prim_mod);
	x_prim_bind(p_base, "~", x_prim_bitnot);
	x_prim_bind(p_base, "&", x_prim_bitand);
	x_prim_bind(p_base, "|", x_prim_bitor);
	x_prim_bind(p_base, "^", x_prim_bitxor);
	x_prim_bind(p_base, "<<", x_prim_shl);
	x_prim_bind(p_base, ">>", x_prim_shr);

	return p_base;
}
