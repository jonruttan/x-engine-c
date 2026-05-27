#ifndef X_INTERP_H
#define X_INTERP_H

/**
 * @file x-interp.h
 * @brief Interpreter object -- x-expr's base object plus the environment,
 *        control-flow, I/O, and metadata fields this evaluator needs.
 *
 * The base object is a pair tree.  x-expr provides the skeleton (io-group,
 * meta-group, profile, hooks, heap-group); this layer fills the
 * environment/control half it leaves nil and appends a few project fields
 * (booleans, eval-list, token-cache, GC hooks, sigint).
 *
 * Layout (base = x_base(X)):
 *
 *   first: env + ctrl              (x-expr leaves nil; filled here)
 *     env    env-alist, env-local-boundary, env-global-tree, shadow-list
 *     ctrl   save-stack, error-handler, tco-expr, tco-env
 *   rest:  io + meta               (x-expr skeleton)
 *     io     type-alist, line, true, false
 *     meta   profile counters, eval-list, token-cache, sigint
 *            (GC hook + root lists -- mark-hooks, free-hooks,
 *             mark-roots -- now live in x-expr's heap-group;
 *             register via x_heap_{mark,free}_hook_add() and
 *             x_heap_mark_root_add().)
 *
 * Each leaf is a stack cell @c (current . saved); read the current value
 * with @c x_firstobj().  Direct-value exceptions (the slot is the value,
 * no wrapping): save-stack, env-local-boundary, env-global-tree,
 * shadow-list.
 *
 * The error handler is itself a pair tree, navigated by x_error_handler_*:
 *   @c (jmp-ptr (saved-env . saved-boundary) error-value . line)
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

#include "x-base.h"

/** The interpreter object: the base object specialized into this project's
 *  execution context.  Serves as the type tag for base/interp objects. */
extern x_satom_t x_interp_obj;

/** Symbol/expression flags.
 *  SHADOW -- a symbol shadows a global BST binding (set/cleared as
 *            environments extend and unwind).
 *  COV    -- an expression has been evaluated (coverage tracking). */
#define X_OBJ_FLAG_SHADOW	X_OBJ_FLAG_1
#define X_OBJ_FLAG_COV		X_OBJ_FLAG_2

/**
 * @defgroup error_handler Error Handler Macros
 * @brief Navigate the error handler pair tree
 *        @c (jmp-ptr (saved-env . saved-boundary) error-value . line).
 * @{
 */
#define x_error_handler_jmp(H)				x_ptrval(x_firstobj(H))
#define x_error_handler_saved_env(H)		x_001(H)
#define x_error_handler_saved_boundary(H)	x_101(H)
#define x_error_handler_error(H)			x_011(H)
#define x_error_handler_line(H)				x_111(H)
/** @} */

/**
 * @defgroup base_field Base Field Accessor Macros
 * @brief Navigate the base object's pair tree.
 *
 * @c x_interp_field_* macros return a field's stack cell @c (current .
 * saved); read the current value with @c x_firstobj().  The @c x_interp_env,
 * @c x_interp_ctrl, @c x_interp_io_state, and @c x_interp_extras macros are
 * the group anchors the fields hang off.  @c x_base, @c io, @c meta, @c
 * hooks, and @c heap come from x-base.h (x-expr).
 * @{
 */

/** @name Environment -- base.first.first
 * @{ */
#define x_interp_env(X)							x_00(x_base(X))
#define x_interp_field_env_alist(X)				x_0(x_interp_env(X))
#define x_interp_field_env_local_boundary(X)	x_01(x_interp_env(X))
#define x_interp_field_env_global_tree(X)		x_011(x_interp_env(X))
#define x_interp_field_shadow_list(X)			x_111(x_interp_env(X))
/** @} */

/** @name Control flow -- base.first.rest
 * @{ */
#define x_interp_ctrl(X)						x_10(x_base(X))
#define x_interp_field_save_stack(X)			x_00(x_interp_ctrl(X))
#define x_interp_field_error_handler(X)			x_10(x_interp_ctrl(X))
#define x_interp_field_tco_expr(X)				x_01(x_interp_ctrl(X))
#define x_interp_field_tco_env(X)				x_11(x_interp_ctrl(X))
/** @} */

