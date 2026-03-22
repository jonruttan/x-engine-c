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
#include "x-base.h"
#include "x-type/int.h"
#include "x-type/ptr.h"
#include "x-type/str.h"
#include "x-type/symbol.h"

#include <string.h>  /* memcpy */
#include <stdio.h>   /* sprintf */
#include <dlfcn.h>   /* dlopen, dlsym */
#include <fcntl.h>   /* O_RDONLY, O_WRONLY, O_CREAT, O_TRUNC, O_APPEND */

/*
 * # Double Bit-Pattern Helpers
 */
#if defined(__LP64__) || defined(_LP64) || defined(_WIN64)

static void x_ffi_to_double(x_obj_t *p_base, x_obj_t *p_bits, double *out)
{
	(void)p_base;
	memcpy(out, &x_intval(p_bits), sizeof(double));
}

static x_obj_t *x_ffi_from_double(x_obj_t *p_base, double *in)
{
	x_int_t bits;
	memcpy(&bits, in, sizeof(double));
	return x_mkint(p_base, bits);
}

#else /* 32-bit */

static void x_ffi_to_double(x_obj_t *p_base, x_obj_t *p_bits, double *out)
{
	x_int_t parts[2];
	(void)p_base;
	parts[0] = x_intval(x_firstobj(p_bits));
	parts[1] = x_intval(x_restobj(p_bits));
	memcpy(out, parts, sizeof(double));
}

static x_obj_t *x_ffi_from_double(x_obj_t *p_base, double *in)
{
	x_int_t parts[2];
	memcpy(parts, in, sizeof(double));
	return x_mkspair(p_base,
		x_mkint(p_base, parts[0]),
		x_mkint(p_base, parts[1]));
}

#endif /* 64-bit vs 32-bit */

/* dlopen: (dlopen path flags) -> ptr */
static x_obj_t *x_prim_dlopen(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_path = x_prim_eval_arg(p_base, x_01(p_args)),
		*p_flags = x_prim_eval_arg(p_base, x_011(p_args));
	void *h = dlopen(x_obj_isnil(p_base, p_path) ? NULL : x_strval(p_path),
		(int)x_intval(p_flags));

	return h ? x_mkptr(p_base, h) : NULL;
}

/* dlsym: (dlsym handle name) -> ptr */
static x_obj_t *x_prim_dlsym(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_handle = x_prim_eval_arg(p_base, x_01(p_args)),
		*p_name = x_prim_eval_arg(p_base, x_011(p_args));
	void *sym = dlsym(x_ptrval(p_handle), x_strval(p_name));

	return sym ? x_mkptr(p_base, sym) : NULL;
}

/* ffi-call: (ffi-call convention fptr args...) -> result */
static x_obj_t *x_prim_ffi_call(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_conv = x_prim_eval_arg(p_base, x_01(p_args)),
		*p_fptr = x_prim_eval_arg(p_base, x_011(p_args)),
		*p_rest = x_111(p_args), *p_a, *p_b;
	x_char_t *conv = x_strval(p_conv);
	void *fptr;
	double a, b, r;

	/* Function call conventions */
	if (x_lib_strcmp(conv, "d->d") == 0) {
		fptr = x_ptrval(p_fptr);
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		x_ffi_to_double(p_base, p_a, &a);
		r = ((double (*)(double))fptr)(a);
		return x_ffi_from_double(p_base, &r);
	}

	if (x_lib_strcmp(conv, "dd->d") == 0) {
		fptr = x_ptrval(p_fptr);
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(p_rest)));
		x_ffi_to_double(p_base, p_a, &a);
		x_ffi_to_double(p_base, p_b, &b);
		r = ((double (*)(double, double))fptr)(a, b);
		return x_ffi_from_double(p_base, &r);
	}

	/* Arithmetic conventions */
	if (x_lib_strcmp(conv, "d+d") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_rest)));
		x_ffi_to_double(p_base, p_a, &a);
		x_ffi_to_double(p_base, p_b, &b);
		r = a + b;
		return x_ffi_from_double(p_base, &r);
	}

	if (x_lib_strcmp(conv, "d-d") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_rest)));
		x_ffi_to_double(p_base, p_a, &a);
		x_ffi_to_double(p_base, p_b, &b);
		r = a - b;
		return x_ffi_from_double(p_base, &r);
	}

	if (x_lib_strcmp(conv, "d*d") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_rest)));
		x_ffi_to_double(p_base, p_a, &a);
		x_ffi_to_double(p_base, p_b, &b);
		r = a * b;
		return x_ffi_from_double(p_base, &r);
	}

	if (x_lib_strcmp(conv, "d/d") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_rest)));
		x_ffi_to_double(p_base, p_a, &a);
		x_ffi_to_double(p_base, p_b, &b);
		r = a / b;
		return x_ffi_from_double(p_base, &r);
	}

	/* Comparison conventions */
	if (x_lib_strcmp(conv, "d<d") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_rest)));
		x_ffi_to_double(p_base, p_a, &a);
		x_ffi_to_double(p_base, p_b, &b);
		return a < b ? x_base_field_true(p_base) : x_base_field_false(p_base);
	}

	if (x_lib_strcmp(conv, "d>d") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_rest)));
		x_ffi_to_double(p_base, p_a, &a);
		x_ffi_to_double(p_base, p_b, &b);
		return a > b ? x_base_field_true(p_base) : x_base_field_false(p_base);
	}

	if (x_lib_strcmp(conv, "d=d") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_rest)));
		x_ffi_to_double(p_base, p_a, &a);
		x_ffi_to_double(p_base, p_b, &b);
		return a == b ? x_base_field_true(p_base) : x_base_field_false(p_base);
	}

	if (x_lib_strcmp(conv, "d<=d") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_rest)));
		x_ffi_to_double(p_base, p_a, &a);
		x_ffi_to_double(p_base, p_b, &b);
		return a <= b ? x_base_field_true(p_base) : x_base_field_false(p_base);
	}

	if (x_lib_strcmp(conv, "d>=d") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_rest)));
		x_ffi_to_double(p_base, p_a, &a);
		x_ffi_to_double(p_base, p_b, &b);
		return a >= b ? x_base_field_true(p_base) : x_base_field_false(p_base);
	}

	/* Cast conventions */
	if (x_lib_strcmp(conv, "i->d") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		a = (double)x_intval(p_a);
		return x_ffi_from_double(p_base, &a);
	}

	if (x_lib_strcmp(conv, "d->i") == 0) {
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		x_ffi_to_double(p_base, p_a, &a);
		return x_mkint(p_base, (x_int_t)a);
	}

	/* String/double conversions */
	if (x_lib_strcmp(conv, "s0->d") == 0) {
		fptr = x_ptrval(p_fptr);
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		r = ((double (*)(const char *, void *))fptr)(
			x_firststr(p_a), NULL);
		return x_ffi_from_double(p_base, &r);
	}

	if (x_lib_strcmp(conv, "d->s") == 0) {
		x_char_t buf[32];
		int len;
		p_a = x_prim_eval_arg(p_base, x_firstobj(p_rest));
		x_ffi_to_double(p_base, p_a, &a);
		len = sprintf((char *)buf, "%.15g", a);
		return x_mkstrown(p_base, x_lib_strndup(buf, len));
	}

	return NULL;
}

