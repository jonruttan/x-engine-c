/*
 * # Unit Tests: *x-prim/core*
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
#include "src/x-prim/core.c"

/* Stubs for primitives not under test. */
x_obj_t *x_prim_arith_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_pred_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_string_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_io_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_type_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
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

static char *test_core_quote(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_obj;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (lit x) -> x unevaluated */
	p_obj = x_mksymbol(p_base, "foo");
	p_args = x_mkspair(p_base, p_obj, NULL);
	p_result = x_prim_quote(p_base, p_args);
	_it_should("(lit x) returns x unevaluated",
		p_result == p_obj);

	/* (lit ()) -> nil */
	p_args = x_mkspair(p_base, NULL, NULL);
	p_result = x_prim_quote(p_base, p_args);
	_it_should("(lit ()) returns nil",
		p_result == NULL);

	test_cleanup(p_base);
	return NULL;
}

static char *test_core_pair_first_rest(void)
{
	x_obj_t *p_base, *p_args, *p_result;
	x_obj_t *p_a, *p_b;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_a = x_mksatom(p_base, (x_int_t)1);
	p_b = x_mksatom(p_base, (x_int_t)2);

	/* (pair 1 2) -> (1 . 2) */
	p_args = x_mkspair(p_base, p_a,
		x_mkspair(p_base, p_b, NULL));
	p_result = x_prim_pair(p_base, p_args);
	_it_should("(pair 1 2) creates a pair",
		p_result != NULL);
	_it_should("first of pair is 1",
		x_firstobj(p_result) == p_a);
	_it_should("rest of pair is 2",
		x_restobj(p_result) == p_b);

	/* Test first/rest prims with satom (self-evaluating) */
	p_args = x_mkspair(p_base, p_a, NULL);
	_it_should("(first 1) extracts first slot",
		x_prim_first(p_base, p_args) != NULL);

	test_cleanup(p_base);
	return NULL;
}

static char *test_core_def_set(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (def x 42) -> bind x to 42 */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "x"),
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)42), NULL));
	p_result = x_prim_define(p_base, p_args);
	_it_should("(def x 42) returns 42",
		x_atomint(p_result) == 42);

	/* Lookup x -> 42 */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "x"), NULL);
	p_result = x_prim_eval_arg(p_base, x_mksymbol(p_base, "x"));
	_it_should("x resolves to 42",
		x_atomint(p_result) == 42);

	/* (set x 99) -> mutate binding */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "x"),
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)99), NULL));
	p_result = x_prim_set(p_base, p_args);
	_it_should("(set x 99) returns 99",
		x_atomint(p_result) == 99);

	/* x now resolves to 99 */
	p_result = x_prim_eval_arg(p_base, x_mksymbol(p_base, "x"));
	_it_should("x now resolves to 99",
		x_atomint(p_result) == 99);

	test_cleanup(p_base);
	return NULL;
}

static char *test_core_fn(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (fn (x) x) -> creates a procedure */
	p_args = x_mkspair(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "x"), NULL),
		x_mkspair(p_base, x_mksymbol(p_base, "x"), NULL));
	p_result = x_prim_closure(p_base, p_args);
	_it_should("(fn (x) x) creates a procedure",
		p_result != NULL);
	_it_should("procedure has params",
		x_procparams(p_result) != NULL);
	_it_should("procedure captures env",
		x_procenv(p_result) != NULL);

	test_cleanup(p_base);
	return NULL;
}

static char *test_core_op(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (op args #f args) -> creates an operative */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "args"),
		x_mkspair(p_base, NULL,
		x_mkspair(p_base, x_mksymbol(p_base, "args"), NULL)));
	p_result = x_prim_operative(p_base, p_args);
	_it_should("(op args #f args) creates an operative",
		p_result != NULL);

	test_cleanup(p_base);
	return NULL;
}

static char *test_core_eval(void)
{
	x_obj_t *p_base, *p_args;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (def myval 42), then (eval myval) -> sets tco_expr to 42 */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "myval"),
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)42), NULL));
	x_prim_define(p_base, p_args);

	/* eval first evaluates its arg: myval -> 42, then sets tco_expr = 42 */
	{
		x_obj_t *p_sym = x_mksymbol(p_base, "myval");
		p_args = x_mkspair(p_base, p_sym, NULL);
		x_prim_eval(p_base, p_args);
		_it_should("eval without env sets tco_expr",
			x_base_field_tco_expr(p_base) != NULL);
	}

	test_cleanup(p_base);
	return NULL;
}

