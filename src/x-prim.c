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
#include "x-sexp.h"
#include "x-type/buffer.h"
#include "x-type/int.h"
#include "x-type/list.h"
#include "x-type/prim.h"
#include "x-type/operative.h"
#include "x-type/procedure.h"
#include "x-type/ptr.h"
#include "x-type/str.h"
#include "x-type/symbol.h"
#include "x-type/char.h"
#include "x-type/whitespace.h"
#include "x-type/comment.h"

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

	return x_mklist(p_base,
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

/* /: (/ a b) -> integer division */
static x_obj_t *x_prim_div(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return x_mkint(p_base, x_intval(a) / x_intval(b));
}

/* %: (% a b) -> integer modulo */
static x_obj_t *x_prim_mod(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return x_mkint(p_base, x_intval(a) % x_intval(b));
}

/* def: (def name value) -> bind name to eval'd value (supports recursion) */
static x_obj_t *x_prim_define(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_name = x_firstobj(p_args),
		*p_pair = x_mkspair(p_base, p_name, p_base),
		*p_val;

	x_base_env_alist_extend(p_base, p_pair);
	p_val = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));
	x_restobj(p_pair) = p_val;

	return p_val;
}

/* if: (if cond then [else]) -> conditional evaluation */
static x_obj_t *x_prim_if(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_cond = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_rest = x_restobj(p_args);

	if (x_obj_isnil(p_base, p_cond)) {
		x_obj_t *p_else = x_restobj(p_rest);

		if (x_obj_isnil(p_base, p_else)) {
			return p_base;
		}

		x_base_field_tco_expr(p_base) = x_firstobj(p_else);

		return p_base;
	}

	x_base_field_tco_expr(p_base) = x_firstobj(p_rest);

	return p_base;
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

/* list: (list a b c) -> (a b c) */
static x_obj_t *x_prim_list(x_obj_t *p_base, x_obj_t *p_args)
{
	if (x_obj_isnil(p_base, p_args)) {
		return p_base;
	}

	return x_mklist(p_base,
		x_prim_eval_arg(p_base, x_firstobj(p_args)),
		x_prim_list(p_base, x_restobj(p_args)));
}

/* and: (and expr...) -> short-circuit, returns last truthy or nil */
static x_obj_t *x_prim_and(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_result = x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE);

	while ( ! x_obj_isnil(p_base, p_args)) {
		p_result = x_prim_eval_arg(p_base, x_firstobj(p_args));
		if (x_obj_isnil(p_base, p_result)) {
			return p_base;
		}
		p_args = x_restobj(p_args);
	}

	return p_result;
}

/* or: (or expr...) -> short-circuit, returns first truthy or nil */
static x_obj_t *x_prim_or(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_result = p_base;

	while ( ! x_obj_isnil(p_base, p_args)) {
		p_result = x_prim_eval_arg(p_base, x_firstobj(p_args));
		if ( ! x_obj_isnil(p_base, p_result)) {
			return p_result;
		}
		p_args = x_restobj(p_args);
	}

	return p_base;
}

/* cond: (cond (test expr)...) -> multi-branch conditional */
static x_obj_t *x_prim_cond(x_obj_t *p_base, x_obj_t *p_args)
{
	while ( ! x_obj_isnil(p_base, p_args)) {
		x_obj_t *p_clause = x_firstobj(p_args),
			*p_test = x_prim_eval_arg(p_base, x_firstobj(p_clause));

		if ( ! x_obj_isnil(p_base, p_test)) {
			x_base_field_tco_expr(p_base) = x_firstobj(x_restobj(p_clause));

			return p_base;
		}

		p_args = x_restobj(p_args);
	}

	return p_base;
}

