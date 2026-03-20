#ifndef X_BASE_H
#define X_BASE_H

/*
 * # Computational Expressions in C
 *
 * ## x-base.h -- Header - Base
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
 * Balanced binary tree layout grouping hot fields near the root.
 * Every leaf field is a stack: `(current-value . saved-values)`.
 *
 * ```
 * base-data
 * +-- first: hot (env + ctrl)
 * |   +-- first: env-group
 * |   |   +-- first: env-alist-stack
 * |   |   +-- rest: (env-local-boundary . (env-global-tree . ()))
 * |   +-- rest: ctrl-group
 * |       +-- first: (save-stack . error-handler-stack)
 * |       +-- rest: (tco-expr-stack . tco-env-stack)
 * +-- rest: cold (io + meta)
 *     +-- first: io-group
 *     |   +-- first: (type-alist-stack . files)
 *     |   +-- rest: (line-stack . (true-stack . false-stack))
 *     +-- rest: meta-group
 *         +-- first: (profile-list . hooks-list)
 *         +-- rest: (eval-list-stack . (buffer-stack .
 *                     (token-cache-stack . (obj-meta-extra-stack .
 *                       (heap-mark-hooks-stack . (heap-free-hooks-stack .
 *                         heap-mark-roots-stack))))))
 * ```
 *
 * Error handler is a pair tree: `(jmp-ptr (saved-env . saved-boundary) error-value)`
 */
/*
 * # Includes
 */
#include "x-obj.h"

/*
 * # Error Handler (pair tree macros)
 */
#define x_error_handler_jmp(H)				x_ptrval(x_firstobj(H))
#define x_error_handler_saved_env(H)		x_firstobj(x_firstobj(x_restobj(H)))
#define x_error_handler_saved_boundary(H)	x_restobj(x_firstobj(x_restobj(H)))
#define x_error_handler_error(H)			x_firstobj(x_restobj(x_restobj(H)))

/* TODO: Add name and version fields. */
#define x_base(X)							x_firstobj(X)

/* === HOT PATH: env + ctrl === */
#define x_base_hot(X)						x_firstobj(x_base(X))

/* -- env group (first of hot) -- */
#define x_base_env_group(X)					x_firstobj(x_base_hot(X))
#define x_base_field_env_alist_stack(X)		x_firstobj(x_base_env_group(X))
#define x_base_field_env_alist(X)			x_firstobj(x_base_field_env_alist_stack(X))
#define x_base_env_aux(X)					x_restobj(x_base_env_group(X))
#define x_base_field_env_local_boundary(X)	x_firstobj(x_base_env_aux(X))
#define x_base_env_bst(X)					x_restobj(x_base_env_aux(X))
#define x_base_field_env_global_tree(X)		x_firstobj(x_base_env_bst(X))
#define x_base_field_flag1_list(X)			x_restobj(x_base_env_bst(X))

/* -- ctrl group (rest of hot) -- */
#define x_base_ctrl_group(X)				x_restobj(x_base_hot(X))
#define x_base_ctrl_head(X)					x_firstobj(x_base_ctrl_group(X))
#define x_base_field_save_stack(X)			x_firstobj(x_base_ctrl_head(X))
#define x_base_field_error_handler_stack(X)	x_restobj(x_base_ctrl_head(X))
#define x_base_field_error_handler(X)		x_firstobj(x_base_field_error_handler_stack(X))
#define x_base_tco(X)						x_restobj(x_base_ctrl_group(X))
#define x_base_field_tco_expr_stack(X)		x_firstobj(x_base_tco(X))
#define x_base_field_tco_expr(X)			x_firstobj(x_base_field_tco_expr_stack(X))
#define x_base_field_tco_env_stack(X)		x_restobj(x_base_tco(X))
#define x_base_field_tco_env(X)				x_firstobj(x_base_field_tco_env_stack(X))

/* === COLD PATH: io + meta === */
#define x_base_cold(X)						x_restobj(x_base(X))

