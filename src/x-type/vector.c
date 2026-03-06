/*
 * # Computational Expressions in C
 *
 * ## x-type.c -- Implementation - Type - Vector
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
#include "x-type/vector.h"
#include "x-type/int.h"
#include "x-type/str.h"

/*
 * The *Vector* object type's byte calculation function.
 *
 * @function x_obj_vector_units
 * @param    {x_obj_t *} p_obj A pointer to the vector object.
 * @returns  int_t The length of the Vector object in atoms.
 */
x_int_t x_obj_vector_units(x_obj_t *p_base, x_obj_t *p_obj)
{
	return x_intval(x_vectorlen(p_obj)) + 1;
}

x_int_t x_obj_vector_length(x_obj_t *p_base, x_obj_t *p_obj)
{
	return x_intval(x_vectorlen(p_obj));
}

x_obj_t *x_obj_vector_proc(x_obj_t *p_base, x_obj_t *p_proc, x_obj_t *p_args)
{
	return x_vectorval(p_proc, x_intval(x_car(p_args)));
}

x_obj_t *x_obj_vector_read(x_obj_t *p_base, x_obj_t *p_args)
{
	/* TODO: Fix this. */
/*	if (x_strval(p_args)[0] == '#' && x_lib_strlen(x_strval(p_args)) == 1) {
		return x_cons(p_base, x_car(x_findsym(p_base, _X_VECTOR)), x_readobj(p_base));
	}
*/
	return p_base;
}

x_obj_t *x_mkvector(x_obj_t *p_base, x_int_t count)
{
	/* TODO: Fix this. */
	x_obj_t *ret = NULL; /*x_obj_alloc(p_base, X_VECTOR, 0, count + 1);
	x_firstint(ret) = x_mkint(p_base, count);*/

	return ret;
}
