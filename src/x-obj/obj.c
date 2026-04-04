/**
 * @file x-obj/obj.c
 * @brief Base type sentinel object.
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

/** Static atom for the "BASE" root type sentinel. */
x_satom_t x_type_base_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, {.s = (x_char_t *)"BASE"});