/* -- io group (first of cold) -- */
#define x_base_io_group(X)					x_firstobj(x_base_cold(X))
#define x_base_io_head(X)					x_firstobj(x_base_io_group(X))
#define x_base_field_type_alist_stack(X)	x_firstobj(x_base_io_head(X))
#define x_base_field_type_alist(X)			x_firstobj(x_base_field_type_alist_stack(X))
#define x_base_field_files(X)				x_restobj(x_base_io_head(X))
#define x_base_field_filein_stack(X)		x_firstobj(x_base_field_files(X))
#define x_base_field_filein(X)				x_firstobj(x_base_field_filein_stack(X))
#define x_base_field_fileout_stack(X)		x_firstobj(x_restobj(x_base_field_files(X)))
#define x_base_field_fileout(X)				x_firstobj(x_base_field_fileout_stack(X))
#define x_base_field_fileerr_stack(X)		x_firstobj(x_restobj(x_restobj(x_base_field_files(X))))
#define x_base_field_fileerr(X)				x_firstobj(x_base_field_fileerr_stack(X))
#define x_base_field_write_buf_stack(X)		x_firstobj(x_restobj(x_restobj(x_restobj(x_base_field_files(X)))))
#define x_base_field_write_buf(X)			x_firstobj(x_base_field_write_buf_stack(X))
#define x_base_io_state(X)					x_restobj(x_base_io_group(X))
#define x_base_field_line_stack(X)			x_firstobj(x_base_io_state(X))
#define x_base_field_line(X)				x_firstobj(x_base_field_line_stack(X))
#define x_base_booleans(X)					x_restobj(x_base_io_state(X))
#define x_base_field_true_stack(X)			x_firstobj(x_base_booleans(X))
#define x_base_field_true(X)				x_firstobj(x_base_field_true_stack(X))
#define x_base_field_false_stack(X)			x_restobj(x_base_booleans(X))
#define x_base_field_false(X)				x_firstobj(x_base_field_false_stack(X))

/* -- meta group (rest of cold) -- */
#define x_base_meta_group(X)				x_restobj(x_base_cold(X))
#define x_base_meta_head(X)					x_firstobj(x_base_meta_group(X))
#define x_base_field_profile(X)				x_firstobj(x_base_meta_head(X))

#define x_base_field_profile_allocs_stack(X)		x_firstobj(x_base_field_profile(X))
#define x_base_field_profile_allocs(X)				x_firstobj(x_base_field_profile_allocs_stack(X))
#define x_base_field_profile_evals_stack(X)			x_firstobj(x_restobj(x_base_field_profile(X)))
#define x_base_field_profile_evals(X)				x_firstobj(x_base_field_profile_evals_stack(X))
#define x_base_field_profile_tco_stack(X)			x_firstobj(x_restobj(x_restobj(x_base_field_profile(X))))
#define x_base_field_profile_tco(X)					x_firstobj(x_base_field_profile_tco_stack(X))
#define x_base_field_profile_assoc_calls_stack(X)	x_firstobj(x_restobj(x_restobj(x_restobj(x_base_field_profile(X)))))
#define x_base_field_profile_assoc_calls(X)			x_firstobj(x_base_field_profile_assoc_calls_stack(X))
#define x_base_field_profile_assoc_steps_stack(X)	x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_base_field_profile(X))))))
#define x_base_field_profile_assoc_steps(X)			x_firstobj(x_base_field_profile_assoc_steps_stack(X))
#define x_base_field_profile_sym_find_calls_stack(X)	x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_base_field_profile(X)))))))
#define x_base_field_profile_sym_find_calls(X)			x_firstobj(x_base_field_profile_sym_find_calls_stack(X))
#define x_base_field_profile_sym_find_steps_stack(X)	x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_base_field_profile(X))))))))
#define x_base_field_profile_sym_find_steps(X)			x_firstobj(x_base_field_profile_sym_find_steps_stack(X))
#define x_base_field_profile_gc_runs_stack(X)	x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_base_field_profile(X)))))))))
#define x_base_field_profile_gc_runs(X)			x_firstobj(x_base_field_profile_gc_runs_stack(X))
#define x_base_field_profile_bst_hits_stack(X)	x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_base_field_profile(X))))))))))
#define x_base_field_profile_bst_hits(X)		x_firstobj(x_base_field_profile_bst_hits_stack(X))
#define x_base_field_profile_bst_misses_stack(X)	x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_base_field_profile(X)))))))))))
#define x_base_field_profile_bst_misses(X)			x_firstobj(x_base_field_profile_bst_misses_stack(X))

