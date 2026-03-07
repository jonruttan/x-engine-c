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
#include "x-type/int.h"
#include "x-type/str.h"
#include "x-type/symbol.h"

/* string-length: (string-length str) -> integer length */
static x_obj_t *x_prim_string_length(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_str = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_mkint(p_base, x_lib_strlen(x_strval(p_str)));
}

/* string-ref: (string-ref str index) -> single-char string */
static x_obj_t *x_prim_string_ref(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_str = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_idx = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));
	x_char_t *s = x_lib_strndup(x_strval(p_str) + x_intval(p_idx), 1);

	return x_mkstrown(p_base, s);
}

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

/* substring: (substring str start end) -> sub-string */
static x_obj_t *x_prim_substring(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_str = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_start = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args))),
		*p_end = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(x_restobj(p_args))));
	x_int_t start = x_intval(p_start), end = x_intval(p_end);

	return x_mkstrown(p_base, x_lib_strndup(x_strval(p_str) + start,
		end - start));
}

/* string=?: (string=? str1 str2) -> t if equal */
static x_obj_t *x_prim_string_eq(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return x_lib_strcmp(x_strval(p_a), x_strval(p_b)) == 0
		? x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE) : p_base;
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

/* number->string: (number->string n) -> string representation */
static x_obj_t *x_prim_number_to_string(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_n = x_prim_eval_arg(p_base, x_firstobj(p_args));
	x_char_t buf[22];

	x_lib_inttostr(x_intval(p_n), buf, 10);

	return x_mkstrown(p_base, x_lib_strndup(buf, x_lib_strlen(buf)));
}

/* string->number: (string->number str) -> integer */
static x_obj_t *x_prim_string_to_number(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_str = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_mkint(p_base, x_lib_strtoint(x_strval(p_str), NULL, 0));
}

x_obj_t *x_prim_string_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_prim_bind(p_base, "string-length", x_prim_string_length);
	x_prim_bind(p_base, "string-ref", x_prim_string_ref);
	x_prim_bind(p_base, "string-append", x_prim_string_append);
	x_prim_bind(p_base, "substring", x_prim_substring);
	x_prim_bind(p_base, "string=?", x_prim_string_eq);
	x_prim_bind(p_base, "string->symbol", x_prim_string_to_symbol);
	x_prim_bind(p_base, "symbol->string", x_prim_symbol_to_string);
	x_prim_bind(p_base, "number->string", x_prim_number_to_string);
	x_prim_bind(p_base, "string->number", x_prim_string_to_number);

	return p_base;
}
