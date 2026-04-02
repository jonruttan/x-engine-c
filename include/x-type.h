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
 #   (mark make free clone units length)
 #   (call eval)
 #   (from to)
 #   (analyse delimit write display error)
 # )
 * ```
 */
/*
 * # Includes
 */
#include "x-base-typesystem.h"

/*
 * # Defines
 */
#define x_mktype(B,T,X)               x_obj_make((B), (T), 0, 1, (X))

#define x_type_types(B)               (x_firstobj((B)))
#define x_type_settypes(B,X)          (x_type_types((B)) = (X))

#define x_type_field_name_stack(X)    x_firstobj(X)
#define x_type_field_name(X)          x_firstobj(x_type_field_name_stack((X)))

#define x_type_field_data_stack(X)    x_firstobj(x_restobj(X))
#define x_type_field_data(X)          x_firstobj(x_type_field_data_stack((X)))

#define x_type_field_heap(X)          x_firstobj(x_restobj(x_restobj(X)))
#define x_type_field_mark_stack(X)    x_firstobj(x_type_field_heap((X)))
#define x_type_field_mark(X)          x_firstobj(x_type_field_mark_stack((X)))
#define x_type_field_make_stack(X)    x_firstobj(x_restobj(x_type_field_heap((X))))
#define x_type_field_make(X)          x_firstobj(x_type_field_make_stack((X)))
#define x_type_field_free_stack(X)    x_firstobj(x_restobj(x_restobj(x_type_field_heap((X)))))
#define x_type_field_free(X)          x_firstobj(x_type_field_free_stack((X)))
#define x_type_field_clone_stack(X)   x_firstobj(x_restobj(x_restobj(x_restobj(x_type_field_heap((X))))))
#define x_type_field_clone(X)         x_firstobj(x_type_field_clone_stack((X)))
#define x_type_field_units_stack(X)   x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_type_field_heap((X)))))))
#define x_type_field_units(X)         x_firstobj(x_type_field_units_stack((X)))
#define x_type_field_length_stack(X)  x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_type_field_heap((X))))))))
#define x_type_field_length(X)        x_firstobj(x_type_field_length_stack((X)))

#define x_type_field_proc(X)          x_firstobj(x_restobj(x_restobj(x_restobj(X))))
#define x_type_field_call_stack(X)    x_firstobj(x_type_field_proc((X)))
#define x_type_field_call(X)          x_firstobj(x_type_field_call_stack((X)))
#define x_type_field_eval_stack(X)    x_firstobj(x_restobj(x_type_field_proc((X))))
#define x_type_field_eval(X)          x_firstobj(x_type_field_eval_stack((X)))

#define x_type_field_cvt(X)           x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(X)))))
#define x_type_field_from_stack(X)    x_firstobj(x_type_field_cvt((X)))
#define x_type_field_from(X)          x_firstobj(x_type_field_from_stack((X)))
#define x_type_field_to_stack(X)      x_firstobj(x_restobj(x_type_field_cvt((X))))
#define x_type_field_to(X)            x_firstobj(x_type_field_to_stack((X)))

#define x_type_field_io(X)            x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(X))))))
#define x_type_field_analyse_stack(X) x_firstobj(x_type_field_io((X)))
#define x_type_field_analyse(X)       x_firstobj(x_type_field_analyse_stack((X)))
#define x_type_field_delimit_stack(X) x_firstobj(x_restobj(x_type_field_io((X))))
#define x_type_field_delimit(X)       x_firstobj(x_type_field_delimit_stack((X)))
#define x_type_field_read_stack(X)    x_firstobj(x_restobj(x_restobj(x_type_field_io((X)))))
#define x_type_field_read(X)          x_firstobj(x_type_field_read_stack((X)))
#define x_type_field_write_stack(X)   x_firstobj(x_restobj(x_restobj(x_restobj(x_type_field_io((X))))))
#define x_type_field_write(X)         x_firstobj(x_type_field_write_stack((X)))
#define x_type_field_display_stack(X) x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_type_field_io((X)))))))
#define x_type_field_display(X)       x_firstobj(x_type_field_display_stack((X)))
#define x_type_field_error_stack(X)   x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_type_field_io((X))))))))
#define x_type_field_error(X)         x_firstobj(x_type_field_error_stack((X)))

#define x_type_field_iter_group(X)    x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(X)))))))
#define x_type_field_iter_stack(X)    x_firstobj(x_type_field_iter_group((X)))
#define x_type_field_iter(X)          x_firstobj(x_type_field_iter_stack((X)))

#define x_type_arg_type(X)            x_firstobj((X))

/*
 * # Data Structures
 */
struct x_type_t
{
	x_obj_t *p_name;
	x_obj_t *p_data;
	x_obj_t *p_mark;
	x_obj_t *p_make;
	x_obj_t *p_free;
	x_obj_t *p_clone;
	x_obj_t *p_units;
	x_obj_t *p_length;
	x_obj_t *p_call;
	x_obj_t *p_eval;
	x_obj_t *p_from;
	x_obj_t *p_to;
	x_obj_t *p_analyse;
	x_obj_t *p_delimit;
	x_obj_t *p_read;
	x_obj_t *p_write;
	x_obj_t *p_display;
	x_obj_t *p_error;
	x_obj_t *p_iter;
};


x_obj_t *x_type_struct_make(x_obj_t *p_base, struct x_type_t type);
x_obj_t *x_type_struct_get(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_write(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_display(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_error(x_obj_t *p_base, x_obj_t *p_args);

x_obj_t *x_type_prim_type_name(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_prim_units(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_prim_length(x_obj_t *p_base, x_obj_t *p_args);

x_obj_t *x_type_heap_mark(x_obj_t *p_base, x_obj_t *p_obj, x_obj_flag_t flags);
void x_type_heap_free(x_obj_t *p_base, x_obj_t *p_obj);

#endif /* X_TYPE_H */
