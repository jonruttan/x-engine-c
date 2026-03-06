/*
 * # Computational Expressions in C
 *
 * ## x-obj.c -- Implementation - S-Expressions
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
#include "x-sexp.h"
#include "x-base.h"
#include "x-type.h"
#include "x-sexp/atom.h"
#include "x-sexp/pair.h"

/*
 * Reads a written representation of _args_ from input.
 *
 * @function x_sexp_read
 * @param {x_obj_t *} p_base A pointer to the p_base of the object structure.
 * @returns {x_obj_t *} A pointer to the object read.
 */
x_obj_t *x_sexp_read(x_obj_t *p_base, x_obj_t *p_obj)
{
	/*x_spair_t args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { { p_obj }, { NULL } })
	};
	x_obj_t *p_ret;*/

/*	if (x_obj_type_isspair(p_obj)) {
		return x_sexp_pair_write(p_base, p_obj);
	} else {*/
/*		if ( ! x_obj_isnil(p_base, x_obj_type(p_obj))) {
			p_ret = x_type_write(p_base, (x_obj_t *)args);

			if ( ! x_obj_isnil(p_base, p_ret)) {
				return p_ret;
			}
		}
*/
		/*return x_sexp_atom_write(p_base, p_obj);*/
/*	}*/
	return p_base;
}

/*
 * Writes a written representation of _args_ to output.
 *
 * @function x_sexp_write
 * @param {x_obj_t *} p_base A pointer to the p_base of the object structure.
 * @param {x_obj_t *} p_obj A pointer to the object to be written.
 * @returns {x_obj_t *} The object pointer passed as _args_.
 */
x_obj_t *x_sexp_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args);

	if (x_obj_type_issatom(p_obj)) {
		return x_sexp_atom_write(p_base, p_args);
	}

	if (x_obj_type_isspair(p_obj)) {
		return x_sexp_pair_write(p_base, p_args);
	}

	if ( ! x_obj_isnil(p_base, x_obj_type(p_obj))) {
		return x_type_write(p_base, p_args);
	}

	return p_base;
}
