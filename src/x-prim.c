/** @file x-prim.c
 *  @brief Primitive evaluation helpers, environment extension, and registration.
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
#include "x-syntax.h"
#include "x-alist.h"
#include "x-base-typesystem.h"
#include "x-eval.h"
#include "x-type/list.h"
#include "x-type/prim.h"
#include "x-type/symbol.h"

/**
 * Clear all shadow flags from symbols on the shadow list.
 *
 * Walks the entire shadow list, removing X_OBJ_FLAG_SHADOW from each
 * symbol, then resets the list to nil.
 *
 * @param p_base  x_obj_t* -- Execution context
 *
 * @see x_prim_clear_shadows_to
 */
void x_prim_clear_shadows(x_obj_t *p_base)
{
	x_obj_t *p_list = x_base_field_shadow_list(p_base);

	while ( ! x_obj_isnil(p_base, p_list)) {
		x_obj_flags(x_firstobj(p_list)) &= ~X_OBJ_FLAG_SHADOW;
		p_list = x_restobj(p_list);
	}
	x_base_field_shadow_list(p_base) = NULL;
}

/**
 * Clear shadow flags back to a saved shadow-list head.
 *
 * Unflags symbols added since @p p_old, then restores the shadow list
 * to that earlier state. Used by TCO and closure restore paths.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_old   x_obj_t* -- Previous shadow-list head to restore to
 *
 * @see x_prim_clear_shadows
 */
void x_prim_clear_shadows_to(x_obj_t *p_base, x_obj_t *p_old)
{
	x_obj_t *p_list = x_base_field_shadow_list(p_base);

	while (p_list != p_old && ! x_obj_isnil(p_base, p_list)) {
		x_obj_flags(x_firstobj(p_list)) &= ~X_OBJ_FLAG_SHADOW;
		p_list = x_restobj(p_list);
	}
	x_base_field_shadow_list(p_base) = p_old;
}

/**
 * Evaluate a single expression.
 *
 * Wraps @p p_arg in a stack-allocated (atom . nil) pair and passes it
 * through x_eval, which unwraps and evaluates the inner expression.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_arg   x_obj_t* -- Expression to evaluate
 * @return x_obj_t* -- Evaluation result
 */
x_obj_t *x_eval_arg(x_obj_t *p_base, x_obj_t *p_arg)
{
	x_satom_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_arg });
	x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { wrap }, { NULL });

	return x_eval(p_base, (x_obj_t *)args);
}

/**
 * Evaluate each element of a list, returning a new list of results.
 *
 * Recursively evaluates via x_eval_arg, rooting the tail on the
 * eval-list GC root so the garbage collector does not free remaining
 * arguments while evaluating the current one.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- List of unevaluated expressions
 * @return x_obj_t* -- New list of evaluated results, or NULL if empty
 */
x_obj_t *x_eval_list(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_val;

	if (x_obj_isnil(p_base, p_args)) {
		return NULL;
	}

	/* Root p_args so GC doesn't free rest while evaluating first */
	x_base_field_eval_list(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		p_args, x_base_field_eval_list(p_base));

	p_val = x_eval_arg(p_base, x_firstobj(p_args));

	x_base_field_eval_list(p_base)
		= x_restobj(x_base_field_eval_list(p_base));

	return x_mklist(p_base, p_val,
		x_eval_list(p_base, x_restobj(p_args)));
}

/**
 * Extend an environment by binding parameters to values.
 *
 * Handles three cases: (1) variadic -- a bare symbol binds to the
 * entire remaining value list, (2) base -- no more params returns
 * the environment unchanged, (3) recursive -- binds first param to
 * first value, then recurses on the rest.
 *
 * Symbols that shadow BST globals are flagged X_OBJ_FLAG_SHADOW and
 * tracked on the shadow list for later clearing.
 *
 * @param p_base   x_obj_t* -- Execution context
 * @param p_env    x_obj_t* -- Current environment alist
 * @param p_params x_obj_t* -- Parameter list (or single symbol for variadic)
 * @param p_vals   x_obj_t* -- Value list
 * @return x_obj_t* -- Extended environment alist
 */
