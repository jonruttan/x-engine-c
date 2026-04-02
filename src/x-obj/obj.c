/*
 * # Computational Expressions in C
 *
 * ## x-obj/obj.c -- Implementation - Objects - Base Type
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
#include "x-obj.h"


x_satom_t x_type_base_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, {.s = (x_char_t *)"BASE"});
