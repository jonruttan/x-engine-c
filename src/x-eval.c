/*
 * # Computational Expressions in C
 *
 * ## x-eval.c -- Implementation - Evaluator
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2021 Jon Ruttan
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
#include "x-eval.h"
#include "x-base-typesystem.h"
#include "x-obj.h"
#include "x-prim.h"
#include "x-type.h"
#include "x-type/prim.h"

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
