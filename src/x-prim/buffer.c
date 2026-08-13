/** @file x-prim/buffer.c
 *  @brief Buffer and tokenizer primitives -- buffer-*, token-read(-string),
 *         buffer-token, buffer-last-char.
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2026 Jon Ruttan
 *  @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
#include "x-prim.h"
#include "x-eval.h"
#include "x-heap.h"
#include "x-type.h"
#include "x-token.h"
#include "x-type/buffer.h"
#include "x-type/char.h"
#include "x-type/int.h"
#include "x-type/iter.h"
#include "x-type/list.h"
#include "x-type/prim.h"
#include "x-type/str.h"

/**
 * @brief Extract the consumed portion of a buffer as a string.
 *
 * x-lang form: @code (buffer-token buffer) @endcode
 *
 * Reads the buffer's current length (bytes consumed by the tokenizer)
 * and duplicates that prefix into a new owned string.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self buffer).
 * @return New string containing the consumed buffer content.
 */
static x_obj_t *x_prim_buffer_token(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer;
	x_int_t len;
	x_char_t *str;

	x_eargs(p_base, p_args, 2, NULL, &p_buffer);
	len = x_bufferlen(p_buffer);
	str = x_lib_strndup(x_bufferval(p_buffer), len);

	return x_mkstrown(p_base, str);
}

/**
 * Return the last character consumed by the tokenizer buffer.
 *
 * x-lang form: @code (buffer-last-char buffer) @endcode
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self buffer).
 * @return Integer character code, or NULL if buffer is empty.
 */
static x_obj_t *x_prim_buffer_last_char(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer;

	x_eargs(p_base, p_args, 2, NULL, &p_buffer);

	if (x_bufferlen(p_buffer) == 0) {
		return NULL;
	}

	return x_mkint(p_base, (x_int_t)x_bufferlastchar(p_buffer));
}

/**
 * @brief Tokenize a string using a token base's registered types.
 *
 * x-lang form: @code (token-read-string token-base string) @endcode
 *
 * Copies the input string into a read-only buffer, then repeatedly calls
 * x_token_read against the token base to produce a linked list of token
 * objects. If metadata tracking is active, the buffer is marked with
 * initial line number 1.
 *
 * @param p_base       Calling execution context (tokens allocated here).
 * @param p_args       Unevaluated: (self token-base string).
 * @return Linked list of token objects, or NULL for empty input.
 * @note The token base should have tokenizer types registered via
 *       make-token-base + base-make-type.
 */
static x_obj_t *x_prim_token_read_string(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_token_base, *p_str;
	x_char_t *str;
	x_int_t len;
	x_char_t *buf;
	x_obj_t *p_buffer, *p_token, *p_result, *p_tail, *p_node;
	x_spair_t read_args[1];

	x_eargs(p_base, p_args, 3, NULL, &p_token_base, &p_str);
	str = x_strval(p_str);
	len = x_lib_strlen(str);
	buf = (x_char_t *)x_sys_malloc(len + 1);

	x_lib_memcpy(buf, str, len);
	buf[len] = '\0';

	p_buffer = x_mkfbufferown(p_token_base, X_OBJ_FLAG_RO, buf);
	x_bufferwrite(p_buffer) = x_bufferval(p_buffer) + len;

	if (x_atomint(x_firstobj(x_base_field_obj_meta_extra(p_base))) > 0
			&& (x_obj_flags(p_buffer) & X_OBJ_FLAG_META)) {
		x_obj_meta_i(p_buffer, 0).i = 1;
	}

	p_result = NULL;
	p_tail = NULL;

	read_args[0][X_OBJ_META_TYPE].p = NULL;
	read_args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)read_args) = p_buffer;
	x_restobj((x_obj_t *)read_args) = p_token_base;

	for (;;) {
		p_token = x_token_read(p_token_base, (x_obj_t *)read_args);

		/* Clean exhaustion of the string.  Checked BEFORE the nil
		 * break so the two stay distinct; the nil break itself is
		 * kept -- read-str has always stopped at a nil token, and
		 * its drop-unterminated-tail contract is relied upon. */
		if (p_token == (x_obj_t *)x_token_eof_prim) {
			break;
		}

		if (x_obj_isnil(p_token_base, p_token)) {
			break;
		}

		p_node = x_mklist(p_base, p_token, NULL);

		if (x_obj_isnil(p_base, p_result)) {
			p_result = p_node;
		} else {
			x_restobj(p_tail) = p_node;
		}

		p_tail = p_node;
	}

	return p_result;
}

