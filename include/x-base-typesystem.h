#ifndef X_BASE_TYPESYSTEM_H
#define X_BASE_TYPESYSTEM_H

/**
 * @file x-base-typesystem.h
 * @brief Base object type system extensions for the x project.
 *
 * Extends x-expr's base object with environment, control flow, I/O, and
 * metadata fields.  Every leaf field is a stack: @c (current-value . saved-values).
 *
 * The base-data pair tree layout (S = stack-wrapped, D = direct value):
 * @code
 * base-data
 * +-- first: hot (env + ctrl)               [x-expr: nil, filled here]
 * |   +-- first: env-group
 * |   |   +-- first: env-alist              [S] (current . saved)
 * |   |   +-- rest: env-aux
 * |   |       +-- first: env-local-boundary [D] direct pointer
 * |   |       +-- rest: env-bst
 * |   |           +-- first: env-global-tree [D] direct pointer
 * |   |           +-- rest: shadow-list      [D] direct list
 * |   +-- rest: ctrl-group
 * |       +-- first: ctrl-head
 * |       |   +-- first: save-stack         [D] direct stack (push/pop)
 * |       |   +-- rest: error-handler       [S] (current . saved)
 * |       +-- rest: tco
 * |           +-- first: tco-expr           [S] (current . saved)
 * |           +-- rest: tco-env             [S] (current . saved)
 * +-- rest: cold (io + meta)                [x-expr skeleton]
 *     +-- first: io-group
 *     |   +-- first: io-head
 *     |   |   +-- first: type-alist         [S] (current . saved)
 *     |   |   +-- rest: files               [from x-expr, all S]
 *     |   |       +-- filein, fileout, fileerr, write-buf, buffer
 *     |   +-- rest: io-state                [x-expr: nil, filled here]
 *     |       +-- first: line               [S] (current . saved)
 *     |       +-- rest: booleans
 *     |           +-- first: true           [S] (current . saved)
 *     |           +-- rest: false           [S] (current . saved)
 *     +-- rest: meta-group
 *         +-- first: meta-head
 *         |   +-- first: profile            [list of S counter cells]
 *         |   +-- rest: hooks               [from x-expr, all S]
 *         +-- rest: meta-rest
 *             +-- first: heap-group         [from x-expr, all S]
 *             +-- rest: x-project-extras
 *                 +-- first: eval-list      [S] (current . saved)
 *                 +-- rest:
 *                     +-- first: token-cache [S] (current . saved)
 *                     +-- rest: gc-hooks
 *                         +-- first: mark-hooks  [S]
 *                         +-- rest:
 *                             +-- first: free-hooks  [S]
 *                             +-- rest: mark-roots   [S]
 * @endcode
 *
 * Stack-wrapped fields [S] hold @c (current-value . saved-values).
 * Use @c x_firstobj() to read the current value, push with
 * @c x_mkspair(), pop with @c x_restobj().
 *
 * Direct-value fields [D] hold the value itself. Read/write directly.
 * @c save_stack is a direct stack (push/pop without wrapping).
 * @c env_local_boundary, @c env_global_tree, @c shadow_list are
 * direct pointers managed by save/restore in procedure calls.
 *
 * Error handler is a pair tree:
 * @c (jmp-ptr (saved-env . saved-boundary) error-value)
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

/**
 * @defgroup error_handler Error Handler Macros
 * @brief Navigate the error handler pair tree.
 *
 * The error handler @c H has the structure:
 * @c (jmp-ptr (saved-env . saved-boundary) error-value)
 * @{
 */
#define x_error_handler_jmp(H)				x_ptrval(x_firstobj(H))           /**< Jump buffer pointer. */
#define x_error_handler_saved_env(H)		x_firstobj(x_firstobj(x_restobj(H))) /**< Saved environment alist. */
#define x_error_handler_saved_boundary(H)	x_restobj(x_firstobj(x_restobj(H)))  /**< Saved local boundary. */
#define x_error_handler_error(H)			x_firstobj(x_restobj(x_restobj(H)))  /**< Error value. */
/** @} */

