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
#include "x-base.h"
#include "x-eval.h"
#include "x-type/list.h"
#include "x-type/prim.h"
#include "x-type/symbol.h"

/*
 * # Helpers
 */
x_obj_t *x_prim_eval_arg(x_obj_t *p_base, x_obj_t *p_arg)
{
	x_satom_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_arg });
	x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { wrap }, { NULL });

	return x_eval(p_base, (x_obj_t *)args);
}

x_obj_t *x_prim_evlis(x_obj_t *p_base, x_obj_t *p_args)
{
	if (x_obj_isnil(p_base, p_args)) {
		return NULL;
	}

	return x_mklist(p_base,
		x_prim_eval_arg(p_base, x_firstobj(p_args)),
		x_prim_evlis(p_base, x_restobj(p_args)));
}

x_obj_t *x_prim_multiple_extend(x_obj_t *p_base, x_obj_t *p_env,
	x_obj_t *p_params, x_obj_t *p_vals)
{
	/* Variadic: single symbol binds to entire remaining arg list. */
	if ( ! x_obj_isnil(p_base, p_params)
		&& x_obj_type_issymbol(p_base, p_params)) {
		return x_mkspair(p_base,
			x_mkspair(p_base, p_params, p_vals), p_env);
	}

	/* Base case: no more params. */
	if (x_obj_isnil(p_base, p_params)) {
		return p_env;
	}

	/* Recursive case: bind first param to first val, continue. */
	return x_prim_multiple_extend(p_base,
		x_mkspair(p_base,
			x_mkspair(p_base, x_firstobj(p_params), x_firstobj(p_vals)),
			p_env),
		x_restobj(p_params),
		x_restobj(p_vals));
}

x_obj_t *x_prim_body_eval(x_obj_t *p_base, x_obj_t *p_body)
{
	x_obj_t *p_result = NULL;

	while ( ! x_obj_isnil(p_base, p_body)) {
		p_result = x_prim_eval_arg(p_base, x_firstobj(p_body));
		p_body = x_restobj(p_body);
	}

	return p_result;
}

x_obj_t *x_prim_body_eval_tco(x_obj_t *p_base, x_obj_t *p_body)
{
	x_obj_t *p_result = NULL;

	while ( ! x_obj_isnil(p_base, p_body)) {
		if (x_obj_isnil(p_base, x_restobj(p_body))) {
			x_base_field_tco_expr(p_base) = x_firstobj(p_body);

			if (x_obj_isnil(p_base,
				x_base_field_tco_expr(p_base))) {
				/* Pop save-stack and restore env */
				x_base_field_env_alist(p_base)
					= x_firstobj(x_base_field_save_stack(p_base));
				x_base_field_save_stack(p_base)
					= x_restobj(x_base_field_save_stack(p_base));
				return NULL;
			}

			if (x_obj_isnil(p_base,
				x_base_field_tco_env(p_base))) {
				x_base_field_tco_env(p_base)
					= x_firstobj(x_base_field_save_stack(p_base));
			}

			/* Pop save-stack */
			x_base_field_save_stack(p_base)
				= x_restobj(x_base_field_save_stack(p_base));

			return NULL;
		}

		p_result = x_prim_eval_arg(p_base, x_firstobj(p_body));
		p_body = x_restobj(p_body);
	}

	/* Pop save-stack and restore env */
	x_base_field_env_alist(p_base)
		= x_firstobj(x_base_field_save_stack(p_base));
	x_base_field_save_stack(p_base)
		= x_restobj(x_base_field_save_stack(p_base));

	return p_result;
}

x_obj_t *x_prim_body_eval_tco_simple(x_obj_t *p_base, x_obj_t *p_body)
{
	x_obj_t *p_result = NULL;

	while ( ! x_obj_isnil(p_base, p_body)) {
		if (x_obj_isnil(p_base, x_restobj(p_body))) {
			x_base_field_tco_expr(p_base) = x_firstobj(p_body);
			return NULL;
		}

		p_result = x_prim_eval_arg(p_base, x_firstobj(p_body));
		p_body = x_restobj(p_body);
	}

	return p_result;
}

x_obj_t *x_prim_tco_trampoline(x_obj_t *p_base, x_obj_t *p_result)
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
		p_result = x_prim_eval_arg(p_base, p_tco);
	}

	if (p_tco_env != NULL && ! x_obj_isnil(p_base, p_tco_env)) {
		x_base_field_env_alist(p_base) = p_tco_env;
	}

	return p_result;
}

/*
 * # Registration
 */
void x_prim_bind(x_obj_t *p_base, x_char_t *name, x_prim_fn fn)
{
	x_obj_t *p_sym = x_make_symbol(p_base, X_OBJ_FLAG_NONE, name),
		*p_prim = x_mkprim(p_base, fn),
		*p_pair = x_mkspair(p_base, p_sym, p_prim);

	x_base_env_alist_extend(p_base, p_pair);
}

void x_prim_bind_table(x_obj_t *p_base, const x_prim_entry_t *table, int count)
{
	int i;

	for (i = 0; i < count; i++) {
		x_prim_bind(p_base, table[i].name, table[i].fn);
	}
}

x_obj_t *x_prim_register(x_obj_t *p_base, x_obj_t *p_args)
{
	/* Bind #t and #f as boolean singletons, cache in base. */
	{
		x_obj_t *p_t = (x_obj_t *)&x_true_obj,
			*p_f = (x_obj_t *)&x_false_obj,
			*p_pair;

		p_pair = x_mkspair(p_base,
			x_mksymbol(p_base, x_atomstr(x_true_obj)), p_t);
		x_base_env_alist_extend(p_base, p_pair);
		x_base_field_true(p_base) = p_t;

		p_pair = x_mkspair(p_base,
			x_mksymbol(p_base, x_atomstr(x_false_obj)), p_f);
		x_base_env_alist_extend(p_base, p_pair);
		x_base_field_false(p_base) = p_f;
	}

	x_prim_core_register(p_base, p_args);
	x_prim_arith_register(p_base, p_args);
	x_prim_pred_register(p_base, p_args);
	x_prim_string_register(p_base, p_args);
	x_prim_io_register(p_base, p_args);
	x_prim_type_register(p_base, p_args);
	x_prim_ffi_register(p_base, p_args);

	return p_base;
}
