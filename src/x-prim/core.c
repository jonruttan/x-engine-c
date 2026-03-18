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

	/* If at top level, insert into global BST and update boundary */
	if (x_base_isset(p_base)
		&& x_obj_isnil(p_base, x_base_field_save_stack(p_base))) {
		x_base_field_env_global_tree(p_base) = x_alist_bst_insert(
			p_base, x_base_field_env_global_tree(p_base), p_pair);
		x_base_field_env_local_boundary(p_base)
			= x_base_field_env_alist(p_base);
	} else if (x_base_isset(p_base)) {
		/* Local def: if symbol is in BST, flag it as shadowed so
		 * lookups skip BST and use alist walk (finds the local). */
		if (x_alist_bst_lookup(p_base,
			x_base_field_env_global_tree(p_base), p_name) != NULL) {
			x_obj_flags(p_name) |= X_OBJ_FLAG_1;
		}
	}

	return p_val;
}

/* set: (set name value) -> mutate existing binding (3-step BST-aware) */
x_obj_t *x_prim_set(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_name = x_firstobj(p_args),
		*p_val = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));
	x_obj_t *p_alist, *p_boundary, *p_entry;

	p_alist = x_base_field_env_alist(p_base);
	p_boundary = x_base_field_env_local_boundary(p_base);

	/* Step 1: walk locals (up to AND INCLUDING boundary) */
	while ( ! x_obj_isnil(p_base, p_alist)) {
		if (x_firstobj(x_firstobj(p_alist)) == p_name) {
			x_restobj(x_firstobj(p_alist)) = p_val;
			return p_val;
		}
		if (p_alist == p_boundary) {
			p_alist = x_restobj(p_alist);
			break;
		}
		p_alist = x_restobj(p_alist);
	}

	/* Step 2: BST lookup (skip re-defined symbols) */
	if ( ! (x_obj_flags(p_name) & X_OBJ_FLAG_1)) {
		p_entry = x_alist_bst_lookup(p_base,
			x_base_field_env_global_tree(p_base), p_name);
		if ( ! x_obj_isnil(p_base, p_entry)) {
			x_restobj(p_entry) = p_val;
			return p_val;
		}
	}

	/* Step 3: continue alist walk from boundary */
	while ( ! x_obj_isnil(p_base, p_alist)) {
		if (x_firstobj(x_firstobj(p_alist)) == p_name) {
			x_restobj(x_firstobj(p_alist)) = p_val;
			return p_val;
		}
		p_alist = x_restobj(p_alist);
	}

	{
		x_satom_t sym_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
			{ .s = x_symbolval(p_name) });
		x_obj_error(p_base, "Unbound "X_TYPE_SYMBOL_NAME,
			(x_obj_t *)&sym_name);
	}

	return NULL;
}

/* apply: (apply f arg1 ... args) -> call callable with prefix + tail arg list */
x_obj_t *x_prim_apply(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_fn = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_evaled = x_prim_evlis(p_base, x_restobj(p_args)),
		*p_vals, *p_walk;

	/* Build combined arg list: prefix args prepended to tail list.
	 * (apply f a b '(c d)) -> p_evaled = (a b (c d))
	 * Single arg: p_evaled = ((c d)) -> p_vals = (c d) */
	if (x_obj_isnil(p_base, x_restobj(p_evaled))) {
		p_vals = x_firstobj(p_evaled);
	} else {
		/* Walk to second-to-last, splice tail list in place.
		 * p_evaled is fresh from x_prim_evlis, safe to mutate. */
		p_walk = p_evaled;
		while ( ! x_obj_isnil(p_base,
			x_restobj(x_restobj(p_walk)))) {
			p_walk = x_restobj(p_walk);
		}
		x_restobj(p_walk) = x_firstobj(x_restobj(p_walk));
		p_vals = p_evaled;
	}

	/* Procedure: bind params, eval body with TCO for eval trampoline. */
	if (x_obj_type_isprocedure(p_base, p_fn)) {
		/* Push ((env . boundary) . bst) onto save-stack */
		x_base_field_save_stack(p_base) = x_mkspair(p_base,
			x_mkspair(p_base,
				x_mkspair(p_base, x_base_field_env_alist(p_base),
				                   x_base_field_env_local_boundary(p_base)),
				x_base_field_env_global_tree(p_base)),
			x_base_field_save_stack(p_base));

		/* Set boundary and BST to closure's captured values */
		x_base_field_env_local_boundary(p_base) = x_procenv(p_fn);
		x_base_field_env_global_tree(p_base) = x_procbst(p_fn);

		x_base_field_env_alist(p_base) = x_prim_multiple_extend(
			p_base, x_procenv(p_fn), x_procparams(p_fn), p_vals);

		return x_prim_body_eval_tco(p_base, x_procbody(p_fn));
	}

	/* Operative / C primitive: delegate to type dispatch. */
	{
		x_spair_t apply_args[1] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_fn }, { p_vals })
		};

		return x_type_prim_apply(p_base, (x_obj_t *)apply_args);
	}
}

