/*
 * # Unit Tests: *x-type/procedure*
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

/* Stubs for primitives not under test. */
x_obj_t *x_prim_core_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_arith_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
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

static char *test_procedure_struct(void)
{
	x_obj_t *p_base, *p_type;

	p_base = x_base_ts_make(NULL, NULL);
	p_type = x_type_procedure_struct(p_base, NULL);

	_it_should("return a type struct",
		p_type != NULL);
	_it_should("have name",
		x_type_field_name(p_type) != NULL);
	_it_should("have make fn",
		x_type_field_make(p_type) != NULL);
	_it_should("have call fn",
		x_type_field_call(p_type) != NULL);
	_it_should("have write fn",
		x_type_field_write(p_type) != NULL);

	test_cleanup(p_base);

	return NULL;
}

static char *test_procedure_register(void)
{
	x_obj_t *p_base, *p_type1, *p_type2;

	p_base = x_base_ts_make(NULL, NULL);
	p_type1 = x_type_procedure_register(p_base, NULL);

	_it_should("return a type struct",
		p_type1 != NULL);

	p_type2 = x_type_procedure_register(p_base, NULL);
	_it_should("return same cached type",
		p_type1 == p_type2);

	test_cleanup(p_base);

	return NULL;
}

static char *test_procedure_make(void)
{
	x_obj_t *p_base, *p_proc;

	p_base = x_base_ts_make(NULL, NULL);

	p_proc = x_make_procedure(p_base, X_OBJ_FLAG_NONE,
		x_mksatom(p_base, X_OBJ_FLAG_NONE, "params"),
		x_mksatom(p_base, X_OBJ_FLAG_NONE, "body"),
		x_mksatom(p_base, X_OBJ_FLAG_NONE, "env"),
		NULL);

	_it_should("create a procedure",
		p_proc != NULL);
	_it_should("have params",
		x_procparams(p_proc) != NULL);
	_it_should("have body",
		x_procbody(p_proc) != NULL);
	_it_should("have env",
		x_procenv(p_proc) != NULL);

	test_cleanup(p_base);

	return NULL;
}

static char *test_procedure_call(void)
{
	x_obj_t *p_base, *p_proc;
	x_obj_t *p_params, *p_body, *p_env, *p_args;

	p_base = x_base_ts_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (fn (x) x) — takes one param, body returns x.
	 * Procedure evaluates args before binding, unlike operative. */
	p_params = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "x"), NULL);
	p_body = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "x"), NULL);
	p_env = x_base_field_env_alist(p_base);

	p_proc = x_make_procedure(p_base, X_OBJ_FLAG_NONE,
		p_params, p_body, p_env,
		x_base_field_env_global_tree(p_base));

	/* Call: (proc 42) — procedure evaluates args then binds. */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_proc,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 42), NULL));

	x_type_procedure_call(p_base, p_args);

	/* body_eval_tco sets tco_expr for tail form */
	_it_should("set tco_expr for tail call",
		x_base_field_tco_expr(p_base) != NULL);

	test_cleanup(p_base);

	return NULL;
}

static char *test_procedure_call_wrapped(void)
{
	x_obj_t *p_base, *p_op, *p_proc;
	x_obj_t *p_body, *p_args;

	p_base = x_base_ts_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Create an operative that returns 77 */
	p_body = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 77), NULL);
	p_op = x_make_operative(p_base, X_OBJ_FLAG_NONE,
		NULL, NULL, p_body, x_base_field_env_alist(p_base));

	/* Wrap the operative in a procedure (applicative wrapper). */
	p_proc = x_make_procedure(p_base, X_OBJ_FLAG_WRAP,
		NULL, NULL, p_op, NULL);

	/* Call: (proc) — wrapped combiner dispatches to underlying */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_proc, NULL);

	x_type_procedure_call(p_base, p_args);

	/* The wrapped path calls x_obj_prim_call on the combiner,
	 * which calls operative_call, which sets tco_expr = 77 */
	_it_should("wrapped combiner dispatches to operative",
		x_base_field_tco_expr(p_base) != NULL
		&& x_atomint(x_base_field_tco_expr(p_base)) == 77);

	test_cleanup(p_base);

	return NULL;
}

static char *test_procedure_write(void)
{
	x_obj_t *p_base, *p_proc, *p_args, *p_ret;
	x_char_t s[64];

	p_base = x_base_ts_make(NULL, NULL);

	p_proc = x_make_procedure(p_base, X_OBJ_FLAG_NONE,
		NULL, NULL, NULL, NULL);

	helper_file_buffer_ptr[TEST_HELPER_FILE_STDOUT] = s;
	helper_file_buffer_length[TEST_HELPER_FILE_STDOUT] = 64;
	helper_file_reset();

	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_proc, NULL);
	p_ret = x_type_procedure_write(p_base, p_args);

	_it_should("return the procedure",
		p_ret == p_proc);
	_it_should("write output",
		s[0] != '\0');

	test_cleanup(p_base);

	return NULL;
}

static char *run_tests() {
	_run_test(test_procedure_struct);
	_run_test(test_procedure_register);
	_run_test(test_procedure_make);
	_run_test(test_procedure_call);
	_run_test(test_procedure_call_wrapped);
	_run_test(test_procedure_write);

	return NULL;
}
