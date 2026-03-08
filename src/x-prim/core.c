/*
 * # Computational Expressions in C
 *
 * ## x-prim/core.c -- Implementation - Primitives - Core Language Forms
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
#include "x-type/list.h"
#include "x-type/operative.h"
#include "x-type/prim.h"
#include "x-type/procedure.h"
#include "x-type/ptr.h"
#include "x-type/str.h"
#include "x-type/symbol.h"

/* quote: (quote x) -> x, unevaluated */
static x_obj_t *x_prim_quote(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_firstobj(p_args);
}

/* pair: (pair a b) -> (a . b) */
static x_obj_t *x_prim_pair(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return x_mklist(p_base, a, b);
}

/* first: (first x) -> first element */
static x_obj_t *x_prim_first(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *x = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_firstobj(x);
}

/* rest: (rest x) -> rest */
static x_obj_t *x_prim_rest(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *x = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_restobj(x);
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
		/* eval with env: save/restore for correct non-tail semantics */
		x_obj_t *p_env = x_prim_eval_arg(p_base, x_firstobj(p_env_arg));
		x_obj_t *p_saved = x_base_field_env_alist(p_base);
		x_obj_t *p_result;

		x_base_field_env_alist(p_base) = p_env;
		p_result = x_prim_eval_arg(p_base, p_expr);
		x_base_field_env_alist(p_base) = p_saved;
		return p_result;
	}

	/* eval without env: use TCO trampoline */
	x_base_field_tco_expr(p_base) = p_expr;

	return p_base;
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

/* %rewrite: (%rewrite pair new-first new-rest) -> mutate pair in-place */
static x_obj_t *x_prim_rewrite(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_pair = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_first = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(p_args))),
		*p_rest = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(x_restobj(p_args))));

	x_firstobj(p_pair) = p_first;
	x_restobj(p_pair) = p_rest;

	return p_pair;
}

x_obj_t *x_prim_core_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_prim_bind(p_base, "lit", x_prim_quote);
	x_prim_bind(p_base, "pair", x_prim_pair);
	x_prim_bind(p_base, "first", x_prim_first);
	x_prim_bind(p_base, "rest", x_prim_rest);
	x_prim_bind(p_base, "def", x_prim_define);
	x_prim_bind(p_base, "if", x_prim_if);
	x_prim_bind(p_base, "do", x_prim_do);
	x_prim_bind(p_base, "set", x_prim_set);
	x_prim_bind(p_base, "let", x_prim_let);
	x_prim_bind(p_base, "apply", x_prim_apply);
	x_prim_bind(p_base, "eval", x_prim_eval);
	x_prim_bind(p_base, "fn", x_prim_closure);
	x_prim_bind(p_base, "op", x_prim_operative);
	x_prim_bind(p_base, "wrap", x_prim_wrap);
	x_prim_bind(p_base, "unwrap", x_prim_unwrap);
	x_prim_bind(p_base, "guard", x_prim_guard);
	x_prim_bind(p_base, "error", x_prim_error);
	x_prim_bind(p_base, "%rewrite", x_prim_rewrite);

	return p_base;
}
