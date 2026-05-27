/*
 * # Unit Tests: *x-prim/arith*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "ext/x-expr/tests/src/test-helper-system.c"

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-lib.c"
#include "ext/x-expr/src/x-obj.c"
#include "src/x-obj/obj.c"
#include "src/x-obj/prim.c"
#include "ext/x-expr/src/x.c"
#include "src/x-alist.c"
#include "ext/x-expr/src/x-base.c"
#include "src/x-interp.c"
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
#include "src/x-prim/arith.c"

/* Stubs for primitives not under test. */
x_obj_t *x_prim_core_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_pred_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_string_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_io_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_type_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_ffi_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_callcc_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_binding_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_closure_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_control_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_quote_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }



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

static char *test_arith_sum(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_interp_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (+ 10 3) -> 13 */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)10),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)3), NULL)));
	p_result = x_prim_sum(p_base, p_args);
	_it_should("(+ 10 3) = 13", x_intval(p_result) == 13);

	/* (+ 0 0) -> 0 */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)0),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)0), NULL)));
	p_result = x_prim_sum(p_base, p_args);
	_it_should("(+ 0 0) = 0", x_intval(p_result) == 0);

	/* (+ -5 3) -> -2 */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)-5),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)3), NULL)));
	p_result = x_prim_sum(p_base, p_args);
	_it_should("(+ -5 3) = -2", x_intval(p_result) == -2);

	test_cleanup(p_base);
	return NULL;
}

static char *test_arith_sub(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_interp_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (- 10 3) -> 7 */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)10),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)3), NULL)));
	p_result = x_prim_sub(p_base, p_args);
	_it_should("(- 10 3) = 7", x_intval(p_result) == 7);

	/* (- 5) -> -5 (unary negate) */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)5), NULL));
	p_result = x_prim_sub(p_base, p_args);
	_it_should("(- 5) = -5", x_intval(p_result) == -5);

	/* (- 0) -> 0 */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)0), NULL));
	p_result = x_prim_sub(p_base, p_args);
	_it_should("(- 0) = 0", x_intval(p_result) == 0);

	test_cleanup(p_base);
	return NULL;
}

static char *test_arith_prod(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_interp_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (* 4 5) -> 20 */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)4),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)5), NULL)));
	p_result = x_prim_prod(p_base, p_args);
	_it_should("(* 4 5) = 20", x_intval(p_result) == 20);

	/* (* 0 100) -> 0 */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)0),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)100), NULL)));
	p_result = x_prim_prod(p_base, p_args);
	_it_should("(* 0 100) = 0", x_intval(p_result) == 0);

	test_cleanup(p_base);
	return NULL;
}

static char *test_arith_div(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_interp_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (/ 10 3) -> 3 (integer division) */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)10),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)3), NULL)));
	p_result = x_prim_div(p_base, p_args);
	_it_should("(/ 10 3) = 3", x_intval(p_result) == 3);

	/* (/ 20 5) -> 4 */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)20),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)5), NULL)));
	p_result = x_prim_div(p_base, p_args);
	_it_should("(/ 20 5) = 4", x_intval(p_result) == 4);

	test_cleanup(p_base);
	return NULL;
}

static char *test_arith_bitwise(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_interp_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (~ 0) -> -1 */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)0), NULL));
	p_result = x_prim_bitnot(p_base, p_args);
	_it_should("(~ 0) = -1", x_intval(p_result) == -1);

	/* (& 0xFF 0x0F) -> 0x0F */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)0xFF),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)0x0F), NULL)));
	p_result = x_prim_bitand(p_base, p_args);
	_it_should("(& 0xFF 0x0F) = 0x0F", x_intval(p_result) == 0x0F);

	/* (| 0xF0 0x0F) -> 0xFF */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)0xF0),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)0x0F), NULL)));
	p_result = x_prim_bitor(p_base, p_args);
	_it_should("(| 0xF0 0x0F) = 0xFF", x_intval(p_result) == 0xFF);

	/* (^ 0xFF 0x0F) -> 0xF0 */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)0xFF),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)0x0F), NULL)));
	p_result = x_prim_bitxor(p_base, p_args);
	_it_should("(^ 0xFF 0x0F) = 0xF0", x_intval(p_result) == 0xF0);

	test_cleanup(p_base);
	return NULL;
}

static char *test_arith_shift(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_interp_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (<< 1 4) -> 16 */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)1),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)4), NULL)));
	p_result = x_prim_shl(p_base, p_args);
	_it_should("(<< 1 4) = 16", x_intval(p_result) == 16);

	/* (>> 16 4) -> 1 */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)16),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)4), NULL)));
	p_result = x_prim_shr(p_base, p_args);
	_it_should("(>> 16 4) = 1", x_intval(p_result) == 1);

	test_cleanup(p_base);
	return NULL;
}

static char *test_arith_register(void)
{
	x_obj_t *p_base, *p_env;

	p_base = x_interp_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_env = x_firstobj(x_interp_field_env_alist(p_base));
	_it_should("env is not empty after register",
		p_env != NULL);

	test_cleanup(p_base);
	return NULL;
}

static char *run_tests() {
	_run_test(test_arith_sum);
	_run_test(test_arith_sub);
	_run_test(test_arith_prod);
	_run_test(test_arith_div);
	_run_test(test_arith_bitwise);
	_run_test(test_arith_shift);
	_run_test(test_arith_register);

	return NULL;
}
