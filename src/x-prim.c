/*
 * # Computational Expressions in C
 *
 * ## x-prim.c -- Implementation - Primitive Registration
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2024 Jon Ruttan
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
#include "x-prim.h"
#include "x-alist.h"
#include "x-base.h"
#include "x-type/list.h"
#include "x-type/prim.h"
#include "x-type/symbol.h"

/*
 * # Registration
 */
void x_prim_bind(x_obj_t *p_base, x_char_t *name, x_prim_fn fn)
{
	x_obj_t *p_sym = x_make_symbol(p_base, X_OBJ_FLAG_NONE, name),
		*p_prim = x_mkprim(p_base, fn),
		*p_pair = x_mkspair(p_base, p_sym, p_prim);

	x_base_env_alist_extend(p_base, p_pair);

	/* Insert into global BST and update boundary */
	x_base_field_env_global_tree(p_base) = x_alist_bst_insert(
		p_base, x_base_field_env_global_tree(p_base), p_pair);
	x_base_field_env_local_boundary(p_base)
		= x_base_field_env_alist(p_base);
}

void x_prim_bind_table(x_obj_t *p_base, const x_prim_entry_t *table, int count)
{
	int i;

	for (i = 0; i < count; i++) {
		x_prim_bind(p_base, table[i].name, table[i].fn);
	}
}

x_obj_t *x_prim_register(x_obj_t *p_base, x_obj_t *p_args)
{
	/* Bind #t and #f as boolean singletons, cache in base. */
	{
		x_obj_t *p_t = (x_obj_t *)&x_true_obj,
			*p_f = (x_obj_t *)&x_false_obj,
			*p_pair;

		p_pair = x_mkspair(p_base,
			x_mksymbol(p_base, x_atomstr(x_true_obj)), p_t);
		x_base_env_alist_extend(p_base, p_pair);
		x_base_field_env_global_tree(p_base) = x_alist_bst_insert(
			p_base, x_base_field_env_global_tree(p_base), p_pair);
		x_base_field_env_local_boundary(p_base)
			= x_base_field_env_alist(p_base);
		x_base_field_true(p_base) = p_t;

		p_pair = x_mkspair(p_base,
			x_mksymbol(p_base, x_atomstr(x_false_obj)), p_f);
		x_base_env_alist_extend(p_base, p_pair);
		x_base_field_env_global_tree(p_base) = x_alist_bst_insert(
			p_base, x_base_field_env_global_tree(p_base), p_pair);
		x_base_field_env_local_boundary(p_base)
			= x_base_field_env_alist(p_base);
		x_base_field_false(p_base) = p_f;
	}

	x_prim_core_register(p_base, p_args);
	x_prim_arith_register(p_base, p_args);
	x_prim_pred_register(p_base, p_args);
	x_prim_string_register(p_base, p_args);
	x_prim_io_register(p_base, p_args);
	x_prim_gc_register(p_base, p_args);
	x_prim_type_register(p_base, p_args);
	x_prim_ffi_register(p_base, p_args);
	x_prim_callcc_register(p_base, p_args);

	return p_base;
}
