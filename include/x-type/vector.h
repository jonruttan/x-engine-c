#ifndef X_TYPE_VECTOR_H
#define X_TYPE_VECTOR_H

/*
 * # Computational Expressions in C
 *
 * ## x-obj.h -- Header - Type - Vector
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

/*
 * # Macros
 */
#define x_vectorlen(X)			x_firstint((X))
#define x_vectorval(X, N)		x_obj(x_obj_data_i((X),(N)+1))

/*
 * # Data Structures
 */
x_int_t x_obj_vector_units(x_obj_t *p_base, x_obj_t *p_obj);
x_int_t x_obj_vector_length(x_obj_t *p_base, x_obj_t *p_obj);
x_obj_t *x_obj_vector_proc(x_obj_t *p_base, x_obj_t *proc, x_obj_t *vals);
x_obj_t *x_obj_vector_read(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_mkvector(x_obj_t *p_base, x_int_t count);

#endif /* X_TYPE_VECTOR_H */
