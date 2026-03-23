/*
 * # Computational Expressions in C
 *
 * ## x-type/prim.c -- Implementation - Type - Primitive
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
#include "x-type/prim.h"
#include "x-type/procedure.h"
#include "x-type/operative.h"
#include "x-base.h"
#include "x-prim.h"

x_satom_t x_type_prim_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_PRIM_NAME }),
	x_type_prim_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_prim_make }),
	x_callable_call_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_callable_call }),
	x_callable_write_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_callable_write }),
	x_type_prim_struct_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_prim_struct });

x_obj_t *x_make_prim(x_obj_t *p_base, x_obj_flag_t flags, x_callable_fn fn)
{
	x_satom_t prim = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = fn }),
		flags_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = flags });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { prim }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { flags_obj }, { NULL })
	};

	return x_type_prim_make(p_base, (x_obj_t *)args);
}

x_obj_t *x_type_prim_struct(x_obj_t *p_base, x_obj_t *p_args)
{
	struct x_type_t type = {
		.p_name = x_type_prim_name,
		.p_make = x_type_prim_make_prim,
		.p_call = x_callable_call_prim,
		.p_write = x_callable_write_prim
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

/*
 * Unified dispatch: all callables have fn-ptr in slot 0.
 * - Closures/operatives: fn-ptr = x_type_procedure_call / x_type_operative_call
 * - C prims (spair): fn-ptr = the C function
 * - Type handlers (satom): fn-ptr = type-internal handler, no self-passing
 */
x_obj_t *x_callable_call(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_fn = x_firstobj(p_args);

	/* Satom: type-internal handler (read/write/display) — no self */
	if (x_obj_type_issatom(p_fn)) {
		return (*x_primval(p_fn))(p_base, x_restobj(p_args));
	}

	/* All spair callables: call through fn-ptr with (fn . args) */
	return (*x_primval(p_fn))(p_base, p_args);
}

x_obj_t *x_callable_apply(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_fn = x_firstobj(p_args);

	/* Satom: type-internal handler — no self */
	if (x_obj_type_issatom(p_fn)) {
		return (*x_primval(p_fn))(p_base, x_restobj(p_args));
	}

	/* Procedure: non-TCO apply path (args already evaluated) */
	if (x_primval(p_fn) == (x_callable_fn)x_type_procedure_call) {
		return x_type_procedure_apply(p_base, p_args);
	}

	/* Operative via apply: trampoline for TCO */
	if (x_primval(p_fn) == (x_callable_fn)x_type_operative_call) {
		return x_eval_tco_trampoline(p_base,
			x_type_operative_call(p_base, p_args));
	}

	/* C prim: call through fn-ptr with (fn . args) */
	return (*x_primval(p_fn))(p_base, p_args);
}

x_obj_t *x_callable_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_satom_t str = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .s = (x_char_t *)X_TYPE_PRIM_WRITE_STR });
	x_spair_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { str }, { NULL });

	x_base_write_str(p_base, (x_obj_t *)&wrap);

	return x_firstobj(p_args);
}
