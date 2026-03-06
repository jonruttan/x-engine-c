/*
 * # Unit Tests: *x-obj*
 */

#include "test-runner.h"

/* No Garbage Collection structures. */
#ifdef X_GC
#undef X_GC
#endif /* X_GC */

#include "src/x-sys.c"
#include "src/x-lib.c"
#include "src/x.c"
#include "src/x-obj.c"

#include "helper-system-functions.c"


/*
 * ## Test Overhead
 */
static void setup(void)
{
	helper_set_alloc(MEM_GUARANTEED);
}

static void teardown(void)
{
}


/*
 * ## Test Runners
 */
static char *test_obj_alloc(void)
{
	x_obj_t *p_obj[3];

	p_obj[0] = x_obj_alloc(NULL, x_type_atom_obj, 2, 3);
	_it_should("make a new object", NULL != p_obj[0]);
	_it_should("set the new object's type to the value given",
		x_type_atom_obj == x_obj_type(p_obj[0])
	);
	_it_should("set the new object's flags to the value given",
		2 == x_obj_flags(p_obj[0])
	);

	p_obj[1] = x_obj_alloc(p_obj[0], x_type_atom_obj, 3, 4);
	_it_should("make a new object", NULL != p_obj[1]);
	_it_should("set the new object's type to the value given",
		x_type_atom_obj == x_obj_type(p_obj[1])
	);
	_it_should("set the new object's flags to the value given",
		3 == x_obj_flags(p_obj[1])
	);

	p_obj[2] = x_obj_alloc(p_obj[0], x_type_atom_obj, 4, 5);
	_it_should("make a new object", NULL != p_obj[2]);
	_it_should("set the new object's type to the value given",
		x_type_atom_obj == x_obj_type(p_obj[2])
	);
	_it_should("set the new object's flags to the value given",
		4 == x_obj_flags(p_obj[2])
	);

	x_sys_free(p_obj[2]);
	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);

	return NULL;
}

static char *test_obj_make(void)
{
	x_obj_t *p_obj;
	enum x_obj_flag_enum flags = rand();

	p_obj = x_obj_make(NULL, x_type_atom_obj, flags, 3, (void *)4, (void *)5);
	_it_should("make a new object", p_obj != NULL);
	_it_should("set the new object's type to the value given",
		x_type_atom_obj == x_obj_type(p_obj));
	_it_should("set the new object's flags to the value given",
		flags == x_obj_flags(p_obj)
	);
	_it_should("set the new object's first element",
		(void *)4 == x_firstobj(p_obj)
	);
	_it_should("set the new object's second element",
		(void *)5 == x_restobj(p_obj)
	);

	x_sys_free(p_obj);

	return NULL;
}

static char *run_tests() {
	_run_test(test_obj_alloc);
	_run_test(test_obj_make);

	return NULL;
}
