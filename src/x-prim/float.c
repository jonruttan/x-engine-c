/*
 * # Computational Expressions in C
 *
 * ## x-prim/float.c -- Implementation - Primitives - Float Conversions
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
#include "x-type/buffer.h"
#include "x-type/int.h"
#include "x-type/str.h"

#include <stdlib.h>  /* strtod */
#include <stdio.h>   /* sprintf */
#include <string.h>  /* memcpy */

/* string->float: (string->float str) -> int (IEEE 754 bits) */
static x_obj_t *x_prim_string_to_float(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_str;
	double d;
	x_int_t bits;

	p_str = x_prim_eval_arg(p_base, x_firstobj(p_args));
	d = strtod(x_strval(p_str), NULL);
	memcpy(&bits, &d, sizeof(double));

	return x_mkint(p_base, bits);
}

/* float->string: (float->string bits) -> str */
static x_obj_t *x_prim_float_to_string(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_bits;
	double d;
	x_char_t buf[32];
	x_char_t *s;
	int len;

	p_bits = x_prim_eval_arg(p_base, x_firstobj(p_args));
	memcpy(&d, &x_intval(p_bits), sizeof(double));

	len = sprintf(buf, "%.15g", d);
	s = x_lib_strndup(buf, len);

	return x_mkstrown(p_base, s);
}

/* int->float: (int->float n) -> int (IEEE 754 bits of (double)n) */
static x_obj_t *x_prim_int_to_float(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_n;
	double d;
	x_int_t bits;

	p_n = x_prim_eval_arg(p_base, x_firstobj(p_args));
	d = (double)x_intval(p_n);
	memcpy(&bits, &d, sizeof(double));

	return x_mkint(p_base, bits);
}

/* float->int: (float->int bits) -> int (truncates double to integer) */
static x_obj_t *x_prim_float_to_int(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_bits;
	double d;

	p_bits = x_prim_eval_arg(p_base, x_firstobj(p_args));
	memcpy(&d, &x_intval(p_bits), sizeof(double));

	return x_mkint(p_base, (x_int_t)d);
}

/* float-read: (float-read buffer) -> int (IEEE 754 bits from buffer) */
static x_obj_t *x_prim_float_read(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer;
	double d;
	x_int_t bits;

	p_buffer = x_prim_eval_arg(p_base, x_firstobj(p_args));
	d = strtod(x_bufferval(p_buffer), NULL);
	memcpy(&bits, &d, sizeof(double));

	return x_mkint(p_base, bits);
}

x_obj_t *x_prim_float_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_prim_bind(p_base, "string->float", x_prim_string_to_float);
	x_prim_bind(p_base, "float->string", x_prim_float_to_string);
	x_prim_bind(p_base, "int->float", x_prim_int_to_float);
	x_prim_bind(p_base, "float->int", x_prim_float_to_int);
	x_prim_bind(p_base, "float-read", x_prim_float_read);

	return p_base;
}
