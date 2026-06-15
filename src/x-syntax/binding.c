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
 * advanced.  Inside a closure, if the name shadows a BST global the symbol
 * is flagged with X_OBJ_FLAG_SHADOW and added to the shadow list.
 *
 * @param p_base  Execution context.
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
 *            If the symbol already exists in the BST (a global with the
 *            same name), X_OBJ_FLAG_SHADOW is set on the symbol and it
 *            is pushed onto the shadow list.  This forces alist linear
 *            scan for that symbol, finding the local binding first.  The
 *            shadow flag is cleared when the closure scope unwinds via
 *            x_prim_clear_shadows_to.
 *
 * @note The shadow flag is set on the interned symbol object itself,
 *       not on the binding pair.  Since symbols are interned (shared),
 *       this globally affects all lookups for that name until the flag
 *       is cleared.
 *
 * @see x_prim_set              -- mutation form (does not create new bindings)
 * @see x_env_extend            -- parameter binding with the same shadow protocol
 * @see x_prim_clear_shadows_to -- unwinds shadow flags on scope exit
 * @see x_callable_bind         -- C-level equivalent for primitive registration
 */
static x_obj_t *x_prim_define(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_name, *p_pair, *p_val;
	x_args(p_args, 2, NULL, &p_name);
	p_val = x_eval_arg(p_base, x_011(p_args));
	p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_name, p_val);

	x_eval_env_alist_extend(p_base, p_pair);

	/* Top-level: insert into BST and advance boundary.
	 * Inside closure: flag symbol if it shadows a BST global. */
	if (x_base_isset(p_base)
		&& x_obj_isnil(p_base, x_eval_field_save_stack(p_base))) {
		x_eval_field_env_global_tree(p_base) = x_alist_bst_insert(
			p_base, x_eval_field_env_global_tree(p_base), p_pair);
		x_eval_field_env_local_boundary(p_base)
			= x_firstobj(x_eval_field_env_alist(p_base));
	} else if (x_base_isset(p_base)) {
		if (x_alist_bst_lookup(p_base,
			x_eval_field_env_global_tree(p_base), p_name) != NULL) {
			if ( ! (x_obj_flags(p_name) & X_OBJ_FLAG_SHADOW)) {
				x_obj_flags(p_name) |= X_OBJ_FLAG_SHADOW;
				x_eval_field_shadow_list(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
					p_name, x_eval_field_shadow_list(p_base));
			}
		}
	}

	return p_val;
}

/**
 * Mutation form. x-lang: (set! name value)
 *
 * Mutates an existing binding for name to the evaluated value (fexpr --
 * name is not evaluated, value is explicitly evaluated).  Uses a 3-step
 * BST-aware lookup: (1) walk locals up to and including the boundary,
 * (2) BST lookup skipping shadowed symbols, (3) continue alist walk from
 * the boundary.  Signals an error if the symbol is unbound.
 *
 * @param p_base  Execution context.
 * @param p_args  Unevaluated argument list; expects (caller name value).
 * @return The evaluated value.
 * @note Raises "Unbound symbol" error if name has no existing binding.
 * @see x_prim_define
 */
static x_obj_t *x_prim_set(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_name, *p_val;
	x_obj_t *p_alist, *p_boundary, *p_entry;
	/* Error-path name wrapper; filled only when the lookup misses. */
	x_satom_t sym_name;

	x_args(p_args, 2, NULL, &p_name);
	p_val = x_eval_arg(p_base, x_011(p_args));

	p_alist = x_firstobj(x_eval_field_env_alist(p_base));
	p_boundary = x_eval_field_env_local_boundary(p_base);

	/* Step 1: walk locals (up to AND INCLUDING boundary) */
	while ( ! x_obj_isnil(p_base, p_alist)) {
		if (x_firstobj(x_firstobj(p_alist)) == p_name) {
			x_restobj(x_firstobj(p_alist)) = p_val;
			return p_val;
		}
		if (p_alist == p_boundary) {
			p_alist = x_restobj(p_alist);
			break;
		}
		p_alist = x_restobj(p_alist);
	}

	/* Step 2: BST lookup (skip re-defined symbols) */
	if ( ! (x_obj_flags(p_name) & X_OBJ_FLAG_SHADOW)) {
		p_entry = x_alist_bst_lookup(p_base,
			x_eval_field_env_global_tree(p_base), p_name);
		if ( ! x_obj_isnil(p_base, p_entry)) {
			x_restobj(p_entry) = p_val;
			return p_val;
		}
	}

	/* Step 3: continue alist walk from boundary */
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
 * @param p_base  Execution context.
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
