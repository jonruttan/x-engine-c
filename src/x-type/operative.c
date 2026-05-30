/**
 * @file x-type/operative.c
 * @brief Operative (fexpr / dynamic-scope combiner) type implementation.
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2024 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
#include "x-type/operative.h"
#include "x-interp.h"
#include "x-heap.h"
#include "x-prim.h"

/**
 * GC mark callback for operative objects.
 *
 * Only marks slot 1 (state list), not slot 0 (fn ptr).
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- (object . (flags))
 * @return NULL always
 */
static x_obj_t *x_type_operative_mark(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args);
	x_obj_flag_t flags = (x_obj_flag_t)x_firstint(x_restobj(p_args));
	x_heap_tree_mark(p_base, x_obj(x_obj_data_i(p_obj, 1)), flags);
	return NULL;
}

static x_satom_t x_type_operative_mark_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_operative_mark });
static x_satom_t x_type_operative_units_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = 2 });

x_satom_t x_type_operative_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_OPERATIVE_NAME }),
	x_type_operative_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_operative_make }),
	x_type_operative_call_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_operative_call }),
	x_type_operative_write_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_operative_write }),
	x_type_operative_struct_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_operative_struct });

/**
 * Allocate a new operative on the heap.
 *
 * Builds the state list (params . (envparam . (body . env))) and stores
 * it in slot 1 of the two-unit callable layout.
 *
 * @param p_base     x_obj_t*    -- Execution context
 * @param flags      x_obj_flag_t -- Object flags
 * @param p_params   x_obj_t*    -- Formal parameter tree
 * @param p_envparam x_obj_t*    -- Environment parameter name (or nil)
 * @param p_body     x_obj_t*    -- Body expression list
 * @param p_env      x_obj_t*    -- Captured environment
 * @return Heap-allocated operative object
 */
x_obj_t *x_make_operative(x_obj_t *p_base, x_obj_flag_t flags,
	x_obj_t *p_params, x_obj_t *p_envparam, x_obj_t *p_body, x_obj_t *p_env)
{
	x_obj_t *p_type = x_type_operative_register(p_base, p_base),
		*p_s3 = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_body, p_env),
		*p_s2 = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_envparam, p_s3),
		*p_state = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_params, p_s2);

	return x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_PAIR,
		(x_fn_t)x_type_operative_call, p_state);
}

/**
 * Build the OPERATIVE type struct descriptor.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Unused
 * @return Type struct pair list
 */
x_obj_t *x_type_operative_struct(x_obj_t *p_base, x_obj_t *p_args)
{
	struct x_type_t type = {
		.p_name = x_type_operative_name,
		.p_units = (x_obj_t *)&x_type_operative_units_obj,
		.p_make = x_type_operative_make_prim,
		.p_call = x_type_operative_call_prim,
		.p_write = x_type_operative_write_prim,
		.p_mark = x_type_operative_mark_prim
	};

	return x_type_struct_make(p_base, type);
}

/**
 * Register (or retrieve) the OPERATIVE type struct on p_base.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Unused
 * @return The registered type struct object
 */
x_obj_t *x_type_operative_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_operative_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_operative_struct_prim }, { NULL })
	};

	return x_type_struct_get(p_base, (x_obj_t *)args);
}

/**
 * Type-dispatch make callback: construct an operative from x-lang args.
 *
 * Expects args: (params envparam body env [flags]).
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Construction arguments
 * @return New operative object
 */
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

