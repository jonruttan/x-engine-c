/*
 * # Unit Tests: *x-sexp*
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
#define STUB_X_EVAL
#include "src/x-eval.c"
#define STUB_X_EVAL
#define STUB_X_OBJ_OBJ
#define STUB_X_STR
#define STUB_X_TYPE_PRIM
#include "helper-stubs.c"

x_obj_t *x_type_heap_mark(x_obj_t *p_base, x_obj_t *p_obj, x_obj_flag_t flags) { return NULL; }
void x_type_heap_free(x_obj_t *p_base, x_obj_t *p_obj) {}


x_obj_t *x_token_read(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }


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

static int x_sexp_atom_write_call_count = 0;
x_obj_t *x_sexp_atom_write(x_obj_t *p_base, x_obj_t *p_args)
{
	++x_sexp_atom_write_call_count;

	return p_args;
}

static int x_sexp_pair_write_call_count = 0;
x_obj_t *x_sexp_pair_write(x_obj_t *p_base, x_obj_t *p_args)
{
	++x_sexp_pair_write_call_count;

	return p_args;
}

static int x_type_write_call_count = 0;
x_obj_t *x_type_write(x_obj_t *p_base, x_obj_t *p_args)
{
	++x_type_write_call_count;

	return p_args;
}

x_obj_t *x_token_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args);

	if (x_obj_type_issatom(p_obj)) {
		return x_sexp_atom_write(p_base, p_args);
	}

	if (x_obj_type_isspair(p_obj)) {
		return x_sexp_pair_write(p_base, p_args);
	}

	if ( ! x_obj_isnil(p_base, x_obj_type(p_obj))) {
		return x_type_write(p_base, p_args);
	}

	return p_base;
}


/*
 * ## Test Runners
 */
static char *run_tests() {

	return NULL;
}