/* let: (let ((name val)...) body...) -> local bindings */
static x_obj_t *x_prim_let(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_bindings = x_firstobj(p_args),
		*p_body = x_restobj(p_args),
		*p_params = p_base,
		*p_vals = p_base,
		*p_b;
	x_obj_t *p_saved_env = x_base_field_env_alist(p_base),
		*p_result = p_base;

	/* Walk bindings to extract params and eval'd values. */
	for (p_b = p_bindings; ! x_obj_isnil(p_base, p_b); p_b = x_restobj(p_b)) {
		x_obj_t *p_binding = x_firstobj(p_b);
		p_params = x_mkspair(p_base, x_firstobj(p_binding), p_params);
		p_vals = x_mkspair(p_base,
			x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_binding))),
			p_vals);
	}

	/* Extend current env with bindings, eval body, restore. */
	x_base_field_env_alist(p_base) = x_prim_multiple_extend(
		p_base, p_saved_env, p_params, p_vals);

	while ( ! x_obj_isnil(p_base, p_body)) {
		if (x_obj_isnil(p_base, x_restobj(p_body))) {
			x_base_field_tco_expr(p_base) = x_firstobj(p_body);

			if (x_obj_isnil(p_base, x_base_field_tco_env(p_base))) {
				x_base_field_tco_env(p_base) = p_saved_env;
			}

			return p_base;
		}

		p_result = x_prim_eval_arg(p_base, x_firstobj(p_body));
		p_body = x_restobj(p_body);
	}

	x_base_field_env_alist(p_base) = p_saved_env;

	return p_result;
}

/* apply: (apply f args) -> call callable with pre-evaluated arg list */
static x_obj_t *x_prim_apply(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_fn = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_vals = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	/* C primitive: call directly with evaluated args. */
	if (x_obj_type_isprim(p_base, p_fn)) {
		return (*x_primval(p_fn))(p_base, p_vals);
	}

	/* Procedure (closure): bind params, evaluate body. */
	{
		x_obj_t *p_params = x_procparams(p_fn),
			*p_body = x_procbody(p_fn),
			*p_closure_env = x_procenv(p_fn),
			*p_saved_env = x_base_field_env_alist(p_base),
			*p_result = p_base;

		x_base_field_env_alist(p_base) = x_prim_multiple_extend(
			p_base, p_closure_env, p_params, p_vals);

		while ( ! x_obj_isnil(p_base, p_body)) {
			if (x_obj_isnil(p_base, x_restobj(p_body))) {
				x_base_field_tco_expr(p_base) = x_firstobj(p_body);

				if (x_obj_isnil(p_base, x_base_field_tco_env(p_base))) {
					x_base_field_tco_env(p_base) = p_saved_env;
				}

				return p_base;
			}

			p_result = x_prim_eval_arg(p_base, x_firstobj(p_body));
			p_body = x_restobj(p_body);
		}

		x_base_field_env_alist(p_base) = p_saved_env;

		return p_result;
	}
}

/* eval: (eval expr [env]) -> evaluate expression, optionally in given env */
static x_obj_t *x_prim_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_expr = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_env_arg = x_restobj(p_args);

	if ( ! x_obj_isnil(p_base, p_env_arg)) {
		x_obj_t *p_env = x_prim_eval_arg(p_base, x_firstobj(p_env_arg)),
			*p_saved_env = x_base_field_env_alist(p_base),
			*p_result;

		x_base_field_env_alist(p_base) = p_env;
		p_result = x_prim_eval_arg(p_base, p_expr);
		x_base_field_env_alist(p_base) = p_saved_env;

		return p_result;
	}

	return x_prim_eval_arg(p_base, p_expr);
}

/* fn: (fn (params) body...) -> create closure */
static x_obj_t *x_prim_closure(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_params = x_firstobj(p_args),
		*p_body = x_restobj(p_args),
		*p_env = x_base_field_env_alist(p_base);

	return x_mkproc(p_base, p_params, p_body, p_env);
}

