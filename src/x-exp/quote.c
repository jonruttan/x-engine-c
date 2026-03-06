/*
 * # Computational Expressions in C
 *
 * ## x-exp/quote.c -- Implementation - Expressions - Quote
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
#include "x-exp/quote.h"


x_satom_t x_exp_quote_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_exp_quote });

x_obj_t *x_exp_quote(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_0(p_args);
}
