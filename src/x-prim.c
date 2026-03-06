/*
 * # Computational Expressions in C
 *
 * ## x-prim.c -- Implementation - Primitives
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
#include "x-base.h"
#include "x-eval.h"
#include "x-exp/quote.h"
#include "x-type/int.h"
#include "x-type/list.h"
#include "x-type/prim.h"
#include "x-type/symbol.h"

/*
 * # Helper
 */
static x_obj_t *x_prim_eval_arg(x_obj_t *p_base, x_obj_t *p_arg)
{
	x_satom_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_arg });
	x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { wrap }, { NULL });

	return x_eval(p_base, (x_obj_t *)args);
}

/*
 * # Primitives
 *
 * All primitives receive unevaluated arguments (fexpr-style).
 * Each primitive evals what it needs.
 */

/* quote: (quote x) -> x, unevaluated */
static x_obj_t *x_prim_quote(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_firstobj(p_args);
}

/* cons: (cons a b) -> (a . b) */
static x_obj_t *x_prim_cons(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return x_mklist(p_base, a, b);
}

/* car: (car x) -> first element */
static x_obj_t *x_prim_car(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *x = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_firstobj(x);
}

/* cdr: (cdr x) -> rest */
static x_obj_t *x_prim_cdr(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *x = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_restobj(x);
}

/* eq?: (eq? a b) -> pointer equality */
static x_obj_t *x_prim_eq(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return a == b ? a : p_base;
}

/* +: (+ a b) -> integer addition */
static x_obj_t *x_prim_sum(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return x_mkint(p_base, x_intval(a) + x_intval(b));
}

/* -: (- a b) -> integer subtraction */
static x_obj_t *x_prim_sub(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return x_mkint(p_base, x_intval(a) - x_intval(b));
}

/* *: (* a b) -> integer multiplication */
static x_obj_t *x_prim_prod(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return x_mkint(p_base, x_intval(a) * x_intval(b));
}

/* define: (define name value) -> bind name to eval'd value */
static x_obj_t *x_prim_define(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_name = x_firstobj(p_args),
		*p_val = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args))),
		*p_pair = x_mkspair(p_base, p_name, p_val);

	x_base_env_alist_extend(p_base, p_pair);

	return p_val;
}

/* if: (if cond then else) -> conditional evaluation */
static x_obj_t *x_prim_if(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_cond = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_rest = x_restobj(p_args);

	if (x_obj_isnil(p_base, p_cond)) {
		return x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_rest)));
	}

	return x_prim_eval_arg(p_base, x_firstobj(p_rest));
}

/*
 * # Registration
 */
static void x_prim_bind(x_obj_t *p_base, x_char_t *name, x_prim_fn fn)
{
	x_obj_t *p_sym = x_make_symbol(p_base, X_OBJ_FLAG_NONE, name),
		*p_prim = x_mkprim(p_base, fn),
		*p_pair = x_mkspair(p_base, p_sym, p_prim);

	x_base_env_alist_extend(p_base, p_pair);
}

x_obj_t *x_prim_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_prim_bind(p_base, "quote", x_prim_quote);
	x_prim_bind(p_base, "cons", x_prim_cons);
	x_prim_bind(p_base, "car", x_prim_car);
	x_prim_bind(p_base, "cdr", x_prim_cdr);
	x_prim_bind(p_base, "eq?", x_prim_eq);
	x_prim_bind(p_base, "+", x_prim_sum);
	x_prim_bind(p_base, "-", x_prim_sub);
	x_prim_bind(p_base, "*", x_prim_prod);
	x_prim_bind(p_base, "define", x_prim_define);
	x_prim_bind(p_base, "if", x_prim_if);

	return p_base;
}
