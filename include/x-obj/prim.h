#ifndef X_OBJ_PRIM_H
#define X_OBJ_PRIM_H

/*
 * # Computational Expressions in C
 *
 * ## x-obj/prim.h -- Header - Object Primitives
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
 * # Includes
 */
#include "x-obj.h"

x_char_t *x_obj_prim_name(x_obj_t *p_base, x_obj_t *p_args);

x_obj_t *x_obj_prim_units(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_obj_prim_length(x_obj_t *p_base, x_obj_t *p_args);

x_obj_t *x_obj_prim_make(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_obj_prim_free(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_obj_prim_clone(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_obj_prim_dump(x_obj_t *p_base, x_obj_t *p_args);

x_obj_t *x_obj_prim_call(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_obj_prim_eval(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_obj_prim_convert(x_obj_t *p_base, x_obj_t *p_args);

x_obj_t *x_obj_prim_identify(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_obj_prim_read(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_obj_prim_write(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_OBJ_PRIM_H */
