#ifndef X_SEXP_SYMBOL_H
#define X_SEXP_SYMBOL_H

/**
 * @file symbol.h
 * @brief S-expression analyser and reader for symbols.
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

/** @name Analyser / reader primitives (satom). */
/** @{ */
extern x_satom_t x_sexp_symbol_analyse_prim,
	x_sexp_symbol_read_prim;
/** @} */

/** Analyse: match any non-delimiter sequence as a symbol (negative score). */
x_obj_t *x_sexp_symbol_analyse(x_obj_t *p_base, x_obj_t *p_args);
/** Read a symbol from the token buffer (intern via type_symbol_make). */
x_obj_t *x_sexp_symbol_read(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_SEXP_SYMBOL_H */
