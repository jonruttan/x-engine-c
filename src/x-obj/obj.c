/**
 * @file x-obj/obj.c
 * @brief The interpreter object (a specialized base object) sentinel, and
 *        the interpreter's allocation of an object.
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
#include "x-obj/prim.h"

/** Static atom for the interpreter object -- this project's root base object
 *  (sentinel label "BASE"). */
x_satom_t x_eval_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, {.s = (x_char_t *)"BASE"});

/**
 * Allocate an object: the routine in the slot vector (X_SLOT_OBJ_ALLOC)
 * the engine allocates an object through, so a base may allocate its own
 * way. It is x_obj_alloc() with its arguments read from the run.
 *
 * @param p_base  Base (execution context), or NULL to allocate without one.
 * @param p_args  Argument run: (type, flags, units).
 * @return The new object, or NULL when no base is attached and the
 *         allocation failed (see x_obj_alloc()).
 */
x_obj_t *x_eval_alloc(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_obj_alloc(p_base, x_obj(p_args[0]),
		(x_obj_flag_t)p_args[1].i, (size_t)p_args[2].i);
}
