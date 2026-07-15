/**
 * @file pair.c
 * @brief S-expression support for stack-allocated pair objects (printing
 *        moved to the pure-x printer, lib/x/boot/printer.x; reading is
 *        handled by lists).
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2023 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */

#include "x-type/pair.h"
#include "x-token.h"

