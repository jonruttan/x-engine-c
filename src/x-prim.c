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
#include "x-alist.h"
#include "x-base.h"
#include "x-eval.h"
#include "x-exp/quote.h"
#include "x-type/int.h"
#include "x-type/list.h"
#include "x-type/prim.h"
#include "x-type/procedure.h"
#include "x-type/symbol.h"

/*
 * # Helpers
 */
x_obj_t *x_prim_eval_arg(x_obj_t *p_base, x_obj_t *p_arg)
{
	x_satom_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_arg });
	x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { wrap }, { NULL });

	return x_eval(p_base, (x_obj_t *)args);
}

x_obj_t *x_prim_evlis(x_obj_t *p_base, x_obj_t *p_args)
{
	if (x_obj_isnil(p_base, p_args)) {
		return p_base;
	}

	return x_mkspair(p_base,
		x_prim_eval_arg(p_base, x_firstobj(p_args)),
		x_prim_evlis(p_base, x_restobj(p_args)));
}

x_obj_t *x_prim_multiple_extend(x_obj_t *p_base, x_obj_t *p_env,
	x_obj_t *p_params, x_obj_t *p_vals)
{
	/* Variadic: single symbol binds to entire remaining arg list. */
	if ( ! x_obj_isnil(p_base, p_params)
		&& x_obj_type_issymbol(p_base, p_params)) {
		return x_mkspair(p_base,
			x_mkspair(p_base, p_params, p_vals), p_env);
	}

	/* Base case: no more params. */
	if (x_obj_isnil(p_base, p_params)) {
		return p_env;
	}

	/* Recursive case: bind first param to first val, continue. */
	return x_prim_multiple_extend(p_base,
		x_mkspair(p_base,
			x_mkspair(p_base, x_firstobj(p_params), x_firstobj(p_vals)),
			p_env),
		x_restobj(p_params),
		x_restobj(p_vals));
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

/* fn: (fn (params) body...) -> create closure */
static x_obj_t *x_prim_closure(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_params = x_firstobj(p_args),
		*p_body = x_restobj(p_args),
		*p_env = x_base_field_env_alist(p_base);

	return x_mkproc(p_base, p_params, p_body, p_env);
}

/* do: (do form...) -> evaluate each form, return last */
static x_obj_t *x_prim_do(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_result = p_base;

	while ( ! x_obj_isnil(p_base, p_args)) {
		p_result = x_prim_eval_arg(p_base, x_firstobj(p_args));
		p_args = x_restobj(p_args);
	}

	return p_result;
}

/* set: (set name value) -> mutate existing binding */
static x_obj_t *x_prim_set(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_name = x_firstobj(p_args),
		*p_val = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));
	x_spair_t assoc_args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_name }, { (x_obj_t *)(assoc_args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL })
	};
	x_obj_t *p_pair;

	x_firstobj((x_obj_t *)assoc_args[1]) = x_base_field_env_alist(p_base);
	p_pair = x_alist_assoc(p_base, (x_obj_t *)assoc_args);

	if (x_obj_isnil(p_base, p_pair)) {
		x_obj_error(p_base, "Unbound "X_TYPE_SYMBOL_NAME, x_symbolval(p_name));

		return p_base;
	}

	x_restobj(p_pair) = p_val;

	return p_val;
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
	x_prim_bind(p_base, "def", x_prim_define);
	x_prim_bind(p_base, "if", x_prim_if);
	x_prim_bind(p_base, "fn", x_prim_closure);
	x_prim_bind(p_base, "do", x_prim_do);
	x_prim_bind(p_base, "set", x_prim_set);

	return p_base;
}
