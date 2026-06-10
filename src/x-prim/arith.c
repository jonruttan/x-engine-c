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
#include "x-type.h"
#include "x-type/int.h"

/**
 * Binary integer addition, with typed-operand dispatch. x-lang: (+ a b)
 *
 * A typed operand (float, bignum, ...) dispatches through its type's ops
 * alist (x_type_op_try); int/int keeps the pure-C path.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Unevaluated args, evaluated via x_eargs
 * @return x_obj_t* -- New integer object, or the handler's result
 */
static x_obj_t *x_prim_sum(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b, *p_result;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	if (x_type_op_try(p_base, (x_char_t *)"+", a, b, &p_result))
		return p_result;

	return x_mkint(p_base, x_intval(a) + x_intval(b));
}

/**
 * Integer subtraction or negation. x-lang: (- a b) or (- a)
 *
 * With one argument, returns the negation. With two, returns
 * the difference. The second argument is evaluated lazily
 * (only if present).
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Unevaluated args (1 or 2)
 * @return x_obj_t* -- New integer object
 */
static x_obj_t *x_prim_sub(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b, *p_result;
	x_eargs(p_base, p_args, 2, NULL, &a);

	/* Unary negation keeps the int path (typed negation is the tower
	 * layer's concern, not binary op dispatch). */
	if (x_obj_isnil(p_base, x_11(p_args)))
		return x_mkint(p_base, -x_intval(a));

	b = x_eval_arg(p_base, x_011(p_args));
	if (x_type_op_try(p_base, (x_char_t *)"-", a, b, &p_result))
		return p_result;

	return x_mkint(p_base, x_intval(a) - x_intval(b));
}

/**
 * Binary integer multiplication. x-lang: (* a b)
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Unevaluated args, evaluated via x_eargs
 * @return x_obj_t* -- New integer object
 */
static x_obj_t *x_prim_prod(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b, *p_result;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	if (x_type_op_try(p_base, (x_char_t *)"*", a, b, &p_result))
		return p_result;

	return x_mkint(p_base, x_intval(a) * x_intval(b));
}

/**
 * Binary integer division (truncates toward zero). x-lang: (/ a b)
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Unevaluated args, evaluated via x_eargs
 * @return x_obj_t* -- New integer object
 */
static x_obj_t *x_prim_div(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b, *p_result;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	if (x_type_op_try(p_base, (x_char_t *)"/", a, b, &p_result))
		return p_result;

	return x_mkint(p_base, x_intval(a) / x_intval(b));
}

/**
 * Binary integer modulo. x-lang: (% a b)
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Unevaluated args, evaluated via x_eargs
 * @return x_obj_t* -- New integer object
 */
static x_obj_t *x_prim_mod(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b, *p_result;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	if (x_type_op_try(p_base, (x_char_t *)"%", a, b, &p_result))
		return p_result;

	return x_mkint(p_base, x_intval(a) % x_intval(b));
}

/**
 * Bitwise NOT. x-lang: (~ n)
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Unevaluated args, evaluated via x_eargs
 * @return x_obj_t* -- New integer object
 */
static x_obj_t *x_prim_bitnot(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a;
	x_eargs(p_base, p_args, 2, NULL, &a);

	return x_mkint(p_base, ~x_intval(a));
}

/**
 * Bitwise AND. x-lang: (& a b)
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Unevaluated args, evaluated via x_eargs
 * @return x_obj_t* -- New integer object
 */
static x_obj_t *x_prim_bitand(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	return x_mkint(p_base, x_intval(a) & x_intval(b));
}

/**
 * Bitwise OR. x-lang: (| a b)
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Unevaluated args, evaluated via x_eargs
 * @return x_obj_t* -- New integer object
 */
static x_obj_t *x_prim_bitor(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	return x_mkint(p_base, x_intval(a) | x_intval(b));
}

/**
 * Bitwise XOR. x-lang: (^ a b)
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Unevaluated args, evaluated via x_eargs
 * @return x_obj_t* -- New integer object
 */
static x_obj_t *x_prim_bitxor(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	return x_mkint(p_base, x_intval(a) ^ x_intval(b));
}

/**
 * Left shift. x-lang: (<< a b)
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Unevaluated args, evaluated via x_eargs
 * @return x_obj_t* -- New integer object
 *
 * @see x_prim_shr
 */
static x_obj_t *x_prim_shl(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	return x_mkint(p_base, x_intval(a) << x_intval(b));
}

/**
 * Right shift. x-lang: (>> a b)
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Unevaluated args, evaluated via x_eargs
 * @return x_obj_t* -- New integer object
 *
 * @see x_prim_shl
 */
static x_obj_t *x_prim_shr(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	return x_mkint(p_base, x_intval(a) >> x_intval(b));
}

/**
 * Register arithmetic primitives into the environment.
 *
 * Binds: +, -, *, /, %, ~, &, |, ^, <<, >>
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Unused
 * @return x_obj_t* -- p_base
 */
x_obj_t *x_prim_arith_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_prim_entry_t entries[] = {
		{ "+",  x_prim_sum,    "int", "+"  },
		{ "-",  x_prim_sub,    "int", "-"  },
		{ "*",  x_prim_prod,   "int", "*"  },
		{ "/",  x_prim_div,    "int", "/"  },
		{ "%",  x_prim_mod,    "int", "%"  },
		{ "~",  x_prim_bitnot, "int", "~"  },
		{ "&",  x_prim_bitand, "int", "&"  },
		{ "|",  x_prim_bitor,  "int", "|"  },
		{ "^",  x_prim_bitxor, "int", "^"  },
		{ "<<", x_prim_shl,    "int", "<<" },
		{ ">>", x_prim_shr,    "int", ">>" }
	};

	x_prims_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
