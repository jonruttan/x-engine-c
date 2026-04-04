#ifndef X_ALIST_H
#define X_ALIST_H

/**
 * @file x-alist.h
 * @brief Association list interface.
 *
 * Provides linear and BST-backed association list operations for the
 * environment model.  An alist is a list of @c (key . value) entries;
 * the BST variant indexes those entries by symbol pointer for faster
 * lookup.
 *
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2021 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */

#include "x-obj.h"

/** @name Linear Association List
 *  Extend and search association lists using linear traversal.
 *  @{ */

/** Prepend an entry to an alist. */
x_obj_t *x_alist_extend(x_obj_t *p_base, x_obj_t *p_args);

/** Look up a symbol in an alist by linear scan. */
x_obj_t *x_alist_assoc(x_obj_t *p_base, x_obj_t *p_args);

/** @} */

/** @name BST Association List
 *  Binary-search-tree index over alist entries, keyed by symbol
 *  pointer.  Node structure: @c (entry . (left . right)).
 *  @{ */

/** Find an entry in a BST alist by symbol pointer. */
x_obj_t *x_alist_bst_lookup(x_obj_t *p_base, x_obj_t *p_tree, x_obj_t *p_sym);

/** Insert an entry into a BST alist. */
x_obj_t *x_alist_bst_insert(x_obj_t *p_base, x_obj_t *p_tree, x_obj_t *p_entry);

/** @} */

#endif /* X_ALIST_H */
