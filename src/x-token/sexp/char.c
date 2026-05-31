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
/* Forward declaration for UTF-8 continuation state */
static x_obj_t *x_sexp_char_analyse_utf8(x_obj_t *p_base, x_obj_t *p_args);

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

static x_spair_t x_sexp_char_analyse_utf8_prim =
	x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = x_sexp_char_analyse_utf8 }, { NULL });

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

	/* UTF-8 lead byte (>= 0x80): keep consuming continuation bytes */
	if ((unsigned char)x_bufferlastchar(p_buffer) >= 0x80) {
		return x_next_state(p_self, &x_sexp_char_analyse_utf8_prim);
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
 * Analyse UTF-8 continuation: consume 10xxxxxx bytes.
 *
 * Reached after a multi-byte lead byte.  Keeps consuming continuation
 * bytes (0x80-0xBF).  On the first non-continuation byte the read
 * pointer is backed up and the accumulated length is scored.
 *
 * @param p_base  Execution context.
 * @param p_args  Pair of (self, read-args).
 * @return Self to keep reading, or score at the end of the sequence.
 */
static x_obj_t *x_sexp_char_analyse_utf8(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_self = x_0(p_args),
		*p_buffer = x_token_read_arg_buffer(x_1(p_args)),
		*p_score = x_token_read_arg_score(x_1(p_args));

	/* Continuation byte 10xxxxxx: keep consuming */
	if (((unsigned char)x_bufferlastchar(p_buffer) & 0xC0) == 0x80) {
		return p_self;
	}

	/* End of sequence: un-read the delimiter and score */
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

	/* UTF-8 multi-byte literal: #\<lead><continuation...> */
	if (len > 3 && (unsigned char)*(x_bufferval(p_buffer) + 2) >= 0x80) {
		return x_mkchar(p_base,
			x_char_utf8_decode(x_bufferval(p_buffer) + 2, len - 2));
	}

	/* Single character: #\c (length 3) */
	if (len == 3) {
		return x_mkchar(p_base, (unsigned char)*(x_bufferval(p_buffer) + 2));
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
			return x_mkchar(p_base, x_intval(x_restobj(p_entry)));
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
	x_int_t cp = x_atomint(x_firstobj(p_args));

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

			if (x_intval(x_restobj(p_entry)) == cp) {
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

	/* Glyph: emit the code point's low byte (ASCII-correct). Boot fallback --
	 * the x-lang write handler (type-push-write) emits full UTF-8 for non-ASCII
	 * via x/codec/utf8 and shadows this before any non-ASCII char is written.
	 * No UTF-8 encoding in C. */
	{
		x_char_t byte = (x_char_t)(cp & 0xFF);
		x_satom_t str = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
			{ .s = &byte }),
			sz = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .i = 1 });
		x_spair_t wrap[2] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE,
				{ str }, { (x_obj_t *)(wrap + 1) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { sz }, { NULL })
		};

		p_ret = x_interp_write_str(p_base, (x_obj_t *)wrap);
	}

	if ( ! x_obj_isnil(p_base, p_ret)) {
		return x_firstobj(p_args);
	}

	return NULL;
}

/**
 * Display a character as a single raw byte (its code point's low 8 bits).
 *
 * Protocol-agnostic boot fallback: this is the bottom of the CHARACTER
 * display stack, correct for ASCII (code point < 0x80). The x-lang layer
 * pushes a display handler (via type-push-display) that emits the full
 * 1-4 byte UTF-8 encoding for non-ASCII code points using the x/codec/utf8
 * encoder; that handler shadows this one before any non-ASCII char is shown.
 * Keeping UTF-8 out of C: this fallback never encodes.
 *
 * @param p_base  Execution context.
 * @param p_args  Pair whose first element is the character to display.
 * @return The character object.
 */
x_obj_t *x_sexp_char_display(x_obj_t *p_base, x_obj_t *p_args)
{
	x_char_t byte = (x_char_t)(x_atomint(x_firstobj(p_args)) & 0xFF);
	x_satom_t str = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = &byte }),
		sz = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .i = 1 });
	x_spair_t wrap[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE,
			{ str }, { (x_obj_t *)(wrap + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { sz }, { NULL })
	};

	x_interp_write_str(p_base, (x_obj_t *)wrap);

	return x_firstobj(p_args);
}
