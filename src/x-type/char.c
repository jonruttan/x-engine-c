/**
 * @file char.c
 * @brief Character type implementation.
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
/*
 * # Includes
 */
#include "x-eval.h"
#include "x-type/buffer.h"
#include "x-type/char.h"
#include "x-type/int.h"
#include "x-type/symbol.h"
#include "x-token/sexp/char.h"

x_satom_t x_type_char_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_CHAR_NAME }),
	x_type_char_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_char_make }),
	x_type_char_struct_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_char_struct });

/**
 * Allocate a heap character object with a given value.
 *
 * Builds a stack-based argument list and delegates to x_type_char_make.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param flags   x_obj_flag_t -- Object flags
 * @param cp      x_int_t -- Unicode code point
 * @return x_obj_t* -- New heap-allocated character object
 */
x_obj_t *x_make_char(x_obj_t *p_base, x_obj_flag_t flags, x_int_t cp)
{
	x_satom_t o_char = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = cp }),
		o_flags = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = flags });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_char }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_flags }, { NULL })
	};

	return x_type_char_make(p_base, (x_obj_t *)args);
}

/**
 * Number of bytes in the UTF-8 encoding of a code point.
 *
 * @param cp  x_int_t -- Unicode code point
 * @return x_int_t -- Byte count (1-4)
 */
x_int_t x_char_utf8_len(x_int_t cp)
{
	if (cp < 0x80)
		return 1;
	if (cp < 0x800)
		return 2;
	if (cp < 0x10000)
		return 3;

	return 4;
}

/**
 * Encode a code point as UTF-8.
 *
 * Out-of-range code points are emitted as U+FFFD (replacement char).
 *
 * @param cp   x_int_t -- Unicode code point
 * @param out  x_char_t* -- Destination buffer (>= 4 bytes)
 * @return x_int_t -- Number of bytes written (1-4)
 */
x_int_t x_char_utf8_encode(x_int_t cp, x_char_t *out)
{
	if (cp < 0 || cp > 0x10FFFF)
		cp = 0xFFFD;

	if (cp < 0x80) {
		out[0] = (x_char_t)cp;
		return 1;
	}
	if (cp < 0x800) {
		out[0] = (x_char_t)(0xC0 | (cp >> 6));
		out[1] = (x_char_t)(0x80 | (cp & 0x3F));
		return 2;
	}
	if (cp < 0x10000) {
		out[0] = (x_char_t)(0xE0 | (cp >> 12));
		out[1] = (x_char_t)(0x80 | ((cp >> 6) & 0x3F));
		out[2] = (x_char_t)(0x80 | (cp & 0x3F));
		return 3;
	}
	out[0] = (x_char_t)(0xF0 | (cp >> 18));
	out[1] = (x_char_t)(0x80 | ((cp >> 12) & 0x3F));
	out[2] = (x_char_t)(0x80 | ((cp >> 6) & 0x3F));
	out[3] = (x_char_t)(0x80 | (cp & 0x3F));

	return 4;
}

/**
 * Decode a UTF-8 byte sequence into a code point.
 *
 * Bytes are read unsigned.  The byte count is supplied by the caller
 * (derived from the lead byte), so no bounds scanning is done here.
 *
 * @param bytes   const x_char_t* -- Start of the UTF-8 sequence
 * @param nbytes  x_int_t -- Number of bytes in the sequence (1-4)
 * @return x_int_t -- The decoded code point
 */
x_int_t x_char_utf8_decode(const x_char_t *bytes, x_int_t nbytes)
{
	const unsigned char *b = (const unsigned char *)bytes;

	if (nbytes <= 1)
		return b[0];
	if (nbytes == 2)
		return ((b[0] & 0x1F) << 6) | (b[1] & 0x3F);
	if (nbytes == 3)
		return ((b[0] & 0x0F) << 12) | ((b[1] & 0x3F) << 6)
			| (b[2] & 0x3F);

	return ((b[0] & 0x07) << 18) | ((b[1] & 0x3F) << 12)
		| ((b[2] & 0x3F) << 6) | (b[3] & 0x3F);
}

#define sym(S)		x_mksymbol(p_base, (x_char_t *)(S))
#define num(N)		x_mkint(p_base, (N))
#define entry(S,N)	x_mkspair(p_base, X_OBJ_FLAG_NONE, sym(S), num(N))
#define pair(A,B)	x_mkspair(p_base, X_OBJ_FLAG_NONE, (A), (B))

/**
 * Build the character type descriptor struct.
 *
 * Populates name, make, analyse, read, write, and display callbacks.
 * Also builds an alist of named character constants (alarm, backspace,
 * delete, escape, newline, null, return, space, tab) as type data.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_obj   x_obj_t* -- Unused
 * @return x_obj_t* -- Type descriptor pair list
 */
x_obj_t *x_type_char_struct(x_obj_t *p_base, x_obj_t *p_obj)
{
	struct x_type_t type = { 0 };

	/* Named character constants: '((name . code) ...) */
	type.p_name = x_type_char_name;
	type.p_make = x_type_char_make_prim;
	type.p_analyse = x_sexp_char_analyse1_prim;
	type.p_read = x_sexp_char_read_prim;
	type.p_data =
		pair(entry("alarm", '\a'),
		pair(entry("backspace", '\b'),
		pair(entry("delete", 127),
		pair(entry("escape", '\033'),
		pair(entry("newline", '\n'),
		pair(entry("null", '\0'),
		pair(entry("return", '\r'),
		pair(entry("space", ' '),
		pair(entry("tab", '\t'),
		NULL)))))))));

	return x_type_struct_make(p_base, type);
}

#undef sym
#undef num
#undef entry
#undef pair

/**
 * Register or retrieve the character type on the base context.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Unused
 * @return x_obj_t* -- Registered type object
 */
x_obj_t *x_type_char_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_char_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_char_struct_prim }, { NULL })
	};

	return x_type_struct_get(p_base, (x_obj_t *)args);
}

/**
 * Type-system make callback for character objects.
 *
 * Extracts the character value and optional flags from p_args,
 * then allocates a heap object via x_obj_make.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- (char-value . (flags | nil))
 * @return x_obj_t* -- New heap-allocated character object
 */
x_obj_t *x_type_char_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = x_type_char_register(p_base, p_base),
		*p_char = x_0(p_args);
	x_obj_flag_t flags = x_obj_isnil(p_base, x_1(p_args))
		? 0 : x_firstint(x_01(p_args));

	return x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_ATOM, x_atomint(p_char));
}
