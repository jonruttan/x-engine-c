/** @file pred.c
 *  @brief Predicate and type-conversion primitives.
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2024 Jon Ruttan
 *  @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
#include "x-prim.h"
#include "x-type/char.h"
#include "x-type/int.h"

/** Object identity test (pointer equality).
 *  x-lang: (same? a b)
 *  @param p_base Interpreter base context.
 *  @param p_args Unevaluated argument list: (same? a b).
 *  @return #t if @p a and @p b are the same object (pointer), #f otherwise.
 *  @note Strict identity: two distinct integer/character objects with equal
 *        values are NOT same?. Use eq? for value equality of scalars.
 */
static x_obj_t *x_prim_same(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	return a == b ? x_firstobj(x_interp_field_true(p_base)) : x_firstobj(x_interp_field_false(p_base));
}

/** Value equality test.
 *  x-lang: (eq? a b)
 *  @param p_base Interpreter base context.
 *  @param p_args Unevaluated argument list: (eq? a b).
 *  @return #t if @p a and @p b are equal, #f otherwise.
 *  @note Identity is the base case (covers interned symbols, nil, booleans,
 *        and identity sentinels via the @c a==b fast path). Beyond that, the
 *        immediate scalar types are compared by value, so equal integers or
 *        equal characters are eq?. Compound values (pairs, strings, ...) are
 *        NOT deep-compared -- they stay identity-only, keeping pair sentinels
 *        distinct. No cross-type coercion (deferred to a future eqv?).
 */
static x_obj_t *x_prim_eq(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b;
	int equal;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	equal = (a == b);
	if (!equal && !x_obj_isnil(p_base, a) && !x_obj_isnil(p_base, b)) {
		if (x_obj_type_isint(p_base, a) && x_obj_type_isint(p_base, b))
			equal = (x_intval(a) == x_intval(b));
		else if (x_obj_type_ischar(p_base, a) && x_obj_type_ischar(p_base, b))
			equal = (x_charval(a) == x_charval(b));
	}

	return equal ? x_firstobj(x_interp_field_true(p_base))
		: x_firstobj(x_interp_field_false(p_base));
}

/** Integer value equality.
 *  x-lang: (= a b)
 *  @param p_base Interpreter base context.
 *  @param p_args Unevaluated argument list: (= a b).
 *  @return The @c t symbol if the integer values of @p a and @p b are equal,
 *          @c f otherwise.
 */
static x_obj_t *x_prim_numeq(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	return x_intval(a) == x_intval(b)
		? x_firstobj(x_interp_field_true(p_base)) : x_firstobj(x_interp_field_false(p_base));
}

/** Integer less-than comparison.
 *  x-lang: (< a b)
 *  @param p_base Interpreter base context.
 *  @param p_args Unevaluated argument list: (< a b).
 *  @return The @c t symbol if @p a is numerically less than @p b, @c f otherwise.
 */
static x_obj_t *x_prim_lt(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	return x_intval(a) < x_intval(b)
		? x_firstobj(x_interp_field_true(p_base)) : x_firstobj(x_interp_field_false(p_base));
}

/** Convert a character to its integer code point.
 *  x-lang: (char->integer c)
 *  @param p_base Interpreter base context.
 *  @param p_args Unevaluated argument list: (char->integer c).
 *  @return An integer object holding the character's code point value.
 *  @note Zero-allocation cast; safe inside tokenizer callbacks.
 */
static x_obj_t *x_prim_char_to_integer(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *c;
	x_eargs(p_base, p_args, 2, NULL, &c);

	return x_mkint(p_base, x_atomint(c));
}

/** Convert an integer code point to a character.
 *  x-lang: (integer->char n)
 *  @param p_base Interpreter base context.
 *  @param p_args Unevaluated argument list: (integer->char n).
 *  @return A character object for the given code point.
 *  @note Zero-allocation cast; safe inside tokenizer callbacks.
 */
static x_obj_t *x_prim_integer_to_char(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *n;
	x_eargs(p_base, p_args, 2, NULL, &n);

	return x_mkchar(p_base, x_intval(n));
}

/** Register predicate and type-conversion primitives.
 *
 *  Binds: @c same?, @c eq?, @c =, @c <, @c char->integer, @c integer->char.
 *
 *  @param p_base Interpreter base context.
 *  @param p_args Unused.
 *  @return @p p_base.
 */
x_obj_t *x_prim_pred_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_callable_entry_t entries[] = {
		{ "same?", x_prim_same },
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
