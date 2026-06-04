/** @file core.c
 *  @brief Core primitives: pair, first, rest, apply, eval, wrap/unwrap, atomic, base.
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2024 Jon Ruttan
 *  @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
/*
 * # Includes
 */
#include "x-prim.h"
#include "x-eval.h"
#include "x-eval.h"
#include "x-type/int.h"
#include "x-type/list.h"
#include "x-type/prim.h"
#include "x-type/procedure.h"

/** Construct a pair from two values.
 *  x-lang: (pair a b)
 *  @param p_base  Execution context.
 *  @param p_args  Unevaluated argument list (a b).
 *  @return New pair (a . b).
 *  @note Fexpr: args unevaluated; x_eargs evaluates them.
 */
static x_obj_t *x_prim_pair(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *a, *b;
	x_eargs(p_base, p_args, 3, NULL, &a, &b);

	return x_mklist(p_base, a, b);
}

/** Return the first element of a pair.
 *  x-lang: (first x)
 *  @param p_base  Execution context.
 *  @param p_args  Unevaluated argument list (x).
 *  @return First element of pair x.
 *  @note Fexpr: args unevaluated; x_eargs evaluates them.
 */
static x_obj_t *x_prim_first(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *x;
	x_eargs(p_base, p_args, 2, NULL, &x);

	return x_firstobj(x);
}

/** Return the rest (tail) of a pair.
 *  x-lang: (rest x)
 *  @param p_base  Execution context.
 *  @param p_args  Unevaluated argument list (x).
 *  @return Rest element of pair x.
 *  @note Fexpr: args unevaluated; x_eargs evaluates them.
 */
static x_obj_t *x_prim_rest(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *x;
	x_eargs(p_base, p_args, 2, NULL, &x);

	return x_restobj(x);
}

/** Apply a callable to arguments with a trailing argument list.
 *  x-lang: (apply f arg1 ... args)
 *  @param p_base  Execution context.
 *  @param p_args  Unevaluated argument list (f arg1 ... args).
 *  @return Result of applying f to the combined argument list.
 *  @note Fexpr: args unevaluated; evaluates args via x_eval_list.
 *  @note For procedures, sets up env and uses TCO trampoline via x_eval_body_tco.
 *  @note Prefix args are spliced onto the final tail list:
 *        (apply f a b '(c d)) calls f with (a b c d).
 *  @see x_eval_body_tco, x_callable_apply
 */
