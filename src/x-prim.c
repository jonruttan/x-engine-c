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
#include "x-eval.h"
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
	x_obj_t *p_list = x_eval_field_shadow_list(p_base);

	while ( ! x_obj_isnil(p_base, p_list)) {
		x_obj_flags(x_firstobj(p_list)) &= ~X_OBJ_FLAG_SHADOW;
		p_list = x_restobj(p_list);
	}
	x_eval_field_shadow_list(p_base) = NULL;
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
 * @details Paired with x_env_extend: every time x_env_extend flags a
 *          symbol with X_OBJ_FLAG_SHADOW and pushes it onto the shadow
 *          list, a corresponding call to this function (or
 *          x_prim_clear_shadows) must eventually unflag it.  Callers
 *          save the shadow-list head before extending the env, then pass
 *          that saved head here when unwinding (TCO restore, closure
 *          exit, guard catch).  Failure to call this leaves stale shadow
 *          flags on interned symbols, causing BST lookups to skip them
 *          permanently.
 *
 * @note The walk stops when it reaches @p p_old by pointer identity.
 *       If @p p_old is NULL, all shadow entries are cleared (equivalent
 *       to x_prim_clear_shadows).
 *
 * @see x_prim_clear_shadows
 * @see x_env_extend          -- sets X_OBJ_FLAG_SHADOW on symbols
 * @see x_eval                -- calls this during outermost env restore
 * @see x_eval_body_tco       -- calls this on early-exit restore
 */
