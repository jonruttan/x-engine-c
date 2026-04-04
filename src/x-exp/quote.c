/**
 * @file x-exp/quote.c
 * @brief Quote expression -- return datum unevaluated.
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2024 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
#include "x-exp/quote.h"


x_satom_t x_exp_quote_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_exp_quote });

/**
 * Return the first argument unevaluated. x-lang: (quote datum)
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- (self datum)
 * @return The datum unchanged
 */
x_obj_t *x_exp_quote(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_0(p_args);
}
