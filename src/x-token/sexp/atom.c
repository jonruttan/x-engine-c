/**
 * @file atom.c
 * @brief S-expression writer for opaque atom objects.
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2023 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */

#include "x-type/atom.h"

/**
 * Write the external representation of an atom to output.
 *
 * Produces the form @c #<type:0xADDR> where @e type is the registered
 * type name and @e ADDR is the object's integer value in hexadecimal.
 * Uses stack-allocated temporaries to avoid heap allocation.
 *
 * @param p_base  Execution context / base object.
 * @param p_args  Pair whose first element is the atom to write.
 * @return The @a p_args pointer unchanged.
 */
x_obj_t *x_sexp_atom_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_char_t tmp[22];
	x_char_t *type = x_obj_type_name(p_base, x_firstobj(p_args));
	x_satom_t str = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = NULL });
	x_spair_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { str }, { NULL });

	x_atomstr(str) = "#<";
	x_eval_write_str(p_base, (x_obj_t *)&wrap);

	x_atomstr(str) = type;
	x_eval_write_str(p_base, (x_obj_t *)&wrap);

	x_atomstr(str) = ":0x";
	x_eval_write_str(p_base, (x_obj_t *)&wrap);

	x_lib_inttostr(x_atomint(x_firstobj(p_args)), tmp, 16);
	x_atomstr(str) = tmp;
	x_eval_write_str(p_base, (x_obj_t *)&wrap);

	x_atomstr(str) = ">";
	x_eval_write_str(p_base, (x_obj_t *)&wrap);

	return p_args;
}