/* op: (op formals env-param body...) -> create operative (user-level fexpr) */
static x_obj_t *x_prim_operative(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_params = x_firstobj(p_args),
		*p_envparam = x_firstobj(x_restobj(p_args)),
		*p_body = x_restobj(x_restobj(p_args)),
		*p_env = x_base_field_env_alist(p_base);

	return x_mkop(p_base, p_params, p_envparam, p_body, p_env);
}

/* wrap: (wrap combiner) -> create applicative from combiner */
static x_obj_t *x_prim_wrap(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_combiner = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_mkwrap(p_base, p_combiner);
}

/* unwrap: (unwrap applicative) -> extract underlying combiner */
static x_obj_t *x_prim_unwrap(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_applicative = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_procenv(p_applicative);
}

/* write: (write obj) -> output s-expression to stdout */
static x_obj_t *x_prim_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_val = x_prim_eval_arg(p_base, x_firstobj(p_args));
	x_spair_t write_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_val }, { NULL })
	};

	x_sexp_write(p_base, (x_obj_t *)write_args);

	return p_base;
}

/* display: (display obj) -> output human-readable (strings unquoted) */
static x_obj_t *x_prim_display(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_val = x_prim_eval_arg(p_base, x_firstobj(p_args));

	if (x_obj_type_isstr(p_base, p_val)) {
		int fd = x_base_isset(p_base)
			? x_atomint(x_base_field_fileout(p_base)) : STDOUT_FILENO;
		x_char_t *s = x_strval(p_val);

		x_sys_write(fd, s, x_lib_strlen(s));
	} else {
		x_spair_t write_args[1] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_val }, { NULL })
		};

		x_sexp_write(p_base, (x_obj_t *)write_args);
	}

	return p_base;
}

/* newline: (newline) -> output newline character */
static x_obj_t *x_prim_newline(x_obj_t *p_base, x_obj_t *p_args)
{
	int fd = x_base_isset(p_base)
		? x_atomint(x_base_field_fileout(p_base)) : STDOUT_FILENO;

	x_sys_write(fd, "\n", 1);

	return p_base;
}

/* read: (read) -> read one s-expression from stdin */
static x_obj_t *x_prim_read_expr(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_base_field_buffer(p_base);
	x_spair_t read_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_buffer }, { p_base })
	};

	return x_sexp_read(p_base, (x_obj_t *)read_args);
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

/* qq_append: append list a to list b (for unquote-splicing) */
static x_obj_t *x_prim_qq_append(x_obj_t *p_base, x_obj_t *p_a, x_obj_t *p_b)
{
	if (x_obj_isnil(p_base, p_a)) {
		return p_b;
	}

	return x_mklist(p_base,
		x_firstobj(p_a),
		x_prim_qq_append(p_base, x_restobj(p_a), p_b));
}

/* qq_expand: recursively process quasiquote template */
static x_obj_t *x_prim_qq_expand(x_obj_t *p_base, x_obj_t *p_tmpl)
{
	x_obj_t *p_car;

	/* Atom or nil -> return as-is. */
	if (x_obj_isnil(p_base, p_tmpl)
		|| ! x_obj_type_islist(p_base, p_tmpl)) {
		return p_tmpl;
	}

	p_car = x_firstobj(p_tmpl);

	/* (unquote x) -> eval x */
	if (x_obj_type_issymbol(p_base, p_car)
		&& 0 == x_lib_strcmp(x_symbolval(p_car), "unquote")) {
		return x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_tmpl)));
	}

	/* ((unquote-splicing x) . rest) -> append (eval x) to expanded rest */
	if (x_obj_type_islist(p_base, p_car)
		&& x_obj_type_issymbol(p_base, x_firstobj(p_car))
		&& 0 == x_lib_strcmp(x_symbolval(x_firstobj(p_car)),
			"unquote-splicing")) {
		x_obj_t *p_spliced = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(p_car)));
		x_obj_t *p_rest = x_prim_qq_expand(p_base, x_restobj(p_tmpl));

		return x_prim_qq_append(p_base, p_spliced, p_rest);
	}

	/* Recursive: expand car and cdr */
	return x_mklist(p_base,
		x_prim_qq_expand(p_base, p_car),
		x_prim_qq_expand(p_base, x_restobj(p_tmpl)));
}

