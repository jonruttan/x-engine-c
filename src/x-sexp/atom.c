/*
 * # Computational Expressions in C
 *
 * ## x-sexp/atom.c -- Implementation - SExp - Atom
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
#include "x-type/atom.h"

/*
 * Writes a written representation of _args_ atom to output.
 *
 * @function x_sexp_atom_write
 * @param {x_obj_t *} p_base A pointer to the p_base of the object structure.
 * @param {x_obj_t *} p_args A pointer to the object to be written.
 * @returns {x_obj_t *} The object pointer passed as _args_.
 */
x_obj_t *x_sexp_atom_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_char_t tmp[22];
	x_char_t *type = x_obj_type_name(p_base, x_firstobj(p_args));
	x_satom_t data_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .s = NULL }),
		size_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = 0 });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { data_obj }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { size_obj }, { NULL })
	};

	x_atomstr(data_obj) = "#<";
	x_atomint(size_obj) = 2;
	x_base_write(p_base, (x_obj_t *)args);

	x_atomstr(data_obj) = type;
	x_atomint(size_obj) = x_lib_strlen(type);
	x_base_write(p_base, (x_obj_t *)args);

	x_atomstr(data_obj) = ":0x";
	x_atomint(size_obj) = 3;
	x_base_write(p_base, (x_obj_t *)args);

	x_lib_inttostr(x_atomint(x_firstobj(p_args)), tmp, 16);
	x_atomstr(data_obj) = tmp;
	x_atomint(size_obj) = x_lib_strlen(tmp);
	x_base_write(p_base, (x_obj_t *)args);

	x_atomstr(data_obj) = ">";
	x_atomint(size_obj) = 1;
	x_base_write(p_base, (x_obj_t *)args);

	return p_args;
}