static x_obj_t *x_prim_apply(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_fn, *p_evaled, *p_vals, *p_walk;
	x_eargs(p_base, p_args, 2, NULL, &p_fn);
	p_evaled = x_eval_list(p_base, x_11(p_args));

	/* Build combined arg list: prefix args prepended to tail list.
	 * (apply f a b '(c d)) -> p_evaled = (a b (c d))
	 * Single arg: p_evaled = ((c d)) -> p_vals = (c d) */
	if (x_obj_isnil(p_base, x_restobj(p_evaled))) {
		p_vals = x_firstobj(p_evaled);
	} else {
		/* Walk to second-to-last, splice tail list in place.
		 * p_evaled is fresh from x_eval_list, safe to mutate. */
		p_walk = p_evaled;
		while ( ! x_obj_isnil(p_base,
			x_restobj(x_restobj(p_walk)))) {
			p_walk = x_restobj(p_walk);
		}
		x_restobj(p_walk) = x_firstobj(x_restobj(p_walk));
		p_vals = p_evaled;
	}

	/* Root p_fn and p_vals so GC doesn't free them during procedure setup */
	x_obj_push_field(p_base, &x_eval_field_eval_list(p_base), p_vals, X_OBJ_FLAG_NONE);
	x_obj_push_field(p_base, &x_eval_field_eval_list(p_base), p_fn, X_OBJ_FLAG_NONE);

	/* Procedure: bind params, eval body with TCO for eval trampoline. */
	if (x_obj_type_isprocedure(p_base, p_fn)) {
		/* Push ((env . boundary) . (bst . shadow_head)) onto save-stack */
		x_eval_field_save_stack(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mkspair(p_base, X_OBJ_FLAG_NONE,
				x_mkspair(p_base, X_OBJ_FLAG_NONE, x_firstobj(x_eval_field_env_alist(p_base)),
				                   x_eval_field_env_local_boundary(p_base)),
				x_mkspair(p_base, X_OBJ_FLAG_NONE, x_eval_field_env_global_tree(p_base),
				                   x_eval_field_shadow_list(p_base))),
			x_eval_field_save_stack(p_base));

		/* Set boundary and BST to closure's captured values.  Skip the
		 * BST swap if the closure was captured before any top-level def
		 * existed (captured BST is NULL): use the current live BST so
		 * lookups can still find anything defined since. */
		x_eval_field_env_local_boundary(p_base) = x_procenv(p_fn);
		if (x_procbst(p_fn) != NULL) {
			x_eval_field_env_global_tree(p_base) = x_procbst(p_fn);
		}

		p_vals = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_fn, p_vals);
		x_firstobj(x_eval_field_env_alist(p_base)) = x_env_extend(
			p_base, x_procenv(p_fn), x_procparams(p_fn), p_vals);

		/* Unroot */
		x_obj_pop_field(p_base, &x_eval_field_eval_list(p_base));
		x_obj_pop_field(p_base, &x_eval_field_eval_list(p_base));

		return x_eval_body_tco(p_base, x_procbody(p_fn));
	}

	/* Operative / C primitive: delegate to type dispatch. */
	{
		x_spair_t apply_args[1] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_fn }, { p_vals })
		};
		x_obj_t *p_result = x_callable_apply(p_base, (x_obj_t *)apply_args);

		/* Unroot */
		x_obj_pop_field(p_base, &x_eval_field_eval_list(p_base));
		x_obj_pop_field(p_base, &x_eval_field_eval_list(p_base));

		return p_result;
	}
}

/** Evaluate an expression, optionally in a given environment.
 *  x-lang: (eval expr [env])
 *  @param p_base  Execution context.
 *  @param p_args  Unevaluated argument list (expr [env]).
 *  @return Result of evaluation (with env), or NULL (without env, uses TCO).
 *  @note Fexpr: args unevaluated; x_eargs evaluates expr.
 *  @note With env arg: saves/restores env via base save-stack; def inside
 *        does not persist (env is restored after).
 *  @note Without env arg: sets tco_expr for tail-call optimization trampoline.
 *  @see x_prim_eval_immediate, x_prim_tail_eval
 */
static x_obj_t *x_prim_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_expr, *p_env_arg;
	x_eargs(p_base, p_args, 2, NULL, &p_expr);
	p_env_arg = x_11(p_args);

	if ( ! x_obj_isnil(p_base, p_env_arg)) {
		/* eval with env: save/restore via base save-stack */
		x_obj_t *p_env = x_eval_arg(p_base, x_firstobj(p_env_arg));
		x_obj_t *p_result;

		/* Push ((env . boundary) . (bst . shadow_head)) onto save-stack */
		x_tco_compound_save(p_base);

		x_firstobj(x_eval_field_env_alist(p_base)) = p_env;
		/* Don't change boundary or BST — eval-with-env preserves scope context */
		p_result = x_eval_arg(p_base, p_expr);

		/* Pop save-stack and restore env + boundary + bst + shadow */
		x_tco_restore(p_base, x_firstobj(x_eval_field_save_stack(p_base)));
		x_eval_field_save_stack(p_base)
			= x_restobj(x_eval_field_save_stack(p_base));

		return p_result;
	}

	/* eval without env: use TCO trampoline */
	x_firstobj(x_eval_field_tco_expr(p_base)) = p_expr;

	return NULL;
}

