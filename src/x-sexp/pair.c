/*
 * # Computational Expressions in C
 *
 * ## x-sexp/pair.c -- Implementation - SExp - Pair
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2023 Jon Ruttan
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
#include "x-type/pair.h"
#include "x-sexp.h"

/*
 * Writes a written representation of _args_ pair to output.
 *
 * @function x_sexp_pair_write
 * @param {x_obj_t *} p_base A pointer to the p_base of the object structure.
 * @param {x_obj_t *} p_args A pointer to the pair to be written.
 * @returns {x_obj_t *} The object pointer passed as _args_.
 */
x_obj_t *x_sexp_pair_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args);
	x_satom_t data_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .s = NULL }),
		size_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = 1 }),
		write_wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { data_obj }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { size_obj }, { NULL })
	};

	x_atomstr(data_obj) = "(";
	x_base_write(p_base, (x_obj_t *)args);

	for (;;) {
		if ( ! x_obj_isnil(p_base, x_firstobj(p_obj))) {
			x_firstobj((x_obj_t *)write_wrap) = x_firstobj(p_obj);
			x_sexp_write(p_base, (x_obj_t *)write_wrap);
		}

		p_obj = x_restobj(p_obj);

		if (x_obj_isnil(p_base, p_obj)) {
			x_atomstr(data_obj) = ")";
			x_base_write(p_base, (x_obj_t *)args);

			break;
		}

		if ( ! x_obj_type_isspair(p_obj)) {
			x_atomstr(data_obj) = " . ";
			x_atomint(size_obj) = 3;
			x_base_write(p_base, (x_obj_t *)args);

			x_firstobj((x_obj_t *)write_wrap) = p_obj;
			x_sexp_write(p_base, (x_obj_t *)write_wrap);

			x_atomstr(data_obj) = ")";
			x_atomint(size_obj) = 1;
			x_base_write(p_base, (x_obj_t *)args);

			break;
		}

		x_atomstr(data_obj) = " ";
		x_base_write(p_base, (x_obj_t *)args);
	}

	return p_obj;
}
