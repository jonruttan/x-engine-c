#ifndef X_SEXP_PAIR_H
#define X_SEXP_PAIR_H

/*
 * # Computational Expressions in C
 *
 * ## x-sexp/pair.h -- Header - SExp - Pair
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2023 Jon Ruttan
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
extern x_satom_t x_sexp_pair_write_prim;

x_obj_t *x_sexp_pair_read(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_sexp_pair_write(x_obj_t *p_base, x_obj_t *args);

#endif /* X_SEXP_PAIR_H */
