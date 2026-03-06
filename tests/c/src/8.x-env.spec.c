/*
 * # Unit Tests: *x-env*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

/* We need the GC structures for cleanup. */
#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "src/x-sys.c"
#include "src/x-lib.c"
#include "src/x.c"
#include "src/x-obj.c"
#include "src/x-alist.c"
#include "src/x-base.c"
#include "src/x-token.c"
#include "src/x-type.c"
#include "src/x-type/iter.c"
#include "src/x-eval.c"
#include "src/x-type/list.c"
#include "src/x-sexp/list.c"
#include "src/x-type/prim.c"
#include "src/x-type/buffer.c"
#include "src/x-type/char.c"
#include "src/x-sexp/char.c"
#include "src/x-type/str.c"
#include "src/x-sexp/str.c"
#include "src/x-type/symbol.c"
#include "src/x-sexp/symbol.c"
#include "src/x-env.c"
#include "src/x-sexp.c"
#include "src/x-sexp/atom.c"
#include "src/x-sexp/pair.c"


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

void test_cleanup(x_obj_t *p_base)
{
	x_obj_t *p_gc = p_base, *p_tmp;

	while (p_gc) {
		p_tmp = x_obj_gc(p_gc);
		x_sys_free(p_gc);
		p_gc = p_tmp;
	}
}

/*
 * ## Test Runners
 */

#define nil			p_base
#define pair(X,Y)	(x_mkspair(p_base, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, (X)))

static char *test_env_assoc(void)
{
	x_obj_t *p_base, *p_symbol, *p_args, *p_obj, *p_ret;
	x_char_t buffer[256];

	helper_alloc_reset();

	/* With p_base object */
	p_base = x_base_make(NULL, NULL);
	p_symbol = x_mksymbol(p_base, "SYMBOL");
	p_args = x_mkspair(p_base, p_symbol, p_base);


	helper_file_buffer_ptr[TEST_HELPER_FILE_STDERR] = buffer;
	helper_file_reset();

	p_ret = x_env_assoc(p_base, p_args);
	*file_buffer_ptr[TEST_HELPER_FILE_STDERR][TEST_HELPER_FILE_WRITE] = 0;
	_it_should("trigger an error and return p_base",
		p_base == p_ret
		&& X_SYS_EXIT_FAILURE == x_sys_exit_status
		&& 0 == strcmp(
			"*** ERROR: Unbound SYMBOL 'SYMBOL",
			buffer
		)
	);

	test_cleanup(p_base);


	helper_file_buffer_ptr[TEST_HELPER_FILE_STDERR] = NULL;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_symbol = x_mksymbol(p_base, "SYMBOL");
	p_obj = x_mksatom(p_base, p_base);
	p_args = x_mkspair(p_base, p_symbol, p_obj);

	p_ret = x_base_env_alist_extend(p_base, p_args);


	p_ret = x_env_assoc(p_base, p_args);
	_it_should("return p_obj",
		p_obj == p_ret
	);


	test_cleanup(p_base);

	return NULL;
}

static char *run_tests() {
	_run_test(test_env_assoc);

	return NULL;
}