/* quasi: (quasi tmpl) -> template with unquote/unquote-splicing */
static x_obj_t *x_prim_quasiquote(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_prim_qq_expand(p_base, x_firstobj(p_args));
}

/* do: (do form...) -> evaluate each form, return last */
static x_obj_t *x_prim_do(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_result = p_base;

	while ( ! x_obj_isnil(p_base, p_args)) {
		if (x_obj_isnil(p_base, x_restobj(p_args))) {
			x_base_field_tco_expr(p_base) = x_firstobj(p_args);

			return p_base;
		}

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

/* guard: (guard (var handler-body...) body...) -> error recovery */
static x_obj_t *x_prim_guard(x_obj_t *p_base, x_obj_t *p_args)
{
	x_error_handler_t handler;
	x_obj_t *p_clause = x_firstobj(p_args),
		*p_var = x_firstobj(p_clause),
		*p_handler_body = x_restobj(p_clause),
		*p_body = x_restobj(p_args),
		*p_prev_handler = x_base_field_error_handler(p_base),
		*p_result = p_base;

	/* Save env and push handler. */
	handler.p_error = p_base;
	handler.error_msg = NULL;
	handler.p_saved_env = x_base_field_env_alist(p_base);
	handler.prev = x_obj_isnil(p_base, p_prev_handler)
		? NULL : (x_error_handler_t *)x_ptrval(p_prev_handler);
	x_base_field_error_handler(p_base) = x_mkptr(p_base, &handler);

	if (setjmp(handler.jmp) == 0) {
		/* Normal execution: evaluate body. */
		while ( ! x_obj_isnil(p_base, p_body)) {
			p_result = x_prim_eval_arg(p_base, x_firstobj(p_body));
			p_body = x_restobj(p_body);
		}
	} else {
		/* Error caught: bind error to var, run handler body. */
		x_obj_t *p_err = x_obj_isnil(p_base, handler.p_error)
			? x_mkstr(p_base, handler.error_msg) : handler.p_error;
		x_obj_t *p_pair = x_mkspair(p_base, p_var, p_err);

		x_base_env_alist_extend(p_base, p_pair);

		while ( ! x_obj_isnil(p_base, p_handler_body)) {
			p_result = x_prim_eval_arg(p_base,
				x_firstobj(p_handler_body));
			p_handler_body = x_restobj(p_handler_body);
		}

		x_base_field_env_alist(p_base) = handler.p_saved_env;
	}

	/* Pop handler. */
	x_base_field_error_handler(p_base) = handler.prev
		? x_mkptr(p_base, handler.prev) : p_base;

	return p_result;
}

/* error: (error message) -> signal an error */
static x_obj_t *x_prim_error(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_msg = x_prim_eval_arg(p_base, x_firstobj(p_args));

	/* If handler installed, use it. */
	if ( ! x_obj_isnil(p_base, x_base_field_error_handler(p_base))) {
		x_error_handler_t *handler =
			(x_error_handler_t *)x_ptrval(x_base_field_error_handler(p_base));

		handler->p_error = p_msg;
		x_base_field_env_alist(p_base) = handler->p_saved_env;
		longjmp(handler->jmp, 1);
	}

	/* No handler: fall through to fatal error. */
	if (x_obj_type_isstr(p_base, p_msg)) {
		x_obj_error(p_base, x_strval(p_msg), NULL);
	} else {
		x_obj_error(p_base, "error", NULL);
	}

	return p_base;
}

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

/* make-type: (make-type name handlers-alist) -> create runtime type */
static x_obj_t *x_prim_make_type(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_name_str = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_handlers = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));
	x_char_t *name = x_lib_strndup(x_strval(p_name_str),
		x_lib_strlen(x_strval(p_name_str)));
	x_obj_t *p_name_atom = x_obj_make(p_base, x_type_atom_obj,
		X_OBJ_FLAG_OWN, X_OBJ_LENGTH_ATOM, name);
	struct x_type_t type = { 0 };
	x_obj_t *p_sym, *p_entry, *p_type;
	x_spair_t assoc_args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { (x_obj_t *)(assoc_args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_handlers }, { NULL })
	};

	type.p_name = p_name_atom;

	/* Look up handler closures from the alist. */
	p_sym = x_mksymbol(p_base, (x_char_t *)"call");
	x_firstobj((x_obj_t *)assoc_args) = p_sym;
	p_entry = x_alist_assoc(p_base, (x_obj_t *)assoc_args);
	if ( ! x_obj_isnil(p_base, p_entry)) {
		type.p_call = x_restobj(p_entry);
	}

	p_sym = x_mksymbol(p_base, (x_char_t *)"write");
	x_firstobj((x_obj_t *)assoc_args) = p_sym;
	p_entry = x_alist_assoc(p_base, (x_obj_t *)assoc_args);
	if ( ! x_obj_isnil(p_base, p_entry)) {
		type.p_write = x_restobj(p_entry);
	}

	p_sym = x_mksymbol(p_base, (x_char_t *)"length");
	x_firstobj((x_obj_t *)assoc_args) = p_sym;
	p_entry = x_alist_assoc(p_base, (x_obj_t *)assoc_args);
	if ( ! x_obj_isnil(p_base, p_entry)) {
		type.p_length = x_restobj(p_entry);
	}

	p_type = x_type_struct_make(p_base, type);
	x_base_type_alist_extend(p_base, p_type);

	return p_name_atom;
}

