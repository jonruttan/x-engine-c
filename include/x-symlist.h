#ifndef X_SYMLIST_H
#define X_SYMLIST_H

/*
 * # Computational Expressions in C
 *
 * ## x-token.h -- Header - Symbol List
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
#include "x-token.h"

x_obj_t *x_symlist_read(x_obj_t *p_base, x_obj_t *p_obj);

#endif /* X_SYMLIST_H */
