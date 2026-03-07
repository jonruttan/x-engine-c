/*
 * # Computational Expressions in C
 *
 * ## x-token.c -- Implementation - Symbol List
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
#include "x-symlist.h"
#include "x-type/symbol.h"


/* #define X_FINDSYM_REORDER */
#if 0
x_obj_t *x_findsym1(x_obj_t *p_base, x_obj_t **pp_top, x_char_t *name)
{
	x_obj_t *p_symlist, *p_top = *pp_top;
#ifdef X_FINDSYM_REORDER
	x_obj_t **prev = pp_top;
#endif /* X_FINDSYM_REORDER */

	for (p_symlist = p_top; p_symlist != p_base; p_symlist = x_restobj(p_symlist)) {
		if ( ! x_lib_strcmp(name, x_symname(x_firstobj(p_symlist)))) {
#ifdef X_FINDSYM_REORDER
			*prev = x_restobj(p_symlist);
			x_restobj(p_symlist) = p_top;
			p_top = symlist;
#endif /* X_FINDSYM_REORDER */
			return p_symlist;
		}
#ifdef FINDSYM_REORDER
		*prev = p_symlist;
#endif /* X_FINDSYM_REORDER */
	}

	return p_base;
}

x_obj_t *x_findsym(x_obj_t *p_base, x_char_t *name)
{
	return x_findsym1(&x_vectorval(p_base, X_I_SYMBOLS), name);
}

x_obj_t *x_intern(x_obj_t *p_base, x_obj_t *p_top, x_char_t *name)
{
	x_obj_t *op = x_findsym(p_top, name);

	if (op != x_nil) {
		return x_firstobj(op);
	}

	op = x_mkownsym(p_top, name);
	x_vectorval(p_top, X_I_SYMBOLS) = x_mkspair(p_top, op, x_vectorval(p_top, X_I_SYMBOLS));

	return op;
}



x_obj_t *x_symlist_read(x_obj_t *p_base, x_obj_t *p_obj)
{
	p_obj = x_symlist_read(p_base, p_obj);

	return /*x_int(x_obj_type(p_obj)) == X_OBJ_TYPE_ATOM
		? x_intern(p_obj, x_atomstr(p_obj))
		:*/ p_obj;
}
#endif
