#ifndef X_EVAL_H
#define X_EVAL_H

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
 * # Macros
 */
#define x_eval_arg_exp(X)		x_0((X))

/*
 * # Data Structures
 */
x_obj_t *x_eval(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_EVAL_H */
