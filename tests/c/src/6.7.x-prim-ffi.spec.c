/*
 * # Unit Tests: *x-prim/ffi*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "ext/x-expr/tests/src/test-helper-system.c"

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-stdlib.c"
#include "ext/x-expr/src/x-lib.c"
#include "ext/x-expr/src/x-obj.c"
#include "src/x-obj/obj.c"
#include "src/x-obj/prim.c"
#include "ext/x-expr/src/x.c"
#include "src/x-alist.c"
#include "ext/x-expr/src/x-base.c"
#include "src/x-eval.c"
#include "src/x-vector.c"
#include "src/x-env.c"
#include "src/x-tco.c"
#include "src/x-toplevel.c"
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
#include "src/x-type/err.c"
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
x_obj_t *x_prim_heap_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
/* The slot table in x-eval.c names the collector's phases. */
x_obj_t *x_heap_mark_phase(x_obj_t *p_base, x_obj_t *p_args) { return NULL; }
x_obj_t *x_heap_sweep_phase(x_obj_t *p_base, x_obj_t *p_args) { return NULL; }
x_obj_t *x_prim_image_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_base_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_buffer_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_iter_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_type_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_callcc_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_binding_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_closure_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_control_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_quote_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }



/* Test helper function for ptr-call */
static long test_ffi_long_add3(long a, long b, long c, long d, long e, long f, long g, long h) { (void)d; (void)e; (void)f; (void)g; (void)h; return a + b + c; }

/* Test helper for ptr-call: the eighth argument, which must arrive */
static long test_ffi_long_eighth(long a, long b, long c, long d, long e, long f, long g, long h) { (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g; return h; }

/*
 * ## Test Overhead
 */

static void _setup(void)
{
	_buffer_index = -1;
	helper_set_alloc(MEM_GUARANTEED);
	helper_sys_funcs.exit = mock_exit;
	helper_sys_funcs.malloc = helper_malloc;
	helper_sys_funcs.free = helper_free;
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

/*
 * ## Test Runners
 */

static char *test_ffi_int_ptr_convert(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_eval_make(NULL, NULL);
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

	p_base = x_eval_make(NULL, NULL);
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

	p_base = x_eval_make(NULL, NULL);
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

	p_base = x_eval_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_env = x_eval_field_env(p_base);
	_it_should("env is not empty after register",
		p_env != NULL);

	test_cleanup(p_base);
	return NULL;
}

static char *test_ffi_ptr_call(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_eval_make(NULL, NULL);
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

static char *test_ffi_ptr_call_eight(void)
{
	x_obj_t *p_base, *p_args, *p_result;
	x_int_t i;

	p_base = x_eval_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (ptr-call fptr 1 2 3 4 5 6 7 8) -> 8, the last argument */
	p_args = NULL;
	for (i = 8; i >= 1; i--) {
		p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mkint(p_base, i), p_args);
	}
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkptr(p_base, (void *)test_ffi_long_eighth), p_args));
	p_result = x_prim_ptr_call(p_base, p_args);
	_it_should("ptr-call: the eighth argument arrives",
		x_intval(p_result) == 8);

	test_cleanup(p_base);
	return NULL;
}

static char *test_ffi_ptr_set_word(void)
{
	x_obj_t *p_base, *p_args, *p_result;
	unsigned char mem[32];
	long val;

	p_base = x_eval_make(NULL, NULL);
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

	p_base = x_eval_make(NULL, NULL);
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
	_run_test(test_ffi_int_ptr_convert);
	_run_test(test_ffi_ptr_set_ref);
	_run_test(test_ffi_string_ptr_convert);
	_run_test(test_ffi_register);
	_run_test(test_ffi_ptr_call);
	_run_test(test_ffi_ptr_call_eight);
	_run_test(test_ffi_ptr_set_word);
	_run_test(test_ffi_ptr_call_str_arg);

	return NULL;
}
