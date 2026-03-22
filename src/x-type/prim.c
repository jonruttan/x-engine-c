/*
 * # Computational Expressions in C
 *
 * ## x-type.c -- Implementation - Type - Primitive
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
#include "x-type/prim.h"
#include "x-type/operative.h"
#include "x-type/procedure.h"
#include "x-base.h"
#include "x-prim.h"

x_satom_t x_type_prim_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_PRIM_NAME }),
	x_type_prim_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_prim_make }),
	x_type_prim_call_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_prim_call }),
	x_type_prim_write_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_prim_write }),
	x_type_prim_struct_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_prim_struct });

x_obj_t *x_make_prim(x_obj_t *p_base, x_obj_flag_t flags, x_prim_fn fn)
{
	x_satom_t prim = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = fn }),
		flags_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = flags });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { prim }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { flags_obj }, { NULL })
	};

	return x_type_prim_make(p_base, (x_obj_t *)args);
}

x_obj_t *x_type_prim_struct(x_obj_t *p_base, x_obj_t *p_obj)
{
	struct x_type_t type = {
		.p_name = x_type_prim_name,
		.p_make = x_type_prim_make_prim,
		.p_call = x_type_prim_call_prim,
		.p_write = x_type_prim_write_prim
	};

	return x_type_struct_make(p_base, type);
}

x_obj_t *x_type_prim_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_prim_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_prim_struct_prim }, { NULL })
	};

	return x_type_struct_get(p_base, (x_obj_t *)args);
}

x_obj_t *x_type_prim_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = x_type_prim_register(p_base, p_base),
		*p_prim = x_0(p_args);
	x_obj_flag_t flags = x_obj_isnil(p_base, x_1(p_args))
		? 0 : x_firstint(x_01(p_args));

	return x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_PAIR,
		x_atomfn(p_prim), NULL);
}

x_obj_t *x_type_prim_call(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_fn = x_firstobj(p_args);

	if (x_obj_type_isprocedure(p_base, p_fn)) {
		return x_type_procedure_call(p_base, p_args);
	}

	if (x_obj_type_isoperative(p_base, p_fn)) {
		return x_type_operative_call(p_base, p_args);
	}

	if (x_obj_type_issatom(p_fn)) {
		return (*x_primval(p_fn))(p_base, x_restobj(p_args));
	}

	{
	x_spair_t sp = x_obj_set(NULL, X_OBJ_FLAG_NONE,
		{ p_fn }, { x_restobj(p_args) });
	return (*x_primval(p_fn))(p_base, (x_obj_t *)&sp);
	}
}

x_obj_t *x_type_prim_apply(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_fn = x_firstobj(p_args);

	if (x_obj_type_isprocedure(p_base, p_fn)) {
		x_obj_t *p_result,
			*p_saved_boundary = x_base_field_env_local_boundary(p_base),
			*p_saved_bst = x_base_field_env_global_tree(p_base),
			*p_saved_flag1 = x_base_field_flag1_list(p_base);

		/* Set boundary and BST from closure */
		x_base_field_env_local_boundary(p_base) = x_procenv(p_fn);
		x_base_field_env_global_tree(p_base) = x_procbst(p_fn);

		/* Push new env onto env_alist_stack */
		{
		x_spair_t sp = x_obj_set(NULL, X_OBJ_FLAG_NONE,
			{ p_fn }, { x_restobj(p_args) });
		x_obj_t *p_apply_args = (x_obj_t *)&sp;
		x_base_field_env_alist_stack(p_base) = x_mkspair(p_base,
			x_prim_multiple_extend(p_base, x_procenv(p_fn),
				x_procparams(p_fn), p_apply_args),
			x_base_field_env_alist_stack(p_base));
		}

		p_result = x_prim_body_eval(p_base, x_procbody(p_fn));

		/* Pop env_alist_stack, restore boundary, BST, and flag1 */
		x_base_field_env_alist_stack(p_base)
			= x_restobj(x_base_field_env_alist_stack(p_base));
		x_base_field_env_local_boundary(p_base) = p_saved_boundary;
		x_base_field_env_global_tree(p_base) = p_saved_bst;
		x_prim_clear_flag1_to(p_base, p_saved_flag1);

		return p_result;
	}

	if (x_obj_type_isoperative(p_base, p_fn)) {
		return x_prim_tco_trampoline(p_base,
			x_type_operative_call(p_base, p_args));
	}

	if (x_obj_type_issatom(p_fn)) {
		return (*x_primval(p_fn))(p_base, x_restobj(p_args));
	}

	{
	x_spair_t sp = x_obj_set(NULL, X_OBJ_FLAG_NONE,
		{ p_fn }, { x_restobj(p_args) });
	return (*x_primval(p_fn))(p_base, (x_obj_t *)&sp);
	}
}

x_obj_t *x_type_prim_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_satom_t str = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .s = (x_char_t *)X_TYPE_PRIM_WRITE_STR });
	x_spair_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { str }, { NULL });

	x_base_write_str(p_base, (x_obj_t *)&wrap);

	return x_firstobj(p_args);
}
