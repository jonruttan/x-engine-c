/*
 * # Computational Expressions in C
 *
 * ## x-prim.c -- Implementation - Primatives
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
#include "x-prim.h"

x_obj_t *x_obj_primop_proc(x_obj_t *p_base, x_obj_t *proc, x_obj_t *vals)
{
	return (*x_primopval(proc))(p_base, vals);
}

x_obj_t *x_prim_read_char(x_obj_t *p_base, x_obj_t *args)
{
	return x_mkchar(p_base, x_read_char(p_base));
}

x_obj_t *x_prim_length(x_obj_t *p_base, x_obj_t *args)
{
	return x_mkint(p_base, x_obj_prim_length(x_car(args)));
}

x_obj_t *x_prim_not(x_obj_t *p_base, x_obj_t *args)
{
	return x_car(args) == x_nil ? x_car(x_findsym(p_base, "#t")) : x_nil;
}

x_obj_t *x_prim_eq(x_obj_t *p_base, x_obj_t *args)
{
	return x_car(args) == x_cadr(args) ? x_car(x_findsym(p_base, "#t")) : x_nil;
}

x_obj_t *x_prim_cons(x_obj_t *p_base, x_obj_t *args)
{
	return x_cons(p_base, x_car(args), x_cadr(args));
}

x_obj_t *x_prim_car(x_obj_t *p_base, x_obj_t *args)
{
	return x_caar(args);
}

x_obj_t *x_prim_cdr(x_obj_t *p_base, x_obj_t *args)
{
	return x_cdar(args);
}

x_obj_t *x_prim_num_not(x_obj_t *p_base, x_obj_t *args)
{
	return x_mkint(p_base, ~x_intval(x_car(args)));
}

#define define_num_op(name, op)\
x_obj_t *x_prim_num_##name(x_obj_t *p_base, x_obj_t *args)\
{\
	x_int_t result = 0;\
\
	if (args != x_nil)\
		for (result = x_intval(x_car(args)); (args = x_cdr(args)) != x_nil; result op x_intval(x_car(args)));\
\
	return x_mkint(p_base, result);\
}

define_num_op(sum, +=)
define_num_op(sub, -=)
define_num_op(prod, *=)
define_num_op(div, /=)
define_num_op(mod, %=)
define_num_op(and, &=)
define_num_op(or, |=)
define_num_op(xor, ^=)
define_num_op(shl, <<=)
define_num_op(shr, >>=)

#define define_num_cmp_op(NAME, OP) \
x_obj_t *x_prim_num_##NAME(x_obj_t *p_base, x_obj_t *args)\
{\
	return x_intval(x_car(args)) OP x_intval(x_cadr(args)) ? x_car(x_findsym(p_base, "#t")) : x_nil;\
}

define_num_cmp_op(eq, ==)
define_num_cmp_op(gt, >)

void x_prim_tmpstr(x_obj_t *p_base, x_obj_t *args)
{
	x_pbuf = x_buf;
	for (; args != x_nil; args = x_cdr(args))
		switch(x_car(args)->type) {
		case X_INTEGER:
			x_pbuf += x_lib_strlen(x_lib_strtoint(x_intval(x_car(args)), x_pbuf, 10));
			break;

		case X_CHAR:
			*(x_pbuf++) = x_charval(x_car(args));
			break;

		case X_STRING:
			x_pbuf += x_lib_strlen(x_lib_strncpy(pbuf, x_strval(x_car(args)), X_TOKEN_SIZE_MAX - (x_pbuf - x_buf)));
			break;

		default:
			break;
		}
	*x_pbuf = 0;
}

x_obj_t *x_prim_mkstr(x_obj_t *p_base, x_obj_t *args)
{
	return x_ownstr(p_base, NULL, x_intval(x_car(args)));
}

x_obj_t *x_prim_str(x_obj_t *p_base, x_obj_t *args)
{
	x_prim_tmpstr(p_base, args);
	return x_ownstr(p_base, buf, pbuf-buf);
}

x_obj_t *x_prim_strcmp(x_obj_t *p_base, x_obj_t *args)
{
	return x_mkint(p_base, x_lib_strcmp(x_strval(x_car(args)), x_strval(x_cadr(args))));
}

x_obj_t *x_prim_vector(x_obj_t *p_base, x_obj_t *args)
{
	x_obj_t *tmp = x_mkvector(p_base, x_obj_prim_length(args)), **p = tmp->data.p;

	for (;args != x_nil; args = x_cdr(args))
		*++p = x_car(args);

	return tmp;
}

#include "x-gc.h"

x_obj_t *x_prim_gc(x_obj_t *p_base, x_obj_t *args)
{
	x_gc_mark(p_base, X_OBJ_FLAG_GC);
	x_gc_sweep(p_base, X_OBJ_FLAG_GC);

	return x_nil;
}

x_obj_t *x_prim_write(x_obj_t *p_base, x_obj_t *args)
{
	x_obj_t *p_obj = x_car(args);

	return obj_types[p_obj->type].write(p_base, p_obj);
}

x_obj_t *x_prim_assoc(x_obj_t *p_base, x_obj_t *args)
{
	return x_assoc(p_base, x_car(args), x_cadr(args));
}

x_obj_t *x_prim_append(x_obj_t *p_base, x_obj_t *args)
{
	return x_append(p_base, args);
}

x_obj_t *x_prim_findsym(x_obj_t *p_base, x_obj_t *args) {
	return x_findsym(p_base, x_symname(x_car(args)));
}

x_obj_t *x_prim_top(x_obj_t *p_base, x_obj_t *args)
{
	return p_base;
}

x_obj_t *x_prim_ptr(x_obj_t *p_base, x_obj_t *args)
{
	pbuf = buf;
	for(; args != x_nil; args = x_cdr(args))
		switch(x_car(args)->type) {
		case X_INTEGER:
			x_lib_memcpy(pbuf, &x_intval(car(args)), sizeof(x_int_t **));
			pbuf += sizeof(x_int_t *);
			break;

		case X_STRING:
			x_lib_memcpy(pbuf, &x_resval(caar(args)), sizeof(x_char_t **));
			pbuf += sizeof(x_char_t *);
			break;

		default:
			break;
		}

	return x_ownstr(p_base, buf, pbuf-buf);
}

x_obj_t *x_prim_type(x_obj_t *p_base, x_obj_t *args)
{
	return x_mkint(p_base, x_car(args)->type);
}

x_obj_t *x_prim_totype(x_obj_t *p_base, x_obj_t *args)
{
	return x_gc_obj_make(p_base, (enum x_obj_t_enum)x_intval(x_cadr(args)), 0, 1, x_caar(args));
}
