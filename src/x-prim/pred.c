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
	x_obj_t *a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return a == b ? x_base_field_true(p_base) : x_base_field_false(p_base);
}

/* =: (= a b) -> integer value equality */
static x_obj_t *x_prim_numeq(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return x_intval(a) == x_intval(b)
		? x_base_field_true(p_base) : x_base_field_false(p_base);
}

/* <: (< a b) -> t if a < b */
static x_obj_t *x_prim_lt(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return x_intval(a) < x_intval(b)
		? x_base_field_true(p_base) : x_base_field_false(p_base);
}

/* char->integer: (char->integer c) -> integer char code */
static x_obj_t *x_prim_char_to_integer(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_c = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_mkint(p_base, (x_int_t)x_charval(p_c));
}

/* integer->char: (integer->char n) -> character */
static x_obj_t *x_prim_integer_to_char(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_n = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_mkchar(p_base, (x_char_t)x_intval(p_n));
}

x_obj_t *x_prim_pred_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_prim_entry_t entries[] = {
		{ "eq?", x_prim_eq },
		{ "=", x_prim_numeq },
		{ "<", x_prim_lt },
		{ "char->integer", x_prim_char_to_integer },
		{ "integer->char", x_prim_integer_to_char }
	};

	x_prim_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
