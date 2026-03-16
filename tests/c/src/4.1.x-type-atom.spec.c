/*
 * # Unit Tests: *x-type/atom*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

/* We need the GC structures for cleanup. */
#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-lib.c"
#include "ext/x-expr/src/x.c"
#include "ext/x-expr/src/x-obj.c"
#include "src/x-alist.c"
#include "src/x-base.c"
#include "src/x-type.c"
#include "src/x-type/prim.c"
#include "src/x-type/atom.c"

#define STUB_X_PRIM
#define STUB_X_PROCEDURE
#define STUB_X_OPERATIVE
#define STUB_X_EVAL
#define STUB_X_TOKEN
#define STUB_X_HEAP
#define STUB_X_OBJ_OBJ
#define STUB_X_STR
#define STUB_X_INT
#define STUB_X_SYMBOL
#define STUB_X_PRIM_REGISTER
#include "helper-stubs.c"

#include "ext/x-expr/tests/src/helper-system-functions.c"

/*
 * ## Test Overhead
 */

static void _setup(void)
{
	helper_set_alloc(MEM_GUARANTEED);
	x_obj_hook_type_name = x_type_prim_type_name;
	x_obj_hook_units = x_type_prim_units;
	x_obj_hook_length = x_type_prim_length;
	x_obj_hook_error = x_base_error;
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

#define nil			p_base
#define pair(X,Y)	(x_mkspair(p_base, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, (X)))

static char *test_obj_type_isatom(void)
{
	x_obj_t *p_obj;

	helper_alloc_reset();

	p_obj = x_mkatom(NULL, 0);
	_it_should("return true when object is an atom",
		1 == x_obj_type_isatom(NULL, p_obj)
	);
	x_obj_free(p_obj);

	p_obj = x_mksatom(NULL, 0);
	_it_should("return true when object is a statically registered atom",
		1 == x_obj_type_isatom(NULL, p_obj)
	);
	x_obj_free(p_obj);

	p_obj = x_mkprim(NULL, 0);
	_it_should("return false when object is not an atom",
		0 == x_obj_type_isatom(NULL, p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}

static char *test_atomval(void)
{
	x_obj_t *p_obj;
	x_int_t i = rand();

	p_obj = x_mkatom(NULL, (void *)i);

	_it_should("return the Atom's value", i == x_atomint(p_obj));

	x_sys_free(p_obj);

	return NULL;
}

static char *test_mkatom(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t i = rand();

	p_obj = x_mkatom(NULL, (void *)i);
	_it_should("make an Atom object and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_isatom(NULL, p_obj)
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj)
		&& i == x_atomint(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkatom(p_base, (void *)i);
	_it_should("make an Atom object, attach it to the Base object, and set its value",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_isatom(NULL, p_obj)
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj)
		&& p_obj == x_obj_heap(p_base)
		&& i == x_atomint(p_obj)
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_mkfatom(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t i = rand();
	x_obj_flag_t flags = rand();

	p_obj = x_mkfatom(NULL, flags, (void *)i);
	_it_should("make an Atom object and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_isatom(NULL, p_obj)
		&& flags == (x_obj_flag_t)x_obj_flags(p_obj)
		&& i == x_atomint(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkfatom(p_base, flags, (void *)i);
	_it_should("make an Atom object, attach it to the Base object, and set its value",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_isatom(p_base, p_obj)
		&& flags == (x_obj_flag_t)x_obj_flags(p_obj)
		&& p_obj == x_obj_heap(p_base)
		&& i == x_atomint(p_obj)
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_make_atom(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t i = rand();
	x_obj_flag_t flags = rand();

	p_obj = x_make_atom(NULL, flags, (void *)i);
	_it_should("make an Atom object and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_isatom(NULL, p_obj)
		&& flags == (x_obj_flag_t)x_obj_flags(p_obj)
		&& i == x_atomint(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_make_atom(p_base, flags, (void *)i);
	_it_should("make an Atom object, attach it to the Base object, and set its value",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_isatom(p_base, p_obj)
		&& flags == (x_obj_flag_t)x_obj_flags(p_obj)
		&& p_obj == x_obj_heap(p_base)
		&& i == x_atomint(p_obj)
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_type_atom_register(void)
{
	x_obj_t *p_base, *p_type;

	p_base = x_base_make(NULL, NULL);

	p_type = x_type_atom_register(p_base, p_base);
	_it_should("return the Atom type object",
		0 == x_lib_strcmp(X_TYPE_ATOM_NAME, x_atomstr(x_type_field_name(p_type)))
	);
	_it_should("add the Atom type to the Type alist",
		p_type == x_restobj(x_firstobj(x_base_field_type_alist(p_base)))
	);

	x_sys_free(p_base);

	return NULL;
}

static char *test_type_atom_struct(void)
{
	x_obj_t *p_base, *p_type;

	helper_alloc_reset();

	p_base = x_mksatom(NULL, 0);
	p_type = x_type_atom_struct(p_base, p_base);
	_it_should("return Atom Type list",
		! x_obj_isnil(p_base, p_type)
		&& 0 == strcmp(X_TYPE_ATOM_NAME, x_atomstr(x_type_field_name(p_type)))
	);

	_it_should("contain the Name object",
		x_type_atom_name == x_type_field_name(p_type)
	);

	_it_should("set the Data object to nil",
		NULL == x_type_field_data(p_type)
	);

	_it_should("set the Make primitive",
		x_type_atom_make == x_atomfn(x_type_field_make(p_type))
	);

	_it_should("not set the Free primitive",
		NULL == x_type_field_free(p_type)
	);

	_it_should("not set the Clone primitive",
		NULL == x_type_field_clone(p_type)
	);

	_it_should("not set the Units primitive",
		NULL == x_type_field_units(p_type)
	);

	_it_should("not set the Length primitive",
		NULL == x_type_field_length(p_type)
	);

	_it_should("not set the Call primitive",
		NULL == x_type_field_call(p_type)
	);

	_it_should("not set the Eval primitive",
		NULL == x_type_field_eval(p_type)
	);

	_it_should("not set the From alist",
		NULL == x_type_field_from(p_type)
	);

	_it_should("not set the To alist",
		NULL == x_type_field_to(p_type)
	);

	_it_should("not set the Analyse primitive",
		NULL == x_type_field_analyse(p_type)
	);

	_it_should("not set the Delimit primitive",
		NULL == x_type_field_delimit(p_type)
	);

	_it_should("set the Write primitive",
		x_type_atom_write_prim == x_type_field_write(p_type)
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_base_alist_assoc(void)
{
	x_obj_t *p_base, *p_type;

	helper_alloc_reset();

	p_base = x_base_make(NULL, NULL);
	x_base_type_alist_extend(p_base, x_mkspair(p_base, x_mkspair(p_base, x_type_atom_name, NULL), atom(1)));

	p_type = x_base_type_alist_assoc(p_base, x_mkspair(p_base, x_type_atom_name, NULL));
	_it_should("find the type in the Type alist and return its properties",
		x_type_atom_name == x_type_field_name(p_type)
	);

	return NULL;
}

static char *test_type_atom_make(void)
{
	x_obj_t *p_base, *p_atom, *p_flags, *p_args, *p_obj[2];

	helper_alloc_reset();

	/* NULL p_base object */
	p_atom = x_mksatom(NULL, rand());
	p_args = x_mkspair(NULL, p_atom, NULL);

	p_obj[0] = x_type_atom_make(NULL, p_args);
	_it_should("make a Atom object and set its value",
		! x_obj_isnil(NULL, p_obj[0])
		&& x_obj_type_isatom(NULL, p_obj[0])
		&& x_atomval(p_atom) == x_atomval(p_obj[0])
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj[0])
	);

	/* w/flags */
	x_firstint(p_atom) = rand();
	p_flags = x_mksatom(NULL, rand());
	x_restobj(p_args) = x_mkspair(NULL, p_flags, NULL);

	p_obj[1] = x_type_atom_make(NULL, p_args);
	_it_should("make a second Atom object and set its value and flags",
		! x_obj_isnil(NULL, p_obj[1])
		&& x_obj_type_isatom(NULL, p_obj[1])
		&& x_atomval(p_atom) == x_atomval(p_obj[1])
		&& x_atomint(p_flags) == x_obj_flags(p_obj[1])
		&& p_obj[0] != p_obj[1]
	);

	_it_should("have not have returned the same type object for both objects",
		x_obj_type(p_obj[0]) != x_obj_type(p_obj[1])
	);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(x_restobj(p_args));
	x_sys_free(p_args);
	x_sys_free(p_flags);
	x_sys_free(p_atom);


	/* Empty p_base object */
	p_base = x_mksatom(NULL, NULL);
	p_atom = x_mksatom(p_base, rand());
	p_args = x_mkspair(p_base, p_atom, NULL);

	p_obj[0] = x_type_atom_make(p_base, p_args);
	_it_should("make a Atom object with an empty base and set its value",
		! x_obj_isnil(p_base, p_obj[0])
		&& x_obj_type_isatom(p_base, p_obj[0])
		&& x_atomval(p_atom) == x_atomval(p_obj[0])
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj[0])
	);

	/* w/flags */
	x_firstint(p_atom) = rand();
	p_flags = x_mksatom(p_base, rand());
	x_restobj(p_args) = x_mkspair(p_base, p_flags, p_base);

	p_obj[1] = x_type_atom_make(p_base, p_args);
	_it_should("make a second Atom object with an empty base and set its value and flags",
		! x_obj_isnil(p_base, p_obj[1])
		&& x_obj_type_isatom(p_base, p_obj[1])
		&& x_atomval(p_atom) == x_atomval(p_obj[1])
		&& x_atomint(p_flags) == x_obj_flags(p_obj[1])
		&& p_obj[0] != p_obj[1]
	);

	_it_should("not have returned the same type object for both objects",
		x_obj_type(p_obj[0]) != x_obj_type(p_obj[1])
	);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(x_restobj(p_args));
	x_sys_free(p_args);
	x_sys_free(p_flags);
	x_sys_free(p_atom);
	x_sys_free(p_base);


	/* With p_base object */
	p_base = x_base_make(NULL, NULL);
	p_atom = x_mksatom(p_base, rand());
	p_args = x_mkspair(p_base, p_atom, NULL);

	p_obj[0] = x_type_atom_make(p_base, p_args);
	_it_should("make an Atom object with a base object and set its value",
		! x_obj_isnil(p_base, p_obj[0])
		&& x_obj_type_isatom(p_base, p_obj[0])
		&& x_atomval(p_atom) == x_atomval(p_obj[0])
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj[0])
	);

	/* w/flags */
	x_firstint(p_atom) = rand();
	p_flags = x_mksatom(p_base, rand());
	x_restobj(p_args) = x_mkspair(p_base, p_flags, p_base);

	p_obj[1] = x_type_atom_make(p_base, p_args);
	_it_should("make a second Atom object a base object and set its value and flags",
		! x_obj_isnil(p_base, p_obj[1])
		&& x_obj_type_isatom(p_base, p_obj[1])
		&& x_atomval(p_atom) == x_atomval(p_obj[1])
		&& x_atomint(p_flags) == x_obj_flags(p_obj[1])
		&& p_obj[0] != p_obj[1]
	);
	_it_should("have returned the same type object for both objects",
		x_obj_type(p_obj[0]) == x_obj_type(p_obj[1])
	);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(x_restobj(p_args));
	x_sys_free(p_args);
	x_sys_free(p_flags);
	x_sys_free(p_atom);
	x_sys_free(p_base);


	return NULL;
}

static char *test_type_atom_write(void)
{
	x_obj_t *p_base, *p_obj, *p_args, *p_ret;
	char buf[64];

	helper_alloc_reset();
	memset(buf, 0, sizeof(buf));

	p_base = x_base_make(NULL, NULL);
	p_obj = x_mkatom(p_base, 0);
	p_args = x_mkspair(p_base, p_obj, NULL);

	helper_file_buffer_ptr[STDOUT_FILENO] = buf;
	file_buffer_ptr[STDOUT_FILENO][TEST_HELPER_FILE_WRITE] = buf;
	p_ret = x_type_atom_write(p_base, p_args);
	file_buffer_ptr[STDOUT_FILENO][TEST_HELPER_FILE_WRITE] = NULL;
	helper_file_buffer_ptr[STDOUT_FILENO] = NULL;

	_it_should("return the original object",
		p_obj == p_ret);
	_it_should("write the atom representation",
		0 == x_lib_strncmp(buf, X_TYPE_ATOM_WRITE_STR,
			X_TYPE_ATOM_WRITE_LEN));

	return NULL;
}

static char *run_tests() {
	_run_test(test_obj_type_isatom);
	_run_test(test_mkatom);
	_run_test(test_mkfatom);
	_run_test(test_make_atom);
	_run_test(test_atomval);
	_run_test(test_type_atom_register);
	_run_test(test_type_atom_struct);
	_run_test(test_base_alist_assoc);
	_run_test(test_type_atom_make);
	_run_test(test_type_atom_write);

	return NULL;
}
