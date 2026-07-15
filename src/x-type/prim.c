/** @file x-type/prim.c
 *  @brief Primitive type -- construction, registration, and unified callable dispatch.
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2024 Jon Ruttan
 *  @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */

#include "x-type/prim.h"
#include "x-type/procedure.h"
#include "x-type/operative.h"
#include "x-eval.h"
#include "x-prim.h"

x_satom_t x_type_prim_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_PRIM_NAME }),
	x_type_prim_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_prim_make }),
	x_callable_call_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_callable_call }),
	x_type_prim_struct_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_prim_struct });

/**
 * Allocate a PRIMITIVE object wrapping a C function pointer.
 *
 * @param p_base  Execution context.
 * @param flags   Object flags.
 * @param fn      C function pointer to wrap.
 * @return Newly allocated PRIMITIVE object.
 */
x_obj_t *x_make_prim(x_obj_t *p_base, x_obj_flag_t flags, x_fn_t fn)
{
	x_satom_t prim = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = fn }),
		flags_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = flags });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { prim }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { flags_obj }, { NULL })
	};

	return x_type_prim_make(p_base, (x_obj_t *)args);
}

/**
 * Build the PRIMITIVE type struct descriptor.
 *
 * Populates name, make, and call hooks.
 *
 * @param p_base  Execution context.
 * @param p_args  Unused.
 * @return Type struct pair-tree for PRIMITIVE.
 */
x_obj_t *x_type_prim_struct(x_obj_t *p_base, x_obj_t *p_args)
{
	struct x_type_t type = {
		.p_name = x_type_prim_name,
		.p_make = x_type_prim_make_prim,
		.p_call = x_callable_call_prim
	};

	return x_type_struct_make(p_base, type);
}

/**
 * Register (or retrieve) the PRIMITIVE type in the type alist.
 *
 * @param p_base  Execution context.
 * @param p_args  Unused.
 * @return The registered PRIMITIVE type object.
 */
x_obj_t *x_type_prim_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_prim_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_prim_struct_prim }, { NULL })
	};

	return x_type_struct_get(p_base, (x_obj_t *)args);
}

/**
 * Type-system make handler for PRIMITIVE objects.
 *
 * Extracts the function pointer from @c p_args[0] and optional flags
 * from @c p_args[1], then allocates a pair-length object via x_obj_make().
 *
 * @param p_base  Execution context.
 * @param p_args  Argument list: (fn-atom [flags-atom]).
 * @return Newly allocated PRIMITIVE object.
 */
x_obj_t *x_type_prim_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = x_type_prim_register(p_base, p_base),
		*p_prim = x_0(p_args);
	x_obj_flag_t flags = x_obj_isnil(p_base, x_1(p_args))
		? 0 : x_firstint(x_01(p_args));

	return x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_PAIR,
		x_atomfn(p_prim), NULL);
}

/**
 * Unified call dispatch for all callable types.
 *
 * All callables store a function pointer in slot 0:
 * - Closures/operatives: fn-ptr is x_type_procedure_call / x_type_operative_call.
 * - C primitives (spair): fn-ptr is the C function itself.
 * - Type handlers (satom): fn-ptr is a type-internal handler (no self-passing).
 *
 * @param p_base  Execution context.
 * @param p_args  Argument list: (callable . args).
 * @return Result of the called function.
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

/**
 * Unified apply dispatch with TCO trampoline support.
 *
 * Like x_callable_call() but uses the non-TCO apply path for procedures
 * (args already evaluated) and a trampoline for operatives.
 *
 * @param p_base  Execution context.
 * @param p_args  Argument list: (callable . args).
 * @return Result of the applied function.
 *
 * @see x_callable_call
 * @see x_eval_tco_trampoline
 */
x_obj_t *x_callable_apply(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_fn = x_firstobj(p_args);

	/* Satom: type-internal handler — no self */
	if (x_obj_type_issatom(p_fn)) {
		return (*x_primval(p_fn))(p_base, x_restobj(p_args));
	}

	/* Procedure: non-TCO apply path (args already evaluated) */
	if (x_primval(p_fn) == (x_fn_t)x_type_procedure_call) {
		return x_type_procedure_apply(p_base, p_args);
	}

	/* Operative via apply: trampoline for TCO */
	if (x_primval(p_fn) == (x_fn_t)x_type_operative_call) {
		return x_eval_tco_trampoline(p_base,
			x_type_operative_call(p_base, p_args));
	}

	/* C prim: call through fn-ptr with (fn . args) */
	return (*x_primval(p_fn))(p_base, p_args);
}