/** Evaluate expression immediately in the current environment.
 *  x-lang: (eval! expr)
 *  @param p_base  Execution context.
 *  @param p_args  Unevaluated argument list (expr).
 *  @return Result of evaluating expr.
 *  @note Fexpr: args unevaluated; x_eargs evaluates expr.
 *  @note No TCO, no env save/restore. Used by the x-lang REPL operative.
 *  @see x_prim_eval
 */
static x_obj_t *x_prim_eval_immediate(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_expr;
	x_eargs(p_base, p_args, 2, NULL, &p_expr);

	return x_eval_arg(p_base, p_expr);
}

/** TCO-compatible eval: set expression and environment for tail-call trampoline.
 *  x-lang: (tail-eval expr env)
 *  @param p_base  Execution context.
 *  @param p_args  Unevaluated argument list (expr env).
 *  @return NULL (result delivered via tco_expr trampoline).
 *  @note Fexpr: args unevaluated; x_eargs evaluates both args.
 *  @note Sets tco_expr for tail-call; switches env to the given environment.
 *  @see x_prim_eval
 */
static x_obj_t *x_prim_tail_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_expr, *p_env;
	x_eargs(p_base, p_args, 3, NULL, &p_expr, &p_env);

	x_firstobj(x_eval_field_env_alist(p_base)) = p_env;
	x_firstobj(x_eval_field_tco_expr(p_base)) = p_expr;

	return NULL;
}

/** Wrap a combiner to create an applicative (args evaluated before call).
 *  x-lang: (wrap combiner)
 *  @param p_base  Execution context.
 *  @param p_args  Unevaluated argument list (combiner).
 *  @return New applicative wrapping the given combiner.
 *  @note Fexpr: args unevaluated; x_eargs evaluates combiner.
 *  @see x_prim_unwrap
 */
static x_obj_t *x_prim_wrap(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_combiner;
	x_eargs(p_base, p_args, 2, NULL, &p_combiner);

	return x_mkwrap(p_base, p_combiner);
}

/** Extract the underlying combiner from an applicative.
 *  x-lang: (unwrap applicative)
 *  @param p_base  Execution context.
 *  @param p_args  Unevaluated argument list (applicative).
 *  @return The underlying combiner.
 *  @note Fexpr: args unevaluated; x_eargs evaluates applicative.
 *  @see x_prim_wrap
 */
static x_obj_t *x_prim_unwrap(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_applicative;
	x_eargs(p_base, p_args, 2, NULL, &p_applicative);

	return x_procenv(p_applicative);
}

/** Evaluate each expression sequentially, blocking between evaluations.
 *  x-lang: (atomic expr ...)
 *  @param p_base  Execution context.
 *  @param p_args  Unevaluated argument list of expressions.
 *  @return Result of the last expression evaluated.
 *  @note Fexpr: args unevaluated; evaluates each expression individually.
 *  @note Roots remaining args during each eval to protect from GC.
 */
static x_obj_t *x_prim_atomic(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_result = NULL;

	while ( ! x_obj_isnil(p_base, p_args)) {
		/* Root remaining args so GC doesn't free them */
		x_obj_push_field(p_base, &x_eval_field_eval_list(p_base), p_args, X_OBJ_FLAG_NONE);

		p_result = x_eval_arg(p_base, x_firstobj(p_args));

		x_obj_pop_field(p_base, &x_eval_field_eval_list(p_base));

		p_args = x_restobj(p_args);
	}

	return p_result;
}

/** Return the current base (execution context) object.
 *  x-lang: (%base)
 *  @param p_base  Execution context.
 *  @param p_args  Unused.
 *  @return The base object itself.
 */
static x_obj_t *x_prim_base(x_obj_t *p_base, x_obj_t *p_args)
{
	(void)p_args;
	return p_base;
}

/** Register core primitives into the environment.
 *
 *  Binds: pair, first, rest, apply, eval, eval!, tail-eval, wrap, unwrap,
 *  atomic, %base.
 *
 *  @param p_base  Execution context.
 *  @param p_args  Unused.
 *  @return The base object.
 */
x_obj_t *x_prim_core_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_callable_entry_t entries[] = {
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

	x_callable_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
