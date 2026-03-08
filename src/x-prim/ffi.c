/*
 * # Computational Expressions in C
 *
 * ## x-prim/ffi.c -- Implementation - Primitives - FFI
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
#include "x-type/ptr.h"
#include "x-type/str.h"
#include "x-type/symbol.h"

#include <stdlib.h>  /* strtod */
#include <stdio.h>   /* sprintf */
#include <string.h>  /* memcpy */
#include <dlfcn.h>   /* dlopen, dlsym */

/* dlopen: (dlopen path flags) -> ptr
 * Pass () for path to get handle to current process (dlopen(NULL, ...)) */
static x_obj_t *x_prim_dlopen(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_path, *p_flags;
	void *h;

	p_path = x_prim_eval_arg(p_base, x_firstobj(p_args));
	p_flags = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	h = dlopen(x_obj_isnil(p_base, p_path) ? NULL : x_strval(p_path),
		(int)x_intval(p_flags));

	if (!h)
		return p_base;

	return x_mkptr(p_base, h);
}

/* dlsym: (dlsym handle name) -> ptr */
static x_obj_t *x_prim_dlsym(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_handle, *p_name;
	void *sym;

	p_handle = x_prim_eval_arg(p_base, x_firstobj(p_args));
	p_name = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	sym = dlsym(x_ptrval(p_handle), x_strval(p_name));

	if (!sym)
		return p_base;

	return x_mkptr(p_base, sym);
}

/* ffi-call: (ffi-call convention fptr args...) -> result
 * Conventions:
 *   "d->d"  : double(*)(double)
 *   "dd->d" : double(*)(double,double)
 *   "d+d"   : double + double (arithmetic)
 *   "d-d", "d*d", "d/d" : arithmetic
 *   "d<d", "d>d", "d=d", "d<=d", "d>=d" : comparison
 */
static x_obj_t *x_prim_ffi_call(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_conv, *p_fptr, *p_rest, *p_a, *p_b;
	x_char_t *conv;
	void *fptr;
	double a, b, r;
	x_int_t bits;

	p_conv = x_prim_eval_arg(p_base, x_firstobj(p_args));
	p_fptr = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));
	p_rest = x_restobj(x_restobj(p_args));

	conv = x_strval(p_conv);

	/* Function call conventions */
	if (x_lib_strcmp(conv, "d->d") == 0) {
		fptr = x_ptrval(p_fptr);
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		memcpy(&a, &x_intval(p_a), sizeof(double));
		r = ((double (*)(double))fptr)(a);
		memcpy(&bits, &r, sizeof(double));
		return x_mkint(p_base, bits);
	}

	if (x_lib_strcmp(conv, "dd->d") == 0) {
		fptr = x_ptrval(p_fptr);
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(p_rest)));
		memcpy(&a, &x_intval(p_a), sizeof(double));
		memcpy(&b, &x_intval(p_b), sizeof(double));
		r = ((double (*)(double, double))fptr)(a, b);
		memcpy(&bits, &r, sizeof(double));
		return x_mkint(p_base, bits);
	}

	/* Arithmetic conventions (no function pointer needed) */
	if (x_lib_strcmp(conv, "d+d") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(p_rest)));
		memcpy(&a, &x_intval(p_a), sizeof(double));
		memcpy(&b, &x_intval(p_b), sizeof(double));
		r = a + b;
		memcpy(&bits, &r, sizeof(double));
		return x_mkint(p_base, bits);
	}

	if (x_lib_strcmp(conv, "d-d") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(p_rest)));
		memcpy(&a, &x_intval(p_a), sizeof(double));
		memcpy(&b, &x_intval(p_b), sizeof(double));
		r = a - b;
		memcpy(&bits, &r, sizeof(double));
		return x_mkint(p_base, bits);
	}

	if (x_lib_strcmp(conv, "d*d") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(p_rest)));
		memcpy(&a, &x_intval(p_a), sizeof(double));
		memcpy(&b, &x_intval(p_b), sizeof(double));
		r = a * b;
		memcpy(&bits, &r, sizeof(double));
		return x_mkint(p_base, bits);
	}

	if (x_lib_strcmp(conv, "d/d") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(p_rest)));
		memcpy(&a, &x_intval(p_a), sizeof(double));
		memcpy(&b, &x_intval(p_b), sizeof(double));
		r = a / b;
		memcpy(&bits, &r, sizeof(double));
		return x_mkint(p_base, bits);
	}

	/* Comparison conventions */
	if (x_lib_strcmp(conv, "d<d") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(p_rest)));
		memcpy(&a, &x_intval(p_a), sizeof(double));
		memcpy(&b, &x_intval(p_b), sizeof(double));
		return a < b
			? x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE)
			: p_base;
	}

	if (x_lib_strcmp(conv, "d>d") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(p_rest)));
		memcpy(&a, &x_intval(p_a), sizeof(double));
		memcpy(&b, &x_intval(p_b), sizeof(double));
		return a > b
			? x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE)
			: p_base;
	}

	if (x_lib_strcmp(conv, "d=d") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(p_rest)));
		memcpy(&a, &x_intval(p_a), sizeof(double));
		memcpy(&b, &x_intval(p_b), sizeof(double));
		return a == b
			? x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE)
			: p_base;
	}

	if (x_lib_strcmp(conv, "d<=d") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(p_rest)));
		memcpy(&a, &x_intval(p_a), sizeof(double));
		memcpy(&b, &x_intval(p_b), sizeof(double));
		return a <= b
			? x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE)
			: p_base;
	}

	if (x_lib_strcmp(conv, "d>=d") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(p_rest)));
		memcpy(&a, &x_intval(p_a), sizeof(double));
		memcpy(&b, &x_intval(p_b), sizeof(double));
		return a >= b
			? x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE)
			: p_base;
	}

	return p_base;
}

