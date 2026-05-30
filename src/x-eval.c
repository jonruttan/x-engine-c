/** @file x-eval.c
 *  @brief Evaluator with TCO trampoline
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2021 Jon Ruttan
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
#include "x-eval.h"
#include "x-interp.h"
#include "x-obj.h"
#include "x-prim.h"
#include "x-type.h"

#include "x-type/prim.h"

/**
 * Push the current environment state as a TCO restore compound.
 *
 * Snapshots env-alist, local-boundary, global-bst, and shadow-head into a
 * compound @c ((env . boundary) . (bst . shadow)) and pushes it onto the
 * save-stack.  Procedure calls and eval-with-env use this to capture the
 * environment before extending it; the trampoline (or x_eval_body_tco's
 * early-exit paths) restores from it via x_tco_restore().
 *
 * @param p_base  x_obj_t* -- Execution context
 * @return x_obj_t* -- The pushed compound
 * @see x_tco_restore
 */
x_obj_t *x_tco_compound_save(x_obj_t *p_base)
{
	x_obj_t *p_compound = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_firstobj(x_interp_field_env_alist(p_base)),
			x_interp_field_env_local_boundary(p_base)),
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_interp_field_env_global_tree(p_base),
			x_interp_field_shadow_list(p_base)));

	x_interp_field_save_stack(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		p_compound, x_interp_field_save_stack(p_base));

	return p_compound;
}

/**
 * Restore env-alist, local-boundary, global-bst, and shadow list from a TCO
 * compound @c ((env . boundary) . (bst . shadow)).
 *
 * Does NOT touch the save-stack -- callers that took the compound from the
 * save-stack top pop it separately.  This is the single restore used by both
 * trampoline exit points (x_eval, x_eval_tco_trampoline), x_eval_body_tco's
 * early-exit paths, and eval-with-env.
 *
 * @param p_base      x_obj_t* -- Execution context
 * @param p_compound  x_obj_t* -- Compound built by x_tco_compound_save()
 * @see x_tco_compound_save
 */
void x_tco_restore(x_obj_t *p_base, x_obj_t *p_compound)
{
	x_firstobj(x_interp_field_env_alist(p_base))
		= x_firstobj(x_firstobj(p_compound));
	x_interp_field_env_local_boundary(p_base)
		= x_restobj(x_firstobj(p_compound));
	x_interp_field_env_global_tree(p_base)
		= x_firstobj(x_restobj(p_compound));
	x_prim_clear_shadows_to(p_base, x_restobj(x_restobj(p_compound)));
}

/**
 * Evaluate an expression with tail-call optimization.
 *
 * Dispatches to the expression's type-level eval handler. If the handler
 * sets a TCO tail expression on p_base, the trampoline loop re-evaluates
 * without growing the C stack. On exit, restores the environment from
 * the saved TCO snapshot.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- (expression . env) pair
 * @return x_obj_t* -- Evaluated result, or NULL for nil
 *
 * @details **Outermost detection.**  The local @c trampolining flag starts
 *          at 0.  When a TCO tail expression is first detected on p_base,
 *          this x_eval instance sets @c trampolining = 1, claiming
 *          ownership of the trampoline loop.  Any nested x_eval called
 *          during handler dispatch will see tco_expr as cleared (this
 *          instance clears it before goto) and will therefore NOT enter
 *          the trampoline -- it returns normally and its result is
 *          discarded in favor of the deferred tail expression.
 *
 * @details **tco_expr / tco_env lifecycle.**
 *          - **Set by:** x_eval_body_tco (full TCO) stores the tail
 *            expression in tco_expr and the compound env snapshot in
 *            tco_env.  x_eval_body_tco_simple and x_prim_match store
 *            only tco_expr (tco_env stays nil -- no env change needed).
 *          - **Consumed by:** This function's trampoline loop.  On each
 *            iteration it copies tco_expr into the eval args, clears
 *            tco_expr on p_base, and jumps to eval_start.
 *          - **tco_env cleared:** Each iteration clears tco_env on p_base
 *            after snapshotting it into the local p_tco_env_save.  This
 *            prevents nested x_eval calls from seeing stale env state.
 *
 * @details **p_tco_env_save snapshot.**
 *          - Captured on first trampoline entry from tco_env on p_base.
 *          - On later iterations, if the initial snapshot was nil (set by
 *            simple forms like if/do/match) but an inner form (fn/let)
 *            now provides a non-nil tco_env, the snapshot is upgraded.
 *          - Used only at exit: the outermost x_eval restores env-alist,
 *            local-boundary, global-BST, and shadow-list from the
 *            compound pair ((env . boundary) . (bst . shadow_head)).
 *
 * @details **Nested x_eval calls do NOT restore env.**  Only the
 *          instance where @c trampolining == 1 executes the env restore
 *          block.  This is critical: a recursive x_eval (e.g. from
 *          evaluating a sub-expression inside a primitive) must not
 *          interfere with the outer trampoline's env management.
 *
 * @note Uses goto-based trampoline; only the outermost x_eval in
 *       a call chain performs env restoration.
 *
 * @see x_eval_body_tco      -- full TCO body evaluator (sets tco_expr + tco_env)
 * @see x_eval_body_tco_simple -- lightweight TCO (sets tco_expr only)
 * @see x_eval_tco_trampoline -- standalone trampoline used by closure call paths
 * @see x_prim_clear_shadows_to -- called during env restore to unwind shadow flags
 */
