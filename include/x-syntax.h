#ifndef X_SYNTAX_H
#define X_SYNTAX_H

/**
 * @file x-syntax.h
 * @brief Syntax form helpers.
 *
 * Registration functions for the built-in syntax forms (operatives).
 * Each function binds a group of related primitives into the base
 * environment.
 *
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2026 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */

#include "x-obj.h"

/** Register quotation syntax: @c lit. */
x_obj_t *x_syntax_quote_register(x_obj_t *p_base, x_obj_t *p_args);

/** Register binding syntax: @c def, @c set!. */
x_obj_t *x_syntax_binding_register(x_obj_t *p_base, x_obj_t *p_args);

/** Register closure syntax: @c fn, @c op. */
x_obj_t *x_syntax_closure_register(x_obj_t *p_base, x_obj_t *p_args);

/** Register control-flow syntax: @c match, @c guard, @c error, @c %%seq. */
x_obj_t *x_syntax_control_register(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_SYNTAX_H */
