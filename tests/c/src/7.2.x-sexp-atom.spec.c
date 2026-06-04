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
static char *test_sexp_atom_write(void)
{
	x_obj_t *p_atom, *p_pair, *p_args;
	char buffer[4096], *expected;

	helper_file_buffer_ptr[TEST_HELPER_FILE_STDOUT] = buffer;

	helper_file_reset();

	p_atom = x_mksatom(NULL, X_OBJ_FLAG_NONE, NULL);
	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_atom, NULL);

	x_sexp_atom_write(NULL, p_args);

	expected = "#<"X_TYPE_ATOM_SYMBOL":0x0>";
	_it_should("write atom s-exp to stdout", 0 == strncmp(expected, buffer, strlen(expected)));

	x_sys_free(p_args);
	x_sys_free(p_atom);


	helper_file_reset();
	memset(buffer, 0, 4096);

	p_pair = x_mkspair(NULL, X_OBJ_FLAG_NONE, NULL, NULL);
	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_pair, NULL);

	x_sexp_atom_write(NULL, p_args);

	expected = "#<"X_TYPE_PAIR_SYMBOL":0x0>";
	_it_should("write pair as atom s-exp to stdout", 0 == strncmp(expected, buffer, strlen(expected)));

	x_sys_free(p_args);
	x_sys_free(p_pair);

	return NULL;
}

static char *run_tests() {
	_run_test(test_sexp_atom_write);

	return NULL;
}
