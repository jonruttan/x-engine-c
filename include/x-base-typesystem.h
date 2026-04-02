#ifndef X_BASE_TYPESYSTEM_H
#define X_BASE_TYPESYSTEM_H

/*
 * # Computational Expressions in C
 *
 * ## x-base-typesystem.h -- Header - Base (Type System)
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
 * # The Base Object
 *
 * Extends x-expr's base object with type system fields.
 * Every leaf field is a stack: `(current-value . saved-values)`.
 *
 * ```
 * base-data
 * +-- first: hot (env + ctrl)               [x-expr: nil, filled here]
 * |   +-- first: env-group
 * |   |   +-- first: env-alist
 * |   |   +-- rest: (env-local-boundary . (env-global-tree . shadow-list))
 * |   +-- rest: ctrl-group
 * |       +-- first: (save-stack . error-handler)
 * |       +-- rest: (tco-expr . tco-env)
 * +-- rest: cold (io + meta)                [x-expr skeleton]
 *     +-- first: io-group
 *     |   +-- first: (type-alist . files)   [files from x-expr]
 *     |   +-- rest: io-state                [x-expr: nil, filled here]
 *     |       +-- first: line
 *     |       +-- rest: (true . false)
 *     +-- rest: meta-group
 *         +-- first: (profile . hooks)      [hooks from x-expr]
 *         +-- rest:
 *             +-- first: heap-group         [from x-expr]
 *             +-- rest: x-project-extras
 *                 +-- first: eval-list
 *                 +-- rest: (token-cache . (mark-hooks .
 *                     (free-hooks . mark-roots)))
 * ```
 *
 * Error handler is a pair tree: `(jmp-ptr (saved-env . saved-boundary) error-value)`
 */
/*
 * # Includes
 */
#include "x-base.h"

/*
 * # Error Handler (pair tree macros)
 */
#define x_error_handler_jmp(H)				x_ptrval(x_firstobj(H))
#define x_error_handler_saved_env(H)		x_firstobj(x_firstobj(x_restobj(H)))
#define x_error_handler_saved_boundary(H)	x_restobj(x_firstobj(x_restobj(H)))
#define x_error_handler_error(H)			x_firstobj(x_restobj(x_restobj(H)))

/*
 * # x_base, x_base_isset, io, meta, hooks, heap macros
 *   inherited from x-base.h (x-expr)
 *
 * x_base_field_* macros return the stack cell (current . saved).
 * Use x_firstobj(x_base_field_*(X)) to get the current value.
 */

/* === HOT PATH: env + ctrl (x-expr leaves nil, we fill) === */
#define x_base_hot(X)						x_firstobj(x_base(X))

/* -- env group (first of hot) -- */
#define x_base_env_group(X)					x_firstobj(x_base_hot(X))
#define x_base_field_env_alist(X)			x_firstobj(x_base_env_group(X))
#define x_base_env_aux(X)					x_restobj(x_base_env_group(X))
#define x_base_field_env_local_boundary(X)	x_firstobj(x_base_env_aux(X))
#define x_base_env_bst(X)					x_restobj(x_base_env_aux(X))
#define x_base_field_env_global_tree(X)		x_firstobj(x_base_env_bst(X))
/* Shadow list: symbols that shadow a global BST binding (flagged FLAG_SHADOW) */
#define X_OBJ_FLAG_SHADOW					X_OBJ_FLAG_1
#define x_base_field_shadow_list(X)			x_restobj(x_base_env_bst(X))

/* Coverage flag: marks expressions that have been evaluated */
#define X_OBJ_FLAG_COV						X_OBJ_FLAG_2

/* -- ctrl group (rest of hot) -- */
#define x_base_ctrl_group(X)				x_restobj(x_base_hot(X))
#define x_base_ctrl_head(X)					x_firstobj(x_base_ctrl_group(X))
#define x_base_field_save_stack(X)			x_firstobj(x_base_ctrl_head(X))
#define x_base_field_error_handler(X)		x_restobj(x_base_ctrl_head(X))
#define x_base_tco(X)						x_restobj(x_base_ctrl_group(X))
#define x_base_field_tco_expr(X)			x_firstobj(x_base_tco(X))
#define x_base_field_tco_env(X)				x_restobj(x_base_tco(X))

