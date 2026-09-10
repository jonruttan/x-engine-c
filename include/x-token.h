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
 * THE VARIANT CHANNEL.  An analyser knows things about the token it accepts
 * that the text alone does not say -- which of its states ran, whether a
 * numeric literal passed through a fraction or an exponent -- and used to
 * throw them away, leaving the reader to rescan the text.  The score cell
 * an analyser is handed now carries a VARIANT cell on its rest: the state
 * writes an integer there as it accepts (x-lang's %score-variant!, a plain
 * set-cell-int! on the cell; compiled states call jit_score_variant), the
 * analyse loop records the winning handler's variant, and x_token_read hands
 * it to the type's reader as the reader's SECOND argument -- a raw ATOM
 * CELL whose value word is the integer (x_atomint in C, %cell-int in
 * x-lang; nil when no state declared one, which is every type that never
 * heard of the channel).  A cell rather than an int because an int object
 * only exists relative to a base that registered the int type, and a
 * tokenizer base has none by design.  Readers already received (buffer ())
 * so nothing about their arity changes.
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

/** In a READER's argument list, (buffer variant): the variant the accepting
 *  analyser declared, a raw atom cell (read it with x_atomint), or NULL
 *  when none did. */
#define x_token_read_arg_variant(X)		x_01((X))

/** @} */

/** @name Tokenization Pipeline
 *  @{ */

/** Check whether @a p_obj delimits the current token for any type. */
x_obj_t *x_token_delimit(x_obj_t *p_base, x_obj_t *p_obj);

/** Run per-type analysis on a completed token buffer.  Writes the winning
 *  handler's declared variant (0 when none) through @p p_variant. */
x_obj_t *x_token_analyse(x_obj_t *p_base, x_obj_t *p_obj, x_int_t *p_variant);

/** Read a single token from the input stream. */
x_obj_t *x_token_read(x_obj_t *p_base, x_obj_t *p_obj);

/** Clean end-of-input sentinel, returned by x_token_read when analysis
 *  finds no token with zero consumption.  Distinguishes true EOF from a
 *  read NIL VALUE (`()` reads as NULL by design).  Compared by ADDRESS
 *  in C; bound as %token-eof for x-lang, where identity is (obj same?)
 *  -- never eq?, which compares value words. */
extern x_satom_t x_token_eof_prim;

/** @} */

#endif /* X_TOKEN_H */
