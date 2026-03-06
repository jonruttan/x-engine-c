#ifndef X_TYPE_ITER_H
#define X_TYPE_ITER_H

/*
 * # Computational Expressions in C
 *
 * ## x-type/iter.h -- Header - Type - Iter
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
#include "x-type.h"

#ifndef X_TYPE_ITER_NAME
#define X_TYPE_ITER_NAME			"ITER"
#endif /* X_TYPE_ITER_NAME */

/*
 * # Macros
 */
#define x_obj_type_isiter(B,X)		x_obj_is_type((B), (X), X_TYPE_ITER_NAME)

#define x_iterprim(X)				x_firstobj((X))
#define x_iterval(X)				x_restobj((X))
#define x_iterempty(B,X)			x_obj_isnil((B), x_iterval(X))

#define x_mkiter(B, FN, L)			x_make_iter((B), X_OBJ_FLAG_NONE, (FN), (L))
#define x_mkfiter(B, F, FN, L)		x_make_iter((B), (F), (FN), (L))

/*
 * # Data Structures
 */
x_obj_t *x_make_iter(x_obj_t *p_base, x_obj_flag_t flags, void *p1, void *p2);
x_obj_t *x_type_iter_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_iter_struct(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_iter_make(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_iter_isempty(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_iter_next(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_ITER_H */