/* ptr-call: (ptr-call fptr args...) -> int */
static x_obj_t *x_prim_ptr_call(x_obj_t *p_base, x_obj_t *p_args)
{
	long i = 0, p[7];
	x_obj_t *arg, *p_fptr = x_prim_eval_arg(p_base, x_01(p_args));
	long (*fn)(long, long, long, long, long, long, long);

	p_args = x_11(p_args); /* skip self + fptr, walk remaining */
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
	x_obj_t *p_n = x_prim_eval_arg(p_base, x_01(p_args));

	return x_mkptr(p_base, (void *)x_intval(p_n));
}

/* ptr->int: (ptr->int p) -> int */
static x_obj_t *x_prim_ptr_to_int(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_p = x_prim_eval_arg(p_base, x_01(p_args));

	return x_mkint(p_base, (x_int_t)x_ptrval(p_p));
}

/* ptr-set!: (ptr-set! ptr offset value nbytes) -> ptr */
static x_obj_t *x_prim_ptr_set(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_ptr = x_prim_eval_arg(p_base, x_01(p_args)),
		*p_offset = x_prim_eval_arg(p_base, x_011(p_args)),
		*p_val = x_prim_eval_arg(p_base, x_0111(p_args)),
		*p_size = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(p_args))))));
	unsigned char *mem = (unsigned char *)x_ptrval(p_ptr);
	x_int_t val = x_intval(p_val);

	memcpy(mem + x_intval(p_offset), &val, x_intval(p_size));

	return p_ptr;
}

/* ptr-ref: (ptr-ref ptr offset nbytes) -> int */
static x_obj_t *x_prim_ptr_ref(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_ptr = x_prim_eval_arg(p_base, x_01(p_args)),
		*p_offset = x_prim_eval_arg(p_base, x_011(p_args)),
		*p_size = x_prim_eval_arg(p_base, x_0111(p_args));
	unsigned char *mem = (unsigned char *)x_ptrval(p_ptr);
	x_int_t val = 0;

	memcpy(&val, mem + x_intval(p_offset), x_intval(p_size));

	return x_mkint(p_base, val);
}

/* ptr-set-word!: (ptr-set-word! ptr offset value) -> ptr */
static x_obj_t *x_prim_ptr_set_word(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_ptr = x_prim_eval_arg(p_base, x_01(p_args)),
		*p_offset = x_prim_eval_arg(p_base, x_011(p_args)),
		*p_val = x_prim_eval_arg(p_base, x_0111(p_args));
	unsigned char *mem = (unsigned char *)x_ptrval(p_ptr);
	long val = (long)x_intval(p_val);

	memcpy(mem + x_intval(p_offset), &val, sizeof(long));

	return p_ptr;
}

