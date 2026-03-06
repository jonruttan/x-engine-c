/*
 * # Computational Expressions in C
 *
 * ## x-type.c -- Implementation - Type - String
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
#include "x-base.h"
#include "x-type/str.h"
#include "x-sexp/str.h"
#include "x-type/char.h"
#include "x-type/int.h"
#include "x-sexp/str.h"

x_satom_t x_type_str_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_STR_NAME }),
	x_type_str_length_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_str_length }),
	x_type_str_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_str_make }),
	x_type_str_call_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_str_call }),
	x_type_str_struct_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_str_struct });

x_obj_t *x_make_str(x_obj_t *p_base, x_obj_flag_t flags, x_char_t *s)
{
	x_satom_t o_str = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .s = s }),
		o_flags = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = flags });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_str }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_flags }, { NULL })
	};

	return x_type_str_make(p_base, (x_obj_t *)args);
}

x_obj_t *x_type_str_struct(x_obj_t *p_base, x_obj_t *p_obj)
{
	struct x_type_t type = {
		.p_name = x_type_str_name,
		.p_length = x_type_str_length_prim,
		.p_make = x_type_str_make_prim,
		.p_call = x_type_str_call_prim,
		.p_analyse = x_sexp_str_analyse1_prim,
		.p_write = x_sexp_str_write_prim
	};

	return x_type_struct_make(p_base, type);
}

x_obj_t *x_type_str_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_str_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_str_struct_prim }, { NULL })
	};

	return x_type_struct_get(p_base, (x_obj_t *)args);
}

x_obj_t *x_type_str_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = x_type_str_register(p_base, p_base),
		*p_str = x_0(p_args);
	x_obj_flag_t flags = x_obj_isnil(p_base, x_1(p_args))
		? 0 : x_firstint(x_01(p_args));

	return x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_ATOM, x_strval(p_str));
}

x_obj_t *x_type_str_length(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_mksatom(p_base, x_strlen(x_firstobj(p_args)));
}

/*x_obj_t *x_type_str_call(x_obj_t *p_base, x_obj_t *proc, x_obj_t *vals)*/
x_obj_t *x_type_str_call(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *proc = x_firstobj(p_args), *vals = x_restobj(p_args);
	/*
	 * TODO: Test whether index exists
	 * TODO: Slices
	 * TODO: Lookup slice function pointer from table using type
	 */
	x_int_t n = x_obj_length(p_base, vals);

	if (n > 1) {
		/* TODO: Convert to Str type when type is available. */
		/*return x_mkstrown(p_base, x_strval(proc) + x_intval(x_car(vals)), x_intval(x_cadr(vals)));*/
		return x_mksatom(p_base, x_mksatomown(p_base, x_lib_strndup(x_strval(proc) + x_atomint(x_car(vals)), x_atomint(x_cadr(vals)))));
	}

	if (n == 1) {
		n = x_intval(x_car(vals));
	}

	if (n < 0) {
		n += x_lib_strlen(x_strval(proc));
	}

	return x_mkchar(p_base, x_strval(proc)[n]);
}