x_obj_t *x_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_exp;
	x_obj_t *p_tco_env_save = NULL;
	x_spair_t prim_args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL });
	int trampolining = 0;
#ifdef X_SIGNAL
	/* Interrupt-flag pointer, resolved once from the base (signal-register
	 * publishes signal.c's static atom here).  Cached so the trampoline pays
	 * a single load per iteration, and so a GC relocation of the base spine
	 * mid-eval can't invalidate it -- the target is a non-heap static. */
	x_obj_t *p_sigint = x_base_isset(p_base)
		? x_firstobj(x_interp_field_sigint(p_base)) : NULL;
#endif

eval_start:
#ifdef X_SIGNAL
	/* SIGINT: throw STOP if a guard is active.  Volatile cast forces a
	 * re-read each iteration; without it -O2 hoists it out of the loop. */
	if (p_sigint != NULL
		&& *(volatile x_int_t *)&x_atomint(p_sigint)
		&& ! x_obj_isnil(p_base,
			x_firstobj(x_interp_field_error_handler(p_base)))) {
		x_atomint(p_sigint) = 0;
		x_interp_error(p_base, "STOP", NULL);
	}
#endif
	if (x_base_isset(p_base))
		x_atomint(x_firstobj(x_interp_field_profile_evals(p_base)))++;
	p_exp = x_firstobj(x_eval_arg_exp(p_args));

	/* Update base line counter from expression's source line metadata.
	 * After this, current-line reflects the eval site (useful for errors). */
	if (p_exp != NULL && (x_obj_flags(p_exp) & X_OBJ_FLAG_META))
		x_atomint(x_firstobj(x_interp_field_line(p_base)))
			= x_obj_meta_i(p_exp, 0).i;

#ifdef X_COV
	if (p_exp != NULL)
		x_obj_flags(p_exp) |= X_OBJ_FLAG_COV;
#endif

	if (x_obj_isnil(p_base, p_exp)) {
		return NULL;
	}

	/* Differentiate simple from complex types.
	 * Guard: NULL-typed (raw stack) objects self-evaluate. */
	if (x_obj_type(p_exp) == NULL
		|| x_obj_isnil(p_base, x_obj_type(x_obj_type(p_exp)))) {
		return p_exp;
	}

	x_firstobj((x_obj_t *)prim_args) = x_type_field_eval(x_obj_type(p_exp));

	if ( ! x_obj_isnil(p_base, x_firstobj((x_obj_t *)prim_args))) {
		x_restobj((x_obj_t *)prim_args) = p_args;
		p_exp = x_callable_call(p_base, (x_obj_t *)prim_args);

		if (p_exp == p_args) {
			goto eval_start;
		}
	}

	/* TCO trampoline: re-evaluate tail expression if set. */
	if (x_base_isset(p_base)
		&& ! x_obj_isnil(p_base, x_firstobj(x_interp_field_tco_expr(p_base)))) {
		if ( ! trampolining) {
			/* First entry: snapshot tco_env and clear global
			 * so nested x_eval calls don't see it. */
			p_tco_env_save = x_firstobj(x_interp_field_tco_env(p_base));
			trampolining = 1;
		} else if ((p_tco_env_save == NULL
			|| x_obj_isnil(p_base, p_tco_env_save))
			&& ! x_obj_isnil(p_base, x_firstobj(x_interp_field_tco_env(p_base)))) {
			/* Later iteration: initial tco_env was nil (from
			 * if/do/match/and/or) but an inner form (fn/let)
			 * now provides tco_env for env restoration. */
			p_tco_env_save = x_firstobj(x_interp_field_tco_env(p_base));
		}

		x_firstobj(x_interp_field_tco_env(p_base)) = NULL;
		x_firstobj(x_eval_arg_exp(p_args)) = x_firstobj(x_interp_field_tco_expr(p_base));
		x_firstobj(x_interp_field_tco_expr(p_base)) = NULL;
		x_atomint(x_firstobj(x_interp_field_profile_tco(p_base)))++;

		goto eval_start;
	}

	/* TCO env restore: only the x_eval that trampolined restores env. */
	if (trampolining && x_base_isset(p_base)) {
		x_firstobj(x_interp_field_tco_env(p_base)) = NULL;

		if (p_tco_env_save != NULL
			&& ! x_obj_isnil(p_base, p_tco_env_save)) {
			x_tco_restore(p_base, p_tco_env_save);
		}
	}

	return p_exp;
}
