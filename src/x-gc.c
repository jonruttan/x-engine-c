/*
 * # Computational Expressions in C
 *
 * ## x-gc.c -- Implementation - Garbage Collection
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
#include "x-gc.h"

#ifdef X_GC

/*
 * # Garbage Collection Functions
 */
x_obj_t *x_gc_mark(x_obj_t *p_base, x_obj_t *p_obj, x_obj_flag_t flags)
{
	union x_datum_union *tmp;
	short units;

	/* Avoid recursion */
	if ((x_obj_flags(p_obj) & flags) == flags
			|| (x_obj_flags(p_obj) |= flags) & X_OBJ_FLAG_PRIM
			|| ! (units = x_obj_units(p_base, p_obj))) {
		return p_obj;
	}

	tmp = x_obj_data_ptr(p_obj) + units - 1;

	while (tmp >= x_obj_data_ptr(p_obj)) {
		x_gc_mark(p_base, x_obj(*tmp--), flags);
	}

	return p_obj;
}

/*
 * NOTE: If the top object is deleted the GC structure will fragment.
 */
x_obj_t *x_gc_sweep(x_obj_t *p_base, x_obj_t *p_obj, x_obj_flag_t flags)
{
	x_obj_t *gc = p_obj, *tmp,
		*prev = x_obj_gc(p_base) == p_obj ? p_base : p_obj;

	while (gc) {
		if (flags && x_obj_flags(gc) & flags) {
			x_obj_flags(gc) &= ~flags;
			prev = gc;
			gc = x_obj_gc(gc);
		} else {
			tmp = x_obj_gc(prev) = x_obj_gc(gc);
			x_obj_free(gc);
			gc = tmp;
		}
	}

	return p_base;
}

#endif /* X_GC */
