#ifndef X_TYPE_H
#define X_TYPE_H

/*
 * # Computational Expressions in C
 *
 * ## x-obj.h -- Header - Type
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
 * # Object Types
 *
 * ```lang=lisp
 * '(
 #   name
 #   data
 #   (make free dup units length)
 #   (call eval convert)
 #   (analyse read write)
 # )
 * ```
 */
/*
 * # Includes
 */
#include "x-base.h"

/*
 * # Defines
 */
#define x_mktype(B,T,X)               x_obj_make((B), (T), 0, 1, (X))
#define x_mkmacro(B,X,Y)              x_obj_make((B), X_MACRO, 0, 2, (X), (Y))
#define x_macroargs(X)                x_firstobj((X))
#define x_macrocode(X)                x_restobj((X))
#define x_proc(B,X,Y,Z)               x_obj_make((B), X_PROCEDURE, 0, 3, (X), (Y), (Z))
#define x_procargs(X)                 x_firstobj((X))
#define x_proccode(X)                 x_secondobj((X))
#define x_procenv(X)                  x_obj(x_obj_data_i((X),2))

#define x_type_types(B)               (x_firstobj((B)))
#define x_type_settypes(B,X)          (x_type_types((B)) = (X))

#define x_type_field_name             x_car

#define x_type_field_data             x_cadr

#define x_type_field_heap             x_caddr
#define x_type_field_make(X)          x_car(x_type_field_heap((X)))
#define x_type_field_free(X)          x_cadr(x_type_field_heap((X)))
#define x_type_field_clone(X)         x_caddr(x_type_field_heap((X)))
#define x_type_field_units(X)         x_cadddr(x_type_field_heap((X)))
#define x_type_field_length(X)        x_car(x_cddddr(x_type_field_heap((X))))

#define x_type_field_proc(X)          x_cadddr((X))
#define x_type_field_call(X)          x_car(x_type_field_proc((X)))
#define x_type_field_eval(X)          x_cadr(x_type_field_proc((X)))
#define x_type_field_convert(X)       x_caddr(x_type_field_proc((X)))

#define x_type_field_io(X)            x_car(x_cddddr((X)))
#define x_type_field_analyse(X)       x_car(x_type_field_io((X)))
#define x_type_field_delimit(X)       x_cadr(x_type_field_io((X)))
#define x_type_field_write(X)         x_caddr(x_type_field_io((X)))

#define x_type_arg_type(X)            x_car((X))

/*
 * # Data Structures
 */
struct x_type_t
{
	x_obj_t *p_name;
	x_obj_t *p_data;
	x_obj_t *p_make;
	x_obj_t *p_free;
	x_obj_t *p_clone;
	x_obj_t *p_units;
	x_obj_t *p_length;
	x_obj_t *p_call;
	x_obj_t *p_eval;
	x_obj_t *p_convert;
	x_obj_t *p_analyse;
	x_obj_t *p_delimit;
	x_obj_t *p_write;
};


x_obj_t *x_type_struct_make(x_obj_t *p_base, struct x_type_t type);
x_obj_t *x_type_struct_get(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_write(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_H */
