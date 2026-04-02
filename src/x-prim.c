/*
 * # Computational Expressions in C
 *
 * ## x-prim.c -- Implementation - Primitives
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
#include "x-syntax.h"
#include "x-alist.h"
#include "x-base-typesystem.h"
#include "x-eval.h"
#include "x-type/list.h"
#include "x-type/prim.h"
#include "x-type/symbol.h"

/*
 * # Helpers
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

void x_prim_clear_shadows_to(x_obj_t *p_base, x_obj_t *p_old)
{
	x_obj_t *p_list = x_base_field_shadow_list(p_base);

	while (p_list != p_old && ! x_obj_isnil(p_base, p_list)) {
		x_obj_flags(x_firstobj(p_list)) &= ~X_OBJ_FLAG_SHADOW;
		p_list = x_restobj(p_list);
	}
	x_base_field_shadow_list(p_base) = p_old;
}

x_obj_t *x_eval_arg(x_obj_t *p_base, x_obj_t *p_arg)
{
	x_satom_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_arg });
	x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { wrap }, { NULL });

	return x_eval(p_base, (x_obj_t *)args);
}

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

/*
 * # Registration
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

void x_callable_bind_table(x_obj_t *p_base, const x_callable_entry_t *table, int count)
{
	int i;

	for (i = 0; i < count; i++) {
		x_callable_bind(p_base, table[i].name, table[i].fn);
	}
}

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
