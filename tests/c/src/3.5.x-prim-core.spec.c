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
#include "src/x-syntax/binding.c"
#include "src/x-syntax/closure.c"
#include "src/x-syntax/control.c"
#include "src/x-syntax/quote.c"

/* Stubs for primitives not under test. */
x_obj_t *x_prim_arith_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_pred_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_string_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_io_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_type_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_ffi_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_callcc_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }

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
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj, NULL));
	p_result = x_prim_quote(p_base, p_args);
	_it_should("(lit x) returns x unevaluated",
		p_result == p_obj);

	/* (lit ()) -> nil */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, NULL));
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

	p_a = x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)1);
	p_b = x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)2);

	/* (pair 1 2) -> (1 . 2) */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_a,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_b, NULL)));
	p_result = x_prim_pair(p_base, p_args);
	_it_should("(pair 1 2) creates a pair",
		p_result != NULL);
	_it_should("first of pair is 1",
		x_firstobj(p_result) == p_a);
	_it_should("rest of pair is 2",
		x_restobj(p_result) == p_b);

	/* Test first/rest prims with satom (self-evaluating) */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_a, NULL));
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
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "x"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)42), NULL)));
	p_result = x_prim_define(p_base, p_args);
	_it_should("(def x 42) returns 42",
		x_atomint(p_result) == 42);

	/* Lookup x -> 42 */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mksymbol(p_base, "x"), NULL);
	p_result = x_eval_arg(p_base, x_mksymbol(p_base, "x"));
	_it_should("x resolves to 42",
		x_atomint(p_result) == 42);

	/* (set x 99) -> mutate binding */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "x"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)99), NULL)));
	p_result = x_prim_set(p_base, p_args);
	_it_should("(set x 99) returns 99",
		x_atomint(p_result) == 99);

	/* x now resolves to 99 */
	p_result = x_eval_arg(p_base, x_mksymbol(p_base, "x"));
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
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "x"), NULL),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "x"), NULL)));
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
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "args"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "args"), NULL))));
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
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "myval"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)42), NULL)));
	x_prim_define(p_base, p_args);

	/* eval first evaluates its arg: myval -> 42, then sets tco_expr = 42 */
	{
		x_obj_t *p_sym = x_mksymbol(p_base, "myval");
		p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, p_sym, NULL));
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
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "y"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)77), NULL)));
	x_prim_define(p_base, p_args);

	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "y"), NULL));
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

	/* (match (nil 1) (#t 42)) -> tco_expr = 42 */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
		/* first clause: (nil 1) -- test is nil, skip */
		x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)1), NULL)),
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
			/* second clause: (#t 42) -- test is #t, match */
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_base_field_true(p_base),
				x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)42), NULL)),
			NULL)));
	x_prim_match(p_base, p_args);
	_it_should("match sets tco_expr to matched body",
		x_base_field_tco_expr(p_base) != NULL
		&& x_atomint(x_base_field_tco_expr(p_base)) == 42);

	/* (match (nil 1)) -> no match, tco_expr unchanged from above, returns nil */
	x_base_field_tco_expr(p_base) = NULL;
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)1), NULL)),
		NULL));
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
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
		/* clause: (e 99) */
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "e"),
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)99), NULL)),
		/* body: (42) */
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)42), NULL)));
	p_result = x_prim_guard(p_base, p_args);
	_it_should("guard returns body result when no error",
		x_atomint(p_result) == 42);
	_it_should("guard pops handler",
		x_base_field_error_handler(p_base) == NULL);

	test_cleanup(p_base);
	return NULL;
}

/* test_core_first_rest_int: moved to X (lib/x-core.x) */
/* test_core_set_first_rest: moved to X (lib/x-core.x) */

