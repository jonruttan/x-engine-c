/**
 * @file ffi.c
 * @brief Foreign Function Interface primitives for x-lang.
 *
 * Provides dynamic library loading (dlopen, dlsym), typed foreign calls
 * (ffi-call with convention strings), raw pointer calls (ptr-call),
 * pointer/integer/string/object conversions (int->ptr, ptr->int, str->ptr,
 * ptr->str, obj->ptr, ptr->obj), raw memory access (ptr-ref, ptr-set!,
 * ptr-ref-word, ptr-set-word!), and callable construction (make-callable).
 * (The obj-meta-* accessors are pure x-lang now: boot/reflect.x files
 * reflective implementations into the catalog over the layout contracts.)
 *
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2026 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
/*
 * # Includes
 */
#include "x-prim.h"
#include "x-eval.h"
#include "x-type/int.h"
#include "x-type/prim.h"
#include "x-type/ptr.h"
#include "x-type/str.h"
#include "x-type/symbol.h"

#include <string.h>  /* memcpy */
#include <stdio.h>   /* sprintf */
#include <math.h>    /* fmod (the d%d convention; -lm on Linux) */
#include <dlfcn.h>   /* dlopen, dlsym */

/**
 * @name Double Bit-Pattern Helpers
 *
 * IEEE 754 doubles are passed through the FFI as raw bit patterns stored in
 * integers. On 64-bit platforms a single x_int_t holds the full 8-byte
 * pattern; on 32-bit platforms two integers (a pair) carry the low and high
 * halves.
 * @{
 */
#if defined(__LP64__) || defined(_LP64) || defined(_WIN64)

/**
 * @brief Convert an integer bit-pattern to a C double (64-bit path).
 *
 * @param p_base  Unused.
 * @param p_bits  Integer object whose value is the raw IEEE 754 bits.
 * @param[out] out  Destination double.
 */
static void x_ffi_to_double(x_obj_t *p_base, x_obj_t *p_bits, double *out)
{
	(void)p_base;
	memcpy(out, &x_intval(p_bits), sizeof(double));
}

/**
 * @brief Convert a C double to an integer bit-pattern (64-bit path).
 *
 * @param p_base  Base (execution context) for allocation.
 * @param[in] in  Pointer to the source double.
 * @return Integer object holding the raw IEEE 754 bits.
 */
static x_obj_t *x_ffi_from_double(x_obj_t *p_base, double *in)
{
	x_int_t bits;
	memcpy(&bits, in, sizeof(double));
	return x_mkint(p_base, bits);
}

#else /* 32-bit */

/**
 * @brief Convert a pair of integers to a C double (32-bit path).
 *
 * @param p_base  Unused.
 * @param p_bits  Pair whose first/rest are the low/high 32-bit halves.
 * @param[out] out  Destination double.
 */
static void x_ffi_to_double(x_obj_t *p_base, x_obj_t *p_bits, double *out)
{
	x_int_t parts[2];
	(void)p_base;
	parts[0] = x_intval(x_firstobj(p_bits));
	parts[1] = x_intval(x_restobj(p_bits));
	memcpy(out, parts, sizeof(double));
}

/**
 * @brief Convert a C double to a pair of integers (32-bit path).
 *
 * @param p_base  Base (execution context) for allocation.
 * @param[in] in  Pointer to the source double.
 * @return Pair of two integers (low . high) holding the raw bits.
 */
static x_obj_t *x_ffi_from_double(x_obj_t *p_base, double *in)
{
	x_int_t parts[2];
	memcpy(parts, in, sizeof(double));
	return x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkint(p_base, parts[0]),
		x_mkint(p_base, parts[1]));
}

#endif /* 64-bit vs 32-bit */
/** @} */

/**
 * @brief Open a dynamic shared library.
 *
 * x-lang form: @code (dlopen path flags) @endcode
 *
 * Wraps POSIX dlopen(3). If @p path is nil, opens the main program handle.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self path-string flags-int).
 * @return Pointer object wrapping the library handle, or NULL on failure.
 * @note FFI: calls dlopen(3) directly.
 */
