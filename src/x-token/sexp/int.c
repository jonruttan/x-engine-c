/**
 * @file int.c
 * @brief S-expression analyser, reader, and writer for integer literals.
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2021 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */

#include <ctype.h>
#include "x-token/sexp/int.h"
#include "x-token.h"
#include "x-type/char.h"
#include "x-type/buffer.h"
#include "x-type/int.h"
#include "x-type/prim.h"
#include "x-token/sexp/int.h"

/* Analyzer states: spair (5-unit) with state slot for composable transitions.
 * State slot (second data unit) holds the default next-state, or NULL. */
x_spair_t x_sexp_int_analyse_sign_prim = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = x_sexp_int_analyse_sign }, { NULL }),
	x_sexp_int_analyse_prefix_prim = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = x_sexp_int_analyse_prefix }, { NULL }),
	x_sexp_int_analyse_base_prim = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = x_sexp_int_analyse_base }, { NULL }),
	x_sexp_int_analyse_digits_prim = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = x_sexp_int_analyse_digits }, { NULL }),
	x_sexp_int_analyse_xdigits_prim = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = x_sexp_int_analyse_xdigits }, { NULL });

/* Read/write: satom (type-internal, no self-passing needed) */
x_satom_t x_sexp_int_read_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_int_read }),
	x_sexp_int_write_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_int_write });

/** Read next-state from callable state slot, with default fallback. */
#define x_next_state(self, dflt) \
	(x_obj_isnil(p_base, x_callable_state(self)) \
		? (x_obj_t *)(dflt) : x_callable_state(self))

/**
 * Analyse: consume decimal digits and score.
 *
 * Returns self while digits are being read.  On a non-digit, backs up
 * the read pointer and scores the buffer length (or returns NULL if
 * no digits were consumed).
 *
 * @param p_base  Execution context.
 * @param p_args  Pair of (self, read-args).
 * @return Self, score, or NULL.
 */
x_obj_t *x_sexp_int_analyse_digits(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_self = x_0(p_args),
		*p_buffer = x_token_read_arg_buffer(x_1(p_args)),
		*p_score = x_token_read_arg_score(x_1(p_args));

	if (isdigit(x_bufferlastchar(p_buffer))) {
		return p_self;
	}

	x_bufferread(p_buffer)--;

	if (x_bufferlen(p_buffer) < 1) {
		return NULL;
	}

	x_firstint(p_score) = x_bufferlen(p_buffer);
	return p_score;
}

/**
 * Analyse: consume hexadecimal digits and score.
 *
 * Identical logic to analyse_digits but accepts @c 0-9, @c a-f, @c A-F.
 *
 * @param p_base  Execution context.
 * @param p_args  Pair of (self, read-args).
 * @return Self, score, or NULL.
 */
x_obj_t *x_sexp_int_analyse_xdigits(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_self = x_0(p_args),
		*p_buffer = x_token_read_arg_buffer(x_1(p_args)),
		*p_score = x_token_read_arg_score(x_1(p_args));

	if (isxdigit(x_bufferlastchar(p_buffer))) {
		return p_self;
	}

	x_bufferread(p_buffer)--;

	if (x_bufferlen(p_buffer) < 1) {
		return NULL;
	}

	x_firstint(p_score) = x_bufferlen(p_buffer);
	return p_score;
}

/**
 * Analyse: detect @c 0x/@c 0X hex prefix or fall through to decimal.
 *
 * If the current character is @c x or @c X, transitions to the
 * xdigits state.  Otherwise delegates to the digits analyser.
 *
 * @param p_base  Execution context.
 * @param p_args  Pair of (self, read-args).
 * @return Next analyser state or score.
 */
x_obj_t *x_sexp_int_analyse_base(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_self = x_0(p_args),
		*p_buffer = x_token_read_arg_buffer(x_1(p_args));

	if (x_lib_strchr("Xx", x_bufferlastchar(p_buffer))) {
		return x_next_state(p_self, &x_sexp_int_analyse_xdigits_prim);
	}

	return x_sexp_int_analyse_digits(p_base,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, (x_obj_t *)&x_sexp_int_analyse_digits_prim, x_1(p_args)));
}

/**
 * Analyse: handle leading @c 0 (possible hex prefix) or first digit.
 *
 * A leading @c 0 transitions to the base state; other digits delegate
 * to the digits analyser; non-digits cause rejection.
 *
 * @param p_base  Execution context.
 * @param p_args  Pair of (self, read-args).
 * @return Next analyser state, score, or NULL.
 */
x_obj_t *x_sexp_int_analyse_prefix(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_self = x_0(p_args),
		*p_buffer = x_token_read_arg_buffer(x_1(p_args));

	if ('0' == x_bufferlastchar(p_buffer)) {
		return x_next_state(p_self, &x_sexp_int_analyse_base_prim);
	}

	if ( ! isdigit(x_bufferlastchar(p_buffer))) {
		x_bufferread(p_buffer)--;
		return NULL;
	}

	return x_sexp_int_analyse_digits(p_base,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, (x_obj_t *)&x_sexp_int_analyse_digits_prim, x_1(p_args)));
}

/**
 * Analyse: handle optional leading @c + or @c - sign.
 *
 * If a sign character is found, transitions to the prefix state.
 * Otherwise delegates directly to the prefix analyser.
 *
 * @param p_base  Execution context.
 * @param p_args  Pair of (self, read-args).
 * @return Next analyser state or score.
 */
x_obj_t *x_sexp_int_analyse_sign(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_self = x_0(p_args),
		*p_buffer = x_token_read_arg_buffer(x_1(p_args));

	if (x_lib_strchr("+-", x_bufferlastchar(p_buffer)) != NULL) {
		return x_next_state(p_self, &x_sexp_int_analyse_prefix_prim);
	}

	return x_sexp_int_analyse_prefix(p_base,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, (x_obj_t *)&x_sexp_int_analyse_prefix_prim, x_1(p_args)));
}

/**
 * Read an integer literal from the token buffer.
 *
 * Parses the buffer contents with @c x_lib_strtoint using base 0
 * (auto-detecting decimal, octal, or hexadecimal).
 *
 * @param p_base  Execution context.
 * @param p_args  Read-args whose first element is the token buffer.
 * @return A newly created integer object, or NULL on empty buffer.
 */
x_obj_t *x_sexp_int_read(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_firstobj(p_args), *p_int;

	if (x_bufferlen(p_buffer) < 1) {
		return NULL;
	}

	/* Not required, final non-numeric char will act as delimiter. */
	/* *x_bufferead(p_buffer) = '\0'; */

	p_int = x_mkint(p_base, x_lib_strtoint(x_bufferval(p_buffer), NULL, 0));

	return p_int;
}

/**
 * Write the decimal representation of an integer to output.
 *
 * Converts the integer value to a base-10 string using a stack buffer
 * and writes it through the base output mechanism.
 *
 * @param p_base  Execution context.
 * @param p_args  Pair whose first element is the integer to write.
 * @return The integer object on success, or NULL on write failure.
 */
x_obj_t *x_sexp_int_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_char_t s[22];
	x_obj_t *p_ret;
	x_satom_t str = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = s });
	x_spair_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { str }, { NULL });

	x_lib_inttostr(x_intval(x_firstobj(p_args)), s, 10);

	p_ret = x_interp_write_str(p_base, (x_obj_t *)&wrap);

	if ( ! x_obj_isnil(p_base, p_ret)) {
		return x_firstobj(p_args);
	}

	return NULL;
}
