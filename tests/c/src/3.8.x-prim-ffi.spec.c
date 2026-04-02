/*
 * # Unit Tests: *x-prim/ffi*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-lib.c"
#include "ext/x-expr/src/x-obj.c"
#include "src/x-obj/obj.c"
#include "src/x-obj/prim.c"
#include "ext/x-expr/src/x.c"
#include "src/x-alist.c"
#include "src/x-base.c"
#include "src/x-eval.c"
#include "src/x-type.c"
#include "src/x-type/atom.c"
#include "src/x-token/sexp/atom.c"
#include "src/x-type/pair.c"
#include "src/x-token/sexp/pair.c"
#include "src/x-type/prim.c"
#include "src/x-type/symbol.c"
#include "src/x-token/sexp/symbol.c"
#include "src/x-type/procedure.c"
#include "src/x-type/operative.c"
#include "src/x-type/list.c"
#include "src/x-token/sexp/list.c"
#include "src/x-type/str.c"
#include "src/x-token/sexp/str.c"
#include "src/x-type/int.c"
#include "src/x-token/sexp/int.c"
#include "src/x-type/char.c"
#include "src/x-token/sexp/char.c"
#include "src/x-type/ptr.c"
#include "src/x-type/whitespace.c"
#include "src/x-token/sexp/whitespace.c"
#include "src/x-type/comment.c"
#include "src/x-token/sexp/comment.c"
#include "src/x-type/buffer.c"
#include "src/x-type/iter.c"
#include "ext/x-expr/src/x-heap.c"
#include "src/x-token.c"
#include "src/x-prim.c"
#include "src/x-prim/ffi.c"

/* Stubs for primitives not under test. */
x_obj_t *x_prim_core_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_arith_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_pred_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_string_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_io_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_type_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_callcc_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_binding_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_closure_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_control_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_quote_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }


#include "ext/x-expr/tests/src/helper-system-functions.c"

/* Test helper functions for ffi-call d->d and dd->d conventions */
static double test_ffi_double_negate(double x) { return -x; }
static double test_ffi_double_add(double a, double b) { return a + b; }
static double test_ffi_strtod(const char *s, void *endp) { (void)endp; return (double)(s[0] - '0'); }
static long test_ffi_long_add3(long a, long b, long c, long d, long e, long f, long g) { (void)d; (void)e; (void)f; (void)g; return a + b + c; }

/*
 * ## Test Overhead
 */

static void _setup(void)
{
	_buffer_index = -1;
	helper_set_alloc(MEM_GUARANTEED);
}

static void _teardown(void)
{
}

void test_cleanup(x_obj_t *p_base)
{
	x_obj_t *p_gc = p_base, *p_tmp;

	while (p_gc) {
		p_tmp = x_obj_heap(p_gc);
		x_sys_free(p_gc);
		p_gc = p_tmp;
	}
}

/* Helper: create double-bits object (int on 64-bit, pair on 32-bit) */
static x_obj_t *test_mk_double(x_obj_t *p_base, double d)
{
	return x_ffi_from_double(p_base, &d);
}

/* Helper: extract C double from double-bits object */
static double test_get_double(x_obj_t *p_base, x_obj_t *p_obj)
{
	double d;
	x_ffi_to_double(p_base, p_obj, &d);
	return d;
}


/*
 * ## Test Runners
 */

static char *test_ffi_arith_add(void)
{
	x_obj_t *p_base, *p_args, *p_result;
	double r;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (ffi-call "d+d" () 3.0 4.0) -> 7.0 */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "d+d"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 3.0),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 4.0),
		NULL)))));
	p_result = x_prim_ffi_call(p_base, p_args);
	r = test_get_double(p_base, p_result);
	_it_should("d+d: 3.0 + 4.0 = 7.0", r == 7.0);

	test_cleanup(p_base);
	return NULL;
}

static char *test_ffi_arith_sub(void)
{
	x_obj_t *p_base, *p_args, *p_result;
	double r;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (ffi-call "d-d" () 10.0 3.0) -> 7.0 */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "d-d"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 10.0),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 3.0),
		NULL)))));
	p_result = x_prim_ffi_call(p_base, p_args);
	r = test_get_double(p_base, p_result);
	_it_should("d-d: 10.0 - 3.0 = 7.0", r == 7.0);

	test_cleanup(p_base);
	return NULL;
}

