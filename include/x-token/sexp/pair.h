#ifndef X_SEXP_PAIR_H
#define X_SEXP_PAIR_H

/**
 * @file pair.h
 * @brief S-expression writer and display for stack-allocated pair objects.
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2023 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */

#include "x-obj.h"

/** @name Write / display primitives (satom). */
/** @{ */
extern x_satom_t x_sexp_pair_write_prim,
	x_sexp_pair_display_prim;
/** @} */

/** Read a pair from the token stream (unused -- lists handle reading). */
x_obj_t *x_sexp_pair_read(x_obj_t *p_base, x_obj_t *args);
/** Write the external representation of a pair. */
x_obj_t *x_sexp_pair_write(x_obj_t *p_base, x_obj_t *args);
/** Display a pair in human-readable form. */
x_obj_t *x_sexp_pair_display(x_obj_t *p_base, x_obj_t *args);

#endif /* X_SEXP_PAIR_H */
