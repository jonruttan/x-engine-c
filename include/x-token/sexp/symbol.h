#ifndef X_SEXP_SYMBOL_H
#define X_SEXP_SYMBOL_H

/**
 * @file symbol.h
 * @brief S-expression analyser, reader, writer, and display for symbols.
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

/** @name Analyser / reader / writer / display primitives (satom). */
/** @{ */
extern x_satom_t x_sexp_symbol_analyse_prim,
	x_sexp_symbol_read_prim,
	x_sexp_symbol_write_prim,
	x_sexp_symbol_display_prim;
/** @} */

/** Analyse: match any non-delimiter sequence as a symbol (negative score). */
x_obj_t *x_sexp_symbol_analyse(x_obj_t *p_base, x_obj_t *p_args);
/** Read a symbol from the token buffer (intern via type_symbol_make). */
x_obj_t *x_sexp_symbol_read(x_obj_t *p_base, x_obj_t *p_args);
/** Write the external representation of a symbol (@c (lit name)). */
/** Display a symbol as its bare name. */
x_obj_t *x_sexp_symbol_display(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_SEXP_SYMBOL_H */
