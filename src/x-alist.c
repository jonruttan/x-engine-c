/** @file x-alist.c
 *  @brief Association list and persistent BST operations
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2021 Jon Ruttan
 *  @license MIT No Attribution (MIT-0)
 */
/*
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

/**
 * Prepend an association to an alist.
 *
 * Conses p_assoc onto the front of p_alist, returning the new list.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- (assoc . alist)
 * @return x_obj_t* -- New alist with assoc prepended
 */
x_obj_t *x_alist_extend(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_assoc = x_firstobj(p_args), *p_alist = x_restobj(p_args);

	return x_mkspair(p_base, X_OBJ_FLAG_NONE, p_assoc, p_alist);
}

/**
 * Linear alist lookup by pointer identity on the key's first field.
 *
 * Walks the alist front-to-back, comparing (first (first (first entry)))
 * against (first obj). Returns the first matching entry, or NULL.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- (key . (alist))
 * @return x_obj_t* -- Matching alist entry, or NULL if not found
 */
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

/**
 * BST lookup by symbol pointer.
 *
 * Searches the persistent BST for an entry matching p_sym. Tries
 * pointer equality first (fast path), then falls back to strcmp
 * for traversal direction. Node structure: (entry . (left . right)).
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_tree  x_obj_t* -- BST root node, or NULL
 * @param p_sym   x_obj_t* -- Symbol to look up
 * @return x_obj_t* -- Matching alist entry, or NULL if not found
 *
 * @see x_alist_bst_insert
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

/**
 * Create a SHARED pair immune to GC sweep.
 *
 * BST nodes are structural and shared between persistent tree versions,
 * so they must not be collected.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param a       x_obj_t* -- First element
 * @param b       x_obj_t* -- Rest element
 * @return x_obj_t* -- New pair with X_OBJ_FLAG_SHARED set
 */
static x_obj_t *bst_pair(x_obj_t *p_base, x_obj_t *a, x_obj_t *b)
{
	x_obj_t *p = x_mkspair(p_base, X_OBJ_FLAG_NONE, a, b);
	x_obj_flags(p) |= X_OBJ_FLAG_SHARED;
	return p;
}

/**
 * Persistent (path-copying) BST insert.
 *
 * Returns a new root without mutating the old tree. On duplicate key,
 * the new root contains the updated entry while the old root is unchanged.
 * Unaffected subtrees are shared, not copied. All BST nodes carry
 * X_OBJ_FLAG_SHARED so they are immune to GC sweep.
 *
 * @details
 * Path-copying creates new nodes only along the root-to-insertion path.
 * Siblings and their subtrees are shared by pointer with the old root.
 *
 * @code
 *  Old tree          Insert "d"          New tree (returned)
 *
 *     [c]               [c']  <-- new copy
 *    /   \              /   \
 *  [a]   [e]  -->    [a]   [e']  <-- new copy (right child changed)
 *        /                  /
 *      [d]               [d]  <-- new leaf
 *
 *  [a] and its subtree are SHARED between old and new roots.
 *  [c] and [e] in the old tree are untouched.
 * @endcode
 *
 * @note All new BST nodes are allocated via bst_pair() which sets
 *       X_OBJ_FLAG_SHARED on every node.  Shared objects are immune
 *       to GC sweep -- they persist until the entire tree is abandoned.
 *
 * @note Shared subtrees (unchanged children) are never copied.  Only
 *       ancestor nodes along the insertion path are freshly allocated.
 *       This makes insert O(log n) in both time and allocation.
 *
 * @param p_base   x_obj_t* -- Execution context
 * @param p_tree   x_obj_t* -- Existing BST root, or NULL for empty
 * @param p_entry  x_obj_t* -- (symbol . value) entry to insert
 * @return x_obj_t* -- New BST root
 *
 * @see x_alist_bst_lookup
 * @see x_base_field_env_global_tree
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