static char *test_core_wrap_unwrap(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_op;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Create operative, bind as 'myop' */
	p_op = x_mkop(p_base,
		x_mksymbol(p_base, "args"), NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)1), NULL),
		x_base_field_env_alist(p_base));

	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "myop"), p_op));

	/* (wrap myop) -> applicative */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "myop"), NULL));
	p_result = x_prim_wrap(p_base, p_args);
	_it_should("wrap creates a procedure",
		p_result != NULL);

	/* Bind wrapped, then (unwrap it) -> gets underlying combiner back */
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "wrapped"), p_result));

	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "wrapped"), NULL));
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
	p_body_form = x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)99);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)1),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_body_form, NULL)));
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

	p_expr = x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)42);
	p_env = x_base_field_env_alist(p_base);

	/* Bind expr and env */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "te"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_expr, NULL)));
	x_prim_define(p_base, p_args);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "tenv"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_env, NULL)));
	x_prim_define(p_base, p_args);

	/* (tail-eval te tenv) -> sets tco_expr and env */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "te"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "tenv"), NULL)));
	x_prim_tail_eval(p_base, p_args);
	_it_should("tail-eval sets tco_expr",
		x_base_field_tco_expr(p_base) == p_expr);
	_it_should("tail-eval sets env",
		x_base_field_env_alist(p_base) == p_env);

	test_cleanup(p_base);
	return NULL;
}

static char *test_core_rest(void)
{
	x_obj_t *p_base, *p_args, *p_result;
	x_obj_t *p_a, *p_b, *p_pair;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_a = x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)1);
	p_b = x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)2);
	p_pair = x_mklist(p_base, p_a, p_b);

	/* Bind pair so prim_rest can eval it */
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "pp"), p_pair));

	/* (rest pp) -> p_b */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "pp"), NULL));
	p_result = x_prim_rest(p_base, p_args);
	_it_should("rest returns cdr of pair", p_result == p_b);

	test_cleanup(p_base);
	return NULL;
}

/* test_core_rest_int: moved to X (lib/x-core.x) */
/* test_core_set_first_rest_int: moved to X (lib/x-core.x) */

static char *test_core_eval_with_env(void)
{
	x_obj_t *p_base, *p_args, *p_result;
	x_obj_t *p_env, *p_sym, *p_quote_form;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Create a custom env with binding: z -> 123 */
	p_sym = x_mksymbol(p_base, "z");
	p_env = x_base_field_env_alist(p_base);
	p_env = x_mklist(p_base,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_sym, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)123)),
		p_env);
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "myenv"), p_env));

	/* Build (lit z) — evaluates to the symbol z */
	p_quote_form = x_mklist(p_base,
		x_mksymbol(p_base, "lit"),
		x_mklist(p_base, p_sym, NULL));

	/* (eval (lit z) myenv) -> eval first evals (lit z) -> z,
	 * then evals z in myenv -> 123 */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_quote_form,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "myenv"), NULL)));
	p_result = x_prim_eval(p_base, p_args);
	_it_should("eval with env returns result from that env",
		x_atomint(p_result) == 123);

	/* Verify original env is restored */
	_it_should("eval with env restores original env",
		x_base_field_env_alist(p_base) != p_env);

	test_cleanup(p_base);
	return NULL;
}

