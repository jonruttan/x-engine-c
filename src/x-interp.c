/*
 * # Computational Expressions in C
 *
 * ## x-interp.c -- Implementation - Interpreter Core
 *
 * Evaluation engine: argument evaluation, environment extension,
 * body evaluation, and TCO trampoline.
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
#include "x-interp.h"
#include "x-alist.h"
#include "x-eval.h"
#include "x-type/list.h"
#include "x-type/symbol.h"

/*
 * # BST Shadow Tracking
 */
void x_clear_bst_shadows(x_obj_t *p_base)
{
	x_obj_t *p_list = x_bst_shadow_list(p_base);

	while ( ! x_obj_isnil(p_base, p_list)) {
		x_obj_flags(x_firstobj(p_list)) &= ~X_BST_SHADOW_FLAG;
		p_list = x_restobj(p_list);
	}
	x_bst_shadow_list(p_base) = NULL;
}

void x_clear_bst_shadows_to(x_obj_t *p_base, x_obj_t *p_old)
{
	x_obj_t *p_list = x_bst_shadow_list(p_base);

	while (p_list != p_old && ! x_obj_isnil(p_base, p_list)) {
		x_obj_flags(x_firstobj(p_list)) &= ~X_BST_SHADOW_FLAG;
		p_list = x_restobj(p_list);
	}
	x_bst_shadow_list(p_base) = p_old;
}

/*
 * # Argument Evaluation
 */
x_obj_t *x_interp_eval_arg(x_obj_t *p_base, x_obj_t *p_arg)
{
	x_satom_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_arg });
	x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { wrap }, { NULL });

	return x_eval(p_base, (x_obj_t *)args);
}

x_obj_t *x_interp_evlis(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_val;

	if (x_obj_isnil(p_base, p_args)) {
		return NULL;
	}

	/* Root p_args so GC doesn't free rest while evaluating first */
	x_base_field_eval_list_stack(p_base) = x_mkspair(p_base,
		p_args, x_base_field_eval_list_stack(p_base));

	p_val = x_interp_eval_arg(p_base, x_firstobj(p_args));

	x_base_field_eval_list_stack(p_base)
		= x_restobj(x_base_field_eval_list_stack(p_base));

	return x_mklist(p_base, p_val,
		x_interp_evlis(p_base, x_restobj(p_args)));
}

/*
 * # Environment Extension
 */
x_obj_t *x_interp_extend_env(x_obj_t *p_base, x_obj_t *p_env,
	x_obj_t *p_params, x_obj_t *p_vals)
{
	/* Variadic: single symbol binds to entire remaining arg list. */
	if ( ! x_obj_isnil(p_base, p_params)
		&& x_obj_type_issymbol(p_base, p_params)) {
		x_obj_t *p_pair = x_mkspair(p_base, p_params, p_vals);

		/* Flag if param shadows a BST global; track for clearing */
		if (x_base_isset(p_base)
			&& x_alist_bst_lookup(p_base,
				x_base_field_env_global_tree(p_base),
				p_params) != NULL) {
			if ( ! (x_obj_flags(p_params) & X_BST_SHADOW_FLAG)) {
				x_obj_flags(p_params) |= X_BST_SHADOW_FLAG;
				x_bst_shadow_list(p_base) = x_mkspair(p_base,
					p_params, x_bst_shadow_list(p_base));
			}
		}

		return x_mkspair(p_base, p_pair, p_env);
	}

	/* Base case: no more params. */
	if (x_obj_isnil(p_base, p_params)) {
		return p_env;
	}

	/* Recursive case: bind first param to first val, continue. */
	{
		x_obj_t *p_pair = x_mkspair(p_base,
			x_firstobj(p_params), x_firstobj(p_vals));

		/* Flag if param shadows a BST global; track for clearing */
		if (x_base_isset(p_base)
			&& x_obj_type_issymbol(p_base, x_firstobj(p_params))
			&& x_alist_bst_lookup(p_base,
				x_base_field_env_global_tree(p_base),
				x_firstobj(p_params)) != NULL) {
			if ( ! (x_obj_flags(x_firstobj(p_params)) & X_BST_SHADOW_FLAG)) {
				x_obj_flags(x_firstobj(p_params)) |= X_BST_SHADOW_FLAG;
				x_bst_shadow_list(p_base) = x_mkspair(p_base,
					x_firstobj(p_params),
					x_bst_shadow_list(p_base));
			}
		}

		return x_interp_extend_env(p_base,
			x_mkspair(p_base, p_pair, p_env),
			x_restobj(p_params),
			x_restobj(p_vals));
	}
}

