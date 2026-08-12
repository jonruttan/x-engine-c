/*
 * # Computational Expressions in C
 *
 * ## x-prim/arith.c -- Implementation - Primitives - Arithmetic
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2026 Jon Ruttan
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
 * Shared body for the binary integer ops: evaluate both operands,
 * optionally dispatch through the type-ops registry, guard nil, compute.
 *
 * use_ops selects the x_type_op_try dispatch. The tower ops (+ * / %)
 * pass 1 so a typed operand (float, bignum, ...) reaches its type's
 * handler; the bitwise family passes 0 -- #52 ruled bitwise has no tower
 * semantics (there is no float `&`; lib/x/core/arithmetic.x records the
 * ruling), so the dispatch hook is deliberately not offered there.
 *
 * Nil operands raise instead of reading x_intval(NULL) -- the same
 * nil-safety convention x_prim_eq already follows (#52 ruled: an x-level
 * wrapper test measured +9% on every method dispatch, so the check lives
 * here, where it is two pointer tests). Runs AFTER op_try, so typed
 * operands never reach it. The raw prims are the only guard on the
 * bare-core and (Base make) child paths, where the lib wrappers are
 * absent (#239).
 *
 * The switch keys on the operator's first byte: all nine ops are unique
 * there (`<<`/`>>` via `<`/`>`).
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Unevaluated args, evaluated via x_eargs
 * @param p_op    x_char_t* -- Operator spelling, as registered
 * @param p_err   x_char_t* -- Raise message for a nil operand
 * @param use_ops int -- Non-zero to try x_type_op_try first
 * @return x_obj_t* -- New integer object, or the handler's result
 */
static x_obj_t *x_prim_arith_binop(x_obj_t *p_base, x_obj_t *p_args,
	x_char_t *p_op, x_char_t *p_err, int use_ops)
{
	x_obj_t *a, *b, *p_result;
	x_int_t n;

	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	if (use_ops && x_type_op_try(p_base, p_op, a, b, &p_result))
		return p_result;

	if (x_obj_isnil(p_base, a) || x_obj_isnil(p_base, b))
		x_eval_error(p_base, p_err, NULL);

	switch (*p_op) {
	case '+': n = x_intval(a) + x_intval(b); break;
	case '*': n = x_intval(a) * x_intval(b); break;
	case '/': n = x_intval(a) / x_intval(b); break;
	case '%': n = x_intval(a) % x_intval(b); break;
	case '&': n = x_intval(a) & x_intval(b); break;
	case '|': n = x_intval(a) | x_intval(b); break;
	case '^': n = x_intval(a) ^ x_intval(b); break;
	case '<': n = x_intval(a) << x_intval(b); break;
	case '>': n = x_intval(a) >> x_intval(b); break;
	default: /* unreachable: the register table enumerates the ops */
		n = 0;
		break;
	}

	return x_mkint(p_base, n);
}

/**
 * Binary integer addition, with typed-operand dispatch. x-lang: (+ a b)
 *
 * @see x_prim_arith_binop
 */
static x_obj_t *x_prim_sum(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_prim_arith_binop(p_base, p_args, (x_char_t *)"+",
		(x_char_t *)"+: operand is nil", 1);
}

/**
 * Integer subtraction or negation. x-lang: (- a b) or (- a)
 *
 * With one argument, returns the negation. With two, returns
 * the difference. The second argument is evaluated lazily
 * (only if present).
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Unevaluated args (1 or 2)
 * @return x_obj_t* -- New integer object
 */
