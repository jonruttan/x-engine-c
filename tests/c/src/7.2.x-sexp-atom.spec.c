/*
 * # Unit Tests: *x-sexp/atom*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

#include "ext/x-expr/tests/src/test-helper-system.c"

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-lib.c"
#include "ext/x-expr/src/x-obj.c"
#include "ext/x-expr/src/x.c"
#include "src/x-alist.c"
#include "ext/x-expr/src/x-base.c"
#define X_EVAL_OWN
#include "src/x-eval.c"
#include "src/x-token/sexp/atom.c"

x_obj_t *x_token_read(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_eval(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }

#define STUB_X_OBJ_OBJ
#define STUB_X_STR
#define STUB_X_TYPE_PRIM
#include "helper-stubs.c"

x_obj_t *x_type_heap_mark(x_obj_t *p_base, x_obj_t *p_obj, x_obj_flag_t flags) { return NULL; }
void x_type_heap_free(x_obj_t *p_base, x_obj_t *p_obj) {}



/*
 * ## Test Overhead
 */
static void _setup(void)
{
	helper_set_alloc(MEM_GUARANTEED);
	helper_sys_funcs.exit = mock_exit;
	helper_sys_funcs.malloc = helper_malloc;
	helper_sys_funcs.free = helper_free;
}

static void _teardown(void)
{
}


/*
 * ## Test Runners
 */
static char *run_tests() {

	return NULL;
}