/* make-instance: (make-instance type-handle data) -> create typed instance */
static x_obj_t *x_prim_make_instance(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_handle = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_data = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));
	x_spair_t lookup_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_handle }, { NULL })
	};
	x_obj_t *p_type = x_base_type_alist_assoc(p_base, (x_obj_t *)lookup_args);

	if (x_obj_isnil(p_base, p_type)) {
		return p_base;
	}

	return x_obj_make(p_base, p_type, 0, X_OBJ_LENGTH_PAIR, p_data, p_base);
}

/* type?: (type? obj type-handle) -> t if obj's type matches handle */
static x_obj_t *x_prim_typep(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_handle = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	if (x_obj_isnil(p_base, p_obj) || x_obj_isnil(p_base, x_obj_type(p_obj))) {
		return p_base;
	}

	return x_type_field_name(x_obj_type(p_obj)) == p_handle
		? x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE) : p_base;
}

/* type-name: (type-name obj) -> name string of obj's type */
static x_obj_t *x_prim_type_name(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_name;

	if (x_obj_isnil(p_base, p_obj) || x_obj_isnil(p_base, x_obj_type(p_obj))) {
		return p_base;
	}

	p_name = x_type_field_name(x_obj_type(p_obj));

	if (x_obj_isnil(p_base, p_name)) {
		return p_base;
	}

	return x_mkstr(p_base, x_atomstr(p_name));
}

/* make-base: (make-base) -> create fresh sandboxed interpreter */
static x_obj_t *x_prim_make_base(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_new_base, *p_buffer;
	x_char_t *buffer = (x_char_t *)x_sys_malloc(256);

	p_new_base = x_base_make(NULL, NULL);

	/* Register types. */
	x_type_prim_register(p_new_base, p_new_base);
	x_type_operative_register(p_new_base, p_new_base);
	x_type_procedure_register(p_new_base, p_new_base);
	x_type_symbol_register(p_new_base, p_new_base);
	x_type_list_register(p_new_base, p_new_base);
	x_type_int_register(p_new_base, p_new_base);
	x_type_str_register(p_new_base, p_new_base);
	x_type_char_register(p_new_base, p_new_base);
	x_type_whitespace_register(p_new_base, p_new_base);
	x_type_comment_register(p_new_base, p_new_base);

	/* Set up read buffer. */
	p_buffer = x_mkbuffer(p_new_base, buffer);
	x_base_field_buffer(p_new_base) = p_buffer;

	/* Register primitives. */
	x_prim_register(p_new_base, p_new_base);

	return p_new_base;
}

