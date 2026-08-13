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
#include "x-eval.h"
#include "x-type/symbol.h"

/**
 * Prepend an association to an alist.
 *
 * Conses p_assoc onto the front of p_alist, returning the new list.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
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
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- (key . (alist))
 * @return x_obj_t* -- Matching alist entry, or NULL if not found
 */
x_obj_t *x_alist_assoc(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args),
		*p_alist = x_firstobj(x_restobj(p_args));

#ifdef X_PROFILE
	if (x_base_isset(p_base))
		x_atomint(x_firstobj(x_eval_field_profile_assoc_calls(p_base)))++;
#endif

	while ( ! x_obj_isnil(p_base, p_alist)) {
#ifdef X_PROFILE
		if (x_base_isset(p_base))
			x_atomint(x_firstobj(x_eval_field_profile_assoc_steps(p_base)))++;
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
 * @param p_base  x_obj_t* -- Base (execution context)
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
				x_atomint(x_firstobj(x_eval_field_profile_bst_hits(p_base)))++;
#endif
			return p_entry;
		}
		cmp = x_lib_strcmp(x_symbolval(p_sym),
			x_symbolval(x_firstobj(p_entry)));
		if (cmp == 0) {
#ifdef X_PROFILE
			if (x_base_isset(p_base))
				x_atomint(x_firstobj(x_eval_field_profile_bst_hits(p_base)))++;
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
		x_atomint(x_firstobj(x_eval_field_profile_bst_misses(p_base)))++;
#endif
	return NULL;
}

/**
 * Create a SHARED pair immune to GC sweep.
 *
 * BST nodes are structural -- the global env tree lives for the base's
 * lifetime and is reachable only through this tree, so its nodes must
 * not be collected.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
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
 * In-place BST insert into the global env tree.
 *
 * MUTATES the tree in place; every captured snapshot of the root (a
 * closure's env) sees the new entry, which is required: a top-level
 * (def ...) must become visible to fn closures created before it.
 * On a duplicate key the existing node's value is overwritten.  The
 * returned root equals the input root except when the tree was empty
 * (the caller must then store the returned first node).
 *
 * @note NOT path-copying.  The previous implementation returned a new
 *       root and left the old one unchanged; that made every closure
 *       created before an insertion silently miss any later top-level
 *       def, so it was replaced by in-place mutation (see the body
 *       comment below).  New nodes are allocated via bst_pair(), which
 *       sets X_OBJ_FLAG_SHARED so the long-lived tree is never swept.
 *
 * @param p_base   x_obj_t* -- Base (execution context)
 * @param p_tree   x_obj_t* -- Existing BST root, or NULL for empty
 * @param p_entry  x_obj_t* -- (symbol . value) entry to insert
 * @return x_obj_t* -- The tree root (a fresh node only when @p p_tree
 *                     was empty)
 *
 * @see x_alist_bst_lookup
 * @see x_eval_field_env_global_tree
 */
x_obj_t *x_alist_bst_insert(x_obj_t *p_base, x_obj_t *p_tree,
	x_obj_t *p_entry)
{
	x_obj_t *p_children;
	int cmp;

	/* Empty tree: caller must update their root with the returned node. */
	if (x_obj_isnil(p_base, p_tree)) {
		return bst_pair(p_base, p_entry,
			bst_pair(p_base, NULL, NULL));
	}

	/* Non-empty: MUTATE the existing tree in place so all captured BST
	 * snapshots (in fn closures) see the new entry.  The previous
	 * implementation was path-copying and returned a new root, which
	 * made every closure created before the insertion silently miss any
	 * later top-level def. */

	p_children = x_restobj(p_tree);

	/* Pointer equality (fast path): re-def -- replace entry in place. */
	if (x_firstobj(x_firstobj(p_tree)) == x_firstobj(p_entry)) {
		x_firstobj(p_tree) = p_entry;
		return p_tree;
	}

	cmp = x_lib_strcmp(x_symbolval(x_firstobj(p_entry)),
		x_symbolval(x_firstobj(x_firstobj(p_tree))));

	if (cmp == 0) {
		x_firstobj(p_tree) = p_entry;
		return p_tree;
	}

	/* Walk into the appropriate child; either recurse (mutates the
	 * subtree in place) or attach a new leaf directly. */
	if (cmp < 0) {
		if (x_obj_isnil(p_base, x_firstobj(p_children))) {
			x_firstobj(p_children) = bst_pair(p_base, p_entry,
				bst_pair(p_base, NULL, NULL));
		} else {
			x_alist_bst_insert(p_base, x_firstobj(p_children), p_entry);
		}
	} else {
		if (x_obj_isnil(p_base, x_restobj(p_children))) {
			x_restobj(p_children) = bst_pair(p_base, p_entry,
				bst_pair(p_base, NULL, NULL));
		} else {
			x_alist_bst_insert(p_base, x_restobj(p_children), p_entry);
		}
	}
	return p_tree;
}

