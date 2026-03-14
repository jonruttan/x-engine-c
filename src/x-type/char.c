/*
 * # Computational Expressions in C
 *
 * ## x-type.c -- Implementation - Type - Character
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2021 Jon Ruttan
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
#include "x-base.h"
#include "x-type/buffer.h"
#include "x-type/char.h"
#include "x-type/int.h"
#include "x-type/symbol.h"
#include "x-token/sexp/char.h"

x_satom_t x_type_char_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_CHAR_NAME }),
	x_type_char_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_char_make }),
	x_type_char_struct_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_char_struct });

x_obj_t *x_make_char(x_obj_t *p_base, x_obj_flag_t flags, x_char_t c)
{
	x_satom_t o_char = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .c = c }),
		o_flags = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = flags });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_char }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_flags }, { NULL })
	};

	return x_type_char_make(p_base, (x_obj_t *)args);
}

#define sym(S)		x_mksymbol(p_base, (x_char_t *)(S))
#define num(N)		x_mkint(p_base, (N))
#define entry(S,N)	x_mkspair(p_base, sym(S), num(N))
#define cons(A,B)	x_mkspair(p_base, (A), (B))

x_obj_t *x_type_char_struct(x_obj_t *p_base, x_obj_t *p_obj)
{
	struct x_type_t type = { 0 };

	/* Named character constants: '((name . code) ...) */
	type.p_name = x_type_char_name;
	type.p_make = x_type_char_make_prim;
	type.p_analyse = x_sexp_char_analyse1_prim;
	type.p_write = x_sexp_char_write_prim;
	type.p_display = x_sexp_char_display_prim;
	type.p_data =
		cons(entry("alarm", '\a'),
		cons(entry("backspace", '\b'),
		cons(entry("delete", 127),
		cons(entry("escape", '\033'),
		cons(entry("newline", '\n'),
		cons(entry("null", '\0'),
		cons(entry("return", '\r'),
		cons(entry("space", ' '),
		cons(entry("tab", '\t'),
		NULL)))))))));

	return x_type_struct_make(p_base, type);
}

#undef sym
#undef num
#undef entry
#undef cons

x_obj_t *x_type_char_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_char_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_char_struct_prim }, { NULL })
	};

	return x_type_struct_get(p_base, (x_obj_t *)args);
}

x_obj_t *x_type_char_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = x_type_char_register(p_base, p_base),
		*p_char = x_0(p_args);
	x_obj_flag_t flags = x_obj_isnil(p_base, x_1(p_args))
		? 0 : x_firstint(x_01(p_args));

	return x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_ATOM, x_charval(p_char));
}