static x_obj_t *x_prim_dlopen(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_path, *p_flags;
	void *h;

	x_eargs(p_base, p_args, 3, NULL, &p_path, &p_flags);
	h = dlopen(x_obj_isnil(p_base, p_path) ? NULL : x_strval(p_path),
		(int)x_intval(p_flags));

	return h ? x_mkptr(p_base, h) : NULL;
}

/**
 * @brief Look up a symbol in a dynamic library.
 *
 * x-lang form: @code (dlsym handle name) @endcode
 *
 * Wraps POSIX dlsym(3).
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self handle-ptr name-string).
 * @return Pointer object wrapping the symbol address, or NULL if not found.
 * @note FFI: calls dlsym(3) directly.
 */
static x_obj_t *x_prim_dlsym(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_handle, *p_name;
	void *sym;

	x_eargs(p_base, p_args, 3, NULL, &p_handle, &p_name);
	sym = dlsym(x_ptrval(p_handle), x_strval(p_name));

	return sym ? x_mkptr(p_base, sym) : NULL;
}

/**
 * @brief Call a foreign function using a convention string.
 *
 * x-lang form: @code (ffi-call convention fptr args...) @endcode
 *
 * The convention string selects the calling convention and type coercions:
 * - Function calls: "d->d" (double->double), "dd->d" (double,double->double)
 * - Arithmetic: "d+d", "d-d", "d*d", "d/d", "d%d" (inline double ops -- d%d
 *   is fmod, matching the truncated-division %; no fptr needed)
 * - Comparisons: "d<d", "d>d", "d=d", "d<=d", "d>=d" (return t/f)
 * - Casts: "i->d" (int to double bits), "d->i" (double bits to int)
 * - String: "s0->d" (string,NULL->double via fptr), "d->s" (double to string)
 *
 * Doubles are represented as raw IEEE 754 bit patterns in integers.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self convention-string fptr args...).
 * @return Result of the foreign call, or NULL for unknown convention.
 * @note FFI: double bit-patterns are platform-dependent (64-bit vs 32-bit pair).
 * @see x_ffi_to_double, x_ffi_from_double
 */
