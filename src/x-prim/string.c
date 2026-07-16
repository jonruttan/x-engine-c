/** @file string.c
 *  @brief String manipulation primitives.
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
#include "x-type/char.h"
#include "x-type/int.h"
#include "x-type/str.h"
#include "x-type/symbol.h"

/** Concatenate two strings.
 *  x-lang: (str-append a b)
 *  @param p_base Interpreter base context.
 *  @param p_args Unevaluated argument list: (str-append a b).
 *  @return A newly allocated string containing @p a followed by @p b.
 *  @note The result is heap-allocated and owned by the returned object.
 */
static x_obj_t *x_prim_string_append(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_a, *p_b;
	x_char_t *sa, *sb, *s;
	size_t la, lb;

	x_eargs(p_base, p_args, 3, NULL, &p_a, &p_b);
	sa = x_strval(p_a);
	sb = x_strval(p_b);
	la = x_lib_strlen(sa);
	lb = x_lib_strlen(sb);
	s = (x_char_t *)x_sys_malloc(la + lb + 1);
	x_lib_memcpy(s, sa, la);
	x_lib_memcpy(s + la, sb, lb + 1);

	return x_mkstrown(p_base, s);
}

/** Allocate a fresh writable byte region as a string object.
 *  x-lang: (make-str n)
 *  @param p_base Interpreter base context.
 *  @param p_args Unevaluated argument list: (make-str n).
 *  @return A new OWNED string of exactly @p n bytes.
 *  @note SPACE-filled and NUL-terminated so the strlen-based byte
 *        accessors see exactly n bytes.  Callers fill it in place via
 *        (str ->ptr) + the raw-mem stores, File read, or FFI calls.
 *        The object OWNS the region -- the GC frees it, so there is no
 *        free door and no leak on an error path.  UNCHECKED like
 *        first/rest: n is trusted (the x layer guards).
 */
static x_obj_t *x_prim_make_str(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_n;
	x_char_t *s;
	size_t n;

	x_eargs(p_base, p_args, 2, NULL, &p_n);
	n = (size_t)x_intval(p_n);
	s = (x_char_t *)x_sys_malloc(n + 1);
	x_lib_memset(s, ' ', n);
	s[n] = '\0';

	return x_mkstrown(p_base, s);
}

/** Convert a string to an interned symbol.
 *  x-lang: (str->symbol str)
 *  @param p_base Interpreter base context.
 *  @param p_args Unevaluated argument list: (str->symbol str).
 *  @return A symbol whose name matches the content of @p str.
 */
static x_obj_t *x_prim_string_to_symbol(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_str;
	x_eargs(p_base, p_args, 2, NULL, &p_str);

	return x_mksymbol(p_base, x_strval(p_str));
}

/** Convert a symbol to a string.
 *  x-lang: (symbol->str sym)
 *  @param p_base Interpreter base context.
 *  @param p_args Unevaluated argument list: (symbol->str sym).
 *  @return A string object containing the symbol's name.
 */
static x_obj_t *x_prim_symbol_to_string(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_sym;
	x_eargs(p_base, p_args, 2, NULL, &p_sym);

	return x_mkstr(p_base, x_symbolval(p_sym));
}

/** Build a string from a list of characters, one byte per character.
 *  x-lang: (list->str list-of-chars)
 *  @param p_base Interpreter base context.
 *  @param p_args Unevaluated argument list: (list->str list-of-chars).
 *  @return A newly allocated string holding each character's low byte.
 *  @note Byte-level and protocol-agnostic: each CHARACTER contributes one byte
 *        (its value masked to 0-255). This is the dumb byte-packer; assembling
 *        a multi-byte UTF-8 string from code points is done in the x-lang layer
 *        (str->list / list->str in x/type/string, via the x/codec/utf8 encoder).
 *        Walks the list twice: once to count, once to copy. No x-lang dispatch,
 *        so it stays safe inside tokenizer callbacks.
 */
static x_obj_t *x_prim_list_to_string(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_list, *p_cur;
	x_char_t *s;
	size_t len = 0;

	x_eargs(p_base, p_args, 2, NULL, &p_list);
	for (p_cur = p_list; !x_obj_isnil(p_base, p_cur); p_cur = x_restobj(p_cur))
		len++;
	s = (x_char_t *)x_sys_malloc(len + 1);
	len = 0;
	for (p_cur = p_list; !x_obj_isnil(p_base, p_cur); p_cur = x_restobj(p_cur))
		s[len++] = (x_char_t)(x_atomint(x_firstobj(p_cur)) & 0xFF);
	s[len] = '\0';

	return x_mkstrown(p_base, s);
}

