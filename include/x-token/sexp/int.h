#ifndef X_SEXP_INT_H
#define X_SEXP_INT_H

/**
 * @file int.h
 * @brief S-expression analyser, reader, and writer for integer literals.
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

/** @name Analyser state primitives (spair -- composable state machines). */
/** @{ */
extern x_spair_t x_sexp_int_analyse_sign_prim,
	x_sexp_int_analyse_prefix_prim,
	x_sexp_int_analyse_base_prim,
	x_sexp_int_analyse_digits_prim,
	x_sexp_int_analyse_xdigits_prim;
/** @} */

/** @name Read / write primitives (satom -- type-internal). */
/** @{ */
extern x_satom_t x_sexp_int_read_prim,
	x_sexp_int_write_prim;
/** @} */

/** Analyse: consume decimal digits and score. */
x_obj_t *x_sexp_int_analyse_digits(x_obj_t *p_base, x_obj_t *p_args);
/** Analyse: consume hexadecimal digits and score. */
x_obj_t *x_sexp_int_analyse_xdigits(x_obj_t *p_base, x_obj_t *p_args);
/** Analyse: detect @c 0x hex prefix or fall through to decimal digits. */
x_obj_t *x_sexp_int_analyse_base(x_obj_t *p_base, x_obj_t *p_args);
/** Analyse: handle leading @c 0 (possible hex prefix) or first digit. */
x_obj_t *x_sexp_int_analyse_prefix(x_obj_t *p_base, x_obj_t *p_args);
/** Analyse: handle optional leading @c +/- sign. */
x_obj_t *x_sexp_int_analyse_sign(x_obj_t *p_base, x_obj_t *p_args);
/** Read an integer literal from the token buffer. */
x_obj_t *x_sexp_int_read(x_obj_t *p_base, x_obj_t *p_args);
/** Write the decimal representation of an integer. */

#endif /* X_SEXP_INT_H */
