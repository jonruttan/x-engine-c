#ifndef X_TYPE_SYMBOL_H
#define X_TYPE_SYMBOL_H

/*
 * # Computational Expressions in C
 *
 * ## x-obj.h -- Header - Type - Symbol
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
#include "x-type.h"

#define X_TYPE_SYMBOL_NAME	"SYMBOL"

/*
 * # Macros
 */
#define x_obj_type_issymbol(B,X)	x_obj_is_type((B), (X), X_TYPE_SYMBOL_NAME)

#define x_symbolval(X)				x_firststr((X))
#define x_symbolname(X)				x_firststr((X))

#define x_symbol_data				x_type_field_data
#define x_symbol_data_list(X)		x_0(x_symbol_data((X)))

#define x_mksymbol(B, S)			x_make_symbol((B), X_OBJ_FLAG_NONE, (S))
#define x_mkfsymbol(B, F, S)		x_make_symbol((B), (F), (S))
#define x_mksymbolown(B, S)			x_make_symbol((B), X_OBJ_FLAG_OWN, (S))
#define x_mkfsymbolown(B, F, S)		x_make_symbol((B), X_OBJ_FLAG_OWN | (F), (S))

/*
 * # Data Structures
 */
x_obj_t *x_make_symbol(x_obj_t *p_base, x_obj_flag_t flags, x_char_t *s);

x_obj_t *x_type_symbol_struct(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_symbol_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_symbol_init(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_symbol_make(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_symbol_find(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_symbol_eval(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_symbol_write(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_SYMBOL_H */
