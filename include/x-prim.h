#ifndef X_PRIM_H
#define X_PRIM_H

/*
 * # Computational Expressions in C
 *
 * ## x-prim.h -- Header - Primatives
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

typedef x_obj_t * (*primop)(x_obj_t *, x_obj_t *);

#define x_mkprim(E, X)              x_obj_make(E, X_PRIMITIVE, 0, 1, (X))
#define x_primval(X)                ((x_mkprim)(X)->data.p[0])

x_obj_t *x_prim_read_char(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_length(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_not(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_eq(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_cons(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_car(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_cdr(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_num_not(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_num_sum(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_num_sub(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_num_prod(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_num_div(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_num_mod(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_num_and(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_num_or(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_num_xor(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_num_shl(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_num_shr(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_num_eq(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_num_gt(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_tmpstr(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_mkstr(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_str(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_strcmp(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_vector(x_obj_t *p_base, x_obj_t *args);

#include "x-gc.h"
x_obj_t *x_prim_gc(x_obj_t *p_base, x_obj_t *args)
x_obj_t *x_prim_write(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_assoc(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_append(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_findsym(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_top(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_ptr(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_type(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_prim_totype(x_obj_t *p_base, x_obj_t *args);

#endif /* X_PRIM_H */
