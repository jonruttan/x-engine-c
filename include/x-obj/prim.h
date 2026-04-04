#ifndef X_OBJ_PRIM_H
#define X_OBJ_PRIM_H

/**
 * @file x-obj/prim.h
 * @brief Object-level primitive operations (make, call, write, etc.).
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
#include "x-obj.h"

/** Return the type name string for an object. */
x_char_t *x_obj_prim_name(x_obj_t *p_base, x_obj_t *p_args);

/** Return the number of heap units occupied by an object. */
x_obj_t *x_obj_prim_units(x_obj_t *p_base, x_obj_t *p_args);
/** Return the logical length of an object. */
x_obj_t *x_obj_prim_length(x_obj_t *p_base, x_obj_t *p_args);

/** Construct a new object via its type's make callback. */
x_obj_t *x_obj_prim_make(x_obj_t *p_base, x_obj_t *p_args);
/** Free a heap object. */
x_obj_t *x_obj_prim_free(x_obj_t *p_base, x_obj_t *p_args);
/** Clone (shallow copy) a heap object. */
x_obj_t *x_obj_prim_clone(x_obj_t *p_base, x_obj_t *p_args);
/** Debug-dump an object's raw representation. */
x_obj_t *x_obj_prim_dump(x_obj_t *p_base, x_obj_t *p_args);

/** Invoke an object's type-dispatch call handler. */
x_obj_t *x_obj_prim_call(x_obj_t *p_base, x_obj_t *p_args);
/** Evaluate an object via its type's eval handler. */
x_obj_t *x_obj_prim_eval(x_obj_t *p_base, x_obj_t *p_args);
/** Convert an object to a different type. */
x_obj_t *x_obj_prim_convert(x_obj_t *p_base, x_obj_t *p_args);

/** Return the type identifier object for a value. */
x_obj_t *x_obj_prim_identify(x_obj_t *p_base, x_obj_t *p_args);
/** Read an object from the token stream. */
x_obj_t *x_obj_prim_read(x_obj_t *p_base, x_obj_t *p_args);
/** Write an object's external representation. */
x_obj_t *x_obj_prim_write(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_OBJ_PRIM_H */