static char *test_ffi_arith_mul(void)
{
	x_obj_t *p_base, *p_args, *p_result;
	double r;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (ffi-call "d*d" () 3.0 5.0) -> 15.0 */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "d*d"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 3.0),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 5.0),
		NULL)))));
	p_result = x_prim_ffi_call(p_base, p_args);
	r = test_get_double(p_base, p_result);
	_it_should("d*d: 3.0 * 5.0 = 15.0", r == 15.0);

	test_cleanup(p_base);
	return NULL;
}

static char *test_ffi_arith_div(void)
{
	x_obj_t *p_base, *p_args, *p_result;
	double r;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (ffi-call "d/d" () 15.0 3.0) -> 5.0 */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "d/d"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 15.0),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 3.0),
		NULL)))));
	p_result = x_prim_ffi_call(p_base, p_args);
	r = test_get_double(p_base, p_result);
	_it_should("d/d: 15.0 / 3.0 = 5.0", r == 5.0);

	test_cleanup(p_base);
	return NULL;
}

static char *test_ffi_compare(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* d<d: 1.0 < 2.0 -> t */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "d<d"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 1.0),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 2.0),
		NULL)))));
	p_result = x_prim_ffi_call(p_base, p_args);
	_it_should("d<d: 1.0 < 2.0 is true",
		p_result == x_base_field_true(p_base));

	/* d<d: 2.0 < 1.0 -> #f */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "d<d"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 2.0),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 1.0),
		NULL)))));
	p_result = x_prim_ffi_call(p_base, p_args);
	_it_should("d<d: 2.0 < 1.0 is false",
		p_result == x_base_field_false(p_base));

	/* d=d: 5.0 = 5.0 -> t */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "d=d"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 5.0),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 5.0),
		NULL)))));
	p_result = x_prim_ffi_call(p_base, p_args);
	_it_should("d=d: 5.0 = 5.0 is true",
		p_result == x_base_field_true(p_base));

	/* d>=d: 3.0 >= 3.0 -> t */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "d>=d"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 3.0),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 3.0),
		NULL)))));
	p_result = x_prim_ffi_call(p_base, p_args);
	_it_should("d>=d: 3.0 >= 3.0 is true",
		p_result == x_base_field_true(p_base));

	test_cleanup(p_base);
	return NULL;
}

static char *test_ffi_cast_i_to_d(void)
{
	x_obj_t *p_base, *p_args, *p_result;
	double r;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (ffi-call "i->d" () 42) -> 42.0 as IEEE bits */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "i->d"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkint(p_base, (x_int_t)42),
		NULL))));
	p_result = x_prim_ffi_call(p_base, p_args);
	r = test_get_double(p_base, p_result);
	_it_should("i->d: 42 -> 42.0", r == 42.0);

	test_cleanup(p_base);
	return NULL;
}

static char *test_ffi_cast_d_to_i(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (ffi-call "d->i" () 42.7-as-bits) -> 42 (truncated) */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "d->i"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 42.7),
		NULL))));
	p_result = x_prim_ffi_call(p_base, p_args);
	_it_should("d->i: 42.7 -> 42", x_intval(p_result) == 42);

	test_cleanup(p_base);
	return NULL;
}

static char *test_ffi_d_to_s(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (ffi-call "d->s" () 3.14-as-bits) -> "3.14" */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "d->s"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 3.14),
		NULL))));
	p_result = x_prim_ffi_call(p_base, p_args);
	_it_should("d->s returns a string",
		p_result != NULL);
	_it_should("d->s: 3.14 -> \"3.14\"",
		x_lib_strcmp(x_strval(p_result), "3.14") == 0);

	test_cleanup(p_base);
	return NULL;
}

static char *test_ffi_int_ptr_convert(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* int->ptr: convert 12345 to ptr */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkint(p_base, (x_int_t)12345), NULL));
	p_result = x_prim_int_to_ptr(p_base, p_args);
	_it_should("int->ptr returns a ptr",
		p_result != NULL);

	/* ptr->int: convert back to 12345 */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_result, NULL));
	p_result = x_prim_ptr_to_int(p_base, p_args);
	_it_should("ptr->int round-trips to 12345",
		x_intval(p_result) == 12345);

	test_cleanup(p_base);
	return NULL;
}

static char *test_ffi_ptr_set_ref(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_ptr;
	unsigned char mem[16];

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_ptr = x_mkptr(p_base, mem);

	/* ptr-set!: write byte 0xAB at offset 0 (nbytes=1) */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_ptr,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkint(p_base, (x_int_t)0),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkint(p_base, (x_int_t)0xAB),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkint(p_base, (x_int_t)1),
		NULL)))));
	x_prim_ptr_set(p_base, p_args);
	_it_should("ptr-set! writes byte at offset",
		mem[0] == 0xAB);

	/* ptr-ref: read back 1 byte */
	memset(mem, 0, 16);
	mem[0] = 42;
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_ptr,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkint(p_base, (x_int_t)0),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkint(p_base, (x_int_t)1),
		NULL))));
	p_result = x_prim_ptr_ref(p_base, p_args);
	_it_should("ptr-ref reads byte from offset",
		x_intval(p_result) == 42);

	test_cleanup(p_base);
	return NULL;
}

