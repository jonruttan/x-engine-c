/*
 * # Computational Expressions in C
 *
 * ## x-prim/core.c -- Implementation - Primitives - Core Applicatives
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
#include "x-type/int.h"
#include "x-type/list.h"
#include "x-type/prim.h"
#include "x-type/procedure.h"

/* pair: (pair a b) -> (a . b) */
static x_obj_t *x_prim_pair(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	return x_mklist(p_base, a, b);
}

/* first: (first x) -> car of pair */
static x_obj_t *x_prim_first(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *x;
	x_eargs(p_base, p_args, 2, NULL, &x);

	return x_firstobj(x);
}

/* rest: (rest x) -> cdr of pair */
static x_obj_t *x_prim_rest(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *x;
	x_eargs(p_base, p_args, 2, NULL, &x);

	return x_restobj(x);
}

/* apply: (apply f arg1 ... args) -> call callable with prefix + tail arg list */
static x_obj_t *x_prim_apply(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_fn, *p_evaled, *p_vals, *p_walk;
	x_eargs(p_base, p_args, 2, NULL, &p_fn);
	p_evaled = x_prim_evlis(p_base, x_11(p_args));

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

	/* Root p_fn and p_vals so GC doesn't free them during procedure setup */
	x_base_field_eval_list_stack(p_base) = x_mkspair(p_base,
		p_fn, x_mkspair(p_base,
			p_vals, x_base_field_eval_list_stack(p_base)));

	/* Procedure: bind params, eval body with TCO for eval trampoline. */
	if (x_obj_type_isprocedure(p_base, p_fn)) {
		/* Push ((env . boundary) . (bst . shadow_head)) onto save-stack */
		x_base_field_save_stack(p_base) = x_mkspair(p_base,
			x_mkspair(p_base,
				x_mkspair(p_base, x_base_field_env_alist(p_base),
				                   x_base_field_env_local_boundary(p_base)),
				x_mkspair(p_base, x_base_field_env_global_tree(p_base),
				                   x_base_field_shadow_list(p_base))),
			x_base_field_save_stack(p_base));

		/* Set boundary and BST to closure's captured values */
		x_base_field_env_local_boundary(p_base) = x_procenv(p_fn);
		x_base_field_env_global_tree(p_base) = x_procbst(p_fn);

		p_vals = x_mkspair(p_base, p_fn, p_vals);
		x_base_field_env_alist(p_base) = x_prim_multiple_extend(
			p_base, x_procenv(p_fn), x_procparams(p_fn), p_vals);

		/* Unroot */
		x_base_field_eval_list_stack(p_base)
			= x_restobj(x_restobj(x_base_field_eval_list_stack(p_base)));

		return x_prim_body_eval_tco(p_base, x_procbody(p_fn));
	}

	/* Operative / C primitive: delegate to type dispatch. */
	{
		x_spair_t apply_args[1] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_fn }, { p_vals })
		};
		x_obj_t *p_result = x_type_prim_apply(p_base, (x_obj_t *)apply_args);

		/* Unroot */
		x_base_field_eval_list_stack(p_base)
			= x_restobj(x_restobj(x_base_field_eval_list_stack(p_base)));

		return p_result;
	}
}

/* eval: (eval expr [env]) -> evaluate expression, optionally in given env */
static x_obj_t *x_prim_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_expr, *p_env_arg;
	x_eargs(p_base, p_args, 2, NULL, &p_expr);
	p_env_arg = x_11(p_args);

	if ( ! x_obj_isnil(p_base, p_env_arg)) {
		/* eval with env: save/restore via base save-stack */
		x_obj_t *p_env = x_prim_eval_arg(p_base, x_firstobj(p_env_arg));
		x_obj_t *p_result;

		/* Push ((env . boundary) . (bst . shadow_head)) onto save-stack */
		x_base_field_save_stack(p_base) = x_mkspair(p_base,
			x_mkspair(p_base,
				x_mkspair(p_base, x_base_field_env_alist(p_base),
				                   x_base_field_env_local_boundary(p_base)),
				x_mkspair(p_base, x_base_field_env_global_tree(p_base),
				                   x_base_field_shadow_list(p_base))),
			x_base_field_save_stack(p_base));

		x_base_field_env_alist(p_base) = p_env;
		/* Don't change boundary or BST — eval-with-env preserves scope context */
		p_result = x_prim_eval_arg(p_base, p_expr);

		/* Pop save-stack and restore env + boundary + bst + shadow */
		{
			x_obj_t *p_saved = x_firstobj(x_base_field_save_stack(p_base));
			x_base_field_env_alist(p_base) = x_firstobj(x_firstobj(p_saved));
			x_base_field_env_local_boundary(p_base)
				= x_restobj(x_firstobj(p_saved));
			x_base_field_env_global_tree(p_base)
				= x_firstobj(x_restobj(p_saved));
			x_prim_clear_shadows_to(p_base, x_restobj(x_restobj(p_saved)));
			x_base_field_save_stack(p_base)
				= x_restobj(x_base_field_save_stack(p_base));
		}

		return p_result;
	}

	/* eval without env: use TCO trampoline */
	x_base_field_tco_expr(p_base) = p_expr;

	return NULL;
}

/* eval!: evaluate in current env, return result (no TCO, no env save/restore) */
static x_obj_t *x_prim_eval_immediate(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_expr;
	x_eargs(p_base, p_args, 2, NULL, &p_expr);

	return x_prim_eval_arg(p_base, p_expr);
}

/* tail-eval: (tail-eval expr env) -> TCO-compatible eval in given env */
static x_obj_t *x_prim_tail_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_expr, *p_env;
	x_eargs(p_base, p_args, 3, NULL, &p_expr, &p_env);

	x_base_field_env_alist(p_base) = p_env;
	x_base_field_tco_expr(p_base) = p_expr;

	return NULL;
}

/* wrap: (wrap combiner) -> create applicative from combiner */
static x_obj_t *x_prim_wrap(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_combiner;
	x_eargs(p_base, p_args, 2, NULL, &p_combiner);

	return x_mkwrap(p_base, p_combiner);
}

/* unwrap: (unwrap applicative) -> extract underlying combiner */
static x_obj_t *x_prim_unwrap(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_applicative;
	x_eargs(p_base, p_args, 2, NULL, &p_applicative);

	return x_procenv(p_applicative);
}

/* atomic: (atomic expr...) -> eval each expr blocking */
static x_obj_t *x_prim_atomic(x_obj_t *p_base, x_obj_t *p_args)
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

/* %base: (%base) -> return current base object */
static x_obj_t *x_prim_base(x_obj_t *p_base, x_obj_t *p_args)
{
	(void)p_args;
	return p_base;
}

x_obj_t *x_prim_core_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_prim_entry_t entries[] = {
		{ "pair", x_prim_pair },
		{ "first", x_prim_first },
		{ "rest", x_prim_rest },
		{ "apply", x_prim_apply },
		{ "eval", x_prim_eval },
		{ "eval!", x_prim_eval_immediate },
		{ "tail-eval", x_prim_tail_eval },
		{ "wrap", x_prim_wrap },
		{ "unwrap", x_prim_unwrap },
		{ "atomic", x_prim_atomic },
		{ "%base", x_prim_base }
	};

	x_prim_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