x_obj_t *x_env_extend(x_obj_t *p_base, x_obj_t *p_env,
	x_obj_t *p_params, x_obj_t *p_vals)
{
	/* Variadic: single symbol binds to entire remaining arg list. */
	if ( ! x_obj_isnil(p_base, p_params)
		&& x_obj_type_issymbol(p_base, p_params)) {
		x_obj_t *p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_params, p_vals);

		/* Flag if param shadows a BST global; track for clearing */
		if (x_base_isset(p_base)
			&& x_alist_bst_lookup(p_base,
				x_base_field_env_global_tree(p_base),
				p_params) != NULL) {
			if ( ! (x_obj_flags(p_params) & X_OBJ_FLAG_SHADOW)) {
				x_obj_flags(p_params) |= X_OBJ_FLAG_SHADOW;
				x_base_field_shadow_list(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
					p_params, x_base_field_shadow_list(p_base));
			}
		}

		return x_mkspair(p_base, X_OBJ_FLAG_NONE, p_pair, p_env);
	}

	/* Base case: no more params. */
	if (x_obj_isnil(p_base, p_params)) {
		return p_env;
	}

	/* Recursive case: bind first param to first val, continue. */
	{
		x_obj_t *p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_firstobj(p_params), x_firstobj(p_vals));

		/* Flag if param shadows a BST global; track for clearing */
		if (x_base_isset(p_base)
			&& x_obj_type_issymbol(p_base, x_firstobj(p_params))
			&& x_alist_bst_lookup(p_base,
				x_base_field_env_global_tree(p_base),
				x_firstobj(p_params)) != NULL) {
			if ( ! (x_obj_flags(x_firstobj(p_params)) & X_OBJ_FLAG_SHADOW)) {
				x_obj_flags(x_firstobj(p_params)) |= X_OBJ_FLAG_SHADOW;
				x_base_field_shadow_list(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
					x_firstobj(p_params),
					x_base_field_shadow_list(p_base));
			}
		}

		return x_env_extend(p_base,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, p_pair, p_env),
			x_restobj(p_params),
			x_restobj(p_vals));
	}
}

/**
 * Evaluate a body (list of expressions) sequentially, returning the last result.
 *
 * Each expression is rooted on the eval-list before evaluation so the
 * GC does not collect the remaining body. No tail-call optimization.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_body  x_obj_t* -- List of body expressions
 * @return x_obj_t* -- Result of the last expression, or NULL if empty
 *
 * @note When X_COV is defined, marks each body cell with X_OBJ_FLAG_COV.
 */
x_obj_t *x_eval_body(x_obj_t *p_base, x_obj_t *p_body)
{
	x_obj_t *p_result = NULL;

	while ( ! x_obj_isnil(p_base, p_body)) {
#ifdef X_COV
		x_obj_flags(p_body) |= X_OBJ_FLAG_COV;
#endif
		/* Root body so GC doesn't free remaining exprs */
		x_base_field_eval_list(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
			p_body, x_base_field_eval_list(p_base));

		p_result = x_eval_arg(p_base, x_firstobj(p_body));

		x_base_field_eval_list(p_base)
			= x_restobj(x_base_field_eval_list(p_base));

		p_body = x_restobj(p_body);
	}

	return p_result;
}

/**
 * Evaluate a body with full tail-call optimization.
 *
 * Non-tail expressions are evaluated normally. The tail (last)
 * expression is stored in the TCO expr slot instead of being evaluated
 * directly, and the caller's saved environment is captured in
 * tco-env so the trampoline can restore it after the tail call.
 *
 * On early exit (nil tail) or empty body, pops and restores the
 * compound save-stack frame (env, boundary, BST, shadow list).
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_body  x_obj_t* -- List of body expressions
 * @return x_obj_t* -- Result of non-tail expressions, or NULL when
 *                      tail expression is deferred to the trampoline
 *
 * @note When X_COV is defined, marks each body cell with X_OBJ_FLAG_COV.
 * @see x_eval_tco_trampoline
 */