static x_obj_t *x_prim_ffi_call(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_conv, *p_fptr, *p_rest, *p_a, *p_b;
	x_char_t *conv;
	void *fptr;
	double a, b, r;
	x_char_t buf[32];
	int len;

	x_eargs(p_base, p_args, 3, NULL, &p_conv, &p_fptr);
	p_rest = x_111(p_args);
	conv = x_strval(p_conv);

	/* Function call conventions */
	if (x_lib_strcmp(conv, "d->d") == 0) {
		fptr = x_ptrval(p_fptr);
		p_a = x_eval_arg(p_base, x_firstobj(p_rest));
		x_ffi_to_double(p_base, p_a, &a);
		r = ((double (*)(double))fptr)(a);
		return x_ffi_from_double(p_base, &r);
	}

	if (x_lib_strcmp(conv, "dd->d") == 0) {
		fptr = x_ptrval(p_fptr);
		p_a = x_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_eval_arg(p_base,
			x_firstobj(x_restobj(p_rest)));
		x_ffi_to_double(p_base, p_a, &a);
		x_ffi_to_double(p_base, p_b, &b);
		r = ((double (*)(double, double))fptr)(a, b);
		return x_ffi_from_double(p_base, &r);
	}

	/* Arithmetic conventions */
	if (x_lib_strcmp(conv, "d+d") == 0) {
		p_a = x_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_eval_arg(p_base, x_firstobj(x_restobj(p_rest)));
		x_ffi_to_double(p_base, p_a, &a);
		x_ffi_to_double(p_base, p_b, &b);
		r = a + b;
		return x_ffi_from_double(p_base, &r);
	}

	if (x_lib_strcmp(conv, "d-d") == 0) {
		p_a = x_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_eval_arg(p_base, x_firstobj(x_restobj(p_rest)));
		x_ffi_to_double(p_base, p_a, &a);
		x_ffi_to_double(p_base, p_b, &b);
		r = a - b;
		return x_ffi_from_double(p_base, &r);
	}

	if (x_lib_strcmp(conv, "d*d") == 0) {
		p_a = x_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_eval_arg(p_base, x_firstobj(x_restobj(p_rest)));
		x_ffi_to_double(p_base, p_a, &a);
		x_ffi_to_double(p_base, p_b, &b);
		r = a * b;
		return x_ffi_from_double(p_base, &r);
	}

	if (x_lib_strcmp(conv, "d/d") == 0) {
		p_a = x_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_eval_arg(p_base, x_firstobj(x_restobj(p_rest)));
		x_ffi_to_double(p_base, p_a, &a);
		x_ffi_to_double(p_base, p_b, &b);
		r = a / b;
		return x_ffi_from_double(p_base, &r);
	}

	if (x_lib_strcmp(conv, "d%d") == 0) {
		p_a = x_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_eval_arg(p_base, x_firstobj(x_restobj(p_rest)));
		x_ffi_to_double(p_base, p_a, &a);
		x_ffi_to_double(p_base, p_b, &b);
		r = fmod(a, b);
		return x_ffi_from_double(p_base, &r);
	}

	/* Comparison conventions */
	if (x_lib_strcmp(conv, "d<d") == 0) {
		p_a = x_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_eval_arg(p_base, x_firstobj(x_restobj(p_rest)));
		x_ffi_to_double(p_base, p_a, &a);
		x_ffi_to_double(p_base, p_b, &b);
		return a < b ? x_firstobj(x_eval_field_true(p_base)) : x_firstobj(x_eval_field_false(p_base));
	}

	if (x_lib_strcmp(conv, "d>d") == 0) {
		p_a = x_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_eval_arg(p_base, x_firstobj(x_restobj(p_rest)));
		x_ffi_to_double(p_base, p_a, &a);
		x_ffi_to_double(p_base, p_b, &b);
		return a > b ? x_firstobj(x_eval_field_true(p_base)) : x_firstobj(x_eval_field_false(p_base));
	}

	if (x_lib_strcmp(conv, "d=d") == 0) {
		p_a = x_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_eval_arg(p_base, x_firstobj(x_restobj(p_rest)));
		x_ffi_to_double(p_base, p_a, &a);
		x_ffi_to_double(p_base, p_b, &b);
		return a == b ? x_firstobj(x_eval_field_true(p_base)) : x_firstobj(x_eval_field_false(p_base));
	}

	if (x_lib_strcmp(conv, "d<=d") == 0) {
		p_a = x_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_eval_arg(p_base, x_firstobj(x_restobj(p_rest)));
		x_ffi_to_double(p_base, p_a, &a);
		x_ffi_to_double(p_base, p_b, &b);
		return a <= b ? x_firstobj(x_eval_field_true(p_base)) : x_firstobj(x_eval_field_false(p_base));
	}

	if (x_lib_strcmp(conv, "d>=d") == 0) {
		p_a = x_eval_arg(p_base, x_firstobj(p_rest));
		p_b = x_eval_arg(p_base, x_firstobj(x_restobj(p_rest)));
		x_ffi_to_double(p_base, p_a, &a);
		x_ffi_to_double(p_base, p_b, &b);
		return a >= b ? x_firstobj(x_eval_field_true(p_base)) : x_firstobj(x_eval_field_false(p_base));
	}

	/* Cast conventions */
	if (x_lib_strcmp(conv, "i->d") == 0) {
		p_a = x_eval_arg(p_base, x_firstobj(p_rest));
		a = (double)x_intval(p_a);
		return x_ffi_from_double(p_base, &a);
	}

	if (x_lib_strcmp(conv, "d->i") == 0) {
		p_a = x_eval_arg(p_base, x_firstobj(p_rest));
		x_ffi_to_double(p_base, p_a, &a);
		return x_mkint(p_base, (x_int_t)a);
	}

	/* String/double conversions */
	if (x_lib_strcmp(conv, "s0->d") == 0) {
		fptr = x_ptrval(p_fptr);
		p_a = x_eval_arg(p_base, x_firstobj(p_rest));
		r = ((double (*)(const char *, void *))fptr)(
			x_firststr(p_a), NULL);
		return x_ffi_from_double(p_base, &r);
	}

	if (x_lib_strcmp(conv, "d->s") == 0) {
		p_a = x_eval_arg(p_base, x_firstobj(p_rest));
		x_ffi_to_double(p_base, p_a, &a);
		len = sprintf((char *)buf, "%.15g", a);
		return x_mkstrown(p_base, x_lib_strndup(buf, len));
	}

	return NULL;
}