/* Rebuild expression tree replacing source nil with target nil. */
static x_obj_t *x_prim_renil(x_obj_t *p_src, x_obj_t *p_dst,
	x_obj_t *p_exp)
{
	if (x_obj_isnil(p_src, p_exp)) {
		return p_dst;
	}

	if (x_obj_type_islist(p_src, p_exp)) {
		return x_mklist(p_dst,
			x_prim_renil(p_src, p_dst, x_firstobj(p_exp)),
			x_prim_renil(p_src, p_dst, x_restobj(p_exp)));
	}

	return p_exp;
}

/* base-eval: (base-eval base expr) -> eval expr in target base */
static x_obj_t *x_prim_base_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	x_error_handler_t handler;
	x_obj_t *p_target = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_expr = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args))),
		*p_prev_handler = x_base_field_error_handler(p_target),
		*p_result;

	/* Re-nil: replace parent nil terminators with target nil. */
	p_expr = x_prim_renil(p_base, p_target, p_expr);

	/* Install bridge handler in target to propagate errors to parent. */
	handler.p_error = p_target;
	handler.error_msg = NULL;
	handler.p_saved_env = x_base_field_env_alist(p_target);
	handler.prev = x_obj_isnil(p_target, p_prev_handler)
		? NULL : (x_error_handler_t *)x_ptrval(p_prev_handler);
	x_base_field_error_handler(p_target) = x_mkptr(p_target, &handler);

	if (setjmp(handler.jmp) == 0) {
		p_result = x_prim_eval_arg(p_target, p_expr);
	} else {
		/* Error caught from target: restore and re-signal in parent. */
		x_base_field_error_handler(p_target) = handler.prev
			? x_mkptr(p_target, handler.prev) : p_target;
		x_base_field_env_alist(p_target) = handler.p_saved_env;

		if ( ! x_obj_isnil(p_base, x_base_field_error_handler(p_base))) {
			x_error_handler_t *p_parent =
				(x_error_handler_t *)x_ptrval(
					x_base_field_error_handler(p_base));

			p_parent->p_error = handler.p_error;
			p_parent->error_msg = handler.error_msg;
			x_base_field_env_alist(p_base) = p_parent->p_saved_env;
			longjmp(p_parent->jmp, 1);
		}

		x_obj_error(p_base, "error", NULL);

		return p_base;
	}

	/* Pop handler. */
	x_base_field_error_handler(p_target) = handler.prev
		? x_mkptr(p_target, handler.prev) : p_target;

	return p_result;
}

