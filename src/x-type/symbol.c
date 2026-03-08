/*
 * # Computational Expressions in C
 *
 * ## x-type.c -- Implementation - Type - Symbol
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
#include "x-type/symbol.h"
#include "x-type/str.h"
#include "x-alist.h"
#include "x-eval.h"
#include "x-token/sexp/symbol.h"

x_satom_t x_type_symbol_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_SYMBOL_NAME }),
	x_type_symbol_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_symbol_make }),
	x_type_symbol_eval_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_symbol_eval }),
	x_type_symbol_struct_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_symbol_struct });

x_obj_t *x_make_symbol(x_obj_t *p_base, x_obj_flag_t flags, x_char_t *s)
{
	x_satom_t o_symbol = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .s = s }),
		o_flags = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = flags });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_symbol }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_flags }, { NULL })
	};

	return x_type_symbol_make(p_base, (x_obj_t *)args);
}

x_obj_t *x_type_symbol_struct(x_obj_t *p_base, x_obj_t *p_obj)
{
	struct x_type_t type = {
		.p_name = x_type_symbol_name,
		.p_make = x_type_symbol_make_prim,
		.p_eval = x_type_symbol_eval_prim,
		.p_analyse = x_sexp_symbol_analyse_prim,
		.p_write = x_sexp_symbol_write_prim
	};

	return x_type_struct_make(p_base, type);
}

x_obj_t *x_type_symbol_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_symbol_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_symbol_struct_prim }, { NULL })
	};
	x_obj_t *p_type = x_type_struct_get(p_base, (x_obj_t *)args);

	if (x_obj_isnil(p_base, x_symbol_data(p_type))) {
		x_symbol_data(p_type) = x_mkspair(p_base, p_base, p_base);
	}

	return p_type;
}

/* TODO: Alter so symbol list can be optionally supplied by argument. */
x_obj_t *x_type_symbol_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = x_type_symbol_register(p_base, p_base),
		*p_symbol = x_0(p_args), *p_obj = x_type_symbol_find(p_base, p_args);
	x_obj_flag_t flags = x_obj_isnil(p_base, x_1(p_args))
		? 0 : x_firstint(x_01(p_args));

	if ( ! x_obj_isnil(p_base, p_obj)) {
		return x_firstobj(p_obj);
	}

	p_obj = x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_ATOM, x_symbolval(p_symbol));
	x_symbol_data_list(p_type) = x_mkspair(p_base, p_obj, x_symbol_data_list(p_type));

	return p_obj;
}

x_obj_t *x_type_symbol_find(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = x_type_symbol_register(p_base, p_base),
		*p_list = x_symbol_data_list(p_type);
	x_char_t *name = x_firststr(x_firstobj(p_args));
#ifdef SYMBOL_FIND_REORDER
	x_obj_t *p_prev = p_base;
#endif

	while ( ! x_obj_isnil(p_base, p_list)) {
		if ( 0 == x_lib_strcmp(name, x_symbolval(x_firstobj(p_list)))) {
#ifdef SYMBOL_FIND_REORDER
			/* Move to front: splice out and prepend. */
			if ( ! x_obj_isnil(p_base, p_prev)) {
				x_restobj(p_prev) = x_restobj(p_list);
				x_restobj(p_list) = x_symbol_data_list(p_type);
				x_symbol_data_list(p_type) = p_list;
			}
#endif
			return p_list;
		}
#ifdef SYMBOL_FIND_REORDER
		p_prev = p_list;
#endif
		p_list = x_restobj(p_list);
	}

	return p_base;
}

x_obj_t *x_type_symbol_eval(x_obj_t *p_base, x_obj_t *p_args)
/*x_obj_t *x_type_symbol_eval(x_obj_t *p_base, x_obj_t *p_exp, x_obj_t *p_env)*/
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_firstobj(x_eval_arg_exp(p_args)) }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_base_field_env_alist(p_base) }, { NULL })
	};
	x_obj_t *p_sym = x_alist_assoc(p_base, (x_obj_t *)args);

	if (x_obj_isnil(p_base, p_sym)) {
		/* TODO: Implement type name. */
		x_obj_error(p_base, "Unbound "X_TYPE_SYMBOL_NAME, x_symbolval(x_firstobj(x_eval_arg_exp(p_args))));

		return p_base;
	}

	return x_restobj(p_sym);
}