/**
 * @defgroup base_field Base Field Accessor Macros
 * @brief Navigate the base object's pair tree.
 *
 * Macros named @c x_base_field_* return the stack cell
 * @c (current . saved).  Use @c x_firstobj(x_base_field_*(X)) to get
 * the current value.
 *
 * The @c x_base, @c x_base_isset, @c io, @c meta, @c hooks, and @c heap
 * macros are inherited from x-base.h (x-expr).
 * @{
 */

/** @name Hot Path -- Environment + Control
 *  Fields that x-expr leaves nil and this layer fills.
 * @{ */
#define x_base_hot(X)						x_firstobj(x_base(X))              /**< Hot group (env + ctrl). */

#define x_base_env_group(X)					x_firstobj(x_base_hot(X))          /**< Environment group (first of hot). */
#define x_base_field_env_alist(X)			x_firstobj(x_base_env_group(X))    /**< Environment alist stack. */
#define x_base_env_aux(X)					x_restobj(x_base_env_group(X))     /**< Auxiliary env fields. */
#define x_base_field_env_local_boundary(X)	x_firstobj(x_base_env_aux(X))      /**< Local boundary stack. */
#define x_base_env_bst(X)					x_restobj(x_base_env_aux(X))       /**< BST subgroup. */
#define x_base_field_env_global_tree(X)		x_firstobj(x_base_env_bst(X))      /**< Global BST stack. */

/** Shadow flag: symbol shadows a global BST binding. */
#define X_OBJ_FLAG_SHADOW					X_OBJ_FLAG_1
#define x_base_field_shadow_list(X)			x_restobj(x_base_env_bst(X))       /**< Shadow list stack. */

/** Coverage flag: marks expressions that have been evaluated. */
#define X_OBJ_FLAG_COV						X_OBJ_FLAG_2

#define x_base_ctrl_group(X)				x_restobj(x_base_hot(X))           /**< Control group (rest of hot). */
#define x_base_ctrl_head(X)					x_firstobj(x_base_ctrl_group(X))   /**< Control head (save-stack . error-handler). */
#define x_base_field_save_stack(X)			x_firstobj(x_base_ctrl_head(X))    /**< Save stack. */
#define x_base_field_error_handler(X)		x_restobj(x_base_ctrl_head(X))     /**< Error handler pair tree. */
#define x_base_tco(X)						x_restobj(x_base_ctrl_group(X))    /**< TCO subgroup. */
#define x_base_field_tco_expr(X)			x_firstobj(x_base_tco(X))          /**< TCO pending expression. */
#define x_base_field_tco_env(X)				x_restobj(x_base_tco(X))           /**< TCO pending environment. */
/** @} */

/** @name Cold Path -- I/O + Metadata
 *  Fields from x-expr's skeleton, extended here.
 * @{ */
#define x_base_cold(X)						x_restobj(x_base(X))               /**< Cold group (io + meta). */

#define x_base_field_type_alist(X)			x_firstobj(x_firstobj(x_base_field_io_group(X))) /**< Type alist stack. */
#define x_base_io_state(X)					x_restobj(x_base_field_io_group(X))               /**< I/O state subgroup. */
#define x_base_field_line(X)				x_firstobj(x_base_io_state(X))     /**< Current line number stack. */
#define x_base_booleans(X)					x_restobj(x_base_io_state(X))      /**< Boolean constants pair. */
#define x_base_field_true(X)				x_firstobj(x_base_booleans(X))     /**< Canonical true value. */
#define x_base_field_false(X)				x_restobj(x_base_booleans(X))      /**< Canonical false value. */
/** @} */

/** @name Meta Extensions
 *  Extends x-expr's profile, hooks, and heap-group.
 * @{ */
#define x_base_meta_head(X)					x_firstobj(x_base_field_meta_group(X)) /**< Meta head. */
/** @} */

/** @name Profile Counters
 *  Each counter is @c pair(atom(N), nil).  Use
 *  @c x_atomint(x_firstobj(x_base_field_profile_*(X))) to read.
 * @{ */