/**
 * @brief Call a raw function pointer with up to 7 long-typed arguments.
 *
 * x-lang form: @code (ptr-call fptr args...) @endcode
 *
 * Arguments are evaluated and coerced: integers become long, strings and
 * pointers pass their raw C pointer. The function is called with the
 * C calling convention (long, long, ...) -> long.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self fptr arg0 ... arg6).
 * @return Integer wrapping the long return value.
 * @note FFI: maximum 7 arguments; excess arguments are silently ignored.
 */
static x_obj_t *x_prim_ptr_call(x_obj_t *p_base, x_obj_t *p_args)
{
	long i = 0, p[7];
	x_obj_t *arg, *p_fptr;
	long (*fn)(long, long, long, long, long, long, long);

	x_eargs(p_base, p_args, 2, NULL, &p_fptr);
	p_args = x_11(p_args); /* skip self + fptr, walk remaining */
	p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = p[6] = 0;

	while (!x_obj_isnil(p_base, p_args) && i < 7) {
		arg = x_eval_arg(p_base, x_firstobj(p_args));
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

/**
 * @brief Cast an integer to a pointer object.
 *
 * x-lang form: @code (int->ptr n) @endcode
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self n).
 * @return Pointer object wrapping (void *)n.
 */
static x_obj_t *x_prim_int_to_ptr(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_n;

	x_eargs(p_base, p_args, 2, NULL, &p_n);

	return x_mkptr(p_base, (void *)x_intval(p_n));
}

/**
 * @brief Cast a pointer object to an integer.
 *
 * x-lang form: @code (ptr->int p) @endcode
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self ptr).
 * @return Integer wrapping the pointer's numeric address.
 */
static x_obj_t *x_prim_ptr_to_int(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_p;

	x_eargs(p_base, p_args, 2, NULL, &p_p);

	return x_mkint(p_base, (x_int_t)x_ptrval(p_p));
}

/**
 * @brief Allocate a raw, UNMANAGED byte region.
 *
 * x-lang form: @code (mem-alloc n) @endcode
 *
 * The region is zeroed (calloc semantics -- deterministic contents) and
 * returned as a POINTER object that does NOT own it: the caller must
 * release it with (mem free), or it leaks.  Prefer (str make) -- a
 * GC-owned region -- unless a header-less block is required (FFI structs,
 * alignment-sensitive out-params).  UNCHECKED like first/rest: n is
 * trusted.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self n).
 * @return Pointer object addressing the fresh region.
 */
static x_obj_t *x_prim_mem_alloc(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_n;
	void *p;
	size_t n;

	x_eargs(p_base, p_args, 2, NULL, &p_n);
	n = (size_t)x_intval(p_n);
	p = x_sys_malloc(n);
	x_lib_memset(p, 0, n);

	return x_mkptr(p_base, p);
}

/**
 * @brief Block copy: the machine's rep-movs instruction.
 *
 * x-lang form: @code (mem-copy dst src n) @endcode
 *
 * Copies @p n bytes from src to dst (x_lib_memcpy; regions must not
 * overlap).  UNCHECKED like first/rest: pointers and n are trusted.
 * Side-effect prim: returns NULL.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self dst src n).
 * @return NULL.
 */
static x_obj_t *x_prim_mem_copy(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_dst, *p_src, *p_n;

	x_eargs(p_base, p_args, 4, NULL, &p_dst, &p_src, &p_n);
	x_lib_memcpy(x_ptrval(p_dst), x_ptrval(p_src), (size_t)x_intval(p_n));

	return NULL;
}

/**
 * @brief Block compare: the machine's rep-cmps instruction.
 *
 * x-lang form: @code (mem-cmp a b n) @endcode
 *
 * TRUE memcmp semantics (NUL bytes do not terminate -- strncmp would be
 * wrong for raw regions), implemented as a local byte loop so x-lib
 * needs no new entry.  Returns 0 on equality, else the sign of the
 * first differing byte pair (-1/1).  UNCHECKED.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self a b n).
 * @return Integer 0, -1, or 1.
 */
static x_obj_t *x_prim_mem_cmp(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_a, *p_b, *p_n;
	const unsigned char *a, *b;
	size_t n, i;

	x_eargs(p_base, p_args, 4, NULL, &p_a, &p_b, &p_n);
	a = (const unsigned char *)x_ptrval(p_a);
	b = (const unsigned char *)x_ptrval(p_b);
	n = (size_t)x_intval(p_n);
	for (i = 0; i < n; i++) {
		if (a[i] != b[i])
			return x_mkint(p_base, a[i] < b[i] ? -1 : 1);
	}

	return x_mkint(p_base, 0);
}

/**
 * @brief Block fill: the machine's rep-stos instruction.
 *
 * x-lang form: @code (mem-set p byte n) @endcode
 *
 * Fills @p n bytes at p with @p byte (x_lib_memset).  UNCHECKED.
 * Side-effect prim: returns NULL.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self p byte n).
 * @return NULL.
 */
static x_obj_t *x_prim_mem_set(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_p, *p_byte, *p_n;

	x_eargs(p_base, p_args, 4, NULL, &p_p, &p_byte, &p_n);
	x_lib_memset(x_ptrval(p_p), (int)x_intval(p_byte), (size_t)x_intval(p_n));

	return NULL;
}

/**
 * @brief Release a region obtained from (mem alloc).
 *
 * x-lang form: @code (mem-free ptr) @endcode
 *
 * UNCHECKED: freeing anything not returned by (mem alloc), or freeing
 * twice, is UB exactly as in C.  Side-effect prim: returns NULL.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self ptr).
 * @return NULL.
 */
static x_obj_t *x_prim_mem_free(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_p;

	x_eargs(p_base, p_args, 2, NULL, &p_p);
	x_sys_free(x_ptrval(p_p));

	return NULL;
}

/**
 * @brief Write nbytes of an integer value into raw memory at ptr+offset.
 *
 * x-lang form: @code (ptr-set! ptr offset value nbytes) @endcode
 *
 * Copies the low @p nbytes of @p value via memcpy into the memory at
 * the given pointer plus byte offset.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self ptr offset value nbytes).
 * @return The original pointer object.
 * @note FFI: no bounds checking; caller must ensure valid memory region.
 */
static x_obj_t *x_prim_ptr_set(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_ptr, *p_offset, *p_val, *p_size;
	unsigned char *mem;
	x_int_t val;

	x_eargs(p_base, p_args, 5, NULL, &p_ptr, &p_offset, &p_val, &p_size);
	mem = (unsigned char *)x_ptrval(p_ptr);
	val = x_intval(p_val);

	memcpy(mem + x_intval(p_offset), &val, x_intval(p_size));

	return p_ptr;
}

/**
 * @brief Read nbytes from raw memory at ptr+offset as an integer.
 *
 * x-lang form: @code (ptr-ref ptr offset nbytes) @endcode
 *
 * Copies @p nbytes from the memory at the pointer plus byte offset into
 * a zero-initialized integer value via memcpy.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self ptr offset nbytes).
 * @return Integer holding the read value.
 * @note FFI: no bounds checking; caller must ensure valid memory region.
 */
static x_obj_t *x_prim_ptr_ref(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_ptr, *p_offset, *p_size;
	unsigned char *mem;
	x_int_t val = 0;

	x_eargs(p_base, p_args, 4, NULL, &p_ptr, &p_offset, &p_size);
	mem = (unsigned char *)x_ptrval(p_ptr);

	memcpy(&val, mem + x_intval(p_offset), x_intval(p_size));

	return x_mkint(p_base, val);
}

/**
 * @brief Write a machine-word-sized value into raw memory at ptr+offset.
 *
 * x-lang form: @code (ptr-set-word! ptr offset value) @endcode
 *
 * Writes sizeof(long) bytes of @p value into memory at ptr+offset.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self ptr offset value).
 * @return The original pointer object.
 * @note FFI: word size is sizeof(long); no bounds checking.
 */
static x_obj_t *x_prim_ptr_set_word(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_ptr, *p_offset, *p_val;
	unsigned char *mem;
	long val;

	x_eargs(p_base, p_args, 4, NULL, &p_ptr, &p_offset, &p_val);
	mem = (unsigned char *)x_ptrval(p_ptr);
	val = (long)x_intval(p_val);

	memcpy(mem + x_intval(p_offset), &val, sizeof(long));

	return p_ptr;
}

/**
 * @brief Get the raw C string pointer from a string object.
 *
 * x-lang form: @code (str->ptr str) @endcode
 *
 * Returns a pointer to the string's internal character buffer. The pointer
 * is only valid while the string is not garbage collected.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self str).
 * @return Pointer object wrapping the string's char buffer.
 */
static x_obj_t *x_prim_string_to_ptr(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_str;

	x_eargs(p_base, p_args, 2, NULL, &p_str);

	return x_mkptr(p_base, (void *)x_strval(p_str));
}

/**
 * @brief Copy a C string from a pointer into a new string object.
 *
 * x-lang form: @code (ptr->str ptr) @endcode
 *
 * Reads a NUL-terminated string from the pointer address and duplicates
 * it into a new owned string object.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self ptr).
 * @return New string object containing a copy of the C string.
 */
static x_obj_t *x_prim_ptr_to_string(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_ptr;
	x_char_t *s;

	x_eargs(p_base, p_args, 2, NULL, &p_ptr);
	s = (x_char_t *)x_ptrval(p_ptr);

	return x_mkstrown(p_base, x_lib_strndup(s, x_lib_strlen(s)));
}

/**
 * @brief Get the raw C pointer to an x-lang object.
 *
 * x-lang form: @code (obj->ptr obj) @endcode
 *
 * Returns the object's address as a pointer. The pointer is invalidated
 * if GC relocates the object.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self obj).
 * @return Pointer object wrapping the object's address.
 * @note The returned pointer may be invalidated by garbage collection.
 */
static x_obj_t *x_prim_obj_to_ptr(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj;

	x_eargs(p_base, p_args, 2, NULL, &p_obj);

	return x_mkptr(p_base, (void *)p_obj);
}

/**
 * @brief Materialize an object reference from a raw pointer.
 *
 * x-lang form: @code (ptr->obj p) @endcode
 *
 * The inverse of obj->ptr and the reflective ISA's load-word-as-reference
 * instruction: it lets X read a word (e.g. an object's data or type slot
 * via ptr-ref-word) and treat it as the object it addresses, which is what
 * makes the reflective accessors in lib/x/boot/reflect.x able to RETURN
 * objects, not just integers.
 *
 * UNCHECKED, like first/rest: an address that is not a live object is
 * undefined behavior.  Safe under the explicit-only GC (allocation never
 * relocates), same argument as holding obj->ptr across an expression.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self ptr).
 * @return The object at the pointer's address (nil for a NULL pointer).
 */
static x_obj_t *x_prim_ptr_to_obj(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_p;

	x_eargs(p_base, p_args, 2, NULL, &p_p);

	return (x_obj_t *)x_ptrval(p_p);
}

/**
 * @brief Read a machine-word-sized value from raw memory at ptr+offset.
 *
 * x-lang form: @code (ptr-ref-word ptr offset) @endcode
 *
 * Reads sizeof(long) bytes from memory at ptr+offset via memcpy.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self ptr offset).
 * @return Integer holding the machine-word value.
 * @note FFI: word size is sizeof(long); no bounds checking.
 */
static x_obj_t *x_prim_ptr_ref_word(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_ptr, *p_offset;
	unsigned char *mem;
	long val;

	x_eargs(p_base, p_args, 3, NULL, &p_ptr, &p_offset);
	mem = (unsigned char *)x_ptrval(p_ptr);

	memcpy(&val, mem + x_intval(p_offset), sizeof(long));

	return x_mkint(p_base, (x_int_t)val);
}

/**
 * @brief Create a callable primitive from a raw function pointer.
 *
 * x-lang form: @code (make-callable fn-ptr) @endcode
 *
 * Wraps a PTR object's address as an x_fn_t in a new prim object,
 * making it callable from x-lang as an operative.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self fn-ptr).
 * @return New callable prim object.
 * @note FFI: the function pointer must follow the x_fn_t signature
 *       (x_obj_t *p_base, x_obj_t *p_args) -> x_obj_t *.
 */
static x_obj_t *x_prim_make_callable(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_ptr;

	x_eargs(p_base, p_args, 2, NULL, &p_ptr);

	return x_make_prim(p_base, X_OBJ_FLAG_NONE,
		(x_fn_t)x_ptrval(p_ptr));
}

/**
 * @brief Register all FFI primitives and platform constants.
 *
 * Binds: dlopen, dlsym, ffi-call, ptr-call, int->ptr, ptr->int, ptr->obj,
 * ptr-set!, ptr-ref, ptr-ref-word, ptr-set-word!, obj->ptr, str->ptr,
 * ptr->str, make-callable.
 * (The obj-meta-* accessors are pure x-lang now: boot/reflect.x files
 * reflective implementations into the catalog over tools/obj-layout.x
 * and tools/base-paths.x.)
 *
 * Platform constants live in X, not here: word size is probed by
 * boot/data.x, and the O_* open flags come from the per-OS tables in
 * x/platform/syscall.x.
 *
 * @param p_base  Base (execution context) to bind primitives into.
 * @param p_args  Unused.
 * @return @p p_base.
 */
x_obj_t *x_prim_ffi_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_prim_entry_t entries[] = {
		{ "dlopen",          x_prim_dlopen,             "ffi", "dlopen"        },
		{ "dlsym",           x_prim_dlsym,              "ffi", "dlsym"         },
		{ "ffi-call",        x_prim_ffi_call,           "ffi", "call"          },
		{ "ptr-call",        x_prim_ptr_call,           "ptr", "call"          },
		{ "int->ptr",        x_prim_int_to_ptr,         "int", "->ptr"         },
		{ "ptr->int",        x_prim_ptr_to_int,         "ptr", "->int"         },
		{ "mem-alloc",       x_prim_mem_alloc,          "mem", "alloc"         },
		{ "mem-cmp",         x_prim_mem_cmp,            "mem", "cmp"           },
		{ "mem-copy",        x_prim_mem_copy,           "mem", "copy"          },
		{ "mem-free",        x_prim_mem_free,           "mem", "free"          },
		{ "mem-set",         x_prim_mem_set,            "mem", "set"           },
		{ "ptr-set!",        x_prim_ptr_set,            "ptr", "set!"          },
		{ "ptr-ref",         x_prim_ptr_ref,            "ptr", "ref"           },
		{ "ptr-ref-word",    x_prim_ptr_ref_word,       "ptr", "ref-word"      },
		{ "ptr-set-word!",   x_prim_ptr_set_word,       "ptr", "set-word!"     },
		{ "obj->ptr",        x_prim_obj_to_ptr,         "obj", "->ptr"         },
		{ "ptr->obj",        x_prim_ptr_to_obj,         "ptr", "->obj"         },
		{ "str->ptr",        x_prim_string_to_ptr,      "str", "->ptr"         },
		{ "ptr->str",        x_prim_ptr_to_string,      "ptr", "->str"         },
		{ "make-callable",   x_prim_make_callable,      "obj", "make-callable" }
	};
	x_prims_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