void x_prim_clear_shadows_to(x_obj_t *p_base, x_obj_t *p_old)
{
	x_obj_t *p_list = x_eval_field_shadow_list(p_base);

	while (p_list != p_old && ! x_obj_isnil(p_base, p_list)) {
		x_obj_flags(x_firstobj(p_list)) &= ~X_OBJ_FLAG_SHADOW;
		p_list = x_restobj(p_list);
	}
	x_eval_field_shadow_list(p_base) = p_old;
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
 *
 * @details **GC rooting protocol.**  Before evaluating the current
 *          element, the entire remaining arg list is pushed onto
 *          eval_list (a GC root on p_base) as a stack-allocated pair.
 *          This prevents the collector from freeing the rest of the
 *          list while x_eval_arg runs (which may trigger GC).  After
 *          evaluation, the root is popped.  The push/pop is O(1) per
 *          element, but the recursion itself is O(n) in C stack depth
 *          -- one frame per list element.  This is acceptable for
 *          argument lists (typically short) but would overflow on
 *          very long lists.
 *
 * @note Returns NULL for nil input (empty arg list), which is the
 *       identity for list construction.
 *
 * @see x_eval_arg  -- evaluates a single expression
 * @see x_eval_body -- iterative body evaluator (same GC rooting pattern)
 */
x_obj_t *x_eval_list(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_val, *p_rest;
	x_obj_t **p_cell = x_heap_root_cell(p_base);
	x_spair_t root = x_obj_set((x_obj_t *)x_type_pair_obj, X_OBJ_FLAG_NONE,
		{ NULL }, { NULL });

	if (x_obj_isnil(p_base, p_args)) {
		return NULL;
	}

	/* Root p_args so GC doesn't free rest while evaluating first; the
	 * cell's rest slot then keeps the fresh result alive across the
	 * recursion (a hold the eval-list idiom never covered -- only the
	 * conservative scan did). */
	x_firstobj((x_obj_t *)root) = p_args;
	x_heap_root_push(p_cell, root);

	p_val = x_eval_arg(p_base, x_firstobj(p_args));
	x_restobj((x_obj_t *)root) = p_val;

	p_rest = x_eval_list(p_base, x_restobj(p_args));

	x_heap_root_pop(p_cell);

	return x_mklist(p_base, p_val, p_rest);
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
 * @return x_obj_t* -- Extended environment alist (newly consed pairs)
 *
 * @details **No in-place mutation.**  Each binding creates a new
 *          (symbol . value) pair and a new alist cons cell prepended
 *          to @p p_env.  The original environment is never modified,
 *          which is essential for the TCO env-restore protocol: the
 *          saved env snapshot remains valid even after extension.
 *
 * @details **Shadow flagging.**  When a parameter name already exists
 *          in the global BST (checked via x_alist_bst_lookup), the
 *          symbol object itself is flagged X_OBJ_FLAG_SHADOW and pushed
 *          onto the shadow list.  This causes x_alist_lookup to skip
 *          BST fast-path for that symbol, forcing a linear alist walk
 *          that finds the local binding first.  The flag is cleared
 *          later by x_prim_clear_shadows_to when the scope unwinds.
 *
 * @note The variadic case (bare symbol for p_params) binds the ENTIRE
 *       remaining value list, not just one value.  This implements
 *       rest-parameter semantics: @c (fn (a . rest) ...).
 *
 * @see x_prim_clear_shadows_to -- unwinds shadow flags on scope exit
 * @see x_prim_define           -- uses the same shadow flagging for def inside closures
 * @see x_eval_body_tco         -- saves/restores env around extended scopes
 */
x_obj_t *x_env_extend(x_obj_t *p_base, x_obj_t *p_env,
	x_obj_t *p_params, x_obj_t *p_vals)
{
	x_obj_t *p_pair;
	x_obj_t *p_val;
	x_obj_t *p_rest;

	/* Variadic: single symbol binds to entire remaining arg list. */
	if ( ! x_obj_isnil(p_base, p_params)
		&& x_obj_type_issymbol(p_base, p_params)) {
		p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_params, p_vals);

		/* Flag if param shadows a BST global; track for clearing */
		if (x_base_isset(p_base)
			&& x_alist_bst_lookup(p_base,
				x_eval_field_env_global_tree(p_base),
				p_params) != NULL) {
			if ( ! (x_obj_flags(p_params) & X_OBJ_FLAG_SHADOW)) {
				x_obj_flags(p_params) |= X_OBJ_FLAG_SHADOW;
				x_eval_field_shadow_list(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
					p_params, x_eval_field_shadow_list(p_base));
			}
		}

		return x_mkspair(p_base, X_OBJ_FLAG_NONE, p_pair, p_env);
	}

	/* Base case: no more params. */
	if (x_obj_isnil(p_base, p_params)) {
		return p_env;
	}

	/* Recursive case: bind first param to first val, continue.
	 * When the args run out before the params do (fewer args than params),
	 * bind the remaining params to nil -- symmetric with surplus args, which
	 * are ignored once params run out.  Without this guard x_firstobj/
	 * x_restobj would dereference a nil p_vals and crash. */
	p_val  = x_obj_isnil(p_base, p_vals)
		? NULL : x_firstobj(p_vals);
	p_rest = x_obj_isnil(p_base, p_vals)
		? NULL : x_restobj(p_vals);
	p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_firstobj(p_params), p_val);

	/* Flag if param shadows a BST global; track for clearing */
	if (x_base_isset(p_base)
		&& x_obj_type_issymbol(p_base, x_firstobj(p_params))
		&& x_alist_bst_lookup(p_base,
			x_eval_field_env_global_tree(p_base),
			x_firstobj(p_params)) != NULL) {
		if ( ! (x_obj_flags(x_firstobj(p_params)) & X_OBJ_FLAG_SHADOW)) {
			x_obj_flags(x_firstobj(p_params)) |= X_OBJ_FLAG_SHADOW;
			x_eval_field_shadow_list(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
				x_firstobj(p_params),
				x_eval_field_shadow_list(p_base));
		}
	}

	return x_env_extend(p_base,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_pair, p_env),
		x_restobj(p_params),
		p_rest);
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
	x_obj_t **p_cell = x_heap_root_cell(p_base);
	x_spair_t root = x_obj_set((x_obj_t *)x_type_pair_obj, X_OBJ_FLAG_NONE,
		{ NULL }, { NULL });

	/* Root the advancing body so GC doesn't free remaining exprs --
	 * one registered cell for the whole walk instead of an eval-list
	 * cons per element. */
	x_heap_root_push(p_cell, root);

	while ( ! x_obj_isnil(p_base, p_body)) {
#ifdef X_COV
		x_obj_flags(p_body) |= X_OBJ_FLAG_COV;
#endif
		x_firstobj((x_obj_t *)root) = p_body;

		p_result = x_eval_arg(p_base, x_firstobj(p_body));

		p_body = x_restobj(p_body);
	}

	x_heap_root_pop(p_cell);

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
 * @details **Save-stack protocol.**  The caller (fn/let dispatch)
 *          pushes a compound pair onto save_stack BEFORE calling this
 *          function.  The compound has the shape:
 *          @code
 *          ((env-alist . local-boundary) . (global-bst . shadow-head))
 *          @endcode
 *          This captures the full env state prior to extension so it
 *          can be restored after the tail call completes.
 *
 * @details **tco_env capture.**  When the tail expression is reached
 *          (last element of body), this function checks whether
 *          tco_env is still nil.  If so, it copies the save-stack top
 *          into tco_env, providing the env snapshot that x_eval's
 *          trampoline will use for restoration.  If tco_env is already
 *          set (by a prior TCO iteration), the existing value is kept.
 *
 * @details **Save-stack pop.**  After capturing tco_env (or on early
 *          exit), the save-stack is popped.  On the normal tail-call
 *          path this is a simple pop (the trampoline in x_eval handles
 *          restore).  On early exit (nil tail or empty body), this
 *          function does a full restore from the popped frame before
 *          returning, since no trampoline iteration will follow.
 *
 * @note When X_COV is defined, marks each body cell with X_OBJ_FLAG_COV.
 *
 * @see x_eval                  -- outermost trampoline that consumes tco_expr/tco_env
 * @see x_eval_tco_trampoline   -- standalone trampoline for closure call paths
 * @see x_eval_body_tco_simple  -- lightweight variant without save-stack management
 * @see x_prim_clear_shadows_to -- called during early-exit restore
 */
