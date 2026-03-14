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
	x_satom_t str = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = NULL });
	x_spair_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { str }, { NULL });

	x_atomstr(str) = "#<";
	x_base_write_str(p_base, (x_obj_t *)&wrap);

	x_atomstr(str) = type;
	x_base_write_str(p_base, (x_obj_t *)&wrap);

	x_atomstr(str) = ":0x";
	x_base_write_str(p_base, (x_obj_t *)&wrap);

	x_lib_inttostr(x_atomint(x_firstobj(p_args)), tmp, 16);
	x_atomstr(str) = tmp;
	x_base_write_str(p_base, (x_obj_t *)&wrap);

	x_atomstr(str) = ">";
	x_base_write_str(p_base, (x_obj_t *)&wrap);

	return p_args;
}