#define x_base_field_hooks(X)				x_restobj(x_base_meta_head(X))
#define x_base_field_hook_type_name_stack(X)	x_firstobj(x_base_field_hooks(X))
#define x_base_field_hook_type_name(X)		x_firstobj(x_base_field_hook_type_name_stack(X))
#define x_base_field_hook_units_stack(X)	x_firstobj(x_restobj(x_base_field_hooks(X)))
#define x_base_field_hook_units(X)			x_firstobj(x_base_field_hook_units_stack(X))
#define x_base_field_hook_length_stack(X)	x_firstobj(x_restobj(x_restobj(x_base_field_hooks(X))))
#define x_base_field_hook_length(X)			x_firstobj(x_base_field_hook_length_stack(X))
#define x_base_field_hook_error_stack(X)	x_firstobj(x_restobj(x_restobj(x_restobj(x_base_field_hooks(X)))))
#define x_base_field_hook_error(X)			x_firstobj(x_base_field_hook_error_stack(X))

#define x_base_meta_rest(X)					x_restobj(x_base_meta_group(X))
#define x_base_field_eval_list_stack(X)		x_firstobj(x_base_meta_rest(X))
#define x_base_field_eval_list(X)			x_firstobj(x_base_field_eval_list_stack(X))
#define x_base_meta_more(X)					x_restobj(x_base_meta_rest(X))
#define x_base_field_buffer_stack(X)		x_firstobj(x_base_meta_more(X))
#define x_base_field_buffer(X)				x_firstobj(x_base_field_buffer_stack(X))
#define x_base_meta_last(X)					x_restobj(x_base_meta_more(X))
#define x_base_field_token_cache_stack(X)	x_firstobj(x_base_meta_last(X))
#define x_base_field_token_cache(X)			x_firstobj(x_base_field_token_cache_stack(X))
#define x_base_meta_tail(X)						x_restobj(x_base_meta_last(X))
#define x_base_field_obj_meta_extra_stack(X)	x_firstobj(x_base_meta_tail(X))
#define x_base_field_obj_meta_extra(X)			x_firstobj(x_base_field_obj_meta_extra_stack(X))
#define x_base_gc_hooks(X)						x_restobj(x_base_meta_tail(X))
#define x_base_field_heap_mark_hooks_stack(X)	x_firstobj(x_base_gc_hooks(X))
#define x_base_field_heap_mark_hooks(X)			x_firstobj(x_base_field_heap_mark_hooks_stack(X))
#define x_base_gc_hooks_rest(X)					x_restobj(x_base_gc_hooks(X))
#define x_base_field_heap_free_hooks_stack(X)	x_firstobj(x_base_gc_hooks_rest(X))
#define x_base_field_heap_free_hooks(X)			x_firstobj(x_base_field_heap_free_hooks_stack(X))
#define x_base_field_heap_mark_roots_stack(X)	x_restobj(x_base_gc_hooks_rest(X))
#define x_base_field_heap_mark_roots(X)			x_firstobj(x_base_field_heap_mark_roots_stack(X))

#define x_base_isset(B)						((B) != NULL && x_base((B)) != NULL)

x_obj_t *x_base_make(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_base_type_alist_extend(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_base_type_alist_assoc(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_base_filein_push(x_obj_t *p_base, x_int_t fd);
x_obj_t *x_base_filein_pop(x_obj_t *p_base);
x_obj_t *x_base_buffer_push(x_obj_t *p_base, x_obj_t *p_buffer);
x_obj_t *x_base_buffer_pop(x_obj_t *p_base);
x_obj_t *x_base_env_alist_extend(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_base_load(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_base_read(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_base_write(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_base_write_str(x_obj_t *p_base, x_obj_t *p_args);
void x_base_error(x_obj_t *p_base, x_char_t *message, x_obj_t *p_obj);

#endif /* X_BASE_H */