/* string->ptr: (string->ptr str) -> ptr */
static x_obj_t *x_prim_string_to_ptr(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_str = x_prim_eval_arg(p_base, x_01(p_args));

	return x_mkptr(p_base, (void *)x_strval(p_str));
}

/* ptr->string: (ptr->string ptr) -> string */
static x_obj_t *x_prim_ptr_to_string(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_ptr = x_prim_eval_arg(p_base, x_01(p_args));
	x_char_t *s = (x_char_t *)x_ptrval(p_ptr);

	return x_mkstrown(p_base, x_lib_strndup(s, x_lib_strlen(s)));
}

/* obj->ptr: (obj->ptr obj) -> ptr */
static x_obj_t *x_prim_obj_to_ptr(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_prim_eval_arg(p_base, x_01(p_args));

	return x_mkptr(p_base, (void *)p_obj);
}

/* ptr-ref-word: (ptr-ref-word ptr offset) -> int */
static x_obj_t *x_prim_ptr_ref_word(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_ptr = x_prim_eval_arg(p_base, x_01(p_args)),
		*p_offset = x_prim_eval_arg(p_base, x_011(p_args));
	unsigned char *mem = (unsigned char *)x_ptrval(p_ptr);
	long val;

	memcpy(&val, mem + x_intval(p_offset), sizeof(long));

	return x_mkint(p_base, (x_int_t)val);
}

/* obj-meta-extra: (obj-meta-extra) -> int */
static x_obj_t *x_prim_obj_meta_extra(x_obj_t *p_base, x_obj_t *p_args)
{
	(void)p_args;
	return x_mkint(p_base, (x_int_t)x_obj_meta_extra);
}

/* obj-meta-extra!: (obj-meta-extra! n) -> int */
static x_obj_t *x_prim_obj_meta_extra_set(x_obj_t *p_base, x_obj_t *p_args)
{
	x_int_t old = (x_int_t)x_obj_meta_extra;
	x_obj_t *p_n = x_prim_eval_arg(p_base, x_01(p_args));

	x_obj_meta_extra = (size_t)x_intval(p_n);

	if (x_base_isset(p_base)) {
		x_atomint(x_base_field_obj_meta_extra(p_base)) = x_intval(p_n);
	}

	return x_mkint(p_base, old);
}

/* obj-meta-ref: (obj-meta-ref obj i) -> int */
static x_obj_t *x_prim_obj_meta_ref(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_prim_eval_arg(p_base, x_01(p_args)),
		*p_i = x_prim_eval_arg(p_base, x_011(p_args));

	if (x_obj_isnil(p_base, p_obj)
			|| !(x_obj_flags(p_obj) & X_OBJ_FLAG_EXT)) {
		return x_mkint(p_base, 0);
	}

	return x_mkint(p_base, x_obj_meta_slot(p_obj, x_intval(p_i)).i);
}

/* obj-meta-set!: (obj-meta-set! obj i val) -> val */
static x_obj_t *x_prim_obj_meta_set(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_prim_eval_arg(p_base, x_01(p_args)),
		*p_i = x_prim_eval_arg(p_base, x_011(p_args)),
		*p_val = x_prim_eval_arg(p_base, x_0111(p_args));

	if (!x_obj_isnil(p_base, p_obj)
			&& (x_obj_flags(p_obj) & X_OBJ_FLAG_EXT)) {
		x_obj_meta_slot(p_obj, x_intval(p_i)).i = x_intval(p_val);
	}

	return p_val;
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
		{ "ptr-ref-word", x_prim_ptr_ref_word },
		{ "ptr-set-word!", x_prim_ptr_set_word },
		{ "obj->ptr", x_prim_obj_to_ptr },
		{ "string->ptr", x_prim_string_to_ptr },
		{ "ptr->string", x_prim_ptr_to_string },
		{ "obj-meta-count", x_prim_obj_meta_extra },
		{ "obj-meta-count!", x_prim_obj_meta_extra_set },
		{ "obj-meta-ref", x_prim_obj_meta_ref },
		{ "obj-meta-set!", x_prim_obj_meta_set }
	};

	x_prim_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	/* Bind platform constants */
	{
		static const struct { x_char_t *name; x_int_t val; } consts[] = {
			{ "%word-size", (x_int_t)sizeof(void *) },
			{ "%O_RDONLY",  O_RDONLY },
			{ "%O_WRONLY",  O_WRONLY },
			{ "%O_CREAT",   O_CREAT },
			{ "%O_TRUNC",   O_TRUNC },
			{ "%O_APPEND",  O_APPEND }
		};
		int i;

		for (i = 0; i < (int)(sizeof(consts) / sizeof(consts[0])); i++) {
			x_obj_t *p_sym = x_make_symbol(p_base,
					X_OBJ_FLAG_NONE, consts[i].name),
				*p_val = x_mkint(p_base, consts[i].val),
				*p_pair = x_mkspair(p_base, p_sym, p_val);
			x_base_env_alist_extend(p_base, p_pair);
		}
	}

	return p_base;
}