/**
 * Read the next expression from a tokenizer buffer.
 *
 * Exposes x_token_read to x-lang so that custom reader hooks (defined
 * via make-type) can recursively read sub-expressions from the stream.
 *
 * x-lang form: @code (token-read buffer) @endcode
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self buffer).
 * @return Parsed expression, or NULL on EOF.
 */
static x_obj_t *x_prim_token_read(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer, *p_obj;
	x_spair_t read_args[1];

	x_eargs(p_base, p_args, 2, NULL, &p_buffer);

	read_args[0][X_OBJ_META_TYPE].p = NULL;
	read_args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)read_args) = p_buffer;
	x_restobj((x_obj_t *)read_args) = p_base;

	x_type_buffer_retain(p_base, (x_obj_t *)read_args);

	p_obj = x_token_read(p_base, (x_obj_t *)read_args);

	/* Map the EOF sentinel to nil at this boundary: x-lang reader
	 * handlers (quasi/lit, logo's block loop) test (null? ...) for
	 * end of input and must never see the raw sentinel. */
	return p_obj == (x_obj_t *)x_token_eof_prim ? NULL : p_obj;
}

/** x-lang (buffer-make str [flags]): a BUFFER viewing @p str's bytes.
 *  NON-OWNING (the wrap rule): the buffer's cursors walk the string's own
 *  storage, so the string must outlive the buffer and (str make) is the
 *  natural backing store.  Both cursors start at the base -- append then
 *  read, or fill the string first and walk the read cursor.  Optional
 *  flags (descriptor names, e.g. %obj-flag-ro): a read-only buffer
 *  returns EOF at exhaustion instead of extending from stdin. */
static x_obj_t *x_prim_buffer_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_str, *p_rest;
	x_obj_flag_t flags = 0;

	x_eargs(p_base, p_args, 2, NULL, &p_str);
	p_rest = x_11(p_args);
	if ( ! x_obj_isnil(p_base, p_rest))
		flags = (x_obj_flag_t)x_intval(
			x_eval_arg(p_base, x_firstobj(p_rest)));

	return x_make_buffer(p_base, flags, x_strval(p_str));
}

/** x-lang (buffer-reset buffer): empty the buffer (cursors back to base). */
static x_obj_t *x_prim_buffer_reset(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_prim_op1(p_base, p_args, x_type_buffer_reset);
}

/** x-lang (buffer-retain buffer): compact unread data to the front. */
static x_obj_t *x_prim_buffer_retain(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_prim_op1(p_base, p_args, x_type_buffer_retain);
}

/** x-lang (buffer-append buffer char): write one char at the write cursor. */
static x_obj_t *x_prim_buffer_append(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer, *p_char;
	x_spair_t args[2];

	x_eargs(p_base, p_args, 3, NULL, &p_buffer, &p_char);

	args[0][X_OBJ_META_TYPE].p = NULL;
	args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)args) = p_buffer;
	x_restobj((x_obj_t *)args) = (x_obj_t *)(args + 1);
	args[1][X_OBJ_META_TYPE].p = NULL;
	args[1][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)(args + 1)) = p_char;
	x_restobj((x_obj_t *)(args + 1)) = NULL;

	return x_type_buffer_append(p_base, (x_obj_t *)args);
}

/** x-lang (buffer-read buffer): read one char (extending from input), or nil. */
static x_obj_t *x_prim_buffer_read(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_prim_op1(p_base, p_args, x_type_buffer_read);
}

/** x-lang (buffer-read-text buffer): like buffer-read, NUL counts as EOF. */
static x_obj_t *x_prim_buffer_read_text(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_prim_op1(p_base, p_args, x_type_buffer_read_text);
}


/** Register the buffer and tokenizer primitives. */
x_obj_t *x_prim_buffer_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_prim_entry_t entries[] = {
		{ "buffer-token",      x_prim_buffer_token,      "buf", "tok"         },
		{ "buffer-last-char",  x_prim_buffer_last_char,  "buf", "last-char"     },
		{ "token-read",        x_prim_token_read,        "tok",  "read"          },
		{ "token-read-string", x_prim_token_read_string, "tok",  "read-str"      },
		{ "buffer-make",       x_prim_buffer_make,       "buf", "make"          },
		{ "buffer-reset",      x_prim_buffer_reset,      "buf", "reset"         },
		{ "buffer-retain",     x_prim_buffer_retain,     "buf", "retain"        },
		{ "buffer-append",     x_prim_buffer_append,     "buf", "append"        },
		{ "buffer-read",       x_prim_buffer_read,       "buf", "read"          },
		{ "buffer-read-text",  x_prim_buffer_read_text,  "buf", "read-text"     }
	};

	x_prims_bind_table(p_base, entries, sizeof(entries) / sizeof(entries[0]));
	return p_base;
}