static char *test_ffi_string_ptr_convert(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_ptr;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* string->ptr: get raw pointer to string data */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkstr(p_base, "hello"), NULL));
	p_ptr = x_prim_string_to_ptr(p_base, p_args);
	_it_should("string->ptr returns a ptr",
		p_ptr != NULL);

	/* ptr->string: create string from pointer */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_ptr, NULL));
	p_result = x_prim_ptr_to_string(p_base, p_args);
	_it_should("ptr->string round-trips",
		x_lib_strcmp(x_strval(p_result), "hello") == 0);

	test_cleanup(p_base);
	return NULL;
}

static char *test_ffi_register(void)
{
	x_obj_t *p_base, *p_env;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_env = x_base_field_env_alist(p_base);
	_it_should("env is not empty after register",
		p_env != NULL);

	test_cleanup(p_base);
	return NULL;
}

static char *test_ffi_dlopen_dlsym(void)
{
	x_obj_t *p_base, *p_args, *p_handle, *p_sym;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* dlopen(NULL, RTLD_LAZY) -> handle to current process */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkint(p_base, (x_int_t)RTLD_LAZY),
		NULL)));
	p_handle = x_prim_dlopen(p_base, p_args);
	_it_should("dlopen returns handle for NULL path",
		p_handle != NULL);

	/* dlsym(handle, "x_prim_ffi_register") -> function pointer */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_handle,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "x_prim_ffi_register"),
		NULL)));
	p_sym = x_prim_dlsym(p_base, p_args);
	_it_should("dlsym finds known symbol",
		p_sym != NULL);

	/* dlsym with bogus name -> NULL */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_handle,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "____no_such_symbol____"),
		NULL)));
	p_sym = x_prim_dlsym(p_base, p_args);
	_it_should("dlsym returns NULL for unknown symbol",
		p_sym == NULL);

	/* dlopen with bogus path -> NULL */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkstr(p_base, "/no/such/lib.so"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkint(p_base, (x_int_t)RTLD_LAZY),
		NULL)));
	p_handle = x_prim_dlopen(p_base, p_args);
	_it_should("dlopen returns NULL for bad path",
		p_handle == NULL);

	test_cleanup(p_base);
	return NULL;
}

static char *test_ffi_call_d_to_d(void)
{
	x_obj_t *p_base, *p_args, *p_result;
	double r;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (ffi-call "d->d" fptr 5.0) -> -5.0 */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "d->d"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkptr(p_base, (void *)test_ffi_double_negate),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 5.0),
		NULL))));
	p_result = x_prim_ffi_call(p_base, p_args);
	r = test_get_double(p_base, p_result);
	_it_should("d->d: negate(5.0) = -5.0", r == -5.0);

	test_cleanup(p_base);
	return NULL;
}

static char *test_ffi_call_dd_to_d(void)
{
	x_obj_t *p_base, *p_args, *p_result;
	double r;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (ffi-call "dd->d" fptr 3.0 7.0) -> 10.0 */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "dd->d"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkptr(p_base, (void *)test_ffi_double_add),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 3.0),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 7.0),
		NULL)))));
	p_result = x_prim_ffi_call(p_base, p_args);
	r = test_get_double(p_base, p_result);
	_it_should("dd->d: add(3.0, 7.0) = 10.0", r == 10.0);

	test_cleanup(p_base);
	return NULL;
}

static char *test_ffi_compare_gt_le(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* d>d: 5.0 > 3.0 -> t */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "d>d"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 5.0),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 3.0),
		NULL)))));
	p_result = x_prim_ffi_call(p_base, p_args);
	_it_should("d>d: 5.0 > 3.0 is true",
		p_result == x_base_field_true(p_base));

	/* d>d: 1.0 > 2.0 -> #f */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "d>d"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 1.0),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 2.0),
		NULL)))));
	p_result = x_prim_ffi_call(p_base, p_args);
	_it_should("d>d: 1.0 > 2.0 is false",
		p_result == x_base_field_false(p_base));

	/* d<=d: 3.0 <= 3.0 -> t */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "d<=d"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 3.0),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 3.0),
		NULL)))));
	p_result = x_prim_ffi_call(p_base, p_args);
	_it_should("d<=d: 3.0 <= 3.0 is true",
		p_result == x_base_field_true(p_base));

	/* d<=d: 5.0 <= 3.0 -> #f */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "d<=d"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 5.0),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, test_mk_double(p_base, 3.0),
		NULL)))));
	p_result = x_prim_ffi_call(p_base, p_args);
	_it_should("d<=d: 5.0 <= 3.0 is false",
		p_result == x_base_field_false(p_base));

	test_cleanup(p_base);
	return NULL;
}

