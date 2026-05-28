/**
 * @file char.c
 * @brief S-expression analyser, reader, and writer for character literals.
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2023 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */

#include "x-interp.h"
#include "x-token.h"
#include "x-type/buffer.h"
#include "x-type/char.h"
#include "x-type/int.h"
#include "x-type/prim.h"
#include "x-token/sexp/char.h"

/* Forward declaration for named-character state */
static x_obj_t *x_sexp_char_analyse4(x_obj_t *p_base, x_obj_t *p_args);

/* Analyzer states: spair with state slot for composable transitions */
x_spair_t x_sexp_char_analyse1_prim = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = x_sexp_char_analyse1 }, { NULL }),
	x_sexp_char_analyse2_prim = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = x_sexp_char_analyse2 }, { NULL }),
	x_sexp_char_analyse3_prim = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = x_sexp_char_analyse3 }, { NULL });

/* Read/write/display: satom (type-internal, no self needed) */
x_satom_t x_sexp_char_read_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_char_read }),
	x_sexp_char_write_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_char_write }),
	x_sexp_char_display_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_char_display });

static x_spair_t x_sexp_char_analyse4_prim =
	x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = x_sexp_char_analyse4 }, { NULL });

static int is_lower(x_char_t c)
{
	return c >= 'a' && c <= 'z';
}

/** Read next-state from callable state slot, with default fallback. */
#define x_next_state(self, dflt) \
	(x_obj_isnil(p_base, x_callable_state(self)) \
		? (x_obj_t *)(dflt) : x_callable_state(self))

/**
 * Analyse state 1: match first character of the @c #\\ prefix.
 *
 * @param p_base  Execution context.
 * @param p_args  Pair of (self, read-args).
 * @return Next analyser state, or NULL on mismatch.
 */
x_obj_t *x_sexp_char_analyse1(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_self = x_0(p_args);

	if (X_SEXP_CHAR_PRE_STR[0] != x_bufferlastchar(x_token_read_arg_buffer(x_1(p_args)))) {
		return NULL;
	}

	return x_next_state(p_self, &x_sexp_char_analyse2_prim);
}

/**
 * Analyse state 2: match second character of the @c #\\ prefix.
 *
 * @param p_base  Execution context.
 * @param p_args  Pair of (self, read-args).
 * @return Next analyser state, or NULL on mismatch.
 */
x_obj_t *x_sexp_char_analyse2(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_self = x_0(p_args),
		*p_buffer = x_token_read_arg_buffer(x_1(p_args));

	if (X_SEXP_CHAR_PRE_STR[1] != x_bufferlastchar(p_buffer)) {
		return NULL;
	}

	return x_next_state(p_self, &x_sexp_char_analyse3_prim);
}

/**
 * Analyse state 3: classify character after prefix.
 *
 * A lowercase letter transitions to the named-character state (analyse4).
 * Any other character scores immediately as a single-char literal.
 *
 * @param p_base  Execution context.
 * @param p_args  Pair of (self, read-args).
 * @return Score object on match, next state for named chars, or NULL.
 */
x_obj_t *x_sexp_char_analyse3(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_self = x_0(p_args),
		*p_buffer = x_token_read_arg_buffer(x_1(p_args)),
		*p_score = x_token_read_arg_score(x_1(p_args));

	/* Lowercase letter: may be start of a named character */
	if (is_lower(x_bufferlastchar(p_buffer))) {
		return x_next_state(p_self, &x_sexp_char_analyse4_prim);
	}

	/* Non-letter: single character literal, score immediately */
	x_firstint(p_score) = x_bufferlen(p_buffer);
	return p_score;
}

/**
 * Analyse state 4 (static): named-character continuation.
 *
 * Keeps consuming lowercase letters.  On a non-letter delimiter the
 * read pointer is backed up and the accumulated length is scored.
 *
 * @param p_base  Execution context.
 * @param p_args  Pair of (self, read-args).
 * @return Self to keep reading, or score on delimiter.
 */
static x_obj_t *x_sexp_char_analyse4(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_self = x_0(p_args),
		*p_buffer = x_token_read_arg_buffer(x_1(p_args)),
		*p_score = x_token_read_arg_score(x_1(p_args));

	if (is_lower(x_bufferlastchar(p_buffer))) {
		return p_self;
	}

	/* Non-letter delimiter: un-read it and score */
	x_bufferread(p_buffer)--;

	x_firstint(p_score) = x_bufferlen(p_buffer);
	return p_score;
}

