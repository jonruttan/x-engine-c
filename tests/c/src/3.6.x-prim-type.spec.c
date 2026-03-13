/*
 * # Unit Tests: *x-prim/type*
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
#include "src/x-prim/type.c"

/* Stubs for primitives not under test. */
x_obj_t *x_prim_core_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_arith_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_pred_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_string_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_io_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_ffi_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }

#include "ext/x-expr/tests/src/helper-system-functions.c"


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


/*
 * ## Test Runners
 */

static char *test_type_typep(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_int;
	x_obj_t *p_int_handle;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Create an int and get its type handle */
	p_int = x_mkint(p_base, (x_int_t)42);
	p_int_handle = x_type_field_name(x_obj_type(p_int));

	/* Bind both to env for eval */
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "myint"), p_int));
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "inthandle"), p_int_handle));

	/* (type? myint inthandle) -> t */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "myint"),
		x_mkspair(p_base, x_mksymbol(p_base, "inthandle"), NULL));
	p_result = x_prim_typep(p_base, p_args);
	_it_should("type? matches correct type",
		p_result == x_base_field_true(p_base));

	/* (type? nil inthandle) -> nil */
	p_args = x_mkspair(p_base, NULL,
		x_mkspair(p_base, x_mksymbol(p_base, "inthandle"), NULL));
	p_result = x_prim_typep(p_base, p_args);
	_it_should("type? nil returns nil",
		p_result == NULL);

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_type_of(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_int;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_int = x_mkint(p_base, (x_int_t)42);
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "myint"), p_int));

	/* (type-of myint) -> int handle */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "myint"), NULL);
	p_result = x_prim_type_of(p_base, p_args);
	_it_should("type-of returns type handle",
		p_result != NULL);
	_it_should("type-of matches int type name",
		p_result == x_type_field_name(x_obj_type(p_int)));

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_type_name(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_int;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_int = x_mkint(p_base, (x_int_t)42);
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "myint"), p_int));

	/* (type-name myint) -> "int" */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "myint"), NULL);
	p_result = x_prim_type_name(p_base, p_args);
	_it_should("type-name returns string",
		p_result != NULL);
	_it_should("type-name of int is 'INTEGER'",
		x_lib_strcmp(x_strval(p_result), X_TYPE_INT_NAME) == 0);

	/* (type-name nil) -> nil */
	p_args = x_mkspair(p_base, NULL, NULL);
	p_result = x_prim_type_name(p_base, p_args);
	_it_should("type-name of nil returns nil",
		p_result == NULL);

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_make_instance(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_int;
	x_obj_t *p_int_handle;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Get the int type handle */
	p_int = x_mkint(p_base, (x_int_t)0);
	p_int_handle = x_type_field_name(x_obj_type(p_int));

	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "inthandle"), p_int_handle));

	/* (make-instance inthandle 42) -> int-typed instance with data 42 */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "inthandle"),
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)42), NULL));
	p_result = x_prim_make_instance(p_base, p_args);
	_it_should("make-instance returns an object",
		p_result != NULL);
	_it_should("instance has correct type",
		x_type_field_name(x_obj_type(p_result)) == p_int_handle);

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_make_token_base(void)
{
	x_obj_t *p_base, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_result = x_prim_make_token_base(p_base, NULL);
	_it_should("make-token-base returns a base",
		p_result != NULL);
	_it_should("token-base is not parent",
		p_result != p_base);

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_token_discard(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);

	p_args = x_mksatom(p_base, (x_int_t)42);
	p_result = x_prim_token_discard(p_base, p_args);
	_it_should("token-discard returns p_args",
		p_result == p_args);

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_register(void)
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

static char *run_tests() {
	_run_test(test_type_typep);
	_run_test(test_type_type_of);
	_run_test(test_type_type_name);
	_run_test(test_type_make_instance);
	_run_test(test_type_make_token_base);
	_run_test(test_type_token_discard);
	_run_test(test_type_register);

	return NULL;
}
