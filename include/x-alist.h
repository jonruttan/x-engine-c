#ifndef X_ALIST_H
#define X_ALIST_H

/*
 * # Computational Expressions in C
 *
 * ## x-eval.h -- Header - Evaluator
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
 * # Environment Managment Functions
 */
/*#define x_alist_extend(BASE, ALIST, SYM, VAL) (x_cons((BASE), x_cons((BASE), (SYM), (VAL)), (ALIST)))*/

x_obj_t *x_alist_extend(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_alist_assoc(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_ALIST_H */
