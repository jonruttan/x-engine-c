/*
 * # Computational Expressions in C
 *
 * ## x-prim/pred.c -- Implementation - Primitives - Predicates
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
#include "x-type/char.h"
#include "x-type/int.h"

/* eq?: (eq? a b) -> pointer equality */
static x_obj_t *x_prim_eq(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	return a == b ? x_base_field_true(p_base) : x_base_field_false(p_base);
}

/* =: (= a b) -> integer value equality */
static x_obj_t *x_prim_numeq(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	return x_intval(a) == x_intval(b)
		? x_base_field_true(p_base) : x_base_field_false(p_base);
}

/* <: (< a b) -> t if a < b */
static x_obj_t *x_prim_lt(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	return x_intval(a) < x_intval(b)
		? x_base_field_true(p_base) : x_base_field_false(p_base);
}

/* char->integer: (char->integer c) -> integer char code */
static x_obj_t *x_prim_char_to_integer(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *c;
	x_eargs(p_base, p_args, 2, NULL, &c);

	return x_mkint(p_base, (x_int_t)x_charval(c));
}

/* integer->char: (integer->char n) -> character */
static x_obj_t *x_prim_integer_to_char(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *n;
	x_eargs(p_base, p_args, 2, NULL, &n);

	return x_mkchar(p_base, (x_char_t)x_intval(n));
}

x_obj_t *x_prim_pred_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_callable_entry_t entries[] = {
		{ "eq?", x_prim_eq },
		{ "=", x_prim_numeq },
		{ "<", x_prim_lt },
		{ "char->integer", x_prim_char_to_integer },
		{ "integer->char", x_prim_integer_to_char }
	};

	x_callable_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
