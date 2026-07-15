/**
 * @file atom.c
 * @brief S-expression writer for opaque atom objects.
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

#include "x-type/atom.h"

/**
 * Write the external representation of an atom to output.
 *
 * Produces the form @c #<type:0xADDR> where @e type is the registered
 * type name and @e ADDR is the object's integer value in hexadecimal.
 * Uses stack-allocated temporaries to avoid heap allocation.
 *
 * @param p_base  Execution context / base object.
 * @param p_args  Pair whose first element is the atom to write.
 * @return The @a p_args pointer unchanged.
 */