static char *test_ffi_call_s0_to_d(void)
{
	x_obj_t *p_base, *p_args, *p_result;
	double r;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (ffi-call "s0->d" fptr "5") -> 5.0 (our stub reads first char - '0') */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "s0->d"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkptr(p_base, (void *)test_ffi_strtod),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "5"),
		NULL))));
	p_result = x_prim_ffi_call(p_base, p_args);
	r = test_get_double(p_base, p_result);
	_it_should("s0->d: strtod(\"5\") = 5.0", r == 5.0);

	test_cleanup(p_base);
	return NULL;
}

static char *test_ffi_unknown_convention(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (ffi-call "bogus" () 1) -> NULL */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "bogus"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkint(p_base, (x_int_t)1),
		NULL))));
	p_result = x_prim_ffi_call(p_base, p_args);
	_it_should("unknown convention returns NULL",
		p_result == NULL);

	test_cleanup(p_base);
	return NULL;
}

static char *test_ffi_ptr_call(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (ptr-call fptr 10 20 30) -> 60 */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkptr(p_base, (void *)test_ffi_long_add3),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkint(p_base, (x_int_t)10),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkint(p_base, (x_int_t)20),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkint(p_base, (x_int_t)30),
		NULL)))));
	p_result = x_prim_ptr_call(p_base, p_args);
	_it_should("ptr-call: add3(10,20,30) = 60",
		x_intval(p_result) == 60);

	test_cleanup(p_base);
	return NULL;
}

static char *test_ffi_ptr_set_word(void)
{
	x_obj_t *p_base, *p_args, *p_result;
	unsigned char mem[32];
	long val;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	memset(mem, 0, sizeof(mem));

	/* (ptr-set-word! ptr 0 12345) */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkptr(p_base, mem),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkint(p_base, (x_int_t)0),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkint(p_base, (x_int_t)12345),
		NULL))));
	p_result = x_prim_ptr_set_word(p_base, p_args);
	_it_should("ptr-set-word! returns ptr",
		p_result != NULL);
	memcpy(&val, mem, sizeof(long));
	_it_should("ptr-set-word! writes long value",
		val == 12345);

	test_cleanup(p_base);
	return NULL;
}

static char *test_ffi_ptr_call_str_arg(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* ptr-call with a string arg exercises the str branch */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkptr(p_base, (void *)x_lib_strlen),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "hello"),
		NULL)));
	p_result = x_prim_ptr_call(p_base, p_args);
	_it_should("ptr-call with string arg: strlen(\"hello\") = 5",
		x_intval(p_result) == 5);

	/* ptr-call with a ptr arg exercises the ptr branch */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkptr(p_base, (void *)x_lib_strlen),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkptr(p_base, (void *)"world"),
		NULL)));
	p_result = x_prim_ptr_call(p_base, p_args);
	_it_should("ptr-call with ptr arg: strlen(ptr(\"world\")) = 5",
		x_intval(p_result) == 5);

	test_cleanup(p_base);
	return NULL;
}

static char *run_tests() {
	_run_test(test_ffi_arith_add);
	_run_test(test_ffi_arith_sub);
	_run_test(test_ffi_arith_mul);
	_run_test(test_ffi_arith_div);
	_run_test(test_ffi_compare);
	_run_test(test_ffi_cast_i_to_d);
	_run_test(test_ffi_cast_d_to_i);
	_run_test(test_ffi_d_to_s);
	_run_test(test_ffi_int_ptr_convert);
	_run_test(test_ffi_ptr_set_ref);
	_run_test(test_ffi_string_ptr_convert);
	_run_test(test_ffi_register);
	_run_test(test_ffi_dlopen_dlsym);
	_run_test(test_ffi_call_d_to_d);
	_run_test(test_ffi_call_dd_to_d);
	_run_test(test_ffi_compare_gt_le);
	_run_test(test_ffi_call_s0_to_d);
	_run_test(test_ffi_unknown_convention);
	_run_test(test_ffi_ptr_call);
	_run_test(test_ffi_ptr_set_word);
	_run_test(test_ffi_ptr_call_str_arg);

	return NULL;
}