static char *test_core_apply(void)
{
	x_obj_t *p_base, *p_args;
	x_obj_t *p_fn, *p_arglist;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Create (fn (x) x) — identity function */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "x"), NULL),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "x"), NULL)));
	p_fn = x_prim_closure(p_base, p_args);

	/* Bind fn and arg list */
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "idfn"), p_fn));
	p_arglist = x_mklist(p_base,
		x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)42), NULL);
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "args"), p_arglist));

	/* (apply idfn args) — single trailing list, procedure path.
	 * apply + procedure uses TCO: sets tco_expr = x, returns NULL. */
	x_base_field_tco_expr(p_base) = NULL;
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "idfn"),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "args"), NULL)));
	x_prim_apply(p_base, p_args);
	_it_should("apply single-arg sets tco_expr for TCO",
		! x_obj_isnil(p_base, x_base_field_tco_expr(p_base)));

	/* Test apply with prefix + tail: create (fn (a b) a) */
	{
		x_obj_t *p_fn2, *p_tl;

		p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
			x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "a"),
				x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "b"), NULL)),
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "a"), NULL)));
		p_fn2 = x_prim_closure(p_base, p_args);
		x_base_env_alist_extend(p_base,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "fn2"), p_fn2));

		/* Tail list: bind '(200) to tl */
		p_tl = x_mklist(p_base,
			x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)200), NULL);
		x_base_env_alist_extend(p_base,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "tl"), p_tl));

		/* (apply fn2 100 tl) — prefix 100, tail (200): 2-arg splice path.
		 * evlis on (100 tl) -> (100 (200)), walk to second-to-last (100),
		 * splice: rest(100-node) = first(rest(100-node)) = (200)
		 * -> p_vals = (100 200) */
		x_base_field_tco_expr(p_base) = NULL;
		p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "fn2"),
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)100),
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "tl"), NULL))));
		x_prim_apply(p_base, p_args);
		_it_should("apply prefix+tail sets tco_expr",
			! x_obj_isnil(p_base, x_base_field_tco_expr(p_base)));
	}

	/* Test apply with 3 prefix args + tail: walk loop (line 124) */
	{
		x_obj_t *p_fn3, *p_tl;

		/* Create (fn (a b c) a) */
		p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
			x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "a"),
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "b"),
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "c"), NULL))),
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "a"), NULL)));
		p_fn3 = x_prim_closure(p_base, p_args);
		x_base_env_alist_extend(p_base,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "fn3"), p_fn3));

		p_tl = x_mklist(p_base,
			x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)300), NULL);
		x_base_env_alist_extend(p_base,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "tl3"), p_tl));

		/* (apply fn3 100 200 tl3) — 3 args total: 2 prefix + tail */
		x_base_field_tco_expr(p_base) = NULL;
		p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "fn3"),
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)100),
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)200),
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "tl3"), NULL)))));
		x_prim_apply(p_base, p_args);
		_it_should("apply 3-prefix+tail sets tco_expr",
			! x_obj_isnil(p_base, x_base_field_tco_expr(p_base)));
	}

	test_cleanup(p_base);
	_buffer_index = -1;

	/* Test apply with operative (non-procedure path, lines 143-147). */
	{
		x_obj_t *p_op, *p_tl;

		p_base = x_base_make(NULL, NULL);
		x_prim_register(p_base, NULL);

		/* Create (op (x) x) — identity operative */
		p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
			x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "x"), NULL),
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "x"), NULL)));
		p_op = x_prim_operative(p_base, p_args);
		x_base_env_alist_extend(p_base,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "myop"), p_op));

		p_tl = x_mklist(p_base,
			x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)42), NULL);
		x_base_env_alist_extend(p_base,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "optl"), p_tl));

		/* (apply myop optl) */
		p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "myop"),
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "optl"), NULL)));
		x_prim_apply(p_base, p_args);
		_it_should("apply with operative dispatches to type", 1);

		test_cleanup(p_base);
	}

	return NULL;
}

