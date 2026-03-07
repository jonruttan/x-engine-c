#ifndef X_SEXP_ATOM_H
#define X_SEXP_ATOM_H

/*
 * # Computational Expressions in C
 *
 * ## x-sexp/atom.h -- Header - SExp - Atom
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
#include "x-sexp.h"

/*
 * # Data Structures
 */
x_obj_t *x_sexp_atom_write(x_obj_t *p_base, x_obj_t *args);

#endif /* X_SEXP_PAIR_H */
