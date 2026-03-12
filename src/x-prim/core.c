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
#include <setjmp.h>
#include "x-type/int.h"
#include "x-type/list.h"
#include "x-type/operative.h"
#include "x-type/prim.h"
#include "x-type/procedure.h"
#include "x-type/ptr.h"
#include "x-type/str.h"
#include "x-type/symbol.h"

/* quote: (quote x) -> x, unevaluated */
x_obj_t *x_prim_quote(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_firstobj(p_args);
}

/* pair: (pair a b) -> (a . b) */
x_obj_t *x_prim_pair(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	return x_mklist(p_base, a, b);
}

/* first: (first x) -> first element */
x_obj_t *x_prim_first(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *x = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_firstobj(x);
}

/* rest: (rest x) -> rest */
x_obj_t *x_prim_rest(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *x = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_restobj(x);
}

/* def: (def name value) -> bind name to eval'd value (supports recursion) */
x_obj_t *x_prim_define(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_name = x_firstobj(p_args),
		*p_pair = x_mkspair(p_base, p_name, NULL),
		*p_val;

	x_base_env_alist_extend(p_base, p_pair);
	p_val = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));
	x_restobj(p_pair) = p_val;

	return p_val;
}

/* set: (set name value) -> mutate existing binding */
x_obj_t *x_prim_set(x_obj_t *p_base, x_obj_t *p_args)
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
		x_obj_error(p_base, "Unbound "X_TYPE_SYMBOL_NAME, p_name);

		return NULL;
	}

	x_restobj(p_pair) = p_val;

	return p_val;
}

/* apply: (apply f arg1 ... args) -> call callable with prefix + tail arg list */
x_obj_t *x_prim_apply(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_fn = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_evaled = x_prim_evlis(p_base, x_restobj(p_args)),
		*p_vals, *p_walk, *p_tail;

	/* Build combined arg list: prefix args prepended to tail list.
	 * (apply f a b '(c d)) -> p_evaled = (a b (c d))
	 * Single arg (backward compat): p_evaled = ((c d)) -> p_vals = (c d) */
	if (x_obj_isnil(p_base, x_restobj(p_evaled))) {
		p_vals = x_firstobj(p_evaled);
	} else {
		/* Walk to the last element (tail list), then prepend prefix
		 * args in reverse. Find the second-to-last and last. */
		p_walk = p_evaled;
		while ( ! x_obj_isnil(p_base,
			x_restobj(x_restobj(p_walk)))) {
			p_walk = x_restobj(p_walk);
		}
		/* p_walk->first is second-to-last, p_walk->rest->first is
		 * last (the tail list). */
		p_tail = x_firstobj(x_restobj(p_walk));
		/* Prepend prefix args from back to front. */
		p_tail = x_mklist(p_base, x_firstobj(p_walk), p_tail);
		p_walk = p_evaled;
		/* Collect prefix args before the second-to-last. */
		{
			x_obj_t *p_prefix = NULL, *p_cur;

			while ( ! x_obj_isnil(p_base,
				x_restobj(x_restobj(p_walk)))) {
				p_prefix = x_mklist(p_base,
					x_firstobj(p_walk), p_prefix);
				p_walk = x_restobj(p_walk);
			}
			/* Prepend collected prefix (reversed) to tail. */
			p_cur = p_prefix;
			while ( ! x_obj_isnil(p_base, p_cur)) {
				p_tail = x_mklist(p_base,
					x_firstobj(p_cur), p_tail);
				p_cur = x_restobj(p_cur);
			}
		}
		p_vals = p_tail;
	}

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
			*p_result = NULL;

		x_base_field_env_alist(p_base) = x_prim_multiple_extend(
			p_base, p_closure_env, p_params, p_vals);

		while ( ! x_obj_isnil(p_base, p_body)) {
			if (x_obj_isnil(p_base, x_restobj(p_body))) {
				x_base_field_tco_expr(p_base) = x_firstobj(p_body);

				if (x_obj_isnil(p_base,
					x_base_field_tco_expr(p_base))) {
					x_base_field_env_alist(p_base) =
						p_saved_env;
					return NULL;
				}

				if (x_obj_isnil(p_base, x_base_field_tco_env(p_base))) {
					x_base_field_tco_env(p_base) = p_saved_env;
				}

				return NULL;
			}

			p_result = x_prim_eval_arg(p_base, x_firstobj(p_body));
			p_body = x_restobj(p_body);
		}

		x_base_field_env_alist(p_base) = p_saved_env;

		return p_result;
	}
}