/*
 * # Body Evaluation
 */
x_obj_t *x_interp_body_eval(x_obj_t *p_base, x_obj_t *p_body)
{
	x_obj_t *p_result = NULL;

	while ( ! x_obj_isnil(p_base, p_body)) {
#ifdef X_COV
		x_obj_flags(p_body) |= X_OBJ_FLAG_2;
#endif
		/* Root body so GC doesn't free remaining exprs */
		x_base_field_eval_list_stack(p_base) = x_mkspair(p_base,
			p_body, x_base_field_eval_list_stack(p_base));

		p_result = x_interp_eval_arg(p_base, x_firstobj(p_body));

		x_base_field_eval_list_stack(p_base)
			= x_restobj(x_base_field_eval_list_stack(p_base));

		p_body = x_restobj(p_body);
	}

	return p_result;
}

x_obj_t *x_interp_body_eval_tco(x_obj_t *p_base, x_obj_t *p_body)
{
	x_obj_t *p_result = NULL;

	while ( ! x_obj_isnil(p_base, p_body)) {
#ifdef X_COV
		x_obj_flags(p_body) |= X_OBJ_FLAG_2;
#endif
		if (x_obj_isnil(p_base, x_restobj(p_body))) {
			x_base_field_tco_expr(p_base) = x_firstobj(p_body);

			if (x_obj_isnil(p_base,
				x_base_field_tco_expr(p_base))) {
				x_env_restore_pop(p_base);
				return NULL;
			}

			if (x_obj_isnil(p_base,
				x_base_field_tco_env(p_base))) {
				/* Save compound (env . boundary) for TCO restore */
				x_base_field_tco_env(p_base)
					= x_firstobj(x_base_field_save_stack(p_base));
			}

			/* Pop save-stack */
			x_base_field_save_stack(p_base)
				= x_restobj(x_base_field_save_stack(p_base));

			return NULL;
		}

		/* Root body so GC doesn't free remaining exprs */
		x_base_field_eval_list_stack(p_base) = x_mkspair(p_base,
			p_body, x_base_field_eval_list_stack(p_base));

		p_result = x_interp_eval_arg(p_base, x_firstobj(p_body));

		x_base_field_eval_list_stack(p_base)
			= x_restobj(x_base_field_eval_list_stack(p_base));

		p_body = x_restobj(p_body);
	}

	/* Empty body: pop and restore */
	x_env_restore_pop(p_base);

	return p_result;
}

x_obj_t *x_interp_body_eval_tco_simple(x_obj_t *p_base, x_obj_t *p_body)
{
	x_obj_t *p_result = NULL;

	while ( ! x_obj_isnil(p_base, p_body)) {
#ifdef X_COV
		x_obj_flags(p_body) |= X_OBJ_FLAG_2;
#endif
		if (x_obj_isnil(p_base, x_restobj(p_body))) {
			x_base_field_tco_expr(p_base) = x_firstobj(p_body);
			return NULL;
		}

		p_result = x_interp_eval_arg(p_base, x_firstobj(p_body));
		p_body = x_restobj(p_body);
	}

	return p_result;
}

/*
 * # TCO Trampoline
 */
x_obj_t *x_interp_tco_trampoline(x_obj_t *p_base, x_obj_t *p_result)
{
	x_obj_t *p_tco, *p_tco_env;

	p_tco_env = NULL;

	while ( ! x_obj_isnil(p_base, x_base_field_tco_expr(p_base))) {
		p_tco = x_base_field_tco_expr(p_base);

		if ( ! x_obj_isnil(p_base, x_base_field_tco_env(p_base))) {
			p_tco_env = x_base_field_tco_env(p_base);
		}

		x_base_field_tco_expr(p_base) = NULL;
		x_base_field_tco_env(p_base) = NULL;
		p_result = x_interp_eval_arg(p_base, p_tco);
	}

	/* Restore env + boundary + bst + shadow-head from compound
	 * ((env . boundary) . (bst . shadow-head)) */
	if (p_tco_env != NULL && ! x_obj_isnil(p_base, p_tco_env)) {
		x_env_restore(p_base, p_tco_env);
	}

	return p_result;
}
