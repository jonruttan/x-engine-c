#ifndef X_SEXP_H
#define X_SEXP_H

/*
 * # Computational Expressions in C
 *
 * ## x-sexp.h -- Header - S-Expressions
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
#include "x-token.h"

x_obj_t *x_sexp_atom_write(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_sexp_read(x_obj_t *p_base, x_obj_t *args);
x_obj_t *x_sexp_write(x_obj_t *p_base, x_obj_t *args);

#endif /* X_SEXP_H */
