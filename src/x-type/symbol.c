/*
 * # Computational Expressions in C
 *
 * ## x-type.c -- Implementation - Type - Symbol
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
#include "x-type/symbol.h"
#include "x-type/str.h"
#include "x-alist.h"
#include "x-base.h"
#include "x-eval.h"
#include "x-token/sexp/symbol.h"

x_satom_t x_type_symbol_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_SYMBOL_NAME }),
	x_type_symbol_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_symbol_make }),
	x_type_symbol_eval_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_symbol_eval }),
	x_type_symbol_struct_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_symbol_struct });

x_obj_t *x_make_symbol(x_obj_t *p_base, x_obj_flag_t flags, x_char_t *s)
{
	x_satom_t o_symbol = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .s = s }),
		o_flags = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = flags });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_symbol }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_flags }, { NULL })
	};

	return x_type_symbol_make(p_base, (x_obj_t *)args);
}

x_obj_t *x_type_symbol_struct(x_obj_t *p_base, x_obj_t *p_obj)
{
	struct x_type_t type = {
		.p_name = x_type_symbol_name,
		.p_make = x_type_symbol_make_prim,
		.p_eval = x_type_symbol_eval_prim,
		.p_analyse = x_sexp_symbol_analyse_prim,
		.p_read = x_sexp_symbol_read_prim,
		.p_write = x_sexp_symbol_write_prim
	};

	return x_type_struct_make(p_base, type);
}

/*
 * Symbol intern BST: index for O(log n) symbol lookup by name.
 * Node: (symbol . (left . right)), keyed by x_symbolval.
 * Stored in x_restobj(x_symbol_data(p_type)).
 */
#define x_symbol_bst(T) x_restobj(x_symbol_data((T)))

static x_obj_t *sym_bst_lookup(x_obj_t *p_base, x_obj_t *p_tree,
	x_char_t *name)
{
	x_obj_t *p_sym, *p_children;
	int cmp;

	while ( ! x_obj_isnil(p_base, p_tree)) {
		p_sym = x_firstobj(p_tree);
		cmp = x_lib_strcmp(name, x_symbolval(p_sym));
		if (cmp == 0) {
			return p_tree;
		}
		p_children = x_restobj(p_tree);
		p_tree = (cmp < 0)
			? x_firstobj(p_children)
			: x_restobj(p_children);
	}

	return NULL;
}

static x_obj_t *sym_bst_insert(x_obj_t *p_base, x_obj_t *p_tree,
	x_obj_t *p_sym)
{
	x_obj_t *p_walk, *p_children;
	int cmp;

	if (x_obj_isnil(p_base, p_tree)) {
		return x_mkspair(p_base, p_sym,
			x_mkspair(p_base, NULL, NULL));
	}

	p_walk = p_tree;
	for (;;) {
		cmp = x_lib_strcmp(x_symbolval(p_sym),
			x_symbolval(x_firstobj(p_walk)));
		if (cmp == 0) {
			return p_tree;
		}
		p_children = x_restobj(p_walk);
		if (cmp < 0) {
			if (x_obj_isnil(p_base, x_firstobj(p_children))) {
				x_firstobj(p_children) = x_mkspair(p_base,
					p_sym, x_mkspair(p_base, NULL, NULL));
				return p_tree;
			}
			p_walk = x_firstobj(p_children);
		} else {
			if (x_obj_isnil(p_base, x_restobj(p_children))) {
				x_restobj(p_children) = x_mkspair(p_base,
					p_sym, x_mkspair(p_base, NULL, NULL));
				return p_tree;
			}
			p_walk = x_restobj(p_children);
		}
	}
}

x_obj_t *x_type_symbol_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_symbol_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_symbol_struct_prim }, { NULL })
	};
	x_obj_t *p_type = x_type_struct_get(p_base, (x_obj_t *)args);

	if (x_obj_isnil(p_base, x_symbol_data(p_type))) {
		x_symbol_data(p_type) = x_mkspair(p_base, NULL, NULL);
	}

	return p_type;
}

