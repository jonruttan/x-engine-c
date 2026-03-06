#ifndef X_TOKEN_H
#define X_TOKEN_H

/*
 * # Computational Expressions in C
 *
 * ## x-token.h -- Header - Token
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
#include "x-obj.h"

/*
 * # Defines
 */
#define x_token_read_arg_prim(X)		x_0((X))
#define x_token_read_arg_buffer(X)		x_0((X))
#define x_token_read_arg_score(X)		x_01((X))

/*
 * # Data Structures
 */
x_obj_t *x_token_delimit(x_obj_t *p_base, x_obj_t *p_obj);
x_obj_t *x_token_analyse(x_obj_t *p_base, x_obj_t *p_obj);
x_obj_t *x_token_read(x_obj_t *p_base, x_obj_t *p_obj);

#endif /* X_TOKEN_H */
