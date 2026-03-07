/*
 * # Unit Tests: *x-sexp*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-lib.c"
#include "src/x-obj.c"
#include "ext/x-expr/src/x.c"
#include "src/x-alist.c"
#include "src/x-base.c"
#include "helper-system-functions.c"

x_obj_t *x_token_read(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }


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
static char *test_sexp_write(void)
{
	x_obj_t *p_atom, *p_pair, *p_obj, *p_type, *p_args, *p_ret;


	p_atom = x_mksatom(NULL, NULL);
	p_args = x_mkspair(NULL, p_atom, NULL);

	p_ret = x_token_write(NULL, p_args);

	_it_should("write atom s-exp to stdout",
		p_args == p_ret
		&& 1 == x_sexp_atom_write_call_count
		&& 0 == x_sexp_pair_write_call_count
		&& 0 == x_type_write_call_count
	);

	x_sys_free(p_args);
	x_sys_free(p_atom);


	p_pair = x_mkspair(NULL, NULL, NULL);
	p_args = x_mkspair(NULL, p_pair, NULL);

	p_ret = x_token_write(NULL, p_args);

	_it_should("write atom s-exp to stdout",
		p_args == p_ret
		&& 1 == x_sexp_atom_write_call_count
		&& 1 == x_sexp_pair_write_call_count
		&& 0 == x_type_write_call_count
	);

	x_sys_free(p_args);
	x_sys_free(p_pair);


	p_type = x_mksatom(NULL, NULL);
	p_obj = x_mksatom(NULL, NULL);
	x_obj_type(p_obj) = p_type;
	p_args = x_mkspair(NULL, p_obj, NULL);

	p_ret = x_token_write(NULL, p_args);

	_it_should("write atom s-exp to stdout",
		p_args == p_ret
		&& 1 == x_sexp_atom_write_call_count
		&& 1 == x_sexp_pair_write_call_count
		&& 1 == x_type_write_call_count
	);

	x_sys_free(p_args);
	x_sys_free(p_obj);
	x_sys_free(p_type);


	p_obj = x_mksatom(NULL, NULL);
	x_obj_type(p_obj) = NULL;
	p_args = x_mkspair(NULL, p_obj, NULL);

	p_ret = x_token_write(NULL, p_args);

	_it_should("write atom s-exp to stdout",
		NULL == p_ret
		&& 1 == x_sexp_atom_write_call_count
		&& 1 == x_sexp_pair_write_call_count
		&& 1 == x_type_write_call_count
	);

	x_sys_free(p_args);
	x_sys_free(p_obj);


	return NULL;
}

static char *run_tests() {
	_run_test(test_sexp_write);

	return NULL;
}
