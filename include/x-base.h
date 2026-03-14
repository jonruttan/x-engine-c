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
 * ```lang=lisp
 * '(
 *   (type-alist)
 *   (file:in file:out file:err write-buf)
 *   (env-alist symbol-list expr-list buffer token-cache error-handler tco-expr tco-env)
 *   p-true
 *   line-number
 *   (alloc-count eval-count tco-count)
 *   (hook:type-name hook:units hook:length hook:error)
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
#define x_base(X)						x_firstobj(X)
#define x_base_field_type_alist(X)		x_firstobj(x_firstobj(X))
#define x_base_field_files(X)			x_firstobj(x_restobj(x_firstobj(X)))
#define x_base_field_filein_stack(X)	x_firstobj(x_base_field_files((X)))
#define x_base_field_filein(X)			x_firstobj(x_base_field_filein_stack((X)))
#define x_base_field_fileout(X)			x_firstobj(x_restobj(x_base_field_files((X))))
#define x_base_field_fileerr(X)			x_firstobj(x_restobj(x_restobj(x_base_field_files((X)))))
#define x_base_field_write_buf(X)		x_firstobj(x_restobj(x_restobj(x_restobj(x_base_field_files((X))))))
#define x_base_field_env(X)				x_firstobj(x_restobj(x_restobj(x_firstobj(X))))
#define x_base_field_env_alist(X)		x_firstobj(x_base_field_env((X)))
#define x_base_field_eval_list(X)		x_firstobj(x_restobj(x_base_field_env((X))))
#define x_base_field_buffer_stack(X)	x_firstobj(x_restobj(x_restobj(x_base_field_env((X)))))
#define x_base_field_buffer(X)			x_firstobj(x_base_field_buffer_stack((X)))
#define x_base_field_token_cache(X)		x_firstobj(x_restobj(x_restobj(x_restobj(x_base_field_env((X))))))
#define x_base_field_error_handler(X)	x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_base_field_env((X)))))))
#define x_base_field_tco_expr(X)		x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_base_field_env((X))))))))
#define x_base_field_tco_env(X)			x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_base_field_env((X)))))))))

#define x_base_field_true(X)			x_firstobj(x_restobj(x_restobj(x_restobj(x_firstobj(X)))))
#define x_base_field_line(X)			x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_firstobj(X))))))

#define x_base_field_profile(X)			x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_firstobj(X)))))))
#define x_base_field_profile_allocs(X)	x_firstobj(x_base_field_profile(X))
#define x_base_field_profile_evals(X)	x_firstobj(x_restobj(x_base_field_profile(X)))
#define x_base_field_profile_tco(X)		x_firstobj(x_restobj(x_restobj(x_base_field_profile(X))))

#define x_base_field_hooks(X)			x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_firstobj(X))))))))
#define x_base_field_hook_type_name(X)	x_firstobj(x_base_field_hooks(X))
#define x_base_field_hook_units(X)		x_firstobj(x_restobj(x_base_field_hooks(X)))
#define x_base_field_hook_length(X)		x_firstobj(x_restobj(x_restobj(x_base_field_hooks(X))))
#define x_base_field_hook_error(X)		x_firstobj(x_restobj(x_restobj(x_restobj(x_base_field_hooks(X)))))

#define x_base_isset(B)					((B) != NULL && x_base((B)) != NULL)

x_obj_t *x_base_make(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_base_type_alist_extend(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_base_type_alist_assoc(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_base_env_alist_extend(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_base_filein_push(x_obj_t *p_base, x_int_t fd);
x_obj_t *x_base_filein_pop(x_obj_t *p_base);
x_obj_t *x_base_buffer_push(x_obj_t *p_base, x_obj_t *p_buffer);
x_obj_t *x_base_buffer_pop(x_obj_t *p_base);
x_obj_t *x_base_load(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_base_read(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_base_write(x_obj_t *p_base, x_obj_t *p_args);
void x_base_error(x_obj_t *p_base, x_char_t *message, x_obj_t *p_obj);

#endif /* X_BASE_H */
