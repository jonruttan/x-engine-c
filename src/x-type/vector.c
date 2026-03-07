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
	return x_vectorlen(p_obj) + 1;
}

x_int_t x_obj_vector_length(x_obj_t *p_base, x_obj_t *p_obj)
{
	return x_vectorlen(p_obj);
}

x_obj_t *x_obj_vector_proc(x_obj_t *p_base, x_obj_t *p_proc, x_obj_t *p_args)
{
	return x_vectorval(p_proc, x_firstint(x_firstobj(p_args)));
}

x_obj_t *x_obj_vector_read(x_obj_t *p_base, x_obj_t *p_args)
{
	return p_base;
}

x_obj_t *x_mkvector(x_obj_t *p_base, x_int_t count)
{
	(void)count;
	return NULL;
}
