/*
 * # Unit Tests: *x-prim/pred*
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
#include "src/x-prim/pred.c"

/* Stubs for primitives not under test. */
x_obj_t *x_prim_core_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_arith_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_string_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_io_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_type_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_ffi_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_callcc_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_binding_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_closure_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_control_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_quote_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }

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

static char *test_pred_eq(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_obj;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Same object -> t */
	p_obj = x_mksatom(p_base, (x_int_t)42);
	p_args = x_mkspair(p_base, NULL,
		x_mkspair(p_base, p_obj,
		x_mkspair(p_base, p_obj, NULL)));
	p_result = x_prim_eq(p_base, p_args);
	_it_should("eq? same object returns t",
		p_result == x_base_field_true(p_base));

	/* Different objects -> #f */
	p_args = x_mkspair(p_base, NULL,
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)1),
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)2), NULL)));
	p_result = x_prim_eq(p_base, p_args);
	_it_should("eq? different objects returns #f",
		p_result == x_base_field_false(p_base));

	/* nil == nil -> t */
	p_args = x_mkspair(p_base, NULL,
		x_mkspair(p_base, NULL,
		x_mkspair(p_base, NULL, NULL)));
	p_result = x_prim_eq(p_base, p_args);
	_it_should("eq? nil nil returns t",
		p_result == x_base_field_true(p_base));

	test_cleanup(p_base);
	return NULL;
}

static char *test_pred_numeq(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (= 5 5) -> t */
	p_args = x_mkspair(p_base, NULL,
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)5),
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)5), NULL)));
	p_result = x_prim_numeq(p_base, p_args);
	_it_should("(= 5 5) returns t",
		p_result == x_base_field_true(p_base));

	/* (= 5 3) -> #f */
	p_args = x_mkspair(p_base, NULL,
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)5),
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)3), NULL)));
	p_result = x_prim_numeq(p_base, p_args);
	_it_should("(= 5 3) returns #f",
		p_result == x_base_field_false(p_base));

	test_cleanup(p_base);
	return NULL;
}

static char *test_pred_lt(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (< 3 5) -> t */
	p_args = x_mkspair(p_base, NULL,
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)3),
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)5), NULL)));
	p_result = x_prim_lt(p_base, p_args);
	_it_should("(< 3 5) returns t",
		p_result == x_base_field_true(p_base));

	/* (< 5 3) -> #f */
	p_args = x_mkspair(p_base, NULL,
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)5),
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)3), NULL)));
	p_result = x_prim_lt(p_base, p_args);
	_it_should("(< 5 3) returns #f",
		p_result == x_base_field_false(p_base));

	/* (< 5 5) -> #f */
	p_args = x_mkspair(p_base, NULL,
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)5),
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)5), NULL)));
	p_result = x_prim_lt(p_base, p_args);
	_it_should("(< 5 5) returns #f",
		p_result == x_base_field_false(p_base));

	test_cleanup(p_base);
	return NULL;
}

static char *test_pred_char_to_integer(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (char->integer #\A) -> 65 */
	p_args = x_mkspair(p_base, NULL,
		x_mkspair(p_base, x_mkchar(p_base, 'A'), NULL));
	p_result = x_prim_char_to_integer(p_base, p_args);
	_it_should("(char->integer A) = 65",
		x_intval(p_result) == 65);

	test_cleanup(p_base);
	return NULL;
}

static char *test_pred_integer_to_char(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (integer->char 65) -> #\A */
	p_args = x_mkspair(p_base, NULL,
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)65), NULL));
	p_result = x_prim_integer_to_char(p_base, p_args);
	_it_should("(integer->char 65) = A",
		x_charval(p_result) == 'A');

	test_cleanup(p_base);
	return NULL;
}

static char *run_tests() {
	_run_test(test_pred_eq);
	_run_test(test_pred_numeq);
	_run_test(test_pred_lt);
	_run_test(test_pred_char_to_integer);
	_run_test(test_pred_integer_to_char);

	return NULL;
}
