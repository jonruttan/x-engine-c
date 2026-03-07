#ifndef X_TYPE_OPERATIVE_H
#define X_TYPE_OPERATIVE_H

/*
 * # Computational Expressions in C
 *
 * ## x-type/operative.h -- Header - Type - Operative
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

#define X_TYPE_OPERATIVE_NAME		"OPERATIVE"
#define X_TYPE_OPERATIVE_WRITE_STR	"#<op>"
#define X_TYPE_OPERATIVE_WRITE_LEN	5

/*
 * # Macros
 */
#define x_obj_type_isoperative(B,X)	x_obj_is_type((B), (X), X_TYPE_OPERATIVE_NAME)

#define x_opparams(X)				x_firstobj((X))
#define x_openvparam(X)				x_secondobj((X))
#define x_opbody(X)					x_obj(x_obj_data_i((X),2))
#define x_openv(X)					x_obj(x_obj_data_i((X),3))

#define x_mkop(B,P,EP,BD,E)		x_make_operative((B), X_OBJ_FLAG_NONE, (P), (EP), (BD), (E))

/*
 * # Data Structures
 */
extern x_satom_t x_type_operative_name,
	x_type_operative_make_prim,
	x_type_operative_call_prim,
	x_type_operative_write_prim,
	x_type_operative_struct_prim;

x_obj_t *x_make_operative(x_obj_t *p_base, x_obj_flag_t flags,
	x_obj_t *p_params, x_obj_t *p_envparam, x_obj_t *p_body, x_obj_t *p_env);

x_obj_t *x_type_operative_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_operative_struct(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_operative_make(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_operative_call(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_operative_write(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_OPERATIVE_H */
