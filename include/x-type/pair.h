#ifndef X_TYPE_PAIR_H
#define X_TYPE_PAIR_H

/**
 * @file pair.h
 * @brief Pair (cons cell) type for the x-lang type system.
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
/*
 * # Includes
 */
#include "x-type.h"

#ifndef X_TYPE_PAIR_SYMBOL
#define X_TYPE_PAIR_SYMBOL		"PAIR"		/**< Type-system symbol name */
#endif /* X_TYPE_PAIR_SYMBOL */

/*
 * # Macros
 */
/** Test whether object X is a pair on base B. */
#define x_obj_type_ispair(B,X)		x_obj_is_type((B), (X), X_TYPE_PAIR_SYMBOL)

/** Make a pair with default flags. */
#define x_mkpair(B,P1,P2)			x_make_pair((B), X_OBJ_FLAG_NONE, (P1), (P2))
/** Make a pair with explicit flags F. */
#define x_mkfpair(B,F,P1,P2)		x_make_pair((B), (F), (P1), (P2))

/*
 * # Data Structures
 */
/** Allocate a heap pair from first/rest pointers. */
x_obj_t *x_make_pair(x_obj_t *p_base, x_obj_flag_t flags, void *p1, void *p2);

/** Register (or retrieve) the pair type struct on p_base. */
x_obj_t *x_type_pair_register(x_obj_t *p_base, x_obj_t *p_args);
/** Build the pair type descriptor struct. */
x_obj_t *x_type_pair_struct(x_obj_t *p_base, x_obj_t *p_args);
/** Type-system make callback for pair. */
x_obj_t *x_type_pair_make(x_obj_t *p_base, x_obj_t *p_args);
/** Compute the length of a pair list. */
x_obj_t *x_type_pair_length(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_PAIR_H */