x_obj_t *x_eval_body_tco(x_obj_t *p_base, x_obj_t *p_body)
{
	x_obj_t *p_result = NULL;

	while ( ! x_obj_isnil(p_base, p_body)) {
#ifdef X_COV
		x_obj_flags(p_body) |= X_OBJ_FLAG_COV;
#endif
		if (x_obj_isnil(p_base, x_restobj(p_body))) {
			x_firstobj(x_base_field_tco_expr(p_base)) = x_firstobj(p_body);

			if (x_obj_isnil(p_base,
				x_firstobj(x_base_field_tco_expr(p_base)))) {
				/* Pop compound ((env . boundary) . (bst . shadow)) and restore */
				{
					x_obj_t *p_saved = x_firstobj(x_base_field_save_stack(p_base));
					x_firstobj(x_base_field_env_alist(p_base)) = x_firstobj(x_firstobj(p_saved));
					x_base_field_env_local_boundary(p_base)
						= x_restobj(x_firstobj(p_saved));
					x_base_field_env_global_tree(p_base)
						= x_firstobj(x_restobj(p_saved));
					x_prim_clear_shadows_to(p_base,
						x_restobj(x_restobj(p_saved)));
					x_base_field_save_stack(p_base)
						= x_restobj(x_base_field_save_stack(p_base));
				}
				return NULL;
			}

			if (x_obj_isnil(p_base,
				x_firstobj(x_base_field_tco_env(p_base)))) {
				/* Save compound (env . boundary) for TCO restore */
				x_firstobj(x_base_field_tco_env(p_base))
					= x_firstobj(x_base_field_save_stack(p_base));
			}

			/* Pop save-stack */
			x_base_field_save_stack(p_base)
				= x_restobj(x_base_field_save_stack(p_base));

			return NULL;
		}

		/* Root body so GC doesn't free remaining exprs */
		x_base_field_eval_list(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
			p_body, x_base_field_eval_list(p_base));

		p_result = x_eval_arg(p_base, x_firstobj(p_body));

		x_base_field_eval_list(p_base)
			= x_restobj(x_base_field_eval_list(p_base));

		p_body = x_restobj(p_body);
	}

	/* Pop compound ((env . boundary) . (bst . shadow)) and restore */
	{
		x_obj_t *p_saved = x_firstobj(x_base_field_save_stack(p_base));
		x_firstobj(x_base_field_env_alist(p_base)) = x_firstobj(x_firstobj(p_saved));
		x_base_field_env_local_boundary(p_base) = x_restobj(x_firstobj(p_saved));
		x_base_field_env_global_tree(p_base)
			= x_firstobj(x_restobj(p_saved));
		x_prim_clear_shadows_to(p_base, x_restobj(x_restobj(p_saved)));
		x_base_field_save_stack(p_base)
			= x_restobj(x_base_field_save_stack(p_base));
	}

	return p_result;
}

/**
 * Evaluate a body with simple (lightweight) tail-call optimization.
 *
 * Like x_eval_body_tco but without save-stack management. The tail
 * expression is stored in the TCO expr slot; no environment
 * save/restore is performed. Used by forms that do not alter the
 * environment (e.g. @c if, @c do).
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_body  x_obj_t* -- List of body expressions
 * @return x_obj_t* -- Result of non-tail expressions, or NULL when
 *                      tail expression is deferred
 *
 * @note When X_COV is defined, marks each body cell with X_OBJ_FLAG_COV.
 * @see x_eval_body_tco
 */
x_obj_t *x_eval_body_tco_simple(x_obj_t *p_base, x_obj_t *p_body)
{
	x_obj_t *p_result = NULL;

	while ( ! x_obj_isnil(p_base, p_body)) {
#ifdef X_COV
		x_obj_flags(p_body) |= X_OBJ_FLAG_COV;
#endif
		if (x_obj_isnil(p_base, x_restobj(p_body))) {
			x_firstobj(x_base_field_tco_expr(p_base)) = x_firstobj(p_body);
			return NULL;
		}

		p_result = x_eval_arg(p_base, x_firstobj(p_body));
		p_body = x_restobj(p_body);
	}

	return p_result;
}

/**
 * TCO trampoline: repeatedly evaluate deferred tail expressions.
 *
 * After a TCO-aware body defers its tail expression, this loop
 * evaluates it. If that evaluation itself defers another tail call,
 * the loop continues until no more TCO expressions remain.
 *
 * On exit, restores the environment, local boundary, global BST, and
 * shadow list from the compound saved in tco-env.
 *
 * @param p_base   x_obj_t* -- Execution context
 * @param p_result x_obj_t* -- Initial result (from non-tail evaluation)
 * @return x_obj_t* -- Final evaluation result
 *
 * @see x_eval_body_tco
 */
x_obj_t *x_eval_tco_trampoline(x_obj_t *p_base, x_obj_t *p_result)
{
	x_obj_t *p_tco, *p_tco_env;

	p_tco_env = NULL;

	while ( ! x_obj_isnil(p_base, x_firstobj(x_base_field_tco_expr(p_base)))) {
		p_tco = x_firstobj(x_base_field_tco_expr(p_base));

		if ( ! x_obj_isnil(p_base, x_firstobj(x_base_field_tco_env(p_base)))) {
			p_tco_env = x_firstobj(x_base_field_tco_env(p_base));
		}

		x_firstobj(x_base_field_tco_expr(p_base)) = NULL;
		x_firstobj(x_base_field_tco_env(p_base)) = NULL;
		p_result = x_eval_arg(p_base, p_tco);
	}

	/* Restore env + boundary + bst + shadow from compound
	 * ((env . boundary) . (bst . shadow_head)) */
	if (p_tco_env != NULL && ! x_obj_isnil(p_base, p_tco_env)) {
		x_firstobj(x_base_field_env_alist(p_base)) = x_firstobj(x_firstobj(p_tco_env));
		x_base_field_env_local_boundary(p_base) = x_restobj(x_firstobj(p_tco_env));
		x_base_field_env_global_tree(p_base)
			= x_firstobj(x_restobj(p_tco_env));
		x_prim_clear_shadows_to(p_base, x_restobj(x_restobj(p_tco_env)));
	}

	return p_result;
}

