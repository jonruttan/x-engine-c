#ifndef X_SEXP_PAIR_H
#define X_SEXP_PAIR_H

/**
 * @file pair.h
 * @brief S-expression reader declarations for stack-allocated pair objects.
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

#include "x-obj.h"

/* Pair reading is handled by the list reader; pair printing lives in
 * lib/x/boot/printer.x.  This header remains for the spec TUs that
 * include the (now-empty) pair.c. */

#endif /* X_SEXP_PAIR_H */
