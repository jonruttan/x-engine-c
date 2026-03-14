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
#include "x-type/int.h"
#include "x-type/ptr.h"
#include "x-type/str.h"
#include "x-type/symbol.h"

#include <string.h>  /* memcpy */
#include <stdio.h>   /* sprintf */
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
		return NULL;

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
		return NULL;

	return x_mkptr(p_base, sym);
}

/* ffi-call: (ffi-call convention fptr args...) -> result
 * Conventions:
 *   "d->d"  : double(*)(double)
 *   "dd->d" : double(*)(double,double)
 *   "d+d"   : double + double (arithmetic)
 *   "d-d", "d*d", "d/d" : arithmetic
 *   "d<d", "d>d", "d=d", "d<=d", "d>=d" : comparison
 *   "i->d"  : cast int to double, return IEEE 754 bits
 *   "d->i"  : interpret IEEE 754 bits as double, truncate to int
 *   "s0->d" : double(*)(const char*,void*) with NULL second arg
 *   "d->s"  : format double bits as string ("%.15g")
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
			? x_base_field_true(p_base)
			: NULL;
	}

	if (x_lib_strcmp(conv, "d>d") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(p_rest)));
		memcpy(&a, &x_intval(p_a), sizeof(double));
		memcpy(&b, &x_intval(p_b), sizeof(double));
		return a > b
			? x_base_field_true(p_base)
			: NULL;
	}

	if (x_lib_strcmp(conv, "d=d") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(p_rest)));
		memcpy(&a, &x_intval(p_a), sizeof(double));
		memcpy(&b, &x_intval(p_b), sizeof(double));
		return a == b
			? x_base_field_true(p_base)
			: NULL;
	}

	if (x_lib_strcmp(conv, "d<=d") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(p_rest)));
		memcpy(&a, &x_intval(p_a), sizeof(double));
		memcpy(&b, &x_intval(p_b), sizeof(double));
		return a <= b
			? x_base_field_true(p_base)
			: NULL;
	}

	if (x_lib_strcmp(conv, "d>=d") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(p_rest)));
		memcpy(&a, &x_intval(p_a), sizeof(double));
		memcpy(&b, &x_intval(p_b), sizeof(double));
		return a >= b
			? x_base_field_true(p_base)
			: NULL;
	}

	/* Cast conventions (no function pointer needed) */
	if (x_lib_strcmp(conv, "i->d") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		a = (double)x_intval(p_a);
		memcpy(&bits, &a, sizeof(double));
		return x_mkint(p_base, bits);
	}

	if (x_lib_strcmp(conv, "d->i") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		memcpy(&a, &x_intval(p_a), sizeof(double));
		return x_mkint(p_base, (x_int_t)a);
	}

	/* String/double conversions */
	if (x_lib_strcmp(conv, "s0->d") == 0) {
		fptr = x_ptrval(p_fptr);
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		r = ((double (*)(const char *, void *))fptr)(
			x_firststr(p_a), NULL);
		memcpy(&bits, &r, sizeof(double));
		return x_mkint(p_base, bits);
	}

	if (x_lib_strcmp(conv, "d->s") == 0) {
		x_char_t buf[32];
		int len;
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		memcpy(&a, &x_intval(p_a), sizeof(double));
		len = sprintf((char *)buf, "%.15g", a);
		return x_mkstrown(p_base, x_lib_strndup(buf, len));
	}

	return NULL;
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

/* ptr-ref: (ptr-ref ptr offset) -> int
 * Read sizeof(int) bytes from ptr+offset, return as int. */
static x_obj_t *x_prim_ptr_ref(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_ptr, *p_offset;
	unsigned char *mem;
	int val;

	p_ptr = x_prim_eval_arg(p_base, x_firstobj(p_args));
	p_offset = x_prim_eval_arg(p_base,
		x_firstobj(x_restobj(p_args)));

	mem = (unsigned char *)x_ptrval(p_ptr);
	memcpy(&val, mem + x_intval(p_offset), sizeof(int));

	return x_mkint(p_base, (x_int_t)val);
}

/* ptr-set-word!: (ptr-set-word! ptr offset value) -> ptr
 * Write sizeof(long) bytes at ptr+offset. */
static x_obj_t *x_prim_ptr_set_word(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_ptr, *p_offset, *p_val;
	unsigned char *mem;
	long val;

	p_ptr = x_prim_eval_arg(p_base, x_firstobj(p_args));
	p_offset = x_prim_eval_arg(p_base,
		x_firstobj(x_restobj(p_args)));
	p_val = x_prim_eval_arg(p_base,
		x_firstobj(x_restobj(x_restobj(p_args))));

	mem = (unsigned char *)x_ptrval(p_ptr);
	val = (long)x_intval(p_val);
	memcpy(mem + x_intval(p_offset), &val, sizeof(long));

	return p_ptr;
}

/* string->ptr: (string->ptr str) -> ptr
 * Get raw char* of string as ptr. */
static x_obj_t *x_prim_string_to_ptr(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_str = x_prim_eval_arg(p_base, x_firstobj(p_args));

	return x_mkptr(p_base, (void *)x_strval(p_str));
}

/* ptr->string: (ptr->string ptr) -> string
 * Create string from null-terminated C char* at ptr. */
static x_obj_t *x_prim_ptr_to_string(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_ptr = x_prim_eval_arg(p_base, x_firstobj(p_args));
	x_char_t *s = (x_char_t *)x_ptrval(p_ptr);

	return x_mkstrown(p_base, x_lib_strndup(s, x_lib_strlen(s)));
}

x_obj_t *x_prim_ffi_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_prim_entry_t entries[] = {
		{ "dlopen", x_prim_dlopen },
		{ "dlsym", x_prim_dlsym },
		{ "ffi-call", x_prim_ffi_call },
		{ "ptr-call", x_prim_ptr_call },
		{ "int->ptr", x_prim_int_to_ptr },
		{ "ptr->int", x_prim_ptr_to_int },
		{ "ptr-set!", x_prim_ptr_set },
		{ "ptr-ref", x_prim_ptr_ref },
		{ "ptr-set-word!", x_prim_ptr_set_word },
		{ "string->ptr", x_prim_string_to_ptr },
		{ "ptr->string", x_prim_ptr_to_string }
	};

	x_prim_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