/**
 * Bind a C primitive function into the global environment.
 *
 * Creates a symbol, wraps the C function as a prim object, pairs
 * them, extends the env alist, and inserts into the global BST.
 * Updates the local boundary to mark the new binding as global.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param name    x_char_t* -- Symbol name to bind
 * @param fn      x_fn_t -- C function pointer
 *
 * @see x_callable_bind_table
 */
void x_callable_bind(x_obj_t *p_base, x_char_t *name, x_fn_t fn)
{
	x_obj_t *p_sym = x_make_symbol(p_base, X_OBJ_FLAG_NONE, name),
		*p_prim = x_mkprim(p_base, fn),
		*p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_sym, p_prim);

	x_base_env_alist_extend(p_base, p_pair);

	/* Insert into global BST and update boundary */
	x_base_field_env_global_tree(p_base) = x_alist_bst_insert(
		p_base, x_base_field_env_global_tree(p_base), p_pair);
	x_base_field_env_local_boundary(p_base)
		= x_firstobj(x_base_field_env_alist(p_base));
}

/**
 * Bind an array of C primitive entries into the global environment.
 *
 * Iterates @p table and calls x_callable_bind for each entry.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param table   const x_callable_entry_t* -- Array of (name, fn) entries
 * @param count   int -- Number of entries
 *
 * @see x_callable_bind
 */
void x_callable_bind_table(x_obj_t *p_base, const x_callable_entry_t *table, int count)
{
	int i;

	for (i = 0; i < count; i++) {
		x_callable_bind(p_base, table[i].name, table[i].fn);
	}
}

/**
 * Register all built-in primitives into the environment.
 *
 * Binds the #t and #f boolean singletons, caches them in the base
 * object, then delegates to each primitive module's register function
 * (core, quote, binding, closure, control, arith, pred, string, io,
 * type, ffi, callcc).
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Unused
 * @return x_obj_t* -- p_base
 */
x_obj_t *x_prim_register(x_obj_t *p_base, x_obj_t *p_args)
{
	/* Bind #t and #f as boolean singletons, cache in base. */
	{
		x_obj_t *p_t = (x_obj_t *)&x_true_obj,
			*p_f = (x_obj_t *)&x_false_obj,
			*p_pair;

		p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mksymbol(p_base, x_atomstr(x_true_obj)), p_t);
		x_base_env_alist_extend(p_base, p_pair);
		x_base_field_env_global_tree(p_base) = x_alist_bst_insert(
			p_base, x_base_field_env_global_tree(p_base), p_pair);
		x_base_field_env_local_boundary(p_base)
			= x_firstobj(x_base_field_env_alist(p_base));
		x_firstobj(x_base_field_true(p_base)) = p_t;

		p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mksymbol(p_base, x_atomstr(x_false_obj)), p_f);
		x_base_env_alist_extend(p_base, p_pair);
		x_base_field_env_global_tree(p_base) = x_alist_bst_insert(
			p_base, x_base_field_env_global_tree(p_base), p_pair);
		x_base_field_env_local_boundary(p_base)
			= x_firstobj(x_base_field_env_alist(p_base));
		x_firstobj(x_base_field_false(p_base)) = p_f;
	}

	x_prim_core_register(p_base, p_args);
	x_syntax_quote_register(p_base, p_args);
	x_syntax_binding_register(p_base, p_args);
	x_syntax_closure_register(p_base, p_args);
	x_syntax_control_register(p_base, p_args);
	x_prim_arith_register(p_base, p_args);
	x_prim_pred_register(p_base, p_args);
	x_prim_string_register(p_base, p_args);
	x_prim_io_register(p_base, p_args);
	x_prim_type_register(p_base, p_args);
	x_prim_ffi_register(p_base, p_args);
	x_prim_callcc_register(p_base, p_args);

	return p_base;
}
