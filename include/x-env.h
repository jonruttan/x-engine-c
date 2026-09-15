#ifndef X_ENV_H
#define X_ENV_H

/**
 * @file x-env.h
 * @brief Environments -- first-class values the evaluator binds and looks
 *        up in.
 *
 * An environment is one pair, @c (bindings . parent).  The root's parent
 * is nil and its bindings are a tree (x-alist.c's BST, for the size of a
 * loaded library); every other environment's bindings are an alist of
 * @c (name . value) cells and its parent is the environment it was made
 * in.  A procedure call makes a child of the closure's environment; an
 * operative body runs in a child of its static environment and receives
 * the caller's environment as a value; `def` binds in the current
 * environment; `eval` with an environment makes that one current.  That
 * is the whole of scope, and these four operations are all of it.
 *
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

#include "x-obj.h"

/** @name The pair
 *  @{ */
#define x_env_bindings(E)		x_firstobj(E)	/**< The alist, or the root's tree. */
#define x_env_parent(E)			x_restobj(E)	/**< The enclosing environment, nil at the root. */
#define x_env_isroot(B,E)		x_obj_isnil((B), x_restobj(E))	/**< A root has no parent. */
/** @} */

/** Make an empty environment whose parent is @p p_parent (nil for a root). */
x_obj_t *x_env_make(x_obj_t *p_base, x_obj_t *p_parent);

/** The @c (name . value) cell binding @p p_sym in @p p_env or an ancestor,
 *  or NULL when no environment on the chain binds it. */
x_obj_t *x_env_lookup(x_obj_t *p_base, x_obj_t *p_env, x_obj_t *p_sym);

/** Bind @p p_sym to @p p_val in @p p_env itself: an existing binding there
 *  is updated in place, otherwise one is added.  Never touches a parent.
 *  Returns @p p_val. */
x_obj_t *x_env_bind(x_obj_t *p_base, x_obj_t *p_env,
	x_obj_t *p_sym, x_obj_t *p_val);

/** Make a child of @p p_parent with @p p_params bound to @p p_vals: the
 *  environment a procedure body or an operative body runs in. */
x_obj_t *x_env_extend(x_obj_t *p_base, x_obj_t *p_parent,
	x_obj_t *p_params, x_obj_t *p_vals);

#endif /* X_ENV_H */