/** Raw byte length of a string.
 *  x-lang: (str-byte-len s)
 *  @param p_base Interpreter base context.
 *  @param p_args Unevaluated argument list: (str-byte-len s).
 *  @return Integer: the number of bytes in @p s.
 *  @note Always byte-level, independent of any pushed string-call handler.
 *        This is the explicit 8-bit accessor the Str8 protocol bottoms out in;
 *        readers/tokenizers that need bytes must use this, not the ambient
 *        (s) call (whose meaning depends on the installed protocol).
 */
static x_obj_t *x_prim_str_byte_len(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_str;
	x_eargs(p_base, p_args, 2, NULL, &p_str);

	return x_mkint(p_base, (x_int_t)x_lib_strlen(x_strval(p_str)));
}

/** Byte at index i of a string, as a CHARACTER (0-255).
 *  x-lang: (str-byte-ref s i)
 *  @param p_base Interpreter base context.
 *  @param p_args Unevaluated argument list: (str-byte-ref s i).
 *  @return The i-th byte as a CHARACTER. Negative i counts from the end.
 *  @note Always byte-level, independent of any pushed string-call handler.
 */
static x_obj_t *x_prim_str_byte_ref(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_str, *p_idx;
	x_char_t *s;
	x_int_t n;
	x_eargs(p_base, p_args, 3, NULL, &p_str, &p_idx);

	s = x_strval(p_str);
	n = x_atomint(p_idx);
	if (n < 0)
		n += (x_int_t)x_lib_strlen(s);

	return x_mkchar(p_base, (unsigned char)s[n]);
}

/** Byte substring: len bytes of s starting at byte offset start.
 *  x-lang: (str-byte-sub s start len)
 *  @param p_base Interpreter base context.
 *  @param p_args Unevaluated argument list: (str-byte-sub s start len).
 *  @return A newly allocated string of @p len bytes from byte offset @p start.
 *  @note Always byte-level, independent of any pushed string-call handler.
 */
static x_obj_t *x_prim_str_byte_sub(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_str, *p_start, *p_len;
	x_eargs(p_base, p_args, 4, NULL, &p_str, &p_start, &p_len);

	return x_mkstr(p_base, x_lib_strndup(
		x_strval(p_str) + x_atomint(p_start), x_atomint(p_len)));
}

/** Register string manipulation primitives.
 *
 *  Binds: @c str-append, @c str->symbol, @c symbol->str, @c bytes->str,
 *  @c str-byte-len, @c str-byte-ref, @c str-byte-sub.
 *
 *  @note @c bytes->str is the raw byte-packer (one low byte per char).  The
 *        code-point-aware @c list->str is pure x-lang (x/type/str-utf8),
 *        UTF-8 encoding each char on top of @c bytes->str; boot-time callers
 *        that want bytes call @c bytes->str directly.
 *
 *  @param p_base Interpreter base context.
 *  @param p_args Unused.
 *  @return @p p_base.
 */
x_obj_t *x_prim_string_register(x_obj_t *p_base, x_obj_t *p_args)
{
	/* { env-name, fn, catalog-ns, catalog-method }.  Conversions are filed
	 * under their source type (the eventual method receiver). */
	static const x_prim_entry_t entries[] = {
		{ "str-append",   x_prim_string_append,    "str",   "append"   },
		{ "make-str",     x_prim_make_str,         "str",   "make"     },
		{ "str->symbol",  x_prim_string_to_symbol, "str",   "->sym"    },
		{ "symbol->str",  x_prim_symbol_to_string, "sym",   "->str"    },
		{ "bytes->str",   x_prim_list_to_string,   "bytes", "->str"    },
		{ "str-byte-len", x_prim_str_byte_len,     "str",   "byte-len" },
		{ "str-byte-ref", x_prim_str_byte_ref,     "str",   "byte-ref" },
		{ "str-byte-sub", x_prim_str_byte_sub,     "str",   "byte-sub" }
	};

	x_prims_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
