/*
 * # Computational Expressions in C
 *
 * ## x-type/procedure.c -- Implementation - Type - Procedure
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
#include "x-type/procedure.h"
#include "x-base.h"
#include "x-obj/prim.h"
#include "x-prim.h"

static x_satom_t x_type_procedure_units_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = 2 });

x_satom_t x_type_procedure_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_PROCEDURE_NAME }),
	x_type_procedure_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_procedure_make }),
	x_type_procedure_call_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_procedure_call }),
	x_type_procedure_write_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_procedure_write }),
	x_type_procedure_struct_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_procedure_struct });

x_obj_t *x_make_procedure(x_obj_t *p_base, x_obj_flag_t flags,
	x_obj_t *p_params, x_obj_t *p_body, x_obj_t *p_env, x_obj_t *p_bst)
{
	/* Build state list: (params . (body . (env . bst)))
	 * GC marks via p_units=2 fallback which walks slot 1 (state). */
	x_obj_t *p_type = x_type_procedure_register(p_base, p_base),
		*p_s3 = x_mkspair(p_base, p_env, p_bst),
		*p_s2 = x_mkspair(p_base, p_body, p_s3),
		*p_state = x_mkspair(p_base, p_params, p_s2);

	return x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_PAIR,
		(x_prim_fn)x_type_procedure_call, p_state);
}

x_obj_t *x_type_procedure_struct(x_obj_t *p_base, x_obj_t *p_args)
{
	struct x_type_t type = {
		.p_name = x_type_procedure_name,
		.p_units = (x_obj_t *)&x_type_procedure_units_obj,
		.p_make = x_type_procedure_make_prim,
		.p_call = x_type_procedure_call_prim,
		.p_write = x_type_procedure_write_prim
	};

	return x_type_struct_make(p_base, type);
}

x_obj_t *x_type_procedure_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_procedure_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_procedure_struct_prim }, { NULL })
	};

	return x_type_struct_get(p_base, (x_obj_t *)args);
}

x_obj_t *x_type_procedure_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_params = x_0(p_args),
		*p_body = x_01(p_args),
		*p_env = x_011(p_args),
		*p_bst = x_0111(p_args);
	x_obj_flag_t flags = x_obj_isnil(p_base, x_1111(p_args))
		? 0 : x_firstint(x_0(x_1111(p_args)));

	return x_make_procedure(p_base, flags,
		x_firstobj(p_params), x_firstobj(p_body),
		x_firstobj(p_env), x_firstobj(p_bst));
}

x_obj_t *x_type_procedure_call(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_proc = x_firstobj(p_args),
		*p_unevaluated_args = x_restobj(p_args),
		*p_evaled_args;

	/* Eval each argument in the current env. */
	p_evaled_args = x_prim_evlis(p_base, p_unevaluated_args);

	/* Wrapped combiner: dispatch to underlying combiner with eval'd args. */
	if (x_obj_flags(p_proc) & X_OBJ_FLAG_WRAP) {
		x_obj_t *p_combiner = x_procenv(p_proc),
			*p_call_args = x_mkspair(p_base, p_combiner, p_evaled_args);

		return x_obj_prim_call(p_base, p_call_args);
	}

	{
		/* Push ((env . boundary) . (bst . flag1_head)) onto save-stack */
		x_base_field_save_stack(p_base) = x_mkspair(p_base,
			x_mkspair(p_base,
				x_mkspair(p_base, x_base_field_env_alist(p_base),
				                   x_base_field_env_local_boundary(p_base)),
				x_mkspair(p_base, x_base_field_env_global_tree(p_base),
				                   x_base_field_flag1_list(p_base))),
			x_base_field_save_stack(p_base));

		/* Set boundary to closure env and BST to closure's captured BST */
		x_base_field_env_local_boundary(p_base) = x_procenv(p_proc);
		x_base_field_env_global_tree(p_base) = x_procbst(p_proc);

		/* Self-passing via stack pair (zero allocation) */
		{
		x_spair_t sp = x_obj_set(NULL, X_OBJ_FLAG_NONE,
			{ p_proc }, { p_evaled_args });
		p_evaled_args = (x_obj_t *)&sp;

		x_base_field_env_alist(p_base) = x_prim_multiple_extend(
			p_base, x_procenv(p_proc), x_procparams(p_proc),
			p_evaled_args);
		}

		return x_prim_body_eval_tco(p_base, x_procbody(p_proc));
	}
}

/* Non-TCO apply path: args already evaluated, used by (apply f args) */
x_obj_t *x_type_procedure_apply(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_proc = x_firstobj(p_args),
		*p_result,
		*p_saved_alist = x_base_field_env_alist(p_base),
		*p_saved_boundary = x_base_field_env_local_boundary(p_base),
		*p_saved_bst = x_base_field_env_global_tree(p_base),
		*p_saved_flag1 = x_base_field_flag1_list(p_base);

	/* Set boundary and BST from closure */
	x_base_field_env_local_boundary(p_base) = x_procenv(p_proc);
	x_base_field_env_global_tree(p_base) = x_procbst(p_proc);

	/* Self-passing + extend env */
	{
	x_spair_t sp = x_obj_set(NULL, X_OBJ_FLAG_NONE,
		{ p_proc }, { x_restobj(p_args) });
	x_base_field_env_alist(p_base) = x_prim_multiple_extend(
		p_base, x_procenv(p_proc), x_procparams(p_proc),
		(x_obj_t *)&sp);
	}

	p_result = x_prim_body_eval(p_base, x_procbody(p_proc));

	/* Restore env, boundary, BST, and flag1 */
	x_base_field_env_alist(p_base) = p_saved_alist;
	x_base_field_env_local_boundary(p_base) = p_saved_boundary;
	x_base_field_env_global_tree(p_base) = p_saved_bst;
	x_prim_clear_flag1_to(p_base, p_saved_flag1);

	return p_result;
}

x_obj_t *x_type_procedure_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_satom_t str = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .s = (x_char_t *)X_TYPE_PROCEDURE_WRITE_STR });
	x_spair_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { str }, { NULL });

	x_base_write_str(p_base, (x_obj_t *)&wrap);

	return x_firstobj(p_args);
}