x_obj_t *x_eval_body_tco(x_obj_t *p_base, x_obj_t *p_body)
{
	x_obj_t *p_result = NULL;
	x_obj_t **p_cell = x_heap_root_cell(p_base);
	x_spair_t root = x_obj_set((x_obj_t *)x_type_pair_obj, X_OBJ_FLAG_NONE,
		{ NULL }, { NULL });

	/* Root the advancing body so GC doesn't free remaining exprs (one
	 * cell for the walk; popped on every exit path). */
	x_heap_root_push(p_cell, root);

	while ( ! x_obj_isnil(p_base, p_body)) {
#ifdef X_COV
		x_obj_flags(p_body) |= X_OBJ_FLAG_COV;
#endif
		if (x_obj_isnil(p_base, x_restobj(p_body))) {
			x_firstobj(x_eval_field_tco_expr(p_base)) = x_firstobj(p_body);

			if (x_obj_isnil(p_base,
				x_firstobj(x_eval_field_tco_expr(p_base)))) {
				/* Nil tail: restore from save-stack top and pop. */
				x_tco_restore(p_base,
					x_firstobj(x_eval_field_save_stack(p_base)));
				x_eval_field_save_stack(p_base)
					= x_restobj(x_eval_field_save_stack(p_base));
				x_heap_root_pop(p_cell);
				return NULL;
			}

			if (x_obj_isnil(p_base,
				x_firstobj(x_eval_field_tco_env(p_base)))) {
				/* Save compound (env . boundary) for TCO restore */
				x_firstobj(x_eval_field_tco_env(p_base))
					= x_firstobj(x_eval_field_save_stack(p_base));
			}

			/* Pop save-stack */
			x_eval_field_save_stack(p_base)
				= x_restobj(x_eval_field_save_stack(p_base));

			x_heap_root_pop(p_cell);
			return NULL;
		}

		x_firstobj((x_obj_t *)root) = p_body;

		p_result = x_eval_arg(p_base, x_firstobj(p_body));

		p_body = x_restobj(p_body);
	}

	/* Empty body: restore from save-stack top and pop. */
	x_tco_restore(p_base, x_firstobj(x_eval_field_save_stack(p_base)));
	x_eval_field_save_stack(p_base)
		= x_restobj(x_eval_field_save_stack(p_base));

	x_heap_root_pop(p_cell);

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
			x_firstobj(x_eval_field_tco_expr(p_base)) = x_firstobj(p_body);
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
	x_obj_t *p_tco, *p_te, *p_tco_env = NULL, *p_op_save = NULL;
	int op_outermost = 0, kept_any = 0;
	int is_op;
	int has_proc, has_op;

	while ( ! x_obj_isnil(p_base, x_firstobj(x_eval_field_tco_expr(p_base)))) {
		p_tco = x_firstobj(x_eval_field_tco_expr(p_base));

		/* Keep the first procedure compound and the first operative record,
		 * distinguished by the op tag, and note which was kept first (mirrors
		 * x_eval's trampoline). */
		p_te = x_firstobj(x_eval_field_tco_env(p_base));
		if ( ! x_obj_isnil(p_base, p_te)) {
			is_op = (x_firstobj(p_te) == (x_obj_t *)&x_tco_op_tag);

			if ( ! kept_any) { op_outermost = is_op; kept_any = 1; }
			if (is_op) {
				if (p_op_save == NULL || x_obj_isnil(p_base, p_op_save))
					p_op_save = p_te;
			} else if (p_tco_env == NULL || x_obj_isnil(p_base, p_tco_env)) {
				p_tco_env = p_te;
			}
		}

		x_firstobj(x_eval_field_tco_expr(p_base)) = NULL;
		x_firstobj(x_eval_field_tco_env(p_base)) = NULL;
		p_result = x_eval_arg(p_base, p_tco);
	}

	/* Apply the two channels in REVERSE capture order so the outermost frame
	 * (captured first) wins env-alist; see x_eval for the rationale.  has_proc
	 * forces the op record's env restore to the caller (applied-procedure tail). */
	has_proc = (p_tco_env != NULL && ! x_obj_isnil(p_base, p_tco_env));
	has_op = (p_op_save != NULL && ! x_obj_isnil(p_base, p_op_save));

	if (op_outermost) {
		if (has_proc)
			x_tco_restore(p_base, p_tco_env);
		if (has_op)
			x_op_restore(p_base, p_op_save, has_proc);
	} else {
		if (has_op)
			x_op_restore(p_base, p_op_save, has_proc);
		if (has_proc)
			x_tco_restore(p_base, p_tco_env);
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
 * @details **Symbol interning.**  x_make_symbol interns the name so
 *          that all references to the same name share a single symbol
 *          object.  This enables O(1) pointer-identity comparison in
 *          alist and BST lookups.
 *
 * @details **Dual-index insertion.**  The (symbol . prim) pair is
 *          prepended to the env alist AND inserted into the global BST.
 *          The BST provides O(log n) lookup for global bindings; the
 *          alist provides the authoritative ordered list.  The
 *          local-boundary pointer is advanced to the new alist head,
 *          marking all prior entries as global (below the boundary).
 *
 * @note Called during interpreter bootstrap (x_prim_register) and
 *       by FFI registration.  All bindings created here are permanent
 *       globals that survive scope changes.
 *
 * @see x_value_bind          -- lower-level helper that binds any value
 * @see x_callable_bind_table -- batch registration of multiple primitives
 * @see x_prim_define         -- x-lang-level def with similar BST insertion
 */
void x_callable_bind(x_obj_t *p_base, x_char_t *name, x_fn_t fn)
{
	x_obj_t *p_sym = x_make_symbol(p_base, X_OBJ_FLAG_NONE, name),
		*p_prim = x_mkprim(p_base, fn),
		*p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_sym, p_prim);

	x_eval_env_alist_extend(p_base, p_pair);

	x_eval_field_env_global_tree(p_base) = x_alist_bst_insert(
		p_base, x_eval_field_env_global_tree(p_base), p_pair);
	x_eval_field_env_local_boundary(p_base)
		= x_firstobj(x_eval_field_env_alist(p_base));
}

/**
 * Bind a named symbol to an arbitrary value in the global environment.
 *
 * Creates a (symbol . value) pair, prepends it to the env alist,
 * inserts it into the global BST, and advances the local boundary.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param name    x_char_t* -- Symbol name to bind
 * @param p_val   x_obj_t* -- Value to bind
 *
 * @see x_callable_bind -- convenience wrapper for binding C primitives
 */
void x_value_bind(x_obj_t *p_base, x_char_t *name, x_obj_t *p_val)
{
	x_obj_t *p_sym, *p_pair;
	x_obj_t **p_cell = x_heap_root_cell(p_base);
	x_spair_t root = x_obj_set((x_obj_t *)x_type_pair_obj, X_OBJ_FLAG_NONE,
		{ NULL }, { NULL });

	/* Root p_val while x_make_symbol / x_mkspair allocate.  Allocation
	 * itself cannot collect (the trigger is explicit-only), but the
	 * registered cell keeps this frame correct should that policy ever
	 * change -- at two stores instead of the eval-list cons it replaced. */
	x_firstobj((x_obj_t *)root) = p_val;
	x_heap_root_push(p_cell, root);
	p_sym = x_make_symbol(p_base, X_OBJ_FLAG_NONE, name);
	p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_sym, p_val);
	x_heap_root_pop(p_cell);

	x_eval_env_alist_extend(p_base, p_pair);

	/* Insert into global BST and update boundary */
	x_eval_field_env_global_tree(p_base) = x_alist_bst_insert(
		p_base, x_eval_field_env_global_tree(p_base), p_pair);
	x_eval_field_env_local_boundary(p_base)
		= x_firstobj(x_eval_field_env_alist(p_base));
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

/*
 * # Primitives catalog
 *
 * Stored in the base's prims slot as an alist-of-alists
 * ((type . ((method . #<prim>) ...)) ...) keyed by type/section namespace,
 * with bare method names.  Built at registration time alongside the env
 * bindings; it becomes the source the (lean) env is populated from once
 * de-registration lands, and later migrates onto the type objects as static
 * methods.  Namespace/method names are interned, so lookups compare by pointer.
 */

/** The catalog value (car of the prims cell); nil before any registration. */
x_obj_t *x_prims(x_obj_t *p_base)
{
	return x_firstobj(x_eval_field_prims(p_base));
}

/* The (ns . methods) domain pair for p_ns, or NULL.  ns is interned. */
static x_obj_t *x_prims_domain_pair(x_obj_t *p_base, x_obj_t *p_ns)
{
	x_obj_t *p_cur = x_prims(p_base);

	while ( ! x_obj_isnil(p_base, p_cur)) {
		if (x_firstobj(x_firstobj(p_cur)) == p_ns)
			return x_firstobj(p_cur);
		p_cur = x_restobj(p_cur);
	}
	return NULL;
}

x_obj_t *x_prims_domain(x_obj_t *p_base, x_obj_t *p_ns)
{
	x_obj_t *p_dom = x_prims_domain_pair(p_base, p_ns);

	return p_dom == NULL ? NULL : x_restobj(p_dom);
}

x_obj_t *x_prims_ref(x_obj_t *p_base, x_obj_t *p_ns, x_obj_t *p_method)
{
	x_obj_t *p_cur = x_prims_domain(p_base, p_ns);

	while ( ! x_obj_isnil(p_base, p_cur)) {
		if (x_firstobj(x_firstobj(p_cur)) == p_method)
			return x_restobj(x_firstobj(p_cur));
		p_cur = x_restobj(p_cur);
	}
	return NULL;
}

/* Splice one (method . value) entry into p_ns's catalog domain, creating the
 * domain on first use.  The shared filing core of C-side registration
 * (x_prims_add) and x-lang-side registration (prim-reg!).  p_ns and p_entry
 * must already be pinned by the caller -- the conses here can trigger GC.
 *
 * Conses are built with x_mklist (the list-pair type), NOT x_mkspair: the
 * catalog must be an ordinary iterable x-lang list so (prims) supports pair?,
 * map, fold, etc. -- the x-lang catalog->methods mapping walks it.  Structural
 * x_mkspair pairs are not pair? and segfault the iterator protocol. */
static void x_prims_file(x_obj_t *p_base, x_obj_t *p_ns, x_obj_t *p_entry)
{
	x_obj_t *p_dom, *p_methods, *p_newdom;
	x_obj_t **p_cell = x_heap_root_cell(p_base);
	x_spair_t root = x_obj_set((x_obj_t *)x_type_pair_obj, X_OBJ_FLAG_NONE,
		{ NULL }, { NULL });

	p_dom = x_prims_domain_pair(p_base, p_ns);

	if (p_dom != NULL) {
		/* Existing namespace: prepend the entry to its method alist (a
		 * re-registration shadows the older entry on lookup). */
		x_restobj(p_dom) = x_mklist(p_base,
			p_entry, x_restobj(p_dom));
	} else {
		/* New namespace: prepend (ns . (entry)) to the catalog, rooting the
		 * partial spine across each subsequent cons. */
		p_methods = x_mklist(p_base, p_entry, NULL);
		x_firstobj((x_obj_t *)root) = p_methods;
		x_heap_root_push(p_cell, root);
		p_newdom = x_mklist(p_base, p_ns, p_methods);
		x_restobj((x_obj_t *)root) = p_newdom;
		x_firstobj(x_eval_field_prims(p_base)) = x_mklist(p_base,
			p_newdom, x_prims(p_base));
		x_heap_root_pop(p_cell);
	}
}

/* File one (ns/method -> fn) entry into the catalog.  The freshly interned
 * symbols, prim, and conses are pinned on the eval-list root across the
 * allocations that follow, since they are not yet reachable from the base and
 * -O2 stack scanning is unreliable (see the gc-rooting note). */
static void x_prims_add(x_obj_t *p_base, x_char_t *ns, x_char_t *method, x_fn_t fn)
{
	x_obj_t *p_ns, *p_entry;
	x_obj_t **p_cell = x_heap_root_cell(p_base);
	x_spair_t root = x_obj_set((x_obj_t *)x_type_pair_obj, X_OBJ_FLAG_NONE,
		{ NULL }, { NULL });

	p_ns = x_make_symbol(p_base, X_OBJ_FLAG_NONE, ns);
	x_firstobj((x_obj_t *)root) = p_ns;
	x_heap_root_push(p_cell, root);

	/* (method . #<prim>) */
	p_entry = x_mklist(p_base,
		x_make_symbol(p_base, X_OBJ_FLAG_NONE, method),
		x_mkprim(p_base, fn));
	x_restobj((x_obj_t *)root) = p_entry;

	x_prims_file(p_base, p_ns, p_entry);

	x_heap_root_pop(p_cell);
}

/* Namespaces whose bare env names have been de-registered: their prims live
 * only in the catalog and the object-system classes, not as transitional bare
 * names.  Migrating call sites to the class access (e.g. (clock) -> (Sys clock))
 * lets a namespace move here; the list shrinks the env one namespace at a time,
 * and the whole de-registration check is removed once it is empty. */
static int x_prims_ns_deregistered(const x_char_t *ns)
{
	static const char *const list[] = { "sys", "iter", "base", "heap", "str", NULL };
	int i;

	if (ns == NULL)
		return 0;
	for (i = 0; list[i] != NULL; i++)
		if (x_lib_strcmp(ns, list[i]) == 0)
			return 1;
	return 0;
}

/**
 * Bind a table into the env and file its cataloged entries.
 *
 * Each entry is bound into the env by @c name (transitional, so existing code
 * keeps working) and, when it carries a namespace, filed into the catalog
 * under @c (ns . ((method . prim) ...)).  De-registration removes the env
 * binding here, leaving the catalog as the single source.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param table   const x_prim_entry_t* -- Array of entries
 * @param count   int -- Number of entries
 */
void x_prims_bind_table(x_obj_t *p_base, const x_prim_entry_t *table, int count)
{
	int i;

	for (i = 0; i < count; i++) {
		/* Bind the transitional bare name unless the namespace has been
		 * de-registered -- then the catalog/classes are the only source. */
		if ( ! x_prims_ns_deregistered(table[i].ns))
			x_callable_bind(p_base, table[i].name, table[i].fn);
		if (table[i].ns != NULL)
			x_prims_add(p_base, table[i].ns, table[i].method, table[i].fn);
	}
}

/** (prims) -- the primitives catalog, for introspection and the X fetch protocol. */
static x_obj_t *x_prim_prims(x_obj_t *p_base, x_obj_t *p_args)
{
	(void)p_args;

	return x_prims(p_base);
}

/** (prim-domain ns) -- the method alist filed under a namespace, or nil. */
static x_obj_t *x_prim_prim_domain(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_ns;

	x_eargs(p_base, p_args, 2, NULL, &p_ns);

	return x_prims_domain(p_base, p_ns);
}

/** (prim-ref ns method) -- the prim filed under ns/method, or nil. */
static x_obj_t *x_prim_prim_ref(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_ns, *p_method;

	x_eargs(p_base, p_args, 3, NULL, &p_ns, &p_method);

	return x_prims_ref(p_base, p_ns, p_method);
}

/** (prim-reg! ns method value) -- file an x-lang value into the catalog under
 *  ns/method: the producer half of the fetch protocol (prim-ref is the
 *  consumer half), so library implementations register under the same stable
 *  identities as C prims.  Registration prepends; a re-registration shadows
 *  the older entry on lookup.  Side-effecting: returns nil. */
static x_obj_t *x_prim_prim_reg(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_ns, *p_method, *p_value, *p_entry;
	x_obj_t **p_cell = x_heap_root_cell(p_base);
	x_spair_t root_a = x_obj_set((x_obj_t *)x_type_pair_obj, X_OBJ_FLAG_NONE,
		{ NULL }, { NULL });
	x_spair_t root_b = x_obj_set((x_obj_t *)x_type_pair_obj, X_OBJ_FLAG_NONE,
		{ NULL }, { NULL });

	x_eargs(p_base, p_args, 4, NULL, &p_ns, &p_method, &p_value);

	/* Pin the evaluated args across the conses below: they may be fresh
	 * allocations reachable only from C locals.  Two registered cells
	 * hold all four values (see the gc-rooting note). */
	x_firstobj((x_obj_t *)root_a) = p_ns;
	x_restobj((x_obj_t *)root_a) = p_value;
	x_heap_root_push(p_cell, root_a);
	x_firstobj((x_obj_t *)root_b) = p_method;
	x_heap_root_push(p_cell, root_b);

	/* (method . value) */
	p_entry = x_mklist(p_base, p_method, p_value);
	x_restobj((x_obj_t *)root_b) = p_entry;

	x_prims_file(p_base, p_ns, p_entry);

	x_heap_root_pop(p_cell);
	x_heap_root_pop(p_cell);

	return NULL;
}

/** (use ns) -- define ns's catalog methods into the env as @c ns/method.
 *  Qualified names keep the bulk collision-free (e.g. obj/ref vs ptr/ref vs
 *  buf/ref).  Side-effecting: returns nil.  This is the fetch+define step
 *  that replaces direct registration under the lean env. */
static x_obj_t *x_prim_use(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t  *p_ns, *p_dom;
	x_obj_t  *p_entry;
	x_obj_t **p_cell = x_heap_root_cell(p_base);
	x_spair_t root = x_obj_set((x_obj_t *)x_type_pair_obj, X_OBJ_FLAG_NONE,
		{ NULL }, { NULL });
	x_char_t *p_ns_str;
	x_char_t *p_m_str;
	x_char_t *p_name;
	size_t    ns_len;
	size_t    m_len;

	x_eargs(p_base, p_args, 2, NULL, &p_ns);
	p_ns_str = x_atomstr(p_ns);
	ns_len = x_lib_strlen(p_ns_str);

	/* Root the advancing domain across the allocations in the loop (one
	 * registered cell for the whole walk). */
	x_heap_root_push(p_cell, root);

	p_dom = x_prims_domain(p_base, p_ns);
	while ( ! x_obj_isnil(p_base, p_dom)) {
		x_firstobj((x_obj_t *)root) = p_dom;
		p_entry = x_firstobj(p_dom);       /* (method . #<prim>) */
		p_m_str = x_atomstr(x_firstobj(p_entry));
		m_len = x_lib_strlen(p_m_str);
		/* "ns/method".  x_make_symbol stores the name pointer (it does not
		 * copy unless OWN), so the name must outlive the symbol -- allocate it
		 * persistently, exactly as an interned symbol's name would live. */
		p_name = (x_char_t *)x_sys_malloc(ns_len + m_len + 2);

		x_lib_memcpy(p_name, p_ns_str, ns_len);
		p_name[ns_len] = '/';
		x_lib_memcpy(p_name + ns_len + 1, p_m_str, m_len + 1);

		/* Idempotent: define ns/method only when it is not already bound, so a
		 * later (use ns) never clobbers an intervening redefinition -- e.g.
		 * arithmetic.x's variadic int/+, set after its own (use (lit int)). */
		if (x_alist_bst_lookup(p_base, x_eval_field_env_global_tree(p_base),
				x_make_symbol(p_base, X_OBJ_FLAG_NONE, p_name)) == NULL)
			x_value_bind(p_base, p_name, x_restobj(p_entry));

		p_dom = x_restobj(p_dom);
	}

	x_heap_root_pop(p_cell);

	return NULL;
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
	x_value_bind(p_base, x_atomstr(x_true_obj), (x_obj_t *)&x_true_obj);
	x_firstobj(x_eval_field_true(p_base)) = (x_obj_t *)&x_true_obj;

	x_value_bind(p_base, x_atomstr(x_false_obj), (x_obj_t *)&x_false_obj);
	x_firstobj(x_eval_field_false(p_base)) = (x_obj_t *)&x_false_obj;

	/* Catalog access protocol -- part of the lean core (stays in the env so X
	 * code can reach the catalog to fetch+define the rest). */
	x_callable_bind(p_base, "prims", x_prim_prims);
	x_callable_bind(p_base, "prim-domain", x_prim_prim_domain);
	x_callable_bind(p_base, "prim-ref", x_prim_prim_ref);
	x_callable_bind(p_base, "prim-reg!", x_prim_prim_reg);
	x_callable_bind(p_base, "use", x_prim_use);

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
#ifdef X_SIGNAL
	x_prim_signal_register(p_base, p_args);
#endif

	return p_base;
}
