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
 * Every leaf field is a stack: `(current-value . saved-values)`.
 * Access the current value via the field macro (top of stack).
 * Use the `_stack` macro for push/pop operations.
 *
 * ```lang=lisp
 * '(
 *   type-alist-stack
 *   (file:in-stack file:out-stack file:err-stack write-buf-stack)
 *   (env-alist-stack eval-list-stack buffer-stack token-cache-stack
 *    error-handler-stack tco-expr-stack tco-env-stack)
 *   true-stack
 *   line-stack
 *   (alloc-count-stack eval-count-stack tco-count-stack)
 *   (hook:type-name-stack hook:units-stack hook:length-stack hook:error-stack)
 * )
 * ```
 *
 * Error handler is a pair tree: `(jmp-ptr saved-env error-value)`
 */
/*
 * # Includes
 */
#include "x-obj.h"

#define X_BASE_TRUE_STR		"t"

/*
 * # Error Handler (pair tree macros)
 */
#define x_error_handler_jmp(H)			x_ptrval(x_firstobj(H))
#define x_error_handler_saved_env(H)	x_firstobj(x_restobj(H))
#define x_error_handler_error(H)		x_firstobj(x_restobj(x_restobj(H)))

/* TODO: Add name and version fields. */
#define x_base(X)							x_firstobj(X)
#define x_base_field_type_alist_stack(X)	x_firstobj(x_firstobj(X))
#define x_base_field_type_alist(X)			x_firstobj(x_base_field_type_alist_stack((X)))

#define x_base_field_files(X)				x_firstobj(x_restobj(x_firstobj(X)))
#define x_base_field_filein_stack(X)		x_firstobj(x_base_field_files((X)))
#define x_base_field_filein(X)				x_firstobj(x_base_field_filein_stack((X)))
#define x_base_field_fileout_stack(X)		x_firstobj(x_restobj(x_base_field_files((X))))
#define x_base_field_fileout(X)				x_firstobj(x_base_field_fileout_stack((X)))
#define x_base_field_fileerr_stack(X)		x_firstobj(x_restobj(x_restobj(x_base_field_files((X)))))
#define x_base_field_fileerr(X)				x_firstobj(x_base_field_fileerr_stack((X)))
#define x_base_field_write_buf_stack(X)		x_firstobj(x_restobj(x_restobj(x_restobj(x_base_field_files((X))))))
#define x_base_field_write_buf(X)			x_firstobj(x_base_field_write_buf_stack((X)))

#define x_base_field_env(X)					x_firstobj(x_restobj(x_restobj(x_firstobj(X))))
#define x_base_field_env_alist_stack(X)		x_firstobj(x_base_field_env((X)))
#define x_base_field_env_alist(X)			x_firstobj(x_base_field_env_alist_stack((X)))
#define x_base_field_eval_list_stack(X)		x_firstobj(x_restobj(x_base_field_env((X))))
#define x_base_field_eval_list(X)			x_firstobj(x_base_field_eval_list_stack((X)))
#define x_base_field_buffer_stack(X)		x_firstobj(x_restobj(x_restobj(x_base_field_env((X)))))
#define x_base_field_buffer(X)				x_firstobj(x_base_field_buffer_stack((X)))
#define x_base_field_token_cache_stack(X)	x_firstobj(x_restobj(x_restobj(x_restobj(x_base_field_env((X))))))
#define x_base_field_token_cache(X)			x_firstobj(x_base_field_token_cache_stack((X)))
#define x_base_field_error_handler_stack(X)	x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_base_field_env((X)))))))
#define x_base_field_error_handler(X)		x_firstobj(x_base_field_error_handler_stack((X)))
#define x_base_field_tco_expr_stack(X)		x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_base_field_env((X))))))))
#define x_base_field_tco_expr(X)			x_firstobj(x_base_field_tco_expr_stack((X)))
#define x_base_field_tco_env_stack(X)		x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_base_field_env((X)))))))))
#define x_base_field_tco_env(X)				x_firstobj(x_base_field_tco_env_stack((X)))

#define x_base_field_true_stack(X)			x_firstobj(x_restobj(x_restobj(x_restobj(x_firstobj(X)))))
#define x_base_field_true(X)				x_firstobj(x_base_field_true_stack((X)))
#define x_base_field_line_stack(X)			x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_firstobj(X))))))
#define x_base_field_line(X)				x_firstobj(x_base_field_line_stack((X)))

#define x_base_field_profile(X)				x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_firstobj(X)))))))
#define x_base_field_profile_allocs_stack(X)	x_firstobj(x_base_field_profile(X))
#define x_base_field_profile_allocs(X)		x_firstobj(x_base_field_profile_allocs_stack((X)))
#define x_base_field_profile_evals_stack(X)	x_firstobj(x_restobj(x_base_field_profile(X)))
#define x_base_field_profile_evals(X)		x_firstobj(x_base_field_profile_evals_stack((X)))
#define x_base_field_profile_tco_stack(X)	x_firstobj(x_restobj(x_restobj(x_base_field_profile(X))))
#define x_base_field_profile_tco(X)			x_firstobj(x_base_field_profile_tco_stack((X)))

#define x_base_field_hooks(X)				x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_firstobj(X))))))))
#define x_base_field_hook_type_name_stack(X)	x_firstobj(x_base_field_hooks(X))
#define x_base_field_hook_type_name(X)		x_firstobj(x_base_field_hook_type_name_stack((X)))
#define x_base_field_hook_units_stack(X)	x_firstobj(x_restobj(x_base_field_hooks(X)))
#define x_base_field_hook_units(X)			x_firstobj(x_base_field_hook_units_stack((X)))
#define x_base_field_hook_length_stack(X)	x_firstobj(x_restobj(x_restobj(x_base_field_hooks(X))))
#define x_base_field_hook_length(X)			x_firstobj(x_base_field_hook_length_stack((X)))
#define x_base_field_hook_error_stack(X)	x_firstobj(x_restobj(x_restobj(x_restobj(x_base_field_hooks(X)))))
#define x_base_field_hook_error(X)			x_firstobj(x_base_field_hook_error_stack((X)))

#define x_base_field_save_stack(X)			x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_firstobj(X)))))))))

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
