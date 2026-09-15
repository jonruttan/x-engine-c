#ifndef X_TOPLEVEL_H
#define X_TOPLEVEL_H

/**
 * @file x-toplevel.h
 * @brief The top-level bracket: what a form evaluated at top level sees,
 *        whatever environment is current when it is asked for.
 *
 * One implementation, two doors -- x_eval_load around a file's forms,
 * eval! around one form.  The save-stack is hidden and the root is made
 * current; the displaced state is parked on the root chain for the
 * bracket's length, because a C local is not a root.
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

/** The caller's bracket state: what was displaced, and the node it is
 *  parked on. */
typedef struct x_toplevel_t {
	x_obj_t *p_saved_stack, *p_saved_env;
	x_spair_t parked;   /* the displaced state, rooted for the bracket's length */
} x_toplevel_t;

/** Enter the bracket, filling @p p_t. */
void x_toplevel_enter(x_obj_t *p_base, x_toplevel_t *p_t);

/** Leave the bracket: unroot the parked state and put it back. */
void x_toplevel_leave(x_obj_t *p_base, x_toplevel_t *p_t);

#endif /* X_TOPLEVEL_H */
