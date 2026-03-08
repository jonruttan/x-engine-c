#ifndef X_SEXP_SYMBOL_H
#define X_SEXP_SYMBOL_H

/*
 * # Computational Expressions in C
 *
 * ## x-obj.h -- Header - Sexp - Symbol
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
extern x_satom_t x_sexp_symbol_analyse_prim,
	x_sexp_symbol_read_prim,
	x_sexp_symbol_write_prim;

x_obj_t *x_sexp_symbol_analyse(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_sexp_symbol_read(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_sexp_symbol_write(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_SEXP_SYMBOL_H */
