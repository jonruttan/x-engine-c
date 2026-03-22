#ifndef X_SYNTAX_H
#define X_SYNTAX_H

/*
 * # Computational Expressions in C
 *
 * ## x-syntax.h -- Header - Syntax Forms (Operatives)
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2024 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 *
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */

#include "x-obj.h"

x_obj_t *x_syntax_quote_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_syntax_binding_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_syntax_closure_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_syntax_control_register(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_SYNTAX_H */
