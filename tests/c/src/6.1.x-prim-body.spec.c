/*
 * # Unit Tests: *x-prim body-eval helpers*
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

/* Stubs for primitives not under test. */
x_obj_t *x_prim_core_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_arith_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_pred_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_string_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_io_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_type_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_ffi_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }

#include "helper-system-functions.c"


/*
 * ## Test Overhead
 */

static void _setup(void)
{
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
 * ## x_prim_body_eval
 */
static char *test_body_eval(void)
{
	x_obj_t *p_base, *p_body, *p_result;

	/* nil body returns NULL */
	p_base = x_base_make(NULL, NULL);
	p_result = x_prim_body_eval(p_base, NULL);
	_it_should("return NULL for nil body", p_result == NULL);
	test_cleanup(p_base);

	/* single self-evaluating form returns it */
	p_base = x_base_make(NULL, NULL);
	p_body = x_mkspair(p_base, x_mksatom(p_base, 42), NULL);
	p_result = x_prim_body_eval(p_base, p_body);
	_it_should("return single form's value",
		p_result != NULL && x_atomint(p_result) == 42);
	test_cleanup(p_base);

	/* multi-form body returns last result */
	p_base = x_base_make(NULL, NULL);
	p_body = x_mkspair(p_base, x_mksatom(p_base, 10),
		x_mkspair(p_base, x_mksatom(p_base, 20),
		x_mkspair(p_base, x_mksatom(p_base, 30), NULL)));
	p_result = x_prim_body_eval(p_base, p_body);
	_it_should("return last form's value in multi-form body",
		p_result != NULL && x_atomint(p_result) == 30);
	test_cleanup(p_base);

	return NULL;
}

/*
 * ## x_prim_body_eval_tco
 */
static char *test_body_eval_tco(void)
{
	x_obj_t *p_base, *p_body, *p_saved_env, *p_result;

	/* nil body restores env and returns NULL */
	p_base = x_base_make(NULL, NULL);
	p_saved_env = x_mkspair(p_base, x_mksatom(p_base, 99), NULL);
	x_base_field_env_alist(p_base) = NULL;
	p_result = x_prim_body_eval_tco(p_base, NULL, p_saved_env);
	_it_should("restore env for nil body",
		x_base_field_env_alist(p_base) == p_saved_env);
	_it_should("return NULL for nil body (tco)", p_result == NULL);
	test_cleanup(p_base);

	/* single form sets tco_expr and tco_env */
	p_base = x_base_make(NULL, NULL);
	p_saved_env = x_mkspair(p_base, x_mksatom(p_base, 88), NULL);
	p_body = x_mkspair(p_base, x_mksatom(p_base, 42), NULL);
	x_base_field_tco_env(p_base) = NULL;
	p_result = x_prim_body_eval_tco(p_base, p_body, p_saved_env);
	_it_should("set tco_expr for single form",
		x_base_field_tco_expr(p_base) != NULL
		&& x_atomint(x_base_field_tco_expr(p_base)) == 42);
	_it_should("set tco_env to saved_env",
		x_base_field_tco_env(p_base) == p_saved_env);
	_it_should("return NULL when setting tco_expr", p_result == NULL);
	/* Clean up tco state so it doesn't interfere with cleanup. */
	x_base_field_tco_expr(p_base) = NULL;
	x_base_field_tco_env(p_base) = NULL;
	test_cleanup(p_base);

	/* nil last form: restores env, no TCO */
	p_base = x_base_make(NULL, NULL);
	p_saved_env = x_mkspair(p_base, x_mksatom(p_base, 77), NULL);
	p_body = x_mkspair(p_base, NULL, NULL);
	p_result = x_prim_body_eval_tco(p_base, p_body, p_saved_env);
	_it_should("restore env for nil last form",
		x_base_field_env_alist(p_base) == p_saved_env);
	_it_should("return NULL for nil last form", p_result == NULL);
	_it_should("not set tco_expr for nil last form",
		x_obj_isnil(p_base, x_base_field_tco_expr(p_base)));
	test_cleanup(p_base);

	/* tco_env idempotent: doesn't overwrite if already set */
	p_base = x_base_make(NULL, NULL);
	{
		x_obj_t *p_existing_tco_env = x_mkspair(p_base,
			x_mksatom(p_base, 66), NULL);
		p_saved_env = x_mkspair(p_base, x_mksatom(p_base, 55), NULL);
		p_body = x_mkspair(p_base, x_mksatom(p_base, 42), NULL);
		x_base_field_tco_env(p_base) = p_existing_tco_env;
		p_result = x_prim_body_eval_tco(p_base, p_body, p_saved_env);
		_it_should("not overwrite existing tco_env",
			x_base_field_tco_env(p_base) == p_existing_tco_env);
		x_base_field_tco_expr(p_base) = NULL;
		x_base_field_tco_env(p_base) = NULL;
	}
	test_cleanup(p_base);

	return NULL;
}

/*
 * ## x_prim_body_eval_tco_simple
 */
static char *test_body_eval_tco_simple(void)
{
	x_obj_t *p_base, *p_body, *p_result;

	/* nil body returns NULL */
	p_base = x_base_make(NULL, NULL);
	p_result = x_prim_body_eval_tco_simple(p_base, NULL);
	_it_should("return NULL for nil body (simple)", p_result == NULL);
	test_cleanup(p_base);

	/* single form sets tco_expr, returns NULL */
	p_base = x_base_make(NULL, NULL);
	p_body = x_mkspair(p_base, x_mksatom(p_base, 42), NULL);
	x_base_field_tco_env(p_base) = NULL;
	p_result = x_prim_body_eval_tco_simple(p_base, p_body);
	_it_should("set tco_expr for single form (simple)",
		x_base_field_tco_expr(p_base) != NULL
		&& x_atomint(x_base_field_tco_expr(p_base)) == 42);
	_it_should("return NULL when setting tco_expr (simple)",
		p_result == NULL);
	_it_should("not set tco_env (simple)",
		x_obj_isnil(p_base, x_base_field_tco_env(p_base)));
	x_base_field_tco_expr(p_base) = NULL;
	test_cleanup(p_base);

	/* multi-form: evals all but last, sets tco_expr for last */
	p_base = x_base_make(NULL, NULL);
	p_body = x_mkspair(p_base, x_mksatom(p_base, 10),
		x_mkspair(p_base, x_mksatom(p_base, 20), NULL));
	x_base_field_tco_env(p_base) = NULL;
	p_result = x_prim_body_eval_tco_simple(p_base, p_body);
	_it_should("set tco_expr to last form in multi-form (simple)",
		x_base_field_tco_expr(p_base) != NULL
		&& x_atomint(x_base_field_tco_expr(p_base)) == 20);
	_it_should("return NULL for multi-form (simple)",
		p_result == NULL);
	x_base_field_tco_expr(p_base) = NULL;
	test_cleanup(p_base);

	return NULL;
}

/*
 * ## x_prim_tco_trampoline
 */
static char *test_tco_trampoline(void)
{
	x_obj_t *p_base, *p_result, *p_initial;

	/* no tco_expr: returns p_result unchanged */
	p_base = x_base_make(NULL, NULL);
	p_initial = x_mksatom(p_base, 42);
	x_base_field_tco_expr(p_base) = NULL;
	p_result = x_prim_tco_trampoline(p_base, p_initial);
	_it_should("return p_result when no tco_expr",
		p_result == p_initial);
	test_cleanup(p_base);

	/* single tco_expr: evaluates it (self-eval atom) */
	p_base = x_base_make(NULL, NULL);
	x_base_field_tco_expr(p_base) = x_mksatom(p_base, 99);
	x_base_field_tco_env(p_base) = NULL;
	p_result = x_prim_tco_trampoline(p_base, NULL);
	_it_should("evaluate single tco_expr",
		p_result != NULL && x_atomint(p_result) == 99);
	test_cleanup(p_base);

	/* tco_env restore */
	p_base = x_base_make(NULL, NULL);
	{
		x_obj_t *p_tco_env = x_mkspair(p_base,
			x_mksatom(p_base, 77), NULL);
		x_base_field_tco_expr(p_base) = x_mksatom(p_base, 55);
		x_base_field_tco_env(p_base) = p_tco_env;
		x_base_field_env_alist(p_base) = NULL;
		p_result = x_prim_tco_trampoline(p_base, NULL);
		_it_should("restore env from tco_env",
			x_base_field_env_alist(p_base) == p_tco_env);
	}
	test_cleanup(p_base);

	return NULL;
}

static char *run_tests() {
	_run_test(test_body_eval);
	_run_test(test_body_eval_tco);
	_run_test(test_body_eval_tco_simple);
	_run_test(test_tco_trampoline);

	return NULL;
}
