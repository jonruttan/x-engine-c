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
#include "x-base-typesystem.h"
#include "x-type/symbol.h"

x_obj_t *x_alist_extend(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_assoc = x_firstobj(p_args), *p_alist = x_restobj(p_args);

	return x_mkspair(p_base, X_OBJ_FLAG_NONE, p_assoc, p_alist);
}

x_obj_t *x_alist_assoc(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args),
		*p_alist = x_firstobj(x_restobj(p_args));

#ifdef X_PROFILE
	if (x_base_isset(p_base))
		x_atomint(x_firstobj(x_base_field_profile_assoc_calls(p_base)))++;
#endif

	while ( ! x_obj_isnil(p_base, p_alist)) {
#ifdef X_PROFILE
		if (x_base_isset(p_base))
			x_atomint(x_firstobj(x_base_field_profile_assoc_steps(p_base)))++;
#endif
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
#ifdef X_PROFILE
			if (x_base_isset(p_base))
				x_atomint(x_firstobj(x_base_field_profile_bst_hits(p_base)))++;
#endif
			return p_entry;
		}
		cmp = x_lib_strcmp(x_symbolval(p_sym),
			x_symbolval(x_firstobj(p_entry)));
		if (cmp == 0) {
#ifdef X_PROFILE
			if (x_base_isset(p_base))
				x_atomint(x_firstobj(x_base_field_profile_bst_hits(p_base)))++;
#endif
			return p_entry;
		}
		p_children = x_restobj(p_tree);
		p_tree = (cmp < 0)
			? x_firstobj(p_children)
			: x_restobj(p_children);
	}

#ifdef X_PROFILE
	if (x_base_isset(p_base))
		x_atomint(x_firstobj(x_base_field_profile_bst_misses(p_base)))++;
#endif
	return NULL;
}

/*
 * Create a SHARED pair (never freed by GC).
 * BST nodes are structural and shared between persistent tree versions.
 */
static x_obj_t *bst_pair(x_obj_t *p_base, x_obj_t *a, x_obj_t *b)
{
	x_obj_t *p = x_mkspair(p_base, X_OBJ_FLAG_NONE, a, b);
	x_obj_flags(p) |= X_OBJ_FLAG_SHARED;
	return p;
}

/*
 * BST insert: persistent (path-copying) insert.
 * Returns a NEW root without mutating the old tree.
 * On duplicate: new root has updated entry; old root unchanged.
 * Shared subtrees are referenced, not copied.
 * All BST nodes are SHARED (immune to GC sweep).
 */
x_obj_t *x_alist_bst_insert(x_obj_t *p_base, x_obj_t *p_tree,
	x_obj_t *p_entry)
{
	x_obj_t *p_node, *p_children, *p_left, *p_right;
	int cmp;

	/* Empty tree: create leaf node */
	if (x_obj_isnil(p_base, p_tree)) {
		return bst_pair(p_base, p_entry,
			bst_pair(p_base, NULL, NULL));
	}

	p_node = x_firstobj(p_tree);
	p_children = x_restobj(p_tree);
	p_left = x_firstobj(p_children);
	p_right = x_restobj(p_children);

	/* Pointer equality (fast path) */
	if (x_firstobj(p_node) == x_firstobj(p_entry)) {
		/* Re-def: copy this node with new entry, share children */
		return bst_pair(p_base, p_entry,
			bst_pair(p_base, p_left, p_right));
	}

	cmp = x_lib_strcmp(x_symbolval(x_firstobj(p_entry)),
		x_symbolval(x_firstobj(p_node)));

	if (cmp == 0) {
		return bst_pair(p_base, p_entry,
			bst_pair(p_base, p_left, p_right));
	}

	/* Recurse into subtree, copy this node with new child */
	if (cmp < 0) {
		return bst_pair(p_base, p_node,
			bst_pair(p_base,
				x_alist_bst_insert(p_base, p_left, p_entry),
				p_right));
	} else {
		return bst_pair(p_base, p_node,
			bst_pair(p_base, p_left,
				x_alist_bst_insert(p_base, p_right, p_entry)));
	}
}