/* TODO: Alter so symbol list can be optionally supplied by argument. */
x_obj_t *x_type_symbol_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = x_type_symbol_register(p_base, p_base),
		*p_symbol = x_0(p_args), *p_obj = x_type_symbol_find(p_base, p_args);
	x_obj_flag_t flags = x_obj_isnil(p_base, x_1(p_args))
		? 0 : x_firstint(x_01(p_args));

	if ( ! x_obj_isnil(p_base, p_obj)) {
		return x_firstobj(p_obj);
	}

	/* Transfer string ownership from source to new symbol. */
	if (x_obj_flags(p_symbol) & X_OBJ_FLAG_OWN) {
		flags |= X_OBJ_FLAG_OWN;
		x_obj_flags(p_symbol) &= ~X_OBJ_FLAG_OWN;
	}

	p_obj = x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_ATOM,
		x_symbolval(p_symbol));
	x_symbol_data_list(p_type) = x_mkspair(p_base, p_obj,
		x_symbol_data_list(p_type));
	x_symbol_bst(p_type) = sym_bst_insert(p_base,
		x_symbol_bst(p_type), p_obj);

	return p_obj;
}

x_obj_t *x_type_symbol_find(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = x_type_symbol_register(p_base, p_base),
		*p_result;
	x_char_t *name = x_firststr(x_firstobj(p_args));

#ifdef X_PROFILE
	if (x_base_isset(p_base))
		x_atomint(x_base_field_profile_sym_find_calls(p_base))++;
#endif

	/* BST lookup: O(log n) */
	p_result = sym_bst_lookup(p_base, x_symbol_bst(p_type), name);
	if ( ! x_obj_isnil(p_base, p_result)) {
		return p_result;
	}

	return NULL;
}

/*
 * Symbol eval: 3-step lookup.
 *   1. Walk alist head to local_boundary (catches locals in 2-3 steps)
 *   2. BST lookup for globals (O(log n))
 *   3. Continue alist walk from boundary (nested closure fallback)
 */
x_obj_t *x_type_symbol_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_sym_obj = x_firstobj(x_eval_arg_exp(p_args));
	x_obj_t *p_alist, *p_boundary, *p_entry;

	if ( ! x_base_isset(p_base)) {
		return NULL;
	}

	p_alist = x_base_field_env_alist(p_base);
	p_boundary = x_base_field_env_local_boundary(p_base);

	/* Step 1: walk locals (head of alist up to AND INCLUDING boundary) */
	while ( ! x_obj_isnil(p_base, p_alist)) {
		if (x_firstobj(x_firstobj(p_alist)) == p_sym_obj) {
			return x_restobj(x_firstobj(p_alist));
		}
		if (p_alist == p_boundary) {
			p_alist = x_restobj(p_alist);
			break;
		}
		p_alist = x_restobj(p_alist);
	}

	/* Step 2: BST lookup for globals (skip re-defined symbols) */
	if ( ! (x_obj_flags(p_sym_obj) & X_OBJ_FLAG_1)) {
		p_entry = x_alist_bst_lookup(p_base,
			x_base_field_env_global_tree(p_base), p_sym_obj);
		if ( ! x_obj_isnil(p_base, p_entry)) {
			return x_restobj(p_entry);
		}
	}

	/* Step 3: continue alist walk from boundary (enclosing scope locals) */
	while ( ! x_obj_isnil(p_base, p_alist)) {
		if (x_firstobj(x_firstobj(p_alist)) == p_sym_obj) {
			return x_restobj(x_firstobj(p_alist));
		}
		p_alist = x_restobj(p_alist);
	}

	/* Not found: error */
	{
		x_satom_t sym_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
			{ .s = x_symbolval(p_sym_obj) });
		x_obj_error(p_base, "Unbound "X_TYPE_SYMBOL_NAME,
			(x_obj_t *)&sym_name);
	}

	return NULL;
}