static char *test_core_error_guard_catch(void)
{
	x_obj_t *p_base, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (guard (e e) (error 42)) -> guard catches error, e = 42, returns e */
	/* Build: guard clause = (e e), body = (error 42) */
	/* error is a prim, so body needs to be a call: we need (error 42) as an
	 * expression that eval will dispatch. But x_eval_body evaluates
	 * each form. We can call error directly inside guard. */

	/* Simpler: call x_prim_guard with handler body that returns the error
	 * variable, and a body that calls x_prim_error directly. */

	/* Actually, the test body needs to trigger an error via x_prim_error.
	 * Since body forms are evaluated by x_eval_body, we need the body
	 * to be an expression that when evaluated calls error. The simplest:
	 * use a C-level call approach.
	 *
	 * guard body = list of forms, each form evaluated by x_eval_arg.
	 * A self-evaluating atom won't trigger error. We need to construct
	 * a form like (error 42) that evaluates to an error call. */

	/* Build: (guard (e e) (error 42))
	 * clause: (e . (e . nil)) = (e e)
	 * body: ((error 42) . nil)
	 * (error 42) = a list: (error-symbol . (42 . nil)) */
	{
		x_obj_t *p_error_sym, *p_error_form, *p_clause, *p_guard_args;

		p_error_sym = x_mksymbol(p_base, "error");
		p_error_form = x_mklist(p_base,
			p_error_sym,
			x_mklist(p_base,
				x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)42), NULL));

		p_clause = x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mksymbol(p_base, "e"),
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "e"), NULL));

		p_guard_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, p_clause,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, p_error_form, NULL)));

		p_result = x_prim_guard(p_base, p_guard_args);
		_it_should("guard catches error and runs handler",
			p_result != NULL);
		_it_should("guard handler receives error value",
			x_atomint(p_result) == 42);
		_it_should("guard pops handler after catch",
			x_base_field_error_handler(p_base) == NULL);
	}

	test_cleanup(p_base);
	return NULL;
}

static char *test_core_set_unbound(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (set unbound-name 42) should trigger error path.
	 * Wrap in guard to catch it. */
	{
		jmp_buf jmp;
		x_obj_t *p_handler;

		p_handler = x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mkptr(p_base, &jmp),
			x_mkspair(p_base, X_OBJ_FLAG_NONE,
				x_base_field_env_alist(p_base),
				x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, NULL)));
		x_base_field_error_handler(p_base) = p_handler;

		if (setjmp(jmp) == 0) {
			p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
				x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksymbol(p_base, "nonexistent"),
				x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, (x_int_t)42), NULL)));
			x_prim_set(p_base, p_args);
			/* Should not reach here */
			_it_should("set unbound should have jumped", 0);
		} else {
			/* Error was caught */
			p_result = x_error_handler_error(p_handler);
			_it_should("set unbound triggers error",
				p_result != NULL);
		}

		x_base_field_error_handler(p_base) = NULL;
	}

	test_cleanup(p_base);
	return NULL;
}

static int test_error_hook_called;
static void test_error_hook(x_obj_t *p_base, x_char_t *msg, x_obj_t *p_obj)
{
	test_error_hook_called = 1;
}

static char *test_core_error_no_handler_str(void)
{
	x_obj_t *p_base, *p_args, *p_ret;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* No guard handler; string error message */
	x_obj_hook_error = test_error_hook;
	test_error_hook_called = 0;
	x_base_field_error_handler(p_base) = NULL;
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "test error"), NULL));
	p_ret = x_prim_error(p_base, p_args);
	_it_should("error with string msg calls error hook",
		test_error_hook_called == 1);
	_it_should("error with no handler returns NULL",
		p_ret == NULL);

	/* No guard handler; non-string error message */
	test_error_hook_called = 0;
	x_base_field_error_handler(p_base) = NULL;
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 42), NULL));
	p_ret = x_prim_error(p_base, p_args);
	_it_should("error with non-string msg calls error hook",
		test_error_hook_called == 1);
	_it_should("error with non-string msg returns NULL",
		p_ret == NULL);

	x_obj_hook_error = x_base_error;

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
	/* test_core_first_rest_int: moved to X (lib/x-core.x) */
	/* test_core_set_first_rest: moved to X (lib/x-core.x) */
	_run_test(test_core_wrap_unwrap);
	_run_test(test_core_base);
	_run_test(test_core_seq);
	_run_test(test_core_tail_eval);
	_run_test(test_core_rest);
	/* test_core_rest_int: moved to X (lib/x-core.x) */
	/* test_core_set_first_rest_int: moved to X (lib/x-core.x) */
	_run_test(test_core_eval_with_env);
	_run_test(test_core_apply);
	_run_test(test_core_error_guard_catch);
	_run_test(test_core_set_unbound);
	_run_test(test_core_error_no_handler_str);

	return NULL;
}