/**
 * Read a character literal from the token buffer.
 *
 * Handles single-character literals (@c #\\c, length 3) and named
 * characters (e.g. @c #\\newline) by looking up the name in the
 * character type's data alist.
 *
 * @param p_base  Execution context.
 * @param p_args  Read-args containing the token buffer.
 * @return A newly created character object, or NULL on error.
 * @note Signals an error for unknown character names.
 */
x_obj_t *x_sexp_char_read(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);
	x_int_t len = x_bufferlen(p_buffer);
	x_obj_t *p_type, *p_data, *p_entry;
	x_char_t *buf_name, *sym_name;
	x_int_t name_len;

	/* Single character: #\c (length 3) */
	if (len == 3) {
		return x_mkchar(p_base, *(x_bufferval(p_buffer) + 2));
	}

	/* Named character: lookup in type data alist */
	buf_name = x_bufferval(p_buffer) + 2;
	name_len = len - 2;
	p_type = x_type_char_register(p_base, p_base);
	p_data = x_type_field_data(p_type);

	while ( ! x_obj_isnil(p_base, p_data)) {
		p_entry = x_firstobj(p_data);
		sym_name = x_atomstr(x_firstobj(p_entry));

		if (name_len == (x_int_t)x_lib_strlen(sym_name)
			&& 0 == x_lib_strncmp(buf_name, sym_name, name_len)) {
			return x_mkchar(p_base, (x_char_t)x_intval(x_restobj(p_entry)));
		}

		p_data = x_restobj(p_data);
	}

	x_obj_error(p_base, "read: unknown character name", NULL);

	return NULL;
}

/**
 * Write the external representation of a character.
 *
 * Outputs the @c #\\ prefix followed by either a named character
 * (reverse-looked-up from the type data alist) or the raw byte.
 * Uses stack-allocated temporaries to avoid heap allocation.
 *
 * @param p_base  Execution context.
 * @param p_args  Pair whose first element is the character to write.
 * @return The character object on success, or NULL on write failure.
 */
x_obj_t *x_sexp_char_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_ret, *p_type, *p_data, *p_entry;
	x_char_t ch = x_atomchar(x_firstobj(p_args));
	x_satom_t str = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .s = &x_atomchar(x_firstobj(p_args)) }),
		sz = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .i = 1 });
	x_spair_t wrap[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE,
			{ str }, { (x_obj_t *)(wrap + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { sz }, { NULL })
	};

	/* Write #\ prefix */
	{
		x_satom_t prefix_str = x_obj_set(x_type_atom_obj,
			X_OBJ_FLAG_NONE, { .s = (x_char_t *)"#\\" });
		x_spair_t prefix_wrap = x_obj_set(NULL,
			X_OBJ_FLAG_NONE, { prefix_str }, { NULL });
		x_interp_write_str(p_base, (x_obj_t *)&prefix_wrap);
	}

	/* Named characters: reverse-lookup in type data alist */
	if (x_base_isset(p_base)) {
		p_type = x_type_char_register(p_base, p_base);
		p_data = x_type_field_data(p_type);

		while ( ! x_obj_isnil(p_base, p_data)) {
			p_entry = x_firstobj(p_data);

			if ((x_char_t)x_intval(x_restobj(p_entry)) == ch) {
				x_satom_t name_str = x_obj_set(x_type_atom_obj,
					X_OBJ_FLAG_NONE,
					{ .s = x_atomstr(x_firstobj(p_entry)) });
				x_spair_t name_wrap = x_obj_set(NULL,
					X_OBJ_FLAG_NONE, { name_str }, { NULL });
				x_interp_write_str(p_base, (x_obj_t *)&name_wrap);
				return x_firstobj(p_args);
			}

			p_data = x_restobj(p_data);
		}
	}

	/* Single character: write raw byte (explicit size, not null-terminated) */
	p_ret = x_interp_write_str(p_base, (x_obj_t *)wrap);

	if ( ! x_obj_isnil(p_base, p_ret)) {
		return x_firstobj(p_args);
	}

	return NULL;
}

/**
 * Display a character as a raw byte (no @c #\\ prefix).
 *
 * Writes exactly one byte using an explicit size parameter so that
 * NUL characters are handled correctly.
 *
 * @param p_base  Execution context.
 * @param p_args  Pair whose first element is the character to display.
 * @return The character object.
 */
x_obj_t *x_sexp_char_display(x_obj_t *p_base, x_obj_t *p_args)
{
	x_satom_t str = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .s = &x_atomchar(x_firstobj(p_args)) }),
		sz = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .i = 1 });
	x_spair_t wrap[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE,
			{ str }, { (x_obj_t *)(wrap + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { sz }, { NULL })
	};

	x_interp_write_str(p_base, (x_obj_t *)wrap);

	return x_firstobj(p_args);
}
