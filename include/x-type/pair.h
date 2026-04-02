#ifndef X_TYPE_PAIR_H
#define X_TYPE_PAIR_H

/*
 * # Computational Expressions in C
 *
 * ## x-type/pair.h -- Header - Type - Pair
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2021 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 *
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
#define X_TYPE_PAIR_SYMBOL		"PAIR"
#endif /* X_TYPE_PAIR_SYMBOL */

/*
 * # Macros
 */
#define x_obj_type_ispair(B,X)		x_obj_is_type((B), (X), X_TYPE_PAIR_SYMBOL)

#define x_mkpair(B,P1,P2)			x_make_pair((B), X_OBJ_FLAG_NONE, (P1), (P2))
#define x_mkfpair(B,F,P1,P2)		x_make_pair((B), (F), (P1), (P2))

/*
 * # Data Structures
 */
x_obj_t *x_make_pair(x_obj_t *p_base, x_obj_flag_t flags, void *p1, void *p2);

x_obj_t *x_type_pair_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_pair_struct(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_pair_make(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_pair_length(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_PAIR_H */
