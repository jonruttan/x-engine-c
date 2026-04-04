#ifndef X_EXP_QUOTE_H
#define X_EXP_QUOTE_H

/**
 * @file x-exp/quote.h
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
#include "x-obj.h"

/** Static atom wrapping the x_exp_quote function pointer. */
extern x_satom_t x_exp_quote_prim;

/** Return the first argument unevaluated. x-lang: (quote datum) */
x_obj_t *x_exp_quote(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_EXP_QUOTE_H */
