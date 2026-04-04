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
#include "x-base-typesystem.h"
#include "x-obj.h"
#include "x-prim.h"
#include "x-type.h"
#include "x-type/prim.h"

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

eval_start:
	if (x_base_isset(p_base))
		x_atomint(x_firstobj(x_base_field_profile_evals(p_base)))++;
	p_exp = x_firstobj(x_eval_arg_exp(p_args));

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
		&& ! x_obj_isnil(p_base, x_firstobj(x_base_field_tco_expr(p_base)))) {
		if ( ! trampolining) {
			/* First entry: snapshot tco_env and clear global
			 * so nested x_eval calls don't see it. */
			p_tco_env_save = x_firstobj(x_base_field_tco_env(p_base));
			trampolining = 1;
		} else if ((p_tco_env_save == NULL
			|| x_obj_isnil(p_base, p_tco_env_save))
			&& ! x_obj_isnil(p_base, x_firstobj(x_base_field_tco_env(p_base)))) {
			/* Later iteration: initial tco_env was nil (from
			 * if/do/match/and/or) but an inner form (fn/let)
			 * now provides tco_env for env restoration. */
			p_tco_env_save = x_firstobj(x_base_field_tco_env(p_base));
		}

		x_firstobj(x_base_field_tco_env(p_base)) = NULL;
		x_firstobj(x_eval_arg_exp(p_args)) = x_firstobj(x_base_field_tco_expr(p_base));
		x_firstobj(x_base_field_tco_expr(p_base)) = NULL;
		x_atomint(x_firstobj(x_base_field_profile_tco(p_base)))++;
		goto eval_start;
	}

	/* TCO env restore: only the x_eval that trampolined restores env. */
	if (trampolining && x_base_isset(p_base)) {
		x_firstobj(x_base_field_tco_env(p_base)) = NULL;

		/* Restore from compound ((env . boundary) . (bst . shadow)) */
		if (p_tco_env_save != NULL
			&& ! x_obj_isnil(p_base, p_tco_env_save)) {
			x_firstobj(x_base_field_env_alist(p_base))
				= x_firstobj(x_firstobj(p_tco_env_save));
			x_base_field_env_local_boundary(p_base)
				= x_restobj(x_firstobj(p_tco_env_save));
			x_base_field_env_global_tree(p_base)
				= x_firstobj(x_restobj(p_tco_env_save));
			x_prim_clear_shadows_to(p_base,
				x_restobj(x_restobj(p_tco_env_save)));
		}

	}

	return p_exp;
}
