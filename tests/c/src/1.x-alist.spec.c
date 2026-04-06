/*
 * # Unit Tests: *x-alist*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "ext/x-expr/tests/src/test-helper-system.c"

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-lib.c"
#include "ext/x-expr/src/x.c"
#include "ext/x-expr/src/x-obj.c"
#include "src/x-alist.c"

#define STUB_X_BASE_ERROR
#include "helper-stubs.c"


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
static char *test_alist_extend(void)
{
	x_obj_t *p_base, *p_alist, *p_atoms[3], *p_args;

	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);

	p_atoms[0] = x_mksatom(p_base, X_OBJ_FLAG_NONE, 1);
	p_atoms[1] = x_mksatom(p_base, X_OBJ_FLAG_NONE, 2);
	p_atoms[2] = x_mksatom(p_base, X_OBJ_FLAG_NONE, 3);


	p_alist = NULL;

	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_atoms[0], p_atoms[1]),
		p_alist
	);
	p_alist = x_alist_extend(p_base, p_args);
	_it_should("extend alist with (#<0x1:0x1> . #<0x1:0x2>)",
		x_obj_type_isspair(p_alist)
		&& x_obj_type_isspair(x_firstobj(p_alist))
		&& x_firstobj(x_firstobj(p_alist)) == p_atoms[0]
		&& x_restobj(x_firstobj(p_alist)) == p_atoms[1]
	);


	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_atoms[1], p_atoms[2]),
		p_alist
	);
	p_alist = x_alist_extend(p_base, p_args);
	_it_should("extend alist with (#<0x1:0x2> . #<0x1:0x3>)",
		x_obj_type_isspair(p_alist)
		&& x_obj_type_isspair(x_firstobj(p_alist))
		&& x_firstobj(x_firstobj(p_alist)) == p_atoms[1]
		&& x_restobj(x_firstobj(p_alist)) == p_atoms[2]
		&& x_obj_type_isspair(x_firstobj(x_restobj(p_alist)))
		&& x_firstobj(x_firstobj(x_restobj(p_alist))) == p_atoms[0]
		&& x_restobj(x_firstobj(x_restobj(p_alist))) == p_atoms[1]
	);


	return NULL;
}

static char *test_alist_assoc(void)
{
	x_obj_t *p_base, *p_obj, *p_alist, *p_atoms[3], *p_args;


	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);

	p_atoms[0] = x_mksatom(p_base, X_OBJ_FLAG_NONE, 1);
	p_atoms[1] = x_mksatom(p_base, X_OBJ_FLAG_NONE, 2);
	p_atoms[2] = x_mksatom(p_base, X_OBJ_FLAG_NONE, 3);

	p_alist = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_atoms[0], p_atoms[0]), x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_atoms[1], p_atoms[1]), NULL));

	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_atoms[0], x_mkspair(p_base, X_OBJ_FLAG_NONE, p_alist, NULL));
	p_obj = x_alist_assoc(p_base, p_args);
	_it_should("assoc 1 with (#<0x1:0x1> . #<0x1:0x1>)",
		x_obj_type_isspair(p_obj)
		/*&& x_firstobj(p_obj) == p_atoms[0]
		&& x_restobj(p_obj) == p_atoms[0]*/
		&& x_firstobj(x_firstobj(p_obj)) == x_firstobj(p_atoms[0])
		&& x_firstobj(x_restobj(p_obj)) == x_firstobj(p_atoms[0])
	);

	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_atoms[1], x_mkspair(p_base, X_OBJ_FLAG_NONE, p_alist, NULL));
	p_obj = x_alist_assoc(p_base, p_args);
	_it_should("assoc 2 with (#<0x1:0x2> . #<0x1:0x2>)",
		x_obj_type_isspair(p_obj)
		&& x_firstobj(p_obj) == p_atoms[1]
		&& x_restobj(p_obj) == p_atoms[1]
	);


	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_atoms[2], x_mkspair(p_base, X_OBJ_FLAG_NONE, p_alist, NULL));
	p_obj = x_alist_assoc(p_base, p_args);
	_it_should("assoc 3 with nil", x_obj_isnil(p_base, p_obj));


	/* Note: assoc with nil key is undefined with nil=NULL (would deref NULL) */


	test_cleanup(p_base);

	return NULL;
}

static char *run_tests() {
	_run_test(test_alist_extend);
	_run_test(test_alist_assoc);

	return NULL;
}