/* eval: (eval expr [env]) -> evaluate expression, optionally in given env */
x_obj_t *x_prim_eval(x_obj_t *p_base, x_obj_t *p_args)
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

	return NULL;
}

/* fn: (fn (params) body...) -> create closure */
x_obj_t *x_prim_closure(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_params = x_firstobj(p_args),
		*p_body = x_restobj(p_args),
		*p_env = x_base_field_env_alist(p_base);

	return x_mkproc(p_base, p_params, p_body, p_env);
}

/* op: (op formals env-param body...) -> create operative (user-level fexpr) */
x_obj_t *x_prim_operative(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_params = x_firstobj(p_args),
		*p_envparam = x_firstobj(x_restobj(p_args)),
		*p_body = x_restobj(x_restobj(p_args)),
		*p_env = x_base_field_env_alist(p_base);

	return x_mkop(p_base, p_params, p_envparam, p_body, p_env);
}

/* wrap: (wrap combiner) -> create applicative from combiner */
x_obj_t *x_prim_wrap(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_combiner = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_mkwrap(p_base, p_combiner);
}

/* unwrap: (unwrap applicative) -> extract underlying combiner */
x_obj_t *x_prim_unwrap(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_applicative = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_procenv(p_applicative);
}

/* guard: (guard (var handler-body...) body...) -> error recovery */
x_obj_t *x_prim_guard(x_obj_t *p_base, x_obj_t *p_args)
{
	jmp_buf jmp;
	x_obj_t *p_clause = x_firstobj(p_args),
		*p_var = x_firstobj(p_clause),
		*p_handler_body = x_restobj(p_clause),
		*p_body = x_restobj(p_args),
		*p_prev_handler = x_base_field_error_handler(p_base),
		*p_handler, *p_result = NULL;

	/* Build handler pair tree: (jmp-ptr saved-env error-value) */
	p_handler = x_mkspair(p_base,
		x_mkptr(p_base, &jmp),
		x_mkspair(p_base,
			x_base_field_env_alist(p_base),
			x_mkspair(p_base, NULL, NULL)));
	x_base_field_error_handler(p_base) = p_handler;

	if (setjmp(jmp) == 0) {
		/* Normal execution: evaluate body. */
		while ( ! x_obj_isnil(p_base, p_body)) {
			p_result = x_prim_eval_arg(p_base, x_firstobj(p_body));
			p_body = x_restobj(p_body);
		}
	} else {
		/* Error caught: bind error to var, run handler body. */
		x_obj_t *p_err = x_error_handler_error(p_handler);
		x_obj_t *p_pair = x_mkspair(p_base, p_var, p_err);

		x_base_env_alist_extend(p_base, p_pair);

		while ( ! x_obj_isnil(p_base, p_handler_body)) {
			p_result = x_prim_eval_arg(p_base,
				x_firstobj(p_handler_body));
			p_handler_body = x_restobj(p_handler_body);
		}

		x_base_field_env_alist(p_base)
			= x_error_handler_saved_env(p_handler);
	}

	/* Pop handler. */
	x_base_field_error_handler(p_base) = p_prev_handler;

	return p_result;
}

/* error: (error message) -> signal an error */
x_obj_t *x_prim_error(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_msg = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_handler = x_base_field_error_handler(p_base);

	/* If handler installed, use it. */
	if ( ! x_obj_isnil(p_base, p_handler)) {
		x_error_handler_error(p_handler) = p_msg;
		x_base_field_env_alist(p_base)
			= x_error_handler_saved_env(p_handler);
		longjmp(*(jmp_buf *)x_error_handler_jmp(p_handler), 1);
	}

	/* No handler: fall through to fatal error. */
	if (x_obj_type_isstr(p_base, p_msg)) {
		x_obj_error(p_base, x_strval(p_msg), NULL);
	} else {
		x_obj_error(p_base, "error", p_msg);
	}

	return NULL;
}

/* match: (match (test body)...) -> first truthy test's body (tail-eval) */
x_obj_t *x_prim_match(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_clause, *p_test;

	while ( ! x_obj_isnil(p_base, p_args)) {
		p_clause = x_firstobj(p_args);
		p_test = x_prim_eval_arg(p_base, x_firstobj(p_clause));

		if ( ! x_obj_isnil(p_base, p_test)) {
			x_base_field_tco_expr(p_base) =
				x_firstobj(x_restobj(p_clause));

			return NULL;
		}

		p_args = x_restobj(p_args);
	}

	return NULL;
}

