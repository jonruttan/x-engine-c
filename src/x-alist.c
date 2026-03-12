/*
 * # Computational Expressions in C
 *
 * ## x-obj.c -- Implementation - Objects
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
#include "x-alist.h"

x_obj_t *x_alist_extend(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_assoc = x_firstobj(p_args), *p_alist = x_restobj(p_args);

	return x_mkspair(p_base, p_assoc, p_alist);
}

x_obj_t *x_alist_assoc(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args),
		*p_alist = x_firstobj(x_restobj(p_args));

	while ( ! x_obj_isnil(p_base, p_alist)) {
		if (x_firstobj(x_firstobj(x_firstobj(p_alist))) == x_firstobj(p_obj)) {
			return x_firstobj(p_alist);
		}

		p_alist = x_restobj(p_alist);
	}

	return NULL;
}
