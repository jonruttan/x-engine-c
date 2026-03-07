/*
 * # Unit Tests: *x-alist*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-lib.c"
#include "ext/x-expr/src/x.c"
#include "src/x-obj.c"
#include "src/x-alist.c"

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
static char *test_alist_extend(void)
{
	x_obj_t *p_base, *p_alist, *p_atoms[3], *p_args;

	p_base = x_mksatom(NULL, 0);

	p_atoms[0] = x_mksatom(p_base, 1);
	p_atoms[1] = x_mksatom(p_base, 2);
	p_atoms[2] = x_mksatom(p_base, 3);


	p_alist = p_base;

	p_args = x_mkspair(p_base,
		x_mkspair(p_base, p_atoms[0], p_atoms[1]),
		p_alist
	);
	p_alist = x_alist_extend(p_base, p_args);
	_it_should("extend alist with (#<0x1:0x1> . #<0x1:0x2>)",
		x_obj_type_isspair(p_alist)
		&& x_obj_type_isspair(x_car(p_alist))
		&& x_caar(p_alist) == p_atoms[0]
		&& x_cdar(p_alist) == p_atoms[1]
	);


	p_args = x_mkspair(p_base,
		x_mkspair(p_base, p_atoms[1], p_atoms[2]),
		p_alist
	);
	p_alist = x_alist_extend(p_base, p_args);
	_it_should("extend alist with (#<0x1:0x2> . #<0x1:0x3>)",
		x_obj_type_isspair(p_alist)
		&& x_obj_type_isspair(x_car(p_alist))
		&& x_caar(p_alist) == p_atoms[1]
		&& x_cdar(p_alist) == p_atoms[2]
		&& x_obj_type_isspair(x_cadr(p_alist))
		&& x_caadr(p_alist) == p_atoms[0]
		&& x_cdadr(p_alist) == p_atoms[1]
	);


	return NULL;
}

static char *test_alist_assoc(void)
{
	x_obj_t *p_base, *p_obj, *p_alist, *p_atoms[3], *p_args;


	p_base = x_mksatom(NULL, 0);

	p_atoms[0] = x_mksatom(p_base, 1);
	p_atoms[1] = x_mksatom(p_base, 2);
	p_atoms[2] = x_mksatom(p_base, 3);

	p_alist = x_cons(p_base,
		x_cons(p_base, p_atoms[0], p_atoms[0]), x_cons(p_base,
		x_cons(p_base, p_atoms[1], p_atoms[1]), p_base));

	p_args = x_cons(p_base, p_atoms[0], x_cons(p_base, p_alist, p_base));
	p_obj = x_alist_assoc(p_base, p_args);
	_it_should("assoc 1 with (#<0x1:0x1> . #<0x1:0x1>)",
		x_obj_type_isspair(p_obj)
		/*&& x_firstobj(p_obj) == p_atoms[0]
		&& x_restobj(p_obj) == p_atoms[0]*/
		&& x_caar(p_obj) == x_car(p_atoms[0])
		&& x_cadr(p_obj) == x_car(p_atoms[0])
	);

	p_args = x_cons(p_base, p_atoms[1], x_cons(p_base, p_alist, p_base));
	p_obj = x_alist_assoc(p_base, p_args);
	_it_should("assoc 2 with (#<0x1:0x2> . #<0x1:0x2>)",
		x_obj_type_isspair(p_obj)
		&& x_firstobj(p_obj) == p_atoms[1]
		&& x_restobj(p_obj) == p_atoms[1]
	);


	p_args = x_cons(p_base, p_atoms[2], x_cons(p_base, p_alist, p_base));
	p_obj = x_alist_assoc(p_base, p_args);
	_it_should("assoc 3 with nil", x_obj_isnil(p_base, p_obj));


	p_args = x_cons(p_base, p_base, x_cons(p_base, p_alist, p_base));
	p_obj = x_alist_assoc(p_base, p_args);
	_it_should("assoc nil with nil", x_obj_isnil(p_base, p_obj));


	test_cleanup(p_base);

	return NULL;
}

static char *run_tests() {
	_run_test(test_alist_extend);
	_run_test(test_alist_assoc);

	return NULL;
}