static x_obj_t *x_prim_sub(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b, *p_result;
	x_eargs(p_base, p_args, 2, NULL, &a);

	/* Unary negation keeps the int path (typed negation is the tower
	 * layer's concern, not binary op dispatch). */
	if (x_obj_isnil(p_base, x_11(p_args))) {
		if (x_obj_isnil(p_base, a))
			x_eval_error(p_base, (x_char_t *)"-: operand is nil", NULL);
		return x_mkint(p_base, -x_intval(a));
	}

	b = x_eval_arg(p_base, x_011(p_args));
	if (x_type_op_try(p_base, (x_char_t *)"-", a, b, &p_result))
		return p_result;

	/* Nil operands raise instead of reading x_intval(NULL) -- the same
	 * nil-safety convention x_prim_eq already follows (#52 ruled). */
	if (x_obj_isnil(p_base, a) || x_obj_isnil(p_base, b))
		x_eval_error(p_base, (x_char_t *)"-: operand is nil", NULL);

	return x_mkint(p_base, x_intval(a) - x_intval(b));
}

/**
 * Binary integer multiplication. x-lang: (* a b)
 *
 * @see x_prim_arith_binop
 */
static x_obj_t *x_prim_prod(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_prim_arith_binop(p_base, p_args, (x_char_t *)"*",
		(x_char_t *)"*: operand is nil", 1);
}

/**
 * Binary integer division (truncates toward zero). x-lang: (/ a b)
 *
 * @see x_prim_arith_binop
 */
static x_obj_t *x_prim_div(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_prim_arith_binop(p_base, p_args, (x_char_t *)"/",
		(x_char_t *)"/: operand is nil", 1);
}

/**
 * Binary integer modulo. x-lang: (% a b)
 *
 * @see x_prim_arith_binop
 */
static x_obj_t *x_prim_mod(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_prim_arith_binop(p_base, p_args, (x_char_t *)"%",
		(x_char_t *)"%: operand is nil", 1);
}

/**
 * Bitwise NOT. x-lang: (~ n)
 *
 * Unary: no binary op dispatch; the nil guard mirrors x_prim_sub's
 * unary branch (#239 -- the raw prim is the only guard on bare-core
 * and (Base make) child paths).
 */
static x_obj_t *x_prim_bitnot(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a;
	x_eargs(p_base, p_args, 2, NULL, &a);

	if (x_obj_isnil(p_base, a))
		x_eval_error(p_base, (x_char_t *)"~: operand is nil", NULL);

	return x_mkint(p_base, ~x_intval(a));
}

/**
 * Bitwise AND. x-lang: (& a b)
 *
 * @see x_prim_arith_binop
 */
static x_obj_t *x_prim_bitand(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_prim_arith_binop(p_base, p_args, (x_char_t *)"&",
		(x_char_t *)"&: operand is nil", 0);
}

/**
 * Bitwise OR. x-lang: (| a b)
 *
 * @see x_prim_arith_binop
 */
static x_obj_t *x_prim_bitor(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_prim_arith_binop(p_base, p_args, (x_char_t *)"|",
		(x_char_t *)"|: operand is nil", 0);
}

/**
 * Bitwise XOR. x-lang: (^ a b)
 *
 * @see x_prim_arith_binop
 */
static x_obj_t *x_prim_bitxor(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_prim_arith_binop(p_base, p_args, (x_char_t *)"^",
		(x_char_t *)"^: operand is nil", 0);
}

/**
 * Left shift. x-lang: (<< a b)
 *
 * @see x_prim_arith_binop
 */
static x_obj_t *x_prim_shl(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_prim_arith_binop(p_base, p_args, (x_char_t *)"<<",
		(x_char_t *)"<<: operand is nil", 0);
}

/**
 * Right shift. x-lang: (>> a b)
 *
 * @see x_prim_arith_binop
 */
static x_obj_t *x_prim_shr(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_prim_arith_binop(p_base, p_args, (x_char_t *)">>",
		(x_char_t *)">>: operand is nil", 0);
}

/**
 * Register arithmetic primitives into the environment.
 *
 * Binds: +, -, *, /, %, ~, &, |, ^, <<, >>
 *
 * @param p_base  x_obj_t* -- Base (execution context)
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
