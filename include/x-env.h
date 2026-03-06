#ifndef X_ENV_H
#define X_ENV_H

/*
 * # Computational Expressions in C
 *
 * ## x-env.h -- Header - Environment
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2022 Jon Ruttan
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
#include "src/x-type/symbol.c"

/*
 * # Macros
 */

/*
 * # Data Structures
 */
x_obj_t *x_env_assoc(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_ENV_H */
