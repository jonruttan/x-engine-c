#ifndef X_TOKEN_H
#define X_TOKEN_H

/**
 * @file x-token.h
 * @brief Tokenization interface.
 *
 * Declares the type-dispatched tokenization pipeline: delimiting,
 * analysing, reading, writing, and displaying.  Each stage iterates
 * the registered type alist and delegates to per-type handlers.
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

/** @name Reader Argument Access Macros
 *  Decompose the argument list passed to token reader callbacks.
 *  Layout: @c ((prim/buffer . (score . (... (char . ()) ...)))).
 *  @{ */

#define x_token_read_arg_prim(X)		x_0((X))    /**< Reader primitive / buffer object. */
#define x_token_read_arg_buffer(X)		x_0((X))    /**< Alias -- buffer object for the reader. */
#define x_token_read_arg_score(X)		x_01((X))   /**< Current best score (integer). */
#define x_token_read_arg_char(X)		x_0(x_11((X))) /**< Lookahead character. */

/** @} */

/** @name Tokenization Pipeline
 *  @{ */

/** Check whether @a p_obj delimits the current token for any type. */
x_obj_t *x_token_delimit(x_obj_t *p_base, x_obj_t *p_obj);

/** Run per-type analysis on a completed token buffer. */
x_obj_t *x_token_analyse(x_obj_t *p_base, x_obj_t *p_obj);

/** Read a single token from the input stream. */
x_obj_t *x_token_read(x_obj_t *p_base, x_obj_t *p_obj);

/** Serialise an object to its written (machine-readable) form. */

/** Serialise an object to its display (human-readable) form. */

/** @} */

#endif /* X_TOKEN_H */
