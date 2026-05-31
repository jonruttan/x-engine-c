/** @file string.c
 *  @brief String manipulation primitives.
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

/** Register string manipulation primitives.
 *
 *  Binds: @c str-append, @c str->symbol, @c symbol->str, @c bytes->str,
 *  @c list->str.
 *
 *  @note @c bytes->str and @c list->str are the SAME byte-packer here. The
 *        x-lang string layer (x/type/string) redefines @c list->str to be
 *        code-point aware (UTF-8 encoding each char via x/codec/utf8) on top of
 *        @c bytes->str, which keeps the raw byte-packing name. Until that
 *        redefinition loads, @c list->str is the byte-packer -- fine for the
 *        ASCII-only boot callers (e.g. number->str).
 *
 *  @param p_base Interpreter base context.
 *  @param p_args Unused.
 *  @return @p p_base.
 */
x_obj_t *x_prim_string_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_callable_entry_t entries[] = {
		{ "str-append", x_prim_string_append },
		{ "str->symbol", x_prim_string_to_symbol },
		{ "symbol->str", x_prim_symbol_to_string },
		{ "bytes->str", x_prim_list_to_string },
		{ "list->str", x_prim_list_to_string }
	};

	x_callable_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
