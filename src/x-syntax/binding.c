/*
 * # Computational Expressions in C
 *
 * ## x-syntax/binding.c -- Syntax - Binding Forms (def, set!)
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
#include "x-prim.h"
#include "x-alist.h"
#include "x-base-typesystem.h"
#include "x-type/symbol.h"

/* def: (def name value) -> bind name in current environment */
static x_obj_t *x_prim_define(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_name, *p_pair, *p_val;
	x_args(p_args, 2, NULL, &p_name);
	p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_name, NULL);

	x_base_env_alist_extend(p_base, p_pair);
	p_val = x_eval_arg(p_base, x_011(p_args));
	x_restobj(p_pair) = p_val;

	/* Top-level: insert into BST and advance boundary.
	 * Inside closure: flag symbol if it shadows a BST global. */
	if (x_base_isset(p_base)
		&& x_obj_isnil(p_base, x_base_field_save_stack(p_base))) {
		x_base_field_env_global_tree(p_base) = x_alist_bst_insert(
			p_base, x_base_field_env_global_tree(p_base), p_pair);
		x_base_field_env_local_boundary(p_base)
			= x_firstobj(x_base_field_env_alist(p_base));
	} else if (x_base_isset(p_base)) {
		if (x_alist_bst_lookup(p_base,
			x_base_field_env_global_tree(p_base), p_name) != NULL) {
			if ( ! (x_obj_flags(p_name) & X_OBJ_FLAG_SHADOW)) {
				x_obj_flags(p_name) |= X_OBJ_FLAG_SHADOW;
				x_base_field_shadow_list(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
					p_name, x_base_field_shadow_list(p_base));
			}
		}
	}

	return p_val;
}

/* set: (set name value) -> mutate existing binding (3-step BST-aware) */
static x_obj_t *x_prim_set(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_name, *p_val;
	x_obj_t *p_alist, *p_boundary, *p_entry;
	x_args(p_args, 2, NULL, &p_name);
	p_val = x_eval_arg(p_base, x_011(p_args));

	p_alist = x_firstobj(x_base_field_env_alist(p_base));
	p_boundary = x_base_field_env_local_boundary(p_base);

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
			x_base_field_env_global_tree(p_base), p_name);
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

	{
		x_satom_t sym_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
			{ .s = x_symbolval(p_name) });
		x_obj_error(p_base, "Unbound "X_TYPE_SYMBOL_NAME,
			(x_obj_t *)&sym_name);
	}

	return NULL;
}

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
