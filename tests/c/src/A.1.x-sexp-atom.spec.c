/*
 * # Unit Tests: *x-sexp/atom*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-lib.c"
#include "src/x-obj.c"
#include "ext/x-expr/src/x.c"
#include "src/x-alist.c"
#include "src/x-base.c"
#include "src/x-sexp/atom.c"

x_obj_t *x_token_read(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_eval(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }

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


/*
 * ## Test Runners
 */
static char *test_sexp_atom_write(void)
{
	x_obj_t *p_atom, *p_pair, *p_args;
	char buffer[4096], *expected;

	helper_file_buffer_ptr[TEST_HELPER_FILE_STDOUT] = buffer;

	helper_file_reset();

	p_atom = x_mksatom(NULL, NULL);
	p_args = x_mkspair(NULL, p_atom, NULL);

	x_sexp_atom_write(NULL, p_args);

	expected = "#<"X_TYPE_ATOM_NAME":0x0>";
	_it_should("write atom s-exp to stdout", 0 == strncmp(expected, buffer, strlen(expected)));

	x_sys_free(p_args);
	x_sys_free(p_atom);


	helper_file_reset();
	memset(buffer, 0, 4096);

	p_pair = x_mkspair(NULL, NULL, NULL);
	p_args = x_mkspair(NULL, p_pair, NULL);

	x_sexp_atom_write(NULL, p_args);

	expected = "#<"X_TYPE_PAIR_NAME":0x0>";
	_it_should("write pair as atom s-exp to stdout", 0 == strncmp(expected, buffer, strlen(expected)));

	x_sys_free(p_args);
	x_sys_free(p_pair);

	return NULL;
}

static char *run_tests() {
	_run_test(test_sexp_atom_write);

	return NULL;
}
