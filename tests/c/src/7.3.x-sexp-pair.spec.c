/*
 * # Unit Tests: *x-sexp/pair*
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
#include "src/x-token/sexp/pair.c"

x_obj_t *x_token_read(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_eval(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }

#define STUB_X_OBJ_OBJ
#define STUB_X_STR
#define STUB_X_TYPE_PRIM
#define STUB_X_TOKEN_DISPLAY
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

static int x_token_write_call_count = 0;
x_obj_t *x_token_write(x_obj_t *p_base, x_obj_t *p_obj)
{
	char s[16];
	int fd = x_base_isset(p_base) ? x_atomint(x_firstobj(x_base_field_fileout(p_base))) : STDOUT_FILENO;

	sprintf(s, "<%d>", ++x_token_write_call_count);
	x_sys_write(fd, s, strlen(s));

	return p_base;
}


/*
 * ## Test Runners
 */
static char *run_tests() {

	return NULL;
}
