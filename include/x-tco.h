#ifndef X_TCO_H
#define X_TCO_H

/**
 * @file x-tco.h
 * @brief The environment save and restore around a tail call.
 *
 * A procedure call and eval-with-env push the environment they are leaving
 * onto the save-stack before making another one current; the trampoline,
 * or x_eval_body_tco's early exits, put it back.  The environment is a
 * value, so each is one pointer.
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

/** Push the current environment onto the save-stack and return it. */
x_obj_t *x_tco_env_save(x_obj_t *p_base);

/** Make @p p_env the current environment.  Does NOT pop the save-stack;
 *  a caller that took the environment from the save-stack top pops it
 *  separately. */
void x_tco_restore(x_obj_t *p_base, x_obj_t *p_env);

#endif /* X_TCO_H */
