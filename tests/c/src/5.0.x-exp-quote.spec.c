/*
 * # Unit Tests: *x-type/list*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

/* We need the GC structures for cleanup. */
#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-lib.c"
#include "ext/x-expr/src/x.c"
#include "ext/x-expr/src/x-obj.c"
#include "src/x-alist.c"
#include "ext/x-expr/src/x-base.c"
#include "src/x-base.c"
#include "src/x-eval.c"
#include "ext/x-expr/src/x-heap.c"
#include "src/x-type.c"
#include "src/x-type/buffer.c"
#include "src/x-type/iter.c"
#include "src/x-type/list.c"
#include "src/x-type/prim.c"
#include "src/x-token/sexp/atom.c"
#include "src/x-token/sexp/pair.c"
#include "src/x-token/sexp/list.c"
#include "src/x-token.c"
#include "src/x-exp/quote.c"

#define STUB_X_PRIM
#define STUB_X_PROCEDURE
#define STUB_X_OPERATIVE
#define STUB_X_HEAP
#define STUB_X_OBJ_OBJ
#define STUB_X_STR
#define STUB_X_PRIM_SHADOW
#define STUB_X_PROCEDURE_APPLY
#include "helper-stubs.c"

#include "ext/x-expr/tests/src/test-helper-system.c"

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
 * ## Test Runners
 */

#define nil     p_base
#define pair(X,Y) (x_mkspair(p_base, X_OBJ_FLAG_NONE, (X), (Y)))
#define atom(X)   (x_mksatom(p_base, X_OBJ_FLAG_NONE, (X)))


static char *test_exp_quote(void)
{
	x_obj_t *p_base, *p_prim, *p_list, *p_args, *p_ret;

	helper_alloc_reset();

	/* Make a simple base to help with cleanup. */
	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, NULL);
	p_prim = x_mkprim(p_base, x_exp_quote);
	p_list = x_mklist(p_base, p_prim,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 0),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 1),
		p_base)));
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkspair(p_base, X_OBJ_FLAG_NONE, p_list, p_base), p_base);
	p_ret = x_eval(p_base, p_args);
	_it_should("return the unevaluated list",
		x_01(p_list) == p_ret
	);

	test_cleanup(p_base);

	return NULL;
}

static char *run_tests() {
	_run_test(test_exp_quote);

	return NULL;
}