/* eval: (eval expr [env]) -> evaluate expression, optionally in given env */
x_obj_t *x_prim_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_expr = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_env_arg = x_restobj(p_args);

	if ( ! x_obj_isnil(p_base, p_env_arg)) {
		/* eval with env: save/restore via base save-stack */
		x_obj_t *p_env = x_prim_eval_arg(p_base, x_firstobj(p_env_arg));
		x_obj_t *p_result;

		/* Push ((env . boundary) . bst) onto save-stack */
		x_base_field_save_stack(p_base) = x_mkspair(p_base,
			x_mkspair(p_base,
				x_mkspair(p_base, x_base_field_env_alist(p_base),
				                   x_base_field_env_local_boundary(p_base)),
				x_base_field_env_global_tree(p_base)),
			x_base_field_save_stack(p_base));

		x_base_field_env_alist(p_base) = p_env;
		/* Don't change boundary or BST — eval-with-env preserves scope context */
		p_result = x_prim_eval_arg(p_base, p_expr);

		/* Pop save-stack and restore env + boundary + bst */
		{
			x_obj_t *p_saved = x_firstobj(x_base_field_save_stack(p_base));
			x_base_field_env_alist(p_base) = x_firstobj(x_firstobj(p_saved));
			x_base_field_env_local_boundary(p_base)
				= x_restobj(x_firstobj(p_saved));
			x_base_field_env_global_tree(p_base) = x_restobj(p_saved);
			x_base_field_save_stack(p_base)
				= x_restobj(x_base_field_save_stack(p_base));
		}

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
		*p_env = x_base_field_env_alist(p_base),
		*p_bst = x_base_field_env_global_tree(p_base);

	return x_mkproc(p_base, p_params, p_body, p_env, p_bst);
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
		*p_saved_save_stack = x_base_field_save_stack(p_base),
		*p_handler, *p_result = NULL;

	/* Build handler: (jmp-ptr (saved-env . saved-boundary) error-value) */
	p_handler = x_mkspair(p_base,
		x_mkptr(p_base, &jmp),
		x_mkspair(p_base,
			x_mkspair(p_base, x_base_field_env_alist(p_base),
			                   x_base_field_env_local_boundary(p_base)),
			x_mkspair(p_base, NULL, NULL)));
	x_base_field_error_handler(p_base) = p_handler;

	if (setjmp(jmp) == 0) {
		/* Normal execution: evaluate body. */
		p_result = x_prim_body_eval(p_base, p_body);
	} else {
		/* Error caught: restore save-stack and boundary to guard point. */
		x_obj_t *p_err = x_error_handler_error(p_handler);
		x_obj_t *p_pair = x_mkspair(p_base, p_var, p_err);

		x_base_field_save_stack(p_base) = p_saved_save_stack;
		x_base_env_alist_extend(p_base, p_pair);
		p_result = x_prim_body_eval(p_base, p_handler_body);
		x_base_field_env_alist(p_base)
			= x_error_handler_saved_env(p_handler);
		x_base_field_env_local_boundary(p_base)
			= x_error_handler_saved_boundary(p_handler);
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

		if ( ! x_obj_isnil(p_base, p_test)
				&& p_test != x_base_field_false(p_base)) {
			x_base_field_tco_expr(p_base) =
				x_firstobj(x_restobj(p_clause));

			return NULL;
		}

		p_args = x_restobj(p_args);
	}

	return NULL;
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
	/* Do NOT change local_boundary — the boundary from the enclosing
	 * procedure call is still valid for the env being restored. */
	x_base_field_tco_expr(p_base) = p_expr;

	return NULL;
}

/* atomic: (atomic expr...) -> eval each expr in C with no x-lang
 * allocations between. Useful for (atomic (heap-mark) (heap-sweep)). */
x_obj_t *x_prim_atomic(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_result = NULL;

	while ( ! x_obj_isnil(p_base, p_args)) {
		/* Root remaining args so GC doesn't free them */
		x_base_field_eval_list_stack(p_base) = x_mkspair(p_base,
			p_args, x_base_field_eval_list_stack(p_base));

		p_result = x_prim_eval_arg(p_base, x_firstobj(p_args));

		x_base_field_eval_list_stack(p_base)
			= x_restobj(x_base_field_eval_list_stack(p_base));

		p_args = x_restobj(p_args);
	}

	return p_result;
}

/* %seq: (%seq a b) -> eval a (blocking), tco-eval b */
x_obj_t *x_prim_seq(x_obj_t *p_base, x_obj_t *p_args)
{
	/* Root p_args so GC doesn't free it during eval of first arg */
	x_base_field_eval_list_stack(p_base) = x_mkspair(p_base,
		p_args, x_base_field_eval_list_stack(p_base));

	x_prim_eval_arg(p_base, x_firstobj(p_args));
	x_base_field_tco_expr(p_base) = x_firstobj(x_restobj(p_args));

	/* Unroot */
	x_base_field_eval_list_stack(p_base)
		= x_restobj(x_base_field_eval_list_stack(p_base));

	return NULL;
}

x_obj_t *x_prim_base(x_obj_t *p_base, x_obj_t *p_args)
{
	return p_base;
}

x_obj_t *x_prim_core_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_prim_entry_t entries[] = {
		{ "lit", x_prim_quote },
		{ "pair", x_prim_pair },
		{ "first", x_prim_first },
		{ "rest", x_prim_rest },
		{ "def", x_prim_define },
		{ "set", x_prim_set },
		{ "apply", x_prim_apply },
		{ "eval", x_prim_eval },
		{ "eval!", x_prim_eval_immediate },
		{ "fn", x_prim_closure },
		{ "op", x_prim_operative },
		{ "wrap", x_prim_wrap },
		{ "unwrap", x_prim_unwrap },
		{ "guard", x_prim_guard },
		{ "error", x_prim_error },
		{ "match", x_prim_match },
		{ "first-int", x_prim_first_int },
		{ "rest-int", x_prim_rest_int },
		{ "set-first", x_prim_set_first },
		{ "set-rest", x_prim_set_rest },
		{ "set-first-int", x_prim_set_first_int },
		{ "set-rest-int", x_prim_set_rest_int },
		{ "tail-eval", x_prim_tail_eval },
		{ "%seq", x_prim_seq },
		{ "atomic", x_prim_atomic },
		{ "%base", x_prim_base }
	};

	x_prim_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
