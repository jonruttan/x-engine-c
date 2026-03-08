#ifndef X_SEXP_INT_H
#define X_SEXP_INT_H

/*
 * # Computational Expressions in C
 *
 * ## x-sexp/int.h -- Header - Sexp - Integer
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
extern x_satom_t x_sexp_int_analyse_sign_prim,
	x_sexp_int_analyse_prefix_prim,
	x_sexp_int_analyse_base_prim,
	x_sexp_int_analyse_digits_prim,
	x_sexp_int_analyse_xdigits_prim,
	x_sexp_int_read_prim,
	x_sexp_int_write_prim;


x_obj_t *x_sexp_int_analyse_digits(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_sexp_int_analyse_xdigits(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_sexp_int_analyse_base(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_sexp_int_analyse_prefix(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_sexp_int_analyse_sign(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_sexp_int_read(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_sexp_int_write(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_SEXP_INT_H */
