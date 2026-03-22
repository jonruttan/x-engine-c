/*
 * # Computational Expressions in C
 *
 * ## x-type/operative.c -- Implementation - Type - Operative
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
#include "x-type/operative.h"
#include "x-base.h"
#include "x-prim.h"

static x_satom_t x_type_operative_units_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = 2 });

x_satom_t x_type_operative_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_OPERATIVE_NAME }),
	x_type_operative_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_operative_make }),
	x_type_operative_call_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_operative_call }),
	x_type_operative_write_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_operative_write }),
	x_type_operative_struct_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_operative_struct });

x_obj_t *x_make_operative(x_obj_t *p_base, x_obj_flag_t flags,
	x_obj_t *p_params, x_obj_t *p_envparam, x_obj_t *p_body, x_obj_t *p_env)
{
	/* Build state list: (params . (envparam . (body . env)))
	 * GC marks via p_units=2 fallback which walks slot 1 (state). */
	x_obj_t *p_type = x_type_operative_register(p_base, p_base),
		*p_s3 = x_mkspair(p_base, p_body, p_env),
		*p_s2 = x_mkspair(p_base, p_envparam, p_s3),
		*p_state = x_mkspair(p_base, p_params, p_s2);

	return x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_PAIR,
		(x_prim_fn)x_type_operative_call, p_state);
}

x_obj_t *x_type_operative_struct(x_obj_t *p_base, x_obj_t *p_args)
{
	struct x_type_t type = {
		.p_name = x_type_operative_name,
		.p_units = (x_obj_t *)&x_type_operative_units_obj,
		.p_make = x_type_operative_make_prim,
		.p_call = x_type_operative_call_prim,
		.p_write = x_type_operative_write_prim
	};

	return x_type_struct_make(p_base, type);
}

x_obj_t *x_type_operative_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_operative_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_operative_struct_prim }, { NULL })
	};

	return x_type_struct_get(p_base, (x_obj_t *)args);
}

x_obj_t *x_type_operative_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_params = x_0(p_args),
		*p_envparam = x_01(p_args),
		*p_body = x_011(p_args),
		*p_env = x_0111(p_args);
	x_obj_flag_t flags = x_obj_isnil(p_base, x_1111(p_args))
		? 0 : x_firstint(x_0(x_1111(p_args)));

	return x_make_operative(p_base, flags,
		x_firstobj(p_params), x_firstobj(p_envparam),
		x_firstobj(p_body), x_firstobj(p_env));
}

x_obj_t *x_type_operative_call(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_op = x_firstobj(p_args),
		*p_unevaluated_args = x_restobj(p_args),
		*p_params = x_opparams(p_op),
		*p_envparam = x_openvparam(p_op),
		*p_body = x_opbody(p_op),
		*p_caller_env,
		*p_env;

	/* Capture the caller's environment. */
	p_caller_env = x_base_field_env_alist(p_base);

	/* Operatives do NOT get self-passing — they use dynamic scoping
	 * and need source-form stability for compile-on-first-use (and/or). */

	/* Extend caller's env with param bindings (unevaluated args).
	 * Operatives use dynamic scoping — body runs in caller's context
	 * so that def/set naturally affect the caller's environment. */
	p_env = x_prim_multiple_extend(
		p_base, p_caller_env, p_params, p_unevaluated_args);

	/* Bind the env-param to the caller's environment. */
	if ( ! x_obj_isnil(p_base, p_envparam)) {
		p_env = x_mkspair(p_base,
			x_mkspair(p_base, p_envparam, p_caller_env), p_env);
	}

	x_base_field_env_alist(p_base) = p_env;

	return x_prim_body_eval_tco_simple(p_base, p_body);
}

x_obj_t *x_type_operative_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_satom_t str = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .s = (x_char_t *)X_TYPE_OPERATIVE_WRITE_STR });
	x_spair_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { str }, { NULL });

	x_base_write_str(p_base, (x_obj_t *)&wrap);

	return x_firstobj(p_args);
}
