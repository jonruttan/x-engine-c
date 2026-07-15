/**
 * @file x-type/procedure.c
 * @brief Procedure (applicative closure) type implementation.
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2026 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
#include "x-type/procedure.h"
#include "x-eval.h"
#include "x-heap.h"
#include "x-obj/prim.h"
#include "x-prim.h"

/**
 * GC mark callback for procedure objects.
 *
 * Only marks slot 1 (state list), not slot 0 (fn ptr).
 *
 * @details
 * Procedure heap layout has two data slots:
 * @code
 *   x_obj_data_i(p_obj, 0)  =  fn_ptr   (raw C pointer, NOT a heap obj)
 *   x_obj_data_i(p_obj, 1)  =  state    (pair tree -- must be marked)
 * @endcode
 * Slot 0 is skipped because it holds a raw x_fn_t cast to x_obj_t*.
 * Passing it to x_heap_tree_mark would chase an invalid heap pointer
 * and corrupt the mark bitmap or segfault.  Only slot 1 (the state
 * pair list) contains GC-managed objects that need marking.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- (object . (flags))
 * @return NULL always
 *
 * @see x_type_procedure_call for the callable stored in slot 0
 */
static x_obj_t *x_type_procedure_mark(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args);
	x_obj_flag_t flags = (x_obj_flag_t)x_firstint(x_restobj(p_args));
	x_heap_tree_mark(p_base, x_obj(x_obj_data_i(p_obj, 1)), flags);
	return NULL;
}

static x_satom_t x_type_procedure_mark_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_procedure_mark });
static x_satom_t x_type_procedure_units_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = 2 });

x_satom_t x_type_procedure_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_PROCEDURE_NAME }),
	x_type_procedure_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_procedure_make }),
	x_type_procedure_call_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_procedure_call }),
	x_type_procedure_struct_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_procedure_struct });

/**
 * Allocate a new procedure (closure) on the heap.
 *
 * Builds the state list (params . (body . (env . bst))) and stores
 * it in slot 1 of the two-unit callable layout.
 *
 * @param p_base   x_obj_t*    -- Execution context
 * @param flags    x_obj_flag_t -- Object flags (e.g. X_OBJ_FLAG_WRAP)
 * @param p_params x_obj_t*    -- Formal parameter tree
 * @param p_body   x_obj_t*    -- Body expression list
 * @param p_env    x_obj_t*    -- Captured lexical environment
 * @param p_bst    x_obj_t*    -- Captured global BST
 * @return Heap-allocated procedure object
 */
x_obj_t *x_make_procedure(x_obj_t *p_base, x_obj_flag_t flags,
	x_obj_t *p_params, x_obj_t *p_body, x_obj_t *p_env, x_obj_t *p_bst)
{
	x_obj_t *p_type = x_type_procedure_register(p_base, p_base),
		*p_s3 = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_env, p_bst),
		*p_s2 = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_body, p_s3),
		*p_state = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_params, p_s2);

	return x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_PAIR,
		(x_fn_t)x_type_procedure_call, p_state);
}

/**
 * Build the PROCEDURE type struct descriptor.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Unused
 * @return Type struct pair list
 */
x_obj_t *x_type_procedure_struct(x_obj_t *p_base, x_obj_t *p_args)
{
	struct x_type_t type = {
		.p_name = x_type_procedure_name,
		.p_units = (x_obj_t *)&x_type_procedure_units_obj,
		.p_make = x_type_procedure_make_prim,
		.p_call = x_type_procedure_call_prim,
		.p_mark = x_type_procedure_mark_prim
	};

	return x_type_struct_make(p_base, type);
}

/**
 * Register (or retrieve) the PROCEDURE type struct on p_base.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Unused
 * @return The registered type struct object
 */
x_obj_t *x_type_procedure_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_procedure_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_procedure_struct_prim }, { NULL })
	};

	return x_type_struct_get(p_base, (x_obj_t *)args);
}

/**
 * Type-dispatch make callback: construct a procedure from x-lang args.
 *
 * Expects args: (params body env bst [flags]).
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Construction arguments
 * @return New procedure object
 */
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

/**
 * Type-dispatch call callback: evaluate a procedure application (TCO).
 *
 * For wrapped combiners (applicatives), dispatches to the underlying
 * combiner with evaluated arguments.  For plain closures, extends the
 * environment, pushes a save-stack frame, and enters the body via TCO.
 *
 * @details
 * Two dispatch paths based on X_OBJ_FLAG_WRAP:
 *
 * **Wrapped applicative** (WRAP flag set): The procedure is a thin
 * wrapper around another combiner stored in the @c env slot.  Args are
 * evaluated, then the underlying combiner is called via x_obj_prim_call.
 * This is how @c (wrap op) creates an applicative from an operative.
 *
 * **Plain closure** (no WRAP flag): Pushes a compound save-stack tuple
 * and enters the body via TCO trampoline.
 * @code
 *   save_stack entry = ((env . boundary) . (bst . shadow_head))
 *                          |       |          |        |
 *                          |       |          |        +-- shadow list head
 *                          |       |          +-- global BST root
 *                          |       +-- local env boundary pointer
 *                          +-- current env alist
 * @endcode
 * After pushing, the closure's captured env becomes the local boundary,
 * the closure's BST becomes the global tree, and env is extended with
 * param bindings.  Body is entered via x_eval_body_tco for tail-call
 * optimization -- the trampoline loop in x_eval restores the save-stack
 * frame when the TCO chain completes.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- (procedure . unevaluated-args)
 * @return Result of the procedure body
 * @see x_type_procedure_apply for the non-TCO path
 */
