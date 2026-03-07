#ifndef X_TYPE_PROCEDURE_H
#define X_TYPE_PROCEDURE_H

/*
 * # Computational Expressions in C
 *
 * ## x-type/procedure.h -- Header - Type - Procedure
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

#define X_TYPE_PROCEDURE_NAME	"PROCEDURE"

/*
 * # Macros
 */
#define x_obj_type_isprocedure(B,X)	x_obj_is_type((B), (X), X_TYPE_PROCEDURE_NAME)

#define x_procparams(X)				x_firstobj((X))
#define x_procbody(X)				x_secondobj((X))
#define x_procenv(X)				x_obj(x_obj_data_i((X),2))

#define x_mkproc(B,P,BD,E)			x_make_procedure((B), X_OBJ_FLAG_NONE, (P), (BD), (E))
#define x_mkfproc(B,F,P,BD,E)		x_make_procedure((B), (F), (P), (BD), (E))

/*
 * # Data Structures
 */
extern x_satom_t x_type_procedure_name,
	x_type_procedure_make_prim,
	x_type_procedure_call_prim,
	x_type_procedure_write_prim,
	x_type_procedure_struct_prim;

x_obj_t *x_make_procedure(x_obj_t *p_base, x_obj_flag_t flags,
	x_obj_t *p_params, x_obj_t *p_body, x_obj_t *p_env);

x_obj_t *x_type_procedure_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_procedure_struct(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_procedure_make(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_procedure_call(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_procedure_write(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_PROCEDURE_H */
