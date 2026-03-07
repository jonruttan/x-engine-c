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

#define x_type_types(B)               (x_firstobj((B)))
#define x_type_settypes(B,X)          (x_type_types((B)) = (X))

#define x_type_field_name(X)          x_firstobj(X)

#define x_type_field_data(X)          x_firstobj(x_restobj(X))

#define x_type_field_heap(X)          x_firstobj(x_restobj(x_restobj(X)))
#define x_type_field_make(X)          x_firstobj(x_type_field_heap((X)))
#define x_type_field_free(X)          x_firstobj(x_restobj(x_type_field_heap((X))))
#define x_type_field_clone(X)         x_firstobj(x_restobj(x_restobj(x_type_field_heap((X)))))
#define x_type_field_units(X)         x_firstobj(x_restobj(x_restobj(x_restobj(x_type_field_heap((X))))))
#define x_type_field_length(X)        x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_type_field_heap((X)))))))

#define x_type_field_proc(X)          x_firstobj(x_restobj(x_restobj(x_restobj(X))))
#define x_type_field_call(X)          x_firstobj(x_type_field_proc((X)))
#define x_type_field_eval(X)          x_firstobj(x_restobj(x_type_field_proc((X))))
#define x_type_field_convert(X)       x_firstobj(x_restobj(x_restobj(x_type_field_proc((X)))))

#define x_type_field_io(X)            x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(X)))))
#define x_type_field_analyse(X)       x_firstobj(x_type_field_io((X)))
#define x_type_field_delimit(X)       x_firstobj(x_restobj(x_type_field_io((X))))
#define x_type_field_write(X)         x_firstobj(x_restobj(x_restobj(x_type_field_io((X)))))

#define x_type_arg_type(X)            x_firstobj((X))

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
