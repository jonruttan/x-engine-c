/*
 * # Computational Expressions in C
 *
 * ## x-obj.c -- Implementation - Objects
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
#include "x-alist.h"
#include "x-type/symbol.h"

x_obj_t *x_alist_extend(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_assoc = x_firstobj(p_args), *p_alist = x_restobj(p_args);

	return x_mkspair(p_base, p_assoc, p_alist);
}

x_obj_t *x_alist_assoc(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args),
		*p_alist = x_firstobj(x_restobj(p_args));

	while ( ! x_obj_isnil(p_base, p_alist)) {
		if (x_firstobj(x_firstobj(x_firstobj(p_alist))) == x_firstobj(p_obj)) {
			return x_firstobj(p_alist);
		}

		p_alist = x_restobj(p_alist);
	}

	return NULL;
}

/*
 * BST lookup: find an alist entry by symbol pointer.
 * Node structure: (entry . (left . right))
 * Uses pointer equality first, x_lib_strcmp for direction.
 */
x_obj_t *x_alist_bst_lookup(x_obj_t *p_base, x_obj_t *p_tree,
	x_obj_t *p_sym)
{
	x_obj_t *p_entry, *p_children;
	int cmp;

	while ( ! x_obj_isnil(p_base, p_tree)) {
		p_entry = x_firstobj(p_tree);
		if (x_firstobj(p_entry) == p_sym) {
			return p_entry;
		}
		cmp = x_lib_strcmp(x_symbolval(p_sym),
			x_symbolval(x_firstobj(p_entry)));
		if (cmp == 0) {
			return p_entry;
		}
		p_children = x_restobj(p_tree);
		p_tree = (cmp < 0)
			? x_firstobj(p_children)
			: x_restobj(p_children);
	}

	return NULL;
}

/*
 * BST insert: add or update an alist entry in the tree.
 * Returns the (possibly new) root.
 */
x_obj_t *x_alist_bst_insert(x_obj_t *p_base, x_obj_t *p_tree,
	x_obj_t *p_entry)
{
	x_obj_t *p_walk, *p_node, *p_children;
	int cmp;

	if (x_obj_isnil(p_base, p_tree)) {
		return x_mkspair(p_base, p_entry,
			x_mkspair(p_base, NULL, NULL));
	}

	p_walk = p_tree;
	for (;;) {
		p_node = x_firstobj(p_walk);
		if (x_firstobj(p_node) == x_firstobj(p_entry)) {
			/* Re-def: flag symbol as multi-bound. BST can't
			 * serve re-defined symbols (different closures need
			 * different versions). Skip BST on future lookups. */
			x_obj_flags(x_firstobj(p_entry)) |= X_OBJ_FLAG_1;
			return p_tree;
		}
		cmp = x_lib_strcmp(x_symbolval(x_firstobj(p_entry)),
			x_symbolval(x_firstobj(p_node)));
		if (cmp == 0) {
			x_obj_flags(x_firstobj(p_entry)) |= X_OBJ_FLAG_1;
			return p_tree;
		}
		p_children = x_restobj(p_walk);
		if (cmp < 0) {
			if (x_obj_isnil(p_base, x_firstobj(p_children))) {
				x_firstobj(p_children) = x_mkspair(p_base,
					p_entry,
					x_mkspair(p_base, NULL, NULL));
				return p_tree;
			}
			p_walk = x_firstobj(p_children);
		} else {
			if (x_obj_isnil(p_base, x_restobj(p_children))) {
				x_restobj(p_children) = x_mkspair(p_base,
					p_entry,
					x_mkspair(p_base, NULL, NULL));
				return p_tree;
			}
			p_walk = x_restobj(p_children);
		}
	}
}