/* === COLD PATH: io + meta (from x-expr, extended here) === */
#define x_base_cold(X)						x_restobj(x_base(X))

/* -- io extensions (x-expr provides files; we add type-alist + io-state) -- */
#define x_base_field_type_alist(X)			x_firstobj(x_firstobj(x_base_field_io_group(X)))
#define x_base_io_state(X)					x_restobj(x_base_field_io_group(X))
#define x_base_field_line(X)				x_firstobj(x_base_io_state(X))
#define x_base_booleans(X)					x_restobj(x_base_io_state(X))
#define x_base_field_true(X)				x_firstobj(x_base_booleans(X))
#define x_base_field_false(X)				x_restobj(x_base_booleans(X))

/* -- meta extensions (x-expr provides profile, hooks, heap-group) -- */
#define x_base_meta_head(X)					x_firstobj(x_base_field_meta_group(X))

/* Profile counters: each is pair(atom(N), nil). Macros return the stack cell.
 * Use x_atomint(x_firstobj(x_base_field_profile_*(X))) to read. */
#define x_base_field_profile_evals(X)			x_firstobj(x_restobj(x_base_field_profile(X)))
#define x_base_field_profile_tco(X)				x_firstobj(x_restobj(x_restobj(x_base_field_profile(X))))
#define x_base_field_profile_assoc_calls(X)		x_firstobj(x_restobj(x_restobj(x_restobj(x_base_field_profile(X)))))
#define x_base_field_profile_assoc_steps(X)		x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_base_field_profile(X))))))
#define x_base_field_profile_sym_find_calls(X)	x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_base_field_profile(X)))))))
#define x_base_field_profile_sym_find_steps(X)	x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_base_field_profile(X))))))))
#define x_base_field_profile_gc_runs(X)			x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_base_field_profile(X)))))))))
#define x_base_field_profile_bst_hits(X)		x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_base_field_profile(X))))))))))
#define x_base_field_profile_bst_misses(X)		x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_base_field_profile(X)))))))))))

/* -- x-project extras (after heap-group in meta-rest) -- */
#define x_base_extras(X)					x_restobj(x_restobj(x_base_field_meta_group(X)))
#define x_base_field_eval_list(X)			x_firstobj(x_base_extras(X))
#define x_base_extras_more(X)				x_restobj(x_base_extras(X))
#define x_base_field_token_cache(X)			x_firstobj(x_base_extras_more(X))
#define x_base_gc_hooks(X)					x_restobj(x_base_extras_more(X))
#define x_base_field_heap_mark_hooks(X)		x_firstobj(x_base_gc_hooks(X))
#define x_base_gc_hooks_rest(X)				x_restobj(x_base_gc_hooks(X))
#define x_base_field_heap_free_hooks(X)		x_firstobj(x_base_gc_hooks_rest(X))
#define x_base_field_heap_mark_roots(X)		x_restobj(x_base_gc_hooks_rest(X))

/*
 * # Functions
 */
x_obj_t *x_base_ts_make(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_base_type_alist_extend(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_base_type_alist_assoc(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_base_filein_push(x_obj_t *p_base, x_int_t fd);
x_obj_t *x_base_filein_pop(x_obj_t *p_base);
x_obj_t *x_base_buffer_push(x_obj_t *p_base, x_obj_t *p_buffer);
x_obj_t *x_base_buffer_pop(x_obj_t *p_base);
x_obj_t *x_base_env_alist_extend(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_base_load(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_base_write_str(x_obj_t *p_base, x_obj_t *p_args);
void x_base_error(x_obj_t *p_base, x_char_t *message, x_obj_t *p_obj);

#endif /* X_BASE_TYPESYSTEM_H */