/* base-bind: (base-bind base name value) -> bind in target base */
static x_obj_t *x_prim_base_bind(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_target = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_name = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args))),
		*p_val = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(x_restobj(p_args)))),
		*p_pair;

	/* Re-nil list values for cross-base compatibility. */
	p_val = x_prim_renil(p_base, p_target, p_val);
	p_pair = x_mkspair(p_target, p_name, p_val);

	x_base_env_alist_extend(p_target, p_pair);

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
	/* Bind t as a self-evaluating truth symbol. */
	{
		x_obj_t *p_t = x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE),
			*p_pair = x_mkspair(p_base, p_t, p_t);
		x_base_env_alist_extend(p_base, p_pair);
	}

	x_prim_bind(p_base, "lit", x_prim_quote);
	x_prim_bind(p_base, "pair", x_prim_cons);
	x_prim_bind(p_base, "first", x_prim_car);
	x_prim_bind(p_base, "rest", x_prim_cdr);
	x_prim_bind(p_base, "eq?", x_prim_eq);
	x_prim_bind(p_base, "=", x_prim_numeq);
	x_prim_bind(p_base, "+", x_prim_sum);
	x_prim_bind(p_base, "-", x_prim_sub);
	x_prim_bind(p_base, "*", x_prim_prod);
	x_prim_bind(p_base, "def", x_prim_define);
	x_prim_bind(p_base, "if", x_prim_if);
	x_prim_bind(p_base, "fn", x_prim_closure);
	x_prim_bind(p_base, "do", x_prim_do);
	x_prim_bind(p_base, "set", x_prim_set);
	x_prim_bind(p_base, "null?", x_prim_nullp);
	x_prim_bind(p_base, "pair?", x_prim_pairp);
	x_prim_bind(p_base, "atom?", x_prim_atomp);
	x_prim_bind(p_base, "not", x_prim_not);
	x_prim_bind(p_base, "list", x_prim_list);
	x_prim_bind(p_base, "<", x_prim_lt);
	x_prim_bind(p_base, ">", x_prim_gt);
	x_prim_bind(p_base, "<=", x_prim_lte);
	x_prim_bind(p_base, ">=", x_prim_gte);
	x_prim_bind(p_base, "and", x_prim_and);
	x_prim_bind(p_base, "or", x_prim_or);
	x_prim_bind(p_base, "match", x_prim_cond);
	x_prim_bind(p_base, "let", x_prim_let);
	x_prim_bind(p_base, "/", x_prim_div);
	x_prim_bind(p_base, "%", x_prim_mod);
	x_prim_bind(p_base, "apply", x_prim_apply);
	x_prim_bind(p_base, "eval", x_prim_eval);
	x_prim_bind(p_base, "op", x_prim_operative);
	x_prim_bind(p_base, "write", x_prim_write);
	x_prim_bind(p_base, "display", x_prim_display);
	x_prim_bind(p_base, "newline", x_prim_newline);
	x_prim_bind(p_base, "read", x_prim_read_expr);
	x_prim_bind(p_base, "string->symbol", x_prim_string_to_symbol);
	x_prim_bind(p_base, "symbol->string", x_prim_symbol_to_string);
	x_prim_bind(p_base, "number?", x_prim_numberp);
	x_prim_bind(p_base, "string?", x_prim_stringp);
	x_prim_bind(p_base, "symbol?", x_prim_symbolp);
	x_prim_bind(p_base, "procedure?", x_prim_procedurep);
	x_prim_bind(p_base, "wrap", x_prim_wrap);
	x_prim_bind(p_base, "unwrap", x_prim_unwrap);
	x_prim_bind(p_base, "quasi", x_prim_quasiquote);
	x_prim_bind(p_base, "guard", x_prim_guard);
	x_prim_bind(p_base, "error", x_prim_error);
	x_prim_bind(p_base, "make-base", x_prim_make_base);
	x_prim_bind(p_base, "base-eval", x_prim_base_eval);
	x_prim_bind(p_base, "base-bind", x_prim_base_bind);
	x_prim_bind(p_base, "string-length", x_prim_string_length);
	x_prim_bind(p_base, "string-ref", x_prim_string_ref);
	x_prim_bind(p_base, "string-append", x_prim_string_append);
	x_prim_bind(p_base, "substring", x_prim_substring);
	x_prim_bind(p_base, "string=?", x_prim_string_eq);
	x_prim_bind(p_base, "number->string", x_prim_number_to_string);
	x_prim_bind(p_base, "string->number", x_prim_string_to_number);
	x_prim_bind(p_base, "make-type", x_prim_make_type);
	x_prim_bind(p_base, "make-instance", x_prim_make_instance);
	x_prim_bind(p_base, "type?", x_prim_typep);
	x_prim_bind(p_base, "type-name", x_prim_type_name);

	return p_base;
}
