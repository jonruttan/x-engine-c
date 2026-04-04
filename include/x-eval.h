#ifndef X_EVAL_H
#define X_EVAL_H

/**
 * @file x-eval.h
 * @brief Evaluation interface.
 *
 * Declares the central evaluator entry point and the argument-access
 * macro used to extract the expression from an eval argument list.
 *
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2021 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */

#include "x-obj.h"

/** @name Argument Access Macros
 *  @{ */

#define x_eval_arg_exp(X)		x_0((X)) /**< Extract the expression from eval args. */

/** @} */

/** Evaluate an expression in the current environment. */
x_obj_t *x_eval(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_EVAL_H */
