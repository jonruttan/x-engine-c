#ifndef X_TYPE_LIST_H
#define X_TYPE_LIST_H

/*
 * # Computational Expressions in C
 *
 * ## x-type/list.h -- Header - Type - List
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2024 Jon Ruttan
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

#ifndef X_TYPE_LIST_NAME
#define X_TYPE_LIST_NAME		"LIST"
#endif /* X_TYPE_LIST_NAME */

/*
 * # Macros
 */
#define x_eval_arg_exp(X)			x_0((X))

#define x_obj_type_islist(B,X)		x_obj_is_type((B), (X), X_TYPE_LIST_NAME)

#define x_mklist(B,P1,P2)			x_make_list((B), X_OBJ_FLAG_NONE, (P1), (P2))
#define x_mkflist(B,F,P1,P2)		x_make_list((B), (F), (P1), (P2))

/*
 * # Data Structures
 */
x_obj_t *x_make_list(x_obj_t *p_base, x_obj_flag_t flags, void *p1, void *p2);
x_obj_t *x_type_list_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_list_struct(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_list_make(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_list_length(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_list_call(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_list_eval(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_list_iter(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_LIST_H */