/* ptr-call: (ptr-call fptr args...) -> int
 * Call a function pointer as long(*)(long,...), return result as int.
 * Args can be int, string, or ptr. */
static x_obj_t *x_prim_ptr_call(x_obj_t *p_base, x_obj_t *p_args)
{
	long i = 0, p[7];
	x_obj_t *arg, *p_fptr;
	long (*fn)(long, long, long, long, long, long, long);

	p_fptr = x_prim_eval_arg(p_base, x_firstobj(p_args));
	p_args = x_restobj(p_args);

	p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = p[6] = 0;

	while (!x_obj_isnil(p_base, p_args) && i < 7) {
		arg = x_prim_eval_arg(p_base, x_firstobj(p_args));
		if (x_obj_type_isint(p_base, arg))
			p[i++] = (long)x_intval(arg);
		else if (x_obj_type_isstr(p_base, arg))
			p[i++] = (long)x_strval(arg);
		else if (x_obj_type_isptr(p_base, arg))
			p[i++] = (long)x_ptrval(arg);
		p_args = x_restobj(p_args);
	}

	fn = (long (*)(long, long, long, long, long, long, long))
		x_ptrval(p_fptr);

	return x_mkint(p_base, (x_int_t)fn(
		p[0], p[1], p[2], p[3], p[4], p[5], p[6]));
}

/* int->ptr: (int->ptr n) -> ptr */
static x_obj_t *x_prim_int_to_ptr(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_n = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_mkptr(p_base, (void *)x_intval(p_n));
}

/* ptr->int: (ptr->int p) -> int */
static x_obj_t *x_prim_ptr_to_int(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_p = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_mkint(p_base, (x_int_t)x_ptrval(p_p));
}

/* ptr-set!: (ptr-set! ptr offset byte) -> writes byte at ptr+offset */
static x_obj_t *x_prim_ptr_set(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_ptr, *p_offset, *p_byte;
	unsigned char *mem;

	p_ptr = x_prim_eval_arg(p_base, x_firstobj(p_args));
	p_offset = x_prim_eval_arg(p_base,
		x_firstobj(x_restobj(p_args)));
	p_byte = x_prim_eval_arg(p_base,
		x_firstobj(x_restobj(x_restobj(p_args))));

	mem = (unsigned char *)x_ptrval(p_ptr);
	mem[x_intval(p_offset)] = (unsigned char)x_intval(p_byte);

	return p_ptr;
}

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

x_obj_t *x_prim_ffi_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_prim_bind(p_base, "dlopen", x_prim_dlopen);
	x_prim_bind(p_base, "dlsym", x_prim_dlsym);
	x_prim_bind(p_base, "ffi-call", x_prim_ffi_call);
	x_prim_bind(p_base, "ptr-call", x_prim_ptr_call);
	x_prim_bind(p_base, "int->ptr", x_prim_int_to_ptr);
	x_prim_bind(p_base, "ptr->int", x_prim_ptr_to_int);
	x_prim_bind(p_base, "ptr-set!", x_prim_ptr_set);
	x_prim_bind(p_base, "string->float", x_prim_string_to_float);
	x_prim_bind(p_base, "float->string", x_prim_float_to_string);
	x_prim_bind(p_base, "int->float", x_prim_int_to_float);
	x_prim_bind(p_base, "float->int", x_prim_float_to_int);
	x_prim_bind(p_base, "float-read", x_prim_float_read);

	return p_base;
}
