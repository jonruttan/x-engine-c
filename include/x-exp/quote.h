#ifndef X_EXP_QUOTE_H
#define X_EXP_QUOTE_H

/*
 * # Computational Expressions in C
 *
 * ## x-sexp/comment.h -- Header - Exp - Quote
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
extern x_satom_t x_exp_quote_prim;

x_obj_t *x_exp_quote(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_EXP_QUOTE_H */