#define x_base_field_profile_evals(X)			x_firstobj(x_restobj(x_base_field_profile(X)))                                                                           /**< Eval call count. */
#define x_base_field_profile_tco(X)				x_firstobj(x_restobj(x_restobj(x_base_field_profile(X))))                                                                /**< TCO trampoline count. */
#define x_base_field_profile_assoc_calls(X)		x_firstobj(x_restobj(x_restobj(x_restobj(x_base_field_profile(X)))))                                                     /**< Assoc call count. */
#define x_base_field_profile_assoc_steps(X)		x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_base_field_profile(X))))))                                          /**< Assoc step count. */
#define x_base_field_profile_sym_find_calls(X)	x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_base_field_profile(X)))))))                               /**< Symbol find call count. */
#define x_base_field_profile_sym_find_steps(X)	x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_base_field_profile(X))))))))                    /**< Symbol find step count. */
#define x_base_field_profile_gc_runs(X)			x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_base_field_profile(X)))))))))         /**< GC run count. */
#define x_base_field_profile_bst_hits(X)		x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_base_field_profile(X)))))))))) /**< BST cache hits. */
#define x_base_field_profile_bst_misses(X)		x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_base_field_profile(X))))))))))) /**< BST cache misses. */
/** @} */

/** @name Project Extras
 *  Additional fields appended after heap-group in meta-rest.
 * @{ */
#define x_base_extras(X)					x_restobj(x_restobj(x_base_field_meta_group(X))) /**< Extras subgroup. */
#define x_base_field_eval_list(X)			x_firstobj(x_base_extras(X))       /**< Eval list stack. */
#define x_base_extras_more(X)				x_restobj(x_base_extras(X))        /**< Remaining extras. */
#define x_base_field_token_cache(X)			x_firstobj(x_base_extras_more(X))  /**< Token cache stack. */
#define x_base_gc_hooks(X)					x_restobj(x_base_extras_more(X))   /**< GC hooks subgroup. */
#define x_base_field_heap_mark_hooks(X)		x_firstobj(x_base_gc_hooks(X))     /**< Heap mark hooks list. */
#define x_base_gc_hooks_rest(X)				x_restobj(x_base_gc_hooks(X))      /**< Remaining GC hooks. */
#define x_base_field_heap_free_hooks(X)		x_firstobj(x_base_gc_hooks_rest(X)) /**< Heap free hooks list. */
#define x_base_field_heap_mark_roots(X)		x_restobj(x_base_gc_hooks_rest(X)) /**< Heap mark roots list. */
/** @} */

/** @} */ /* end base_field */

/** Build the type-system-extended base object. */
x_obj_t *x_base_ts_make(x_obj_t *p_base, x_obj_t *p_args);

/** Extend the type alist with a new type entry. */
x_obj_t *x_base_type_alist_extend(x_obj_t *p_base, x_obj_t *p_args);

/** Look up a type in the base type alist. */
x_obj_t *x_base_type_alist_assoc(x_obj_t *p_base, x_obj_t *p_args);

/** Push a file descriptor onto the file-input stack. */
x_obj_t *x_base_filein_push(x_obj_t *p_base, x_int_t fd);

/** Pop the top file descriptor from the file-input stack. */
x_obj_t *x_base_filein_pop(x_obj_t *p_base);

/** Push a buffer onto the input buffer stack. */
x_obj_t *x_base_buffer_push(x_obj_t *p_base, x_obj_t *p_buffer);

/** Pop the top buffer from the input buffer stack. */
x_obj_t *x_base_buffer_pop(x_obj_t *p_base);

/** Extend the environment alist with new bindings. */
x_obj_t *x_base_env_alist_extend(x_obj_t *p_base, x_obj_t *p_args);

/** Load and evaluate a source file. */
x_obj_t *x_base_load(x_obj_t *p_base, x_obj_t *p_args);

/** Write a string to the output. */
x_obj_t *x_base_write_str(x_obj_t *p_base, x_obj_t *p_args);

/** Signal an error with the given message and irritant object. */
void x_base_error(x_obj_t *p_base, x_char_t *message, x_obj_t *p_obj);

#endif /* X_BASE_TYPESYSTEM_H */
