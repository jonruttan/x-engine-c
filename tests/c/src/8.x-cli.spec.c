/*
 * # Unit Tests: *x-cli*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"
#include "x-type/buffer.h"

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
#include "src/x-prim/arith.c"
#include "src/x-prim/pred.c"
#include "src/x-prim/string.c"
#include "src/x-prim/io.c"
#include "src/x-prim/type.c"
#include "src/x-prim/ffi.c"
#include "src/x-exp/quote.c"

#include "src/x-cli.c"

#include "ext/x-expr/tests/src/helper-system-functions.c"


/*
 * ## Test Overhead
 */

static void _setup(void)
{
	_buffer_index = -1;
	helper_set_alloc(MEM_SYSTEM);
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

static char *test_cli_init(void)
{
	x_obj_t *p_base;
	x_char_t buffer[256];

	p_base = init(NULL, buffer);
	_it_should("init returns non-NULL base",
		p_base != NULL);
	_it_should("init sets buffer on base",
		x_base_field_buffer(p_base) != NULL);
	_it_should("init registers primitives (eval is bound)",
		!x_obj_isnil(p_base, x_base_field_env_alist(p_base)));

	test_cleanup(p_base);
	return NULL;
}

static char *run_tests() {
	_run_test(test_cli_init);

	return NULL;
}
