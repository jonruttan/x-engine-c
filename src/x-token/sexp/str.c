/**
 * @file str.c
 * @brief S-expression analyser, reader, writer, and display for string literals.
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

#include "x-token/sexp/str.h"
#include "x-eval.h"
#include "x-token.h"
#include "x-type/str.h"
#include "x-type/buffer.h"
#include "x-type/prim.h"

/* Analyzer states: spair with state slot for composable transitions */
x_spair_t x_sexp_str_analyse1_prim = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = x_sexp_str_analyse1 }, { NULL }),
	x_sexp_str_analyse2_prim = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = x_sexp_str_analyse2 }, { NULL }),
	x_sexp_str_analyse3_prim = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = x_sexp_str_analyse3 }, { NULL });

/* Read: satom (type-internal, no self needed).  Write/display are pure
 * x-lang (boot/printer.x). */
x_satom_t x_sexp_str_read_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_str_read });

/* analyse3 prim: escape state — consumes one char after backslash */

/* hex_digit: convert hex character to integer value, or -1 */
static int hex_digit(x_char_t c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

/** Read next-state from callable state slot, with default fallback. */
#define x_next_state(self, dflt) \
	(x_obj_isnil(p_base, x_callable_state(self)) \
		? (x_obj_t *)(dflt) : x_callable_state(self))

/**
 * Analyse state 1: match the opening double-quote.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Pair of (self, read-args).
 * @return Next analyser state on match, or NULL.
 */
x_obj_t *x_sexp_str_analyse1(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_self = x_0(p_args),
		*p_buffer = x_token_read_arg_buffer(x_1(p_args));

	if (0 == x_lib_strncmp(X_SEXP_STR_PRE_STR, x_bufferval(p_buffer), X_SEXP_STR_PRE_STR_LEN)) {
		return x_next_state(p_self, &x_sexp_str_analyse2_prim);
	}

	return NULL;
}

/**
 * Analyse state 2: consume string body characters.
 *
 * Detects backslash (transitions to escape state) and closing quote
 * (scores the full token length).  All other characters loop.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Pair of (self, read-args).
 * @return Self, score, or next state.
 */
x_obj_t *x_sexp_str_analyse2(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_self = x_0(p_args),
		*p_buffer = x_token_read_arg_buffer(x_1(p_args)),
		*p_score = x_token_read_arg_score(x_1(p_args));

	/* Backslash: enter escape state — next char consumed unconditionally */
	if (*(x_bufferread(p_buffer) - 1) == '\\') {
		return x_next_state(p_self, &x_sexp_str_analyse3_prim);
	}

	/* Closing quote: match */
	if (0 == x_lib_strncmp(X_SEXP_STR_POST_STR,
			x_bufferread(p_buffer) - X_SEXP_STR_PRE_STR_LEN,
			X_SEXP_STR_POST_STR_LEN)) {
		x_firstint(p_score) = x_bufferlen(p_buffer);
		return p_score;
	}

	return p_self;
}

/**
 * Analyse state 3: escape state.
 *
 * Unconditionally consumes one character after a backslash and returns
 * to the normal body-reading state (analyse2).
 *
 * @param p_base  Base (execution context).
 * @param p_args  Pair of (self, read-args).
 * @return The analyse2 state primitive.
 */
x_obj_t *x_sexp_str_analyse3(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_self = x_0(p_args);
	(void)p_self;
	return (x_obj_t *)&x_sexp_str_analyse2_prim;
}

/**
 * Read a string literal from the token buffer.
 *
 * Strips the surrounding quotes and processes escape sequences in-place.
 * Supported escapes: @c \\", @c \\\\, @c \\n, @c \\t, @c \\r, @c \\0,
 * and @c \\xHH (two hex digits).  Unknown escapes are preserved literally.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Read-args containing the token buffer.
 * @return A newly created owned-string object.
 */
x_obj_t *x_sexp_str_read(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);
	x_char_t *raw = x_bufferval(p_buffer) + X_SEXP_STR_PRE_STR_LEN;
	size_t len = x_bufferlen(p_buffer) - X_SEXP_STR_PRE_STR_LEN
		- X_SEXP_STR_POST_STR_LEN;
	x_char_t *s = x_lib_strndup(raw, len);
	size_t i, j;

	/* Unescape in-place (result is always <= source length) */
	for (i = 0, j = 0; i < len; i++, j++) {
		if (s[i] == '\\' && i + 1 < len) {
			i++;
			switch (s[i]) {
			case '"':  s[j] = '"';  break;
			case '\\': s[j] = '\\'; break;
			case 'n':  s[j] = '\n'; break;
			case 't':  s[j] = '\t'; break;
			case 'r':  s[j] = '\r'; break;
			case '0':  s[j] = '\0'; break;
			case 'x':
				if (i + 2 < len
					&& hex_digit(s[i + 1]) >= 0
					&& hex_digit(s[i + 2]) >= 0) {
					s[j] = (x_char_t)(
						hex_digit(s[i + 1]) * 16
						+ hex_digit(s[i + 2]));
					i += 2;
				} else {
					s[j++] = '\\';
					s[j] = 'x';
				}
				break;
			default:
				s[j++] = '\\';
				s[j] = s[i];
				break;
			}
		} else {
			s[j] = s[i];
		}
	}
	s[j] = '\0';

	return x_mkstrown(p_base, s);
}