x_obj_t *x_type_procedure_call(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_proc = x_firstobj(p_args),
		*p_unevaluated_args = x_restobj(p_args),
		*p_evaled_args;
	x_obj_t *p_combiner;
	x_obj_t *p_call_args;
	/* Self-passing stack pair; filled at use (needs p_evaled_args). */
	x_spair_t sp;

	/* Eval each argument in the current env. */
	p_evaled_args = x_eval_list(p_base, p_unevaluated_args);

	/* Wrapped combiner: dispatch to underlying combiner with eval'd args. */
	if (x_obj_flags(p_proc) & X_OBJ_FLAG_WRAP) {
		p_combiner = x_procenv(p_proc);
		p_call_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_combiner, p_evaled_args);

		return x_obj_prim_call(p_base, p_call_args);
	}

	/* Push ((env . boundary) . (bst . shadow_head)) onto save-stack */
	x_tco_compound_save(p_base);

	/* Set boundary to closure env and BST to closure's captured BST.
	 * Skip BST swap if captured is NULL -- see x_obj_prim_call. */
	x_eval_field_env_local_boundary(p_base) = x_procenv(p_proc);
	if (x_procbst(p_proc) != NULL) {
		x_eval_field_env_global_tree(p_base) = x_procbst(p_proc);
	}

	/* Self-passing via stack pair (zero allocation).  Safe across the
	 * TCO deferral below because x_env_extend materializes this head
	 * on the heap if a bare-variadic param binds the spine itself. */
	sp[X_OBJ_META_TYPE].p = NULL;
	sp[X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)sp) = p_proc;
	x_restobj((x_obj_t *)sp) = p_evaled_args;
	p_evaled_args = (x_obj_t *)&sp;

	x_firstobj(x_eval_field_env_alist(p_base)) = x_env_extend(
		p_base, x_procenv(p_proc), x_procparams(p_proc),
		p_evaled_args);

	return x_eval_body_tco(p_base, x_procbody(p_proc));
}

/**
 * Non-TCO apply path for (apply f args).
 *
 * Arguments are already evaluated.  Saves and restores the full
 * environment state around body evaluation.
 *
 * @details
 * Unlike x_type_procedure_call, this path does NOT use the TCO
 * trampoline.  It calls x_eval_body (not x_eval_body_tco), which
 * means the C call stack grows with each nested apply.  This is
 * necessary because @c apply is called from contexts where the
 * caller needs the result immediately (e.g. map, fold, for-each).
 *
 * The four environment components are saved to local variables
 * before body evaluation and explicitly restored afterward:
 * - env alist (current bindings)
 * - local boundary (closure env pointer)
 * - global BST root
 * - shadow list (cleared back via x_prim_clear_shadows_to)
 *
 * @note Shadow list cleanup uses x_prim_clear_shadows_to which
 *       walks the shadow list and clears X_OBJ_FLAG_SHADOW from
 *       each symbol back to the saved head, ensuring BST lookups
 *       are not incorrectly bypassed after the apply returns.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- (procedure . evaluated-args)
 * @return Result of the procedure body
 * @see x_type_procedure_call for the TCO path
 */
x_obj_t *x_type_procedure_apply(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_proc = x_firstobj(p_args),
		*p_result,
		*p_saved_alist = x_firstobj(x_eval_field_env_alist(p_base)),
		*p_saved_boundary = x_eval_field_env_local_boundary(p_base),
		*p_saved_bst = x_eval_field_env_global_tree(p_base),
		*p_saved_shadow = x_eval_field_shadow_list(p_base);

	/* Set boundary and BST from closure.  Skip BST swap if captured is
	 * NULL -- see x_obj_prim_call. */
	x_eval_field_env_local_boundary(p_base) = x_procenv(p_proc);
	if (x_procbst(p_proc) != NULL) {
		x_eval_field_env_global_tree(p_base) = x_procbst(p_proc);
	}

	/* Self-passing + extend env */
	{
	x_spair_t sp = x_obj_set(NULL, X_OBJ_FLAG_NONE,
		{ p_proc }, { x_restobj(p_args) });
	x_firstobj(x_eval_field_env_alist(p_base)) = x_env_extend(
		p_base, x_procenv(p_proc), x_procparams(p_proc),
		(x_obj_t *)&sp);
	}

	p_result = x_eval_body(p_base, x_procbody(p_proc));

	/* Restore env, boundary, BST, and shadow */
	x_firstobj(x_eval_field_env_alist(p_base)) = p_saved_alist;
	x_eval_field_env_local_boundary(p_base) = p_saved_boundary;
	x_eval_field_env_global_tree(p_base) = p_saved_bst;
	x_prim_clear_shadows_to(p_base, p_saved_shadow);

	return p_result;
}
