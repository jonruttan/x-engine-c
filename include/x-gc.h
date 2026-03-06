#ifndef X_GC_H
#define X_GC_H

/*
 * # Computational Expressions in C
 *
 * ## x-gc.h -- Header - Garbage Collection
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

#ifndef X_GC
#warning X_GC is required
#else /* X_GC */

/*
 * # Garbage Collection Functions
 */
x_obj_t *x_gc_mark(x_obj_t *p_base, x_obj_t *p_obj, x_obj_flag_t flags);
x_obj_t *x_gc_sweep(x_obj_t *p_base, x_obj_t *p_obj, x_obj_flag_t flags);
#endif /* X_GC */

#endif /* X_GC_H */
