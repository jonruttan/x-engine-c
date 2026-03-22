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

/* +: (+ a b) -> binary addition */
static x_obj_t *x_prim_sum(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	return x_mkint(p_base, x_intval(a) + x_intval(b));
}

/* -: (- a) -> negate; (- a b) -> subtract */
static x_obj_t *x_prim_sub(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a;
	x_eargs(p_base, p_args, 2, NULL, &a);

	if (x_obj_isnil(p_base, x_11(p_args)))
		return x_mkint(p_base, -x_intval(a));

	return x_mkint(p_base,
		x_intval(a) - x_intval(x_prim_eval_arg(p_base, x_011(p_args))));
}

/* *: (* a b) -> binary multiplication */
static x_obj_t *x_prim_prod(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	return x_mkint(p_base, x_intval(a) * x_intval(b));
}

/* /: (/ a b) -> binary integer division */
static x_obj_t *x_prim_div(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	return x_mkint(p_base, x_intval(a) / x_intval(b));
}

/* %: (% a b) -> binary integer modulo */
static x_obj_t *x_prim_mod(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	return x_mkint(p_base, x_intval(a) % x_intval(b));
}

/* ~: (~ n) -> bitwise NOT */
static x_obj_t *x_prim_bitnot(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a;
	x_eargs(p_base, p_args, 2, NULL, &a);

	return x_mkint(p_base, ~x_intval(a));
}

/* &: (& a b) -> bitwise AND */
static x_obj_t *x_prim_bitand(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	return x_mkint(p_base, x_intval(a) & x_intval(b));
}

/* |: (| a b) -> bitwise OR */
static x_obj_t *x_prim_bitor(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	return x_mkint(p_base, x_intval(a) | x_intval(b));
}

/* ^: (^ a b) -> bitwise XOR */
static x_obj_t *x_prim_bitxor(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	return x_mkint(p_base, x_intval(a) ^ x_intval(b));
}

/* <<: (<< a b) -> shift left */
static x_obj_t *x_prim_shl(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	return x_mkint(p_base, x_intval(a) << x_intval(b));
}

/* >>: (>> a b) -> shift right */
static x_obj_t *x_prim_shr(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	return x_mkint(p_base, x_intval(a) >> x_intval(b));
}

x_obj_t *x_prim_arith_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_prim_entry_t entries[] = {
		{ "+", x_prim_sum },
		{ "-", x_prim_sub },
		{ "*", x_prim_prod },
		{ "/", x_prim_div },
		{ "%", x_prim_mod },
		{ "~", x_prim_bitnot },
		{ "&", x_prim_bitand },
		{ "|", x_prim_bitor },
		{ "^", x_prim_bitxor },
		{ "<<", x_prim_shl },
		{ ">>", x_prim_shr }
	};

	x_prim_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
