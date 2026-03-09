#ifndef X_PRIM_H
#define X_PRIM_H

/*
 * # Computational Expressions in C
 *
 * ## x-prim.h -- Header - Primitives
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2024 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 *
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
/*
 * # Includes
 */
#include "x-obj.h"

/*
 * # Data Structures
 */
x_obj_t *x_prim_eval_arg(x_obj_t *p_base, x_obj_t *p_arg);
x_obj_t *x_prim_evlis(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_prim_multiple_extend(x_obj_t *p_base, x_obj_t *p_env,
	x_obj_t *p_params, x_obj_t *p_vals);

void x_prim_bind(x_obj_t *p_base, x_char_t *name, x_prim_fn fn);

x_obj_t *x_prim_core_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_prim_arith_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_prim_pred_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_prim_string_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_prim_io_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_prim_repl(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_prim_type_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_prim_ffi_register(x_obj_t *p_base, x_obj_t *p_args);

x_obj_t *x_prim_register(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_PRIM_H */