/* %rewrite: (%rewrite pair new-first new-rest) -> mutate pair in-place */
x_obj_t *x_prim_rewrite(x_obj_t *p_base, x_obj_t *p_args)
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

/* first-int: (first-int x) -> car slot as integer atom */
x_obj_t *x_prim_first_int(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *x = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_mkint(p_base, x_firstint(x));
}

/* rest-int: (rest-int x) -> cdr slot as integer atom */
x_obj_t *x_prim_rest_int(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *x = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_mkint(p_base, x_restint(x));
}

/* set-first: (set-first pair val) -> write object pointer to car */
x_obj_t *x_prim_set_first(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_pair = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_val = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(p_args)));

	x_firstobj(p_pair) = p_val;

	return p_pair;
}

/* set-rest: (set-rest pair val) -> write object pointer to cdr */
x_obj_t *x_prim_set_rest(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_pair = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_val = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(p_args)));

	x_restobj(p_pair) = p_val;

	return p_pair;
}

/* set-first-int: (set-first-int pair val) -> write raw integer to car */
x_obj_t *x_prim_set_first_int(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_pair = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_val = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(p_args)));

	x_firstint(p_pair) = x_atomint(p_val);

	return p_pair;
}

/* set-rest-int: (set-rest-int pair val) -> write raw integer to cdr */
x_obj_t *x_prim_set_rest_int(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_pair = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_val = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(p_args)));

	x_restint(p_pair) = x_atomint(p_val);

	return p_pair;
}

/* %base: (%base) -> return current base object */
/* eval!: evaluate in current env, return result (no TCO, no env save/restore) */
x_obj_t *x_prim_eval_immediate(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_expr = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_prim_eval_arg(p_base, p_expr);
}

/* tail-eval: (tail-eval expr env) -> TCO-compatible eval in given env */
x_obj_t *x_prim_tail_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_expr = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_env = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	x_base_field_env_alist(p_base) = p_env;
	x_base_field_tco_expr(p_base) = p_expr;

	return NULL;
}

/* %seq: (%seq a b) -> eval a (blocking), tco-eval b */
x_obj_t *x_prim_seq(x_obj_t *p_base, x_obj_t *p_args)
{
	x_prim_eval_arg(p_base, x_firstobj(p_args));
	x_base_field_tco_expr(p_base) = x_firstobj(x_restobj(p_args));

	return NULL;
}

x_obj_t *x_prim_base(x_obj_t *p_base, x_obj_t *p_args)
{
	return p_base;
}

x_obj_t *x_prim_core_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_prim_bind(p_base, "lit", x_prim_quote);
	x_prim_bind(p_base, "pair", x_prim_pair);
	x_prim_bind(p_base, "first", x_prim_first);
	x_prim_bind(p_base, "rest", x_prim_rest);
	x_prim_bind(p_base, "def", x_prim_define);
	x_prim_bind(p_base, "set", x_prim_set);
	x_prim_bind(p_base, "apply", x_prim_apply);
	x_prim_bind(p_base, "eval", x_prim_eval);
	x_prim_bind(p_base, "eval!", x_prim_eval_immediate);
	x_prim_bind(p_base, "fn", x_prim_closure);
	x_prim_bind(p_base, "op", x_prim_operative);
	x_prim_bind(p_base, "wrap", x_prim_wrap);
	x_prim_bind(p_base, "unwrap", x_prim_unwrap);
	x_prim_bind(p_base, "guard", x_prim_guard);
	x_prim_bind(p_base, "error", x_prim_error);
	x_prim_bind(p_base, "match", x_prim_match);
	x_prim_bind(p_base, "%rewrite", x_prim_rewrite);
	x_prim_bind(p_base, "first-int", x_prim_first_int);
	x_prim_bind(p_base, "rest-int", x_prim_rest_int);
	x_prim_bind(p_base, "set-first", x_prim_set_first);
	x_prim_bind(p_base, "set-rest", x_prim_set_rest);
	x_prim_bind(p_base, "set-first-int", x_prim_set_first_int);
	x_prim_bind(p_base, "set-rest-int", x_prim_set_rest_int);
	x_prim_bind(p_base, "tail-eval", x_prim_tail_eval);
	x_prim_bind(p_base, "%seq", x_prim_seq);
	x_prim_bind(p_base, "%base", x_prim_base);

	return p_base;
}