static char *test_core_eval_immediate(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (def y 77), then (eval! 'y) -> 77 */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "y"),
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)77), NULL));
	x_prim_define(p_base, p_args);

	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "y"), NULL);
	p_result = x_prim_eval_immediate(p_base, p_args);
	_it_should("eval! returns immediate result",
		x_atomint(p_result) == 77);

	test_cleanup(p_base);
	return NULL;
}

static char *test_core_match(void)
{
	x_obj_t *p_base, *p_args;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (match (nil 1) (t 42)) -> tco_expr = 42 */
	p_args = x_mkspair(p_base,
		/* first clause: (nil 1) -- test is nil, skip */
		x_mkspair(p_base, NULL,
			x_mkspair(p_base, x_mksatom(p_base, (x_int_t)1), NULL)),
		x_mkspair(p_base,
			/* second clause: (t 42) -- test is t, match */
			x_mkspair(p_base, x_base_field_true(p_base),
				x_mkspair(p_base, x_mksatom(p_base, (x_int_t)42), NULL)),
			NULL));
	x_prim_match(p_base, p_args);
	_it_should("match sets tco_expr to matched body",
		x_base_field_tco_expr(p_base) != NULL
		&& x_atomint(x_base_field_tco_expr(p_base)) == 42);

	/* (match (nil 1)) -> no match, tco_expr unchanged from above, returns nil */
	x_base_field_tco_expr(p_base) = NULL;
	p_args = x_mkspair(p_base,
		x_mkspair(p_base, NULL,
			x_mkspair(p_base, x_mksatom(p_base, (x_int_t)1), NULL)),
		NULL);
	x_prim_match(p_base, p_args);
	_it_should("match with no truthy test leaves tco_expr nil",
		x_base_field_tco_expr(p_base) == NULL);

	test_cleanup(p_base);
	return NULL;
}

static char *test_core_guard(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (guard (e 99) 42) -> 42 (no error) */
	p_args = x_mkspair(p_base,
		/* clause: (e 99) */
		x_mkspair(p_base, x_mksymbol(p_base, "e"),
			x_mkspair(p_base, x_mksatom(p_base, (x_int_t)99), NULL)),
		/* body: (42) */
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)42), NULL));
	p_result = x_prim_guard(p_base, p_args);
	_it_should("guard returns body result when no error",
		x_atomint(p_result) == 42);
	_it_should("guard pops handler",
		x_base_field_error_handler(p_base) == NULL);

	test_cleanup(p_base);
	return NULL;
}

static char *test_core_rewrite(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_pair;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Create a pair (1 . 2), then rewrite to (3 . 4) */
	p_pair = x_mklist(p_base, x_mksatom(p_base, (x_int_t)1),
		x_mksatom(p_base, (x_int_t)2));

	/* Bind 'p' to the pair at C level (x_prim_define would eval the list) */
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "p"), p_pair));

	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "p"),
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)3),
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)4), NULL)));
	p_result = x_prim_rewrite(p_base, p_args);
	_it_should("rewrite returns the pair",
		p_result == p_pair);
	_it_should("rewrite set first to 3",
		x_atomint(x_firstobj(p_pair)) == 3);
	_it_should("rewrite set rest to 4",
		x_atomint(x_restobj(p_pair)) == 4);

	test_cleanup(p_base);
	return NULL;
}

static char *test_core_first_rest_int(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_obj;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Create satom with int value 42 */
	p_obj = x_mksatom(p_base, (x_int_t)42);

	p_args = x_mkspair(p_base, p_obj, NULL);
	p_result = x_prim_first_int(p_base, p_args);
	_it_should("first-int extracts car as integer",
		x_intval(p_result) == 42);

	test_cleanup(p_base);
	return NULL;
}

