/**
 * @file pair.c
 * @brief S-expression writer and display for stack-allocated pair objects.
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

#include "x-type/pair.h"
#include "x-token.h"

/**
 * Write the external representation of a stack-allocated pair.
 *
 * Walks the pair chain, outputting elements in parenthesised form.
 * Non-spair tails are rendered with dot notation.  Uses
 * @c x_obj_type_isspair to distinguish spairs from heap pairs
 * (heap pairs are handled by the list writer).
 *
 * @param p_base  Execution context.
 * @param p_args  Pair whose first element is the pair to write.
 * @return The tail of the pair after writing.
 */
x_obj_t *x_sexp_pair_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args);
	x_satom_t str = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = NULL }),
		write_wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL });
	x_spair_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { str }, { NULL });

	x_atomstr(str) = "(";
	x_base_write_str(p_base, (x_obj_t *)&wrap);

	for (;;) {
		if (x_obj_isnil(p_base, x_firstobj(p_obj))) {
			x_atomstr(str) = "()";
			x_base_write_str(p_base, (x_obj_t *)&wrap);
		} else {
			x_firstobj((x_obj_t *)write_wrap) = x_firstobj(p_obj);
			x_token_write(p_base, (x_obj_t *)write_wrap);
		}

		p_obj = x_restobj(p_obj);

		if (x_obj_isnil(p_base, p_obj)) {
			x_atomstr(str) = ")";
			x_base_write_str(p_base, (x_obj_t *)&wrap);

			break;
		}

		if ( ! x_obj_type_isspair(p_obj)) {
			x_atomstr(str) = " . ";
			x_base_write_str(p_base, (x_obj_t *)&wrap);

			x_firstobj((x_obj_t *)write_wrap) = p_obj;
			x_token_write(p_base, (x_obj_t *)write_wrap);

			x_atomstr(str) = ")";
			x_base_write_str(p_base, (x_obj_t *)&wrap);

			break;
		}

		x_atomstr(str) = " ";
		x_base_write_str(p_base, (x_obj_t *)&wrap);
	}

	return p_obj;
}

/**
 * Display a stack-allocated pair in human-readable form.
 *
 * Same structure as write but dispatches through @c x_token_display
 * for each element.
 *
 * @param p_base  Execution context.
 * @param p_args  Pair whose first element is the pair to display.
 * @return The tail of the pair after displaying.
 */
x_obj_t *x_sexp_pair_display(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args);
	x_satom_t str = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = NULL }),
		disp_wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL });
	x_spair_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { str }, { NULL });

	x_atomstr(str) = "(";
	x_base_write_str(p_base, (x_obj_t *)&wrap);

	for (;;) {
		if (x_obj_isnil(p_base, x_firstobj(p_obj))) {
			x_atomstr(str) = "()";
			x_base_write_str(p_base, (x_obj_t *)&wrap);
		} else {
			x_firstobj((x_obj_t *)disp_wrap) = x_firstobj(p_obj);
			x_token_display(p_base, (x_obj_t *)disp_wrap);
		}

		p_obj = x_restobj(p_obj);

		if (x_obj_isnil(p_base, p_obj)) {
			x_atomstr(str) = ")";
			x_base_write_str(p_base, (x_obj_t *)&wrap);

			break;
		}

		if ( ! x_obj_type_isspair(p_obj)) {
			x_atomstr(str) = " . ";
			x_base_write_str(p_base, (x_obj_t *)&wrap);

			x_firstobj((x_obj_t *)disp_wrap) = p_obj;
			x_token_display(p_base, (x_obj_t *)disp_wrap);

			x_atomstr(str) = ")";
			x_base_write_str(p_base, (x_obj_t *)&wrap);

			break;
		}

		x_atomstr(str) = " ";
		x_base_write_str(p_base, (x_obj_t *)&wrap);
	}

	return p_obj;
}

x_satom_t x_sexp_pair_write_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_pair_write }),
	x_sexp_pair_display_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_pair_display });
