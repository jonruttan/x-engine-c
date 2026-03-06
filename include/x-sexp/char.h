#ifndef X_SEXP_CHAR_H
#define X_SEXP_CHAR_H

/*
 * # Computational Expressions in C
 *
 * ## x-sexp/char.h -- Header - SExp - Character
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2023 Jon Ruttan
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
#include "x-sexp.h"

#ifndef X_SEXP_CHAR_PRE_STR
#define X_SEXP_CHAR_PRE_STR		"#\\"
#endif /* X_SEXP_CHAR_PRE */

/*
 * # Data Structures
 */
extern x_satom_t x_sexp_char_analyse1_prim,
 	x_sexp_char_analyse2_prim,
 	x_sexp_char_read_prim,
 	x_sexp_char_write_prim;

/*x_obj_t *x_sexp_char_read(x_obj_t *p_base, x_obj_t *p_args);*/
x_obj_t *x_sexp_char_analyse1(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_sexp_char_analyse2(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_sexp_char_read(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_sexp_char_write(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_SEXP_CHAR_H */