/**
 * Type-dispatch call callback: evaluate an operative application.
 *
 * Operatives are lexically scoped: the body runs in the env captured at
 * (op ...) creation time, extended with the formals bound to the
 * UNEVALUATED arguments.  The caller's environment is exposed only
 * through the named env-param (when the op declares one), so that
 * (eval expr e) or (tail-eval expr e) inside the body can reach back
 * into the caller's scope explicitly.  Caller-local names cannot
 * silently shadow the globals the body relies on.  Operatives do NOT
 * get self-passing.
 *
 * Implementation: the "boundary-only" design.
 *
 * - env_alist is set to extend(captured_env, formals).  The new chain
 *   branches off the captured-env head; caller's locals are not on
 *   this chain.
 * - local_boundary is set to captured_env so symbol-lookup Step 1
 *   stops at the captured-env head (caller's locals invisible) and
 *   Step 3 walks past into older top-level entries.
 *
 * Body is run via x_eval_body (NOT a TCO body evaluator).  Each body
 * form is evaluated synchronously to completion -- tail-eval inside a
 * form still TCOs via x_eval's own trampoline, so common patterns like
 * (tail-eval (...) e) work without growing the C stack.  The op itself
 * doesn't TCO into its caller, but lib ops typically dispatch the
 * heavy work via tail-eval into the caller's env, so the op frame
 * isn't on the recursion path.
 *
 * After the body returns, env/boundary/shadow are restored with one
 * subtlety: top-level (def ...) forms run by tail-eval'd code legitimately
 * grow the caller's env_alist chain.  If env_alist's current head reaches
 * the saved head by walking rest pointers, the chain has grown along
 * the saved branch -- keep current to preserve the new defs.  If the
 * saved head isn't on the current chain, the body branched off (op's
 * own formals frame); restore to saved.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- (operative . unevaluated-args)
 * @return Result of the operative body
 */
x_obj_t *x_type_operative_call(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_op = x_firstobj(p_args),
		*p_unevaluated_args = x_restobj(p_args),
		*p_params = x_opparams(p_op),
		*p_envparam = x_openvparam(p_op),
		*p_body = x_opbody(p_op),
		*p_captured_env = x_openv(p_op),
		*p_caller_env,
		*p_env,
		*p_saved_boundary,
		*p_saved_shadow;

	/* Capture caller's env (the head the operative restores to on exit). */
	p_caller_env = x_firstobj(x_interp_field_env_alist(p_base));
	p_saved_boundary = x_interp_field_env_local_boundary(p_base);
	p_saved_shadow = x_interp_field_shadow_list(p_base);

	/* Extend captured env with formals bound to unevaluated args. */
	p_env = x_env_extend(
		p_base, p_captured_env, p_params, p_unevaluated_args);

	/* Bind the env-param to the caller's env.  Only route from body
	 * back into caller's scope (via eval/tail-eval). */
	if ( ! x_obj_isnil(p_base, p_envparam)) {
		p_env = x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, p_envparam, p_caller_env), p_env);
	}

	/* Install op's chain.  Boundary at captured_env makes caller's
	 * locals invisible to symbol lookup. */
	x_firstobj(x_interp_field_env_alist(p_base)) = p_env;
	x_interp_field_env_local_boundary(p_base) = p_captured_env;

	/* Defer the body's tail to the outer trampoline (TCO) instead of
	 * resolving it synchronously, so operative calls stay O(1) on the C
	 * stack: deep tail recursion through operatives (if/let/cond, all built
	 * on op) no longer accumulates a frame per level.  x_eval_op_body sets
	 * tco_expr and a tagged operative record in tco_env; the trampoline runs
	 * x_op_restore after the tail (unconditional env + boundary + shadow
	 * restore to the caller, BST untouched), after any procedure env restore
	 * the tail's own evaluation left behind. */
	return x_eval_op_body(p_base, p_body, p_caller_env, p_env,
		p_saved_boundary, p_saved_shadow);
}

/**
 * Type-dispatch write callback: print "#<op>".
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- (operative)
 * @return The operative object (pass-through)
 */
x_obj_t *x_type_operative_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_satom_t str = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .s = (x_char_t *)X_TYPE_OPERATIVE_WRITE_STR });
	x_spair_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { str }, { NULL });

	x_interp_write_str(p_base, (x_obj_t *)&wrap);

	return x_firstobj(p_args);
}
