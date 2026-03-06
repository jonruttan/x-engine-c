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
 *   (file:in file:out file:err)
 *   (env-alist symbol-list expr-list buffer token-cache)
 * )
 * ```
 */
/*
 * # Includes
 */
#include "x-obj.h"

/* TODO: Add name and version fields. */
#define x_base							x_car
#define x_base_field_type_alist			x_caar
#define x_base_field_files				x_cadar
#define x_base_field_filein(X)			x_car(x_base_field_files((X)))
#define x_base_field_fileout(X)			x_cadr(x_base_field_files((X)))
#define x_base_field_fileerr(X)			x_caddr(x_base_field_files((X)))
#define x_base_field_env				x_caddar
#define x_base_field_env_alist(X)		x_car(x_base_field_env((X)))
#define x_base_field_eval_list(X)		x_cadr(x_base_field_env((X)))
#define x_base_field_buffer(X)			x_caddr(x_base_field_env((X)))
#define x_base_field_token_cache(X)		x_cadddr(x_base_field_env((X)))

#define x_base_isset(B)					((B) != NULL && x_base((B)) != NULL)

x_obj_t *x_base_make(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_base_type_alist_extend(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_base_type_alist_assoc(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_base_env_alist_extend(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_base_env_alist_assoc(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_base_read(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_base_write(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_BASE_H */
