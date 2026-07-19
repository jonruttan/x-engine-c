/** @file binding.c
 *  @brief Syntax - Binding Forms (def, set!)
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2026 Jon Ruttan
 *  @license MIT No Attribution (MIT-0)
 */

/*     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
#include "x-prim.h"
#include "x-alist.h"
#include "x-eval.h"
#include "x-type/symbol.h"

/**
 * Define form. x-lang: (def name value)
 *
 * Binds name to the evaluated value in the current environment (fexpr --
 * name is not evaluated, value is explicitly evaluated).  At top level the
 * binding is also inserted into the BST index and the local boundary is
 * advanced.  Inside a closure, the new spine cell is marked
 * X_OBJ_FLAG_FRAME so lookup treats it as a local frame binding.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated argument list; expects (caller name value).
 * @return The evaluated value.
 *
 * @details **Eval-before-extend.**  The value expression is evaluated
 *          BEFORE the name is bound in the environment.  This means the
 *          value expression cannot reference the binding being created
 *          (no implicit self-reference).  For recursive definitions,
 *          x-lang uses a forward-declare pattern: @c (def name ())
 *          followed by @c (set! name (fn ...)).
 *
 * @details **Top-level vs closure scope.**  The save_stack depth
 *          distinguishes the two cases:
 *          - **Empty save_stack (top-level):** The (symbol . value) pair
 *            is inserted into the global BST via x_alist_bst_insert and
 *            the local-boundary is advanced to the new alist head.  This
 *            makes the binding permanently available via O(log n) BST
 *            lookup.
 *          - **Non-empty save_stack (inside closure):** No BST insertion.
 *            The new alist spine cell is marked X_OBJ_FLAG_FRAME, so the
 *            3-step lookup finds the local binding in its frame walk
 *            ahead of any same-named global (GH #47).  The binding's
 *            visibility ends with the frame chain -- no global state to
 *            unwind.
 *
 * @see x_prim_set      -- mutation form (does not create new bindings)
 * @see x_env_extend    -- parameter binding with the same FRAME marking
 * @see x_callable_bind -- C-level equivalent for primitive registration
 */
static x_obj_t *x_prim_define(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_name, *p_pair, *p_val, *p_entry;
	int toplevel;
	x_args(p_args, 2, NULL, &p_name);
	p_val = x_eval_arg(p_base, x_011(p_args));

	/* Top-level iff the save-stack is empty.  This is TRUE for fn-body
	 * defs in TAIL position (the save frame is popped before the
	 * deferred tail runs) -- the settled tail-def-binds-globally
	 * semantics that include/import and define-sugar rely on. */
	toplevel = x_base_isset(p_base)
		&& x_obj_isnil(p_base, x_eval_field_save_stack(p_base));

	/* Top-level REdefinition: update the existing BST binding in place.
	 * x_alist_bst_insert keeps the OLD pair on a key hit, so consing a
	 * fresh (name . value) would leave the BST answering with the stale
	 * value -- invisible under the old head-first lookup, load-bearing
	 * now that globals resolve through the BST (GH #47). */
	if (toplevel) {
		p_entry = x_alist_bst_lookup(p_base,
			x_eval_field_env_global_tree(p_base), p_name);
		if ( ! x_obj_isnil(p_base, p_entry)) {
			x_restobj(p_entry) = p_val;
			return p_val;
		}
	}

	p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_name, p_val);

	x_eval_env_alist_extend(p_base, p_pair);

	/* Top-level: insert into BST and advance boundary.
	 * Inside closure: mark the new spine cell as a frame cell. */
	if (toplevel) {
		x_eval_field_env_global_tree(p_base) = x_alist_bst_insert(
			p_base, x_eval_field_env_global_tree(p_base), p_pair);
		x_eval_field_env_local_boundary(p_base)
			= x_firstobj(x_eval_field_env_alist(p_base));
	} else if (x_base_isset(p_base)) {
		/* Closure-scope def: mark the new spine cell as a local frame
		 * cell so lookup prefers it over a same-named global (GH #47). */
		x_obj_flags(x_firstobj(x_eval_field_env_alist(p_base)))
			|= X_OBJ_FLAG_FRAME;
	}

	return p_val;
}

/**
 * Mutation form. x-lang: (set! name value)
 *
 * Mutates an existing binding for name to the evaluated value (fexpr --
 * name is not evaluated, value is explicitly evaluated).  Uses the same
 * 3-step lookup as x_type_symbol_eval: (1) walk the FRAME-marked frame
 * region, (2) BST lookup for globals, (3) continue the walk through the
 * remaining chain.  Signals an error if the symbol is unbound.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated argument list; expects (caller name value).
 * @return The evaluated value.
 * @note Raises "Unbound symbol" error if name has no existing binding.
 * @see x_prim_define
 */
static x_obj_t *x_prim_set(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_name, *p_val;
	x_obj_t *p_alist, *p_entry;
	/* Error-path name wrapper; filled only when the lookup misses. */
	x_satom_t sym_name;

	x_args(p_args, 2, NULL, &p_name);
	p_val = x_eval_arg(p_base, x_011(p_args));

	p_alist = x_firstobj(x_eval_field_env_alist(p_base));

	/* Step 1: walk the lexical frame region (FRAME-marked spine cells).
	 * Locals -- including enclosing-frame captures -- must win over
	 * globals, so the walk covers every frame cell before the BST is
	 * consulted (GH #47). */
	while ( ! x_obj_isnil(p_base, p_alist)
		&& (x_obj_flags(p_alist) & X_OBJ_FLAG_FRAME)) {
		if (x_firstobj(x_firstobj(p_alist)) == p_name) {
			x_restobj(x_firstobj(p_alist)) = p_val;
			return p_val;
		}
		p_alist = x_restobj(p_alist);
	}

	/* Step 2: BST lookup (globals) */
	p_entry = x_alist_bst_lookup(p_base,
		x_eval_field_env_global_tree(p_base), p_name);
	if ( ! x_obj_isnil(p_base, p_entry)) {
		x_restobj(p_entry) = p_val;
		return p_val;
	}

	/* Step 3: continue the walk through the remaining chain (bindings
	 * outside both the frame region and the BST, e.g. base-bind cells) */
	while ( ! x_obj_isnil(p_base, p_alist)) {
		if (x_firstobj(x_firstobj(p_alist)) == p_name) {
			x_restobj(x_firstobj(p_alist)) = p_val;
			return p_val;
		}
		p_alist = x_restobj(p_alist);
	}

	sym_name[X_OBJ_META_TYPE].p = (x_obj_t *)x_type_atom_obj;
	sym_name[X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_atomstr((x_obj_t *)sym_name) = x_symbolval(p_name);
	x_obj_error(p_base, "Unbound "X_TYPE_SYMBOL_NAME,
		(x_obj_t *)&sym_name);

	return NULL;
}

/**
 * Register binding syntax primitives.
 *
 * Binds: def, set!.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unused.
 * @return p_base.
 */
x_obj_t *x_syntax_binding_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_callable_entry_t entries[] = {
		{ "def", x_prim_define },
		{ "set!", x_prim_set }
	};

	x_callable_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