static char *test_core_set_first_rest(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_pair;
	x_obj_t *p_a, *p_b, *p_c;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_a = x_mksatom(p_base, (x_int_t)1);
	p_b = x_mksatom(p_base, (x_int_t)2);
	p_c = x_mksatom(p_base, (x_int_t)3);
	p_pair = x_mklist(p_base, p_a, p_b);

	/* Bind pair at C level (x_prim_define would eval the list) */
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "pp"), p_pair));

	/* (set-first pp 3) */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "pp"),
		x_mkspair(p_base, p_c, NULL));
	p_result = x_prim_set_first(p_base, p_args);
	_it_should("set-first returns pair",
		p_result == p_pair);
	_it_should("set-first modifies first",
		x_firstobj(p_pair) == p_c);

	/* (set-rest pp 3) */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "pp"),
		x_mkspair(p_base, p_c, NULL));
	p_result = x_prim_set_rest(p_base, p_args);
	_it_should("set-rest modifies rest",
		x_restobj(p_pair) == p_c);

	test_cleanup(p_base);
	return NULL;
}

static char *test_core_wrap_unwrap(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_op;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Create operative, bind as 'myop' */
	p_op = x_mkop(p_base,
		x_mksymbol(p_base, "args"), NULL,
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)1), NULL),
		x_base_field_env_alist(p_base));

	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "myop"), p_op));

	/* (wrap myop) -> applicative */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "myop"), NULL);
	p_result = x_prim_wrap(p_base, p_args);
	_it_should("wrap creates a procedure",
		p_result != NULL);

	/* Bind wrapped, then (unwrap it) -> gets underlying combiner back */
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "wrapped"), p_result));

	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "wrapped"), NULL);
	p_result = x_prim_unwrap(p_base, p_args);
	_it_should("unwrap returns underlying combiner",
		p_result == p_op);

	test_cleanup(p_base);
	return NULL;
}

static char *test_core_base(void)
{
	x_obj_t *p_base, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_result = x_prim_base(p_base, NULL);
	_it_should("(%base) returns p_base",
		p_result == p_base);

	test_cleanup(p_base);
	return NULL;
}

static char *test_core_seq(void)
{
	x_obj_t *p_base, *p_args, *p_body_form;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (%seq 1 2) -> evals first arg, sets tco_expr to second */
	p_body_form = x_mksatom(p_base, (x_int_t)99);
	p_args = x_mkspair(p_base,
		x_mksatom(p_base, (x_int_t)1),
		x_mkspair(p_base, p_body_form, NULL));
	x_prim_seq(p_base, p_args);
	_it_should("seq sets tco_expr to second arg",
		x_base_field_tco_expr(p_base) == p_body_form);

	test_cleanup(p_base);
	return NULL;
}

static char *test_core_tail_eval(void)
{
	x_obj_t *p_base, *p_args, *p_expr, *p_env;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_expr = x_mksatom(p_base, (x_int_t)42);
	p_env = x_base_field_env_alist(p_base);

	/* Bind expr and env */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "te"),
		x_mkspair(p_base, p_expr, NULL));
	x_prim_define(p_base, p_args);
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "tenv"),
		x_mkspair(p_base, p_env, NULL));
	x_prim_define(p_base, p_args);

	/* (tail-eval te tenv) -> sets tco_expr and env */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "te"),
		x_mkspair(p_base, x_mksymbol(p_base, "tenv"), NULL));
	x_prim_tail_eval(p_base, p_args);
	_it_should("tail-eval sets tco_expr",
		x_base_field_tco_expr(p_base) == p_expr);
	_it_should("tail-eval sets env",
		x_base_field_env_alist(p_base) == p_env);

	test_cleanup(p_base);
	return NULL;
}

static char *run_tests() {
	_run_test(test_core_quote);
	_run_test(test_core_pair_first_rest);
	_run_test(test_core_def_set);
	_run_test(test_core_fn);
	_run_test(test_core_op);
	_run_test(test_core_eval);
	_run_test(test_core_eval_immediate);
	_run_test(test_core_match);
	_run_test(test_core_guard);
	_run_test(test_core_rewrite);
	_run_test(test_core_first_rest_int);
	_run_test(test_core_set_first_rest);
	_run_test(test_core_wrap_unwrap);
	_run_test(test_core_base);
	_run_test(test_core_seq);
	_run_test(test_core_tail_eval);

	return NULL;
}
