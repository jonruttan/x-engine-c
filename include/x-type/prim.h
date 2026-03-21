#ifndef X_TYPE_PRIM_H
#define X_TYPE_PRIM_H

/*
 * # Computational Expressions in C
 *
 * ## x-type/prim.h -- Header - Type - Primitive
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

#define X_TYPE_PRIM_NAME		"PRIMITIVE"
#define X_TYPE_PRIM_WRITE_STR	"#<prim>"
#define X_TYPE_PRIM_WRITE_LEN	7

/*
 * # Macros
 */
#define x_obj_type_isprim(B,X)	x_obj_is_type((B), (X), X_TYPE_PRIM_NAME)

#define x_primval(X)			x_firstfn((X))
#define x_callable_state(X)		x_secondobj((X))

#define x_mkprim(B,FN)			x_make_prim((B), X_OBJ_FLAG_NONE, (FN))
#define x_mkfprim(B,F,FN)		x_make_prim((B), (F), (FN))

/*
 * # Data Structures
 */
extern x_satom_t x_type_prim_name,
	x_type_prim_make_prim,
	x_type_prim_call_prim,
	x_type_prim_struct_prim;

x_obj_t *x_make_prim(x_obj_t *p_base, x_obj_flag_t flags, x_prim_fn fn);

x_obj_t *x_type_prim_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_prim_struct(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_prim_make(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_prim_call(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_prim_apply(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_prim_write(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_PRIM_H */
