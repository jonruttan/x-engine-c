/*
 * # Computational Expressions in C
 *
 * ## x-prim/pred.c -- Implementation - Primitives - Predicates
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
#include "x-type/list.h"
#include "x-type/prim.h"
#include "x-type/procedure.h"
#include "x-type/str.h"
#include "x-type/symbol.h"

/* eq?: (eq? a b) -> pointer equality */
static x_obj_t *x_prim_eq(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return a == b ? x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE) : p_base;
}

/* =: (= a b) -> integer value equality */
static x_obj_t *x_prim_numeq(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return x_intval(a) == x_intval(b)
		? x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE) : p_base;
}

/* <: (< a b) -> t if a < b */
static x_obj_t *x_prim_lt(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return x_intval(a) < x_intval(b)
		? x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE) : p_base;
}

/* >: (> a b) -> t if a > b */
static x_obj_t *x_prim_gt(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return x_intval(a) > x_intval(b)
		? x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE) : p_base;
}

/* <=: (<= a b) -> t if a <= b */
static x_obj_t *x_prim_lte(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return x_intval(a) <= x_intval(b)
		? x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE) : p_base;
}

/* >=: (>= a b) -> t if a >= b */
static x_obj_t *x_prim_gte(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return x_intval(a) >= x_intval(b)
		? x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE) : p_base;
}

/* null?: (null? x) -> t if nil */
static x_obj_t *x_prim_nullp(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *x = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_obj_isnil(p_base, x) ? x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE) : p_base;
}

/* pair?: (pair? x) -> t if list pair */
static x_obj_t *x_prim_pairp(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *x = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_obj_type_islist(p_base, x) ? x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE) : p_base;
}

/* atom?: (atom? x) -> t if not a list pair */
static x_obj_t *x_prim_atomp(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *x = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_obj_type_islist(p_base, x) ? p_base : x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE);
}

/* not: (not x) -> t if nil */
static x_obj_t *x_prim_not(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *x = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_obj_isnil(p_base, x) ? x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE) : p_base;
}

/* number?: (number? x) -> t if integer */
static x_obj_t *x_prim_numberp(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *x = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_obj_type_isint(p_base, x) ? x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE) : p_base;
}

/* string?: (string? x) -> t if string */
static x_obj_t *x_prim_stringp(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *x = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_obj_type_isstr(p_base, x) ? x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE) : p_base;
}

/* symbol?: (symbol? x) -> t if symbol */
static x_obj_t *x_prim_symbolp(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *x = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_obj_type_issymbol(p_base, x) ? x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE) : p_base;
}

/* procedure?: (procedure? x) -> t if callable (fn or prim) */
static x_obj_t *x_prim_procedurep(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *x = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return (x_obj_type_isprocedure(p_base, x) || x_obj_type_isprim(p_base, x))
		? x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE) : p_base;
}

/* char?: (char? x) -> t if character */
static x_obj_t *x_prim_charp(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *x = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_obj_type_ischar(p_base, x)
		? x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE) : p_base;
}

x_obj_t *x_prim_pred_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_prim_bind(p_base, "eq?", x_prim_eq);
	x_prim_bind(p_base, "=", x_prim_numeq);
	x_prim_bind(p_base, "<", x_prim_lt);
	x_prim_bind(p_base, ">", x_prim_gt);
	x_prim_bind(p_base, "<=", x_prim_lte);
	x_prim_bind(p_base, ">=", x_prim_gte);
	x_prim_bind(p_base, "null?", x_prim_nullp);
	x_prim_bind(p_base, "pair?", x_prim_pairp);
	x_prim_bind(p_base, "atom?", x_prim_atomp);
	x_prim_bind(p_base, "not", x_prim_not);
	x_prim_bind(p_base, "number?", x_prim_numberp);
	x_prim_bind(p_base, "string?", x_prim_stringp);
	x_prim_bind(p_base, "symbol?", x_prim_symbolp);
	x_prim_bind(p_base, "procedure?", x_prim_procedurep);
	x_prim_bind(p_base, "char?", x_prim_charp);

	return p_base;
}