/** @name I/O -- base.rest.first
 * @{ */
#define x_interp_field_type_alist(X)			x_00(x_base_field_io_group(X))
#define x_interp_io_state(X)					x_1(x_base_field_io_group(X))
#define x_interp_field_line(X)					x_0(x_interp_io_state(X))
#define x_interp_field_true(X)					x_01(x_interp_io_state(X))
#define x_interp_field_false(X)					x_11(x_interp_io_state(X))
/** @} */

/** @name Profile counters -- a flat list off x_base_field_profile(X)
 *  Read with @c x_atomint(x_firstobj(x_interp_field_profile_*(X))).
 * @{ */
#define x_interp_field_profile_evals(X)				x_01(x_base_field_profile(X))
#define x_interp_field_profile_tco(X)				x_011(x_base_field_profile(X))
#define x_interp_field_profile_assoc_calls(X)		x_0111(x_base_field_profile(X))
#define x_interp_field_profile_assoc_steps(X)		x_0(x_1111(x_base_field_profile(X)))
#define x_interp_field_profile_sym_find_calls(X)	x_01(x_1111(x_base_field_profile(X)))
#define x_interp_field_profile_sym_find_steps(X)	x_011(x_1111(x_base_field_profile(X)))
#define x_interp_field_profile_gc_runs(X)			x_0111(x_1111(x_base_field_profile(X)))
#define x_interp_field_profile_bst_hits(X)			x_0(x_1111(x_1111(x_base_field_profile(X))))
#define x_interp_field_profile_bst_misses(X)		x_01(x_1111(x_1111(x_base_field_profile(X))))
/** @} */

/** @name Project extras -- base.rest.rest.rest.rest
 *  Holds the project-specific bookkeeping that has nowhere better to go:
 *  the eval-roots stack, the reader's token cache, and the inherited
 *  sigint flag.  GC hook + root lists now live in x-expr's heap-group
 *  (see @c x_base_field_heap_mark_hooks etc.).
 * @{ */
#define x_interp_extras(X)						x_11(x_base_field_meta_group(X))
#define x_interp_field_eval_list(X)				x_0(x_interp_extras(X))
#define x_interp_field_token_cache(X)			x_01(x_interp_extras(X))
#define x_interp_field_sigint(X)				x_11(x_interp_extras(X))
/** @} */

/** @} */ /* end base_field */

/** Build the interpreter object (x-expr base object extended). */
x_obj_t *x_interp_make(x_obj_t *p_base, x_obj_t *p_args);

/** Extend the type alist with a new type entry. */
x_obj_t *x_interp_type_alist_extend(x_obj_t *p_base, x_obj_t *p_args);

/** Look up a type in the base type alist. */
x_obj_t *x_interp_type_alist_assoc(x_obj_t *p_base, x_obj_t *p_args);

/** Push a file descriptor onto the file-input stack. */
x_obj_t *x_interp_filein_push(x_obj_t *p_base, x_int_t fd);

/** Pop the top file descriptor from the file-input stack. */
x_obj_t *x_interp_filein_pop(x_obj_t *p_base);

/** Push a buffer onto the input buffer stack. */
x_obj_t *x_interp_buffer_push(x_obj_t *p_base, x_obj_t *p_buffer);

/** Pop the top buffer from the input buffer stack. */
x_obj_t *x_interp_buffer_pop(x_obj_t *p_base);

/** Extend the environment alist with new bindings. */
x_obj_t *x_interp_env_alist_extend(x_obj_t *p_base, x_obj_t *p_args);

/** Load and evaluate a source file. */
x_obj_t *x_interp_load(x_obj_t *p_base, x_obj_t *p_args);

/** Write a string to the output. */
x_obj_t *x_interp_write_str(x_obj_t *p_base, x_obj_t *p_args);

/** Signal an error with the given message and irritant object. */
void x_interp_error(x_obj_t *p_base, x_char_t *message, x_obj_t *p_obj);

#endif /* X_INTERP_H */
