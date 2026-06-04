/**
 * @file x-obj/obj.c
 * @brief The interpreter object (a specialized base object) sentinel.
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

/** Static atom for the interpreter object -- this project's root base object
 *  (sentinel tag "BASE"). */
x_satom_t x_eval_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, {.s = (x_char_t *)"BASE"});
