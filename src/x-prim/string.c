/*
 * # Computational Expressions in C
 *
 * ## x-prim/string.c -- Implementation - Primitives - Strings
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
#include "x-type/str.h"
#include "x-type/symbol.h"

/* string-append: (string-append str...) -> concatenated string */
static x_obj_t *x_prim_string_append(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_a, *p_b;
	x_char_t *sa, *sb, *s;
	size_t la, lb;

	p_a = x_prim_eval_arg(p_base, x_firstobj(p_args));
	p_b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));
	sa = x_strval(p_a);
	sb = x_strval(p_b);
	la = x_lib_strlen(sa);
	lb = x_lib_strlen(sb);
	s = (x_char_t *)x_sys_malloc(la + lb + 1);
	x_lib_memcpy(s, sa, la);
	x_lib_memcpy(s + la, sb, lb + 1);

	return x_mkstrown(p_base, s);
}

/* string=?: (string=? str1 str2) -> t if equal */
static x_obj_t *x_prim_string_eq(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return x_lib_strcmp(x_strval(p_a), x_strval(p_b)) == 0
		? x_base_field_true(p_base) : NULL;
}

/* string->symbol: (string->symbol str) -> convert string to symbol */
static x_obj_t *x_prim_string_to_symbol(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_str = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_mksymbol(p_base, x_strval(p_str));
}

/* symbol->string: (symbol->string sym) -> convert symbol to string */
static x_obj_t *x_prim_symbol_to_string(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_sym = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_mkstr(p_base, x_symbolval(p_sym));
}

/* number->string: (number->string n [radix]) -> string representation */
static x_obj_t *x_prim_number_to_string(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_n = x_prim_eval_arg(p_base, x_firstobj(p_args));
	x_int_t radix = 10;
	x_char_t buf[66];

	if ( ! x_obj_isnil(p_base, x_restobj(p_args))) {
		radix = x_intval(x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args))));
	}

	x_lib_inttostr(x_intval(p_n), buf, radix);

	return x_mkstrown(p_base, x_lib_strndup(buf, x_lib_strlen(buf)));
}

/* string->number: (string->number str [radix]) -> integer */
static x_obj_t *x_prim_string_to_number(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_str = x_prim_eval_arg(p_base, x_firstobj(p_args));
	x_int_t radix = 0;

	if ( ! x_obj_isnil(p_base, x_restobj(p_args))) {
		radix = x_intval(x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args))));
	}

	return x_mkint(p_base, x_lib_strtoint(x_strval(p_str), NULL, radix));
}

/* list->string: (list->string list-of-chars) -> string */
static x_obj_t *x_prim_list_to_string(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_list = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_cur;
	x_char_t *s;
	size_t len = 0;

	for (p_cur = p_list; !x_obj_isnil(p_base, p_cur); p_cur = x_restobj(p_cur))
		len++;
	s = (x_char_t *)x_sys_malloc(len + 1);
	len = 0;
	for (p_cur = p_list; !x_obj_isnil(p_base, p_cur); p_cur = x_restobj(p_cur))
		s[len++] = x_charval(x_firstobj(p_cur));
	s[len] = '\0';

	return x_mkstrown(p_base, s);
}

/* make-string: (make-string k [char]) -> string of k copies of char */
static x_obj_t *x_prim_make_string(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_k = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_rest = x_restobj(p_args);
	x_int_t k = x_intval(p_k);
	x_char_t fill = ' ', *s;
	x_int_t i;

	if (!x_obj_isnil(p_base, p_rest))
		fill = x_charval(x_prim_eval_arg(p_base, x_firstobj(p_rest)));
	s = (x_char_t *)x_sys_malloc(k + 1);
	for (i = 0; i < k; i++)
		s[i] = fill;
	s[k] = '\0';

	return x_mkstrown(p_base, s);
}

x_obj_t *x_prim_string_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_prim_entry_t entries[] = {
		{ "string-append", x_prim_string_append },
		{ "string=?", x_prim_string_eq },
		{ "string->symbol", x_prim_string_to_symbol },
		{ "symbol->string", x_prim_symbol_to_string },
		{ "number->string", x_prim_number_to_string },
		{ "string->number", x_prim_string_to_number },
		{ "list->string", x_prim_list_to_string },
		{ "make-string", x_prim_make_string }
	};

	x_prim_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
