/*
 * # Unit Tests: *x-type/ptr*
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
#include "src/x-obj.c"
#include "src/x-alist.c"
#include "src/x-base.c"
#include "src/x-type.c"
#include "src/x-type/prim.c"
#include "src/x-type/ptr.c"
#include "src/x-type/str.c"
#include "src/x-sexp/str.c"
#include "src/x-type/buffer.c"
#include "src/x-type/char.c"
#include "src/x-sexp/char.c"

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

#define X_TEST_PTR_VALUE		(void *)0xa5

#define nil			p_base
#define pair(X,Y)	(x_mkspair(p_base, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, (X)))

static char *test_obj_type_isptr(void)
{
	x_obj_t *p_obj;
	x_char_t *p_ptr, *ptr = X_TEST_PTR_VALUE;

	p_obj = x_mksatom(NULL, ptr);

	p_ptr = x_ptrval(p_obj);
	_it_should("return the Pointer's value", ptr == p_ptr);

	x_sys_free(p_obj);

	return NULL;
}

static char *test_ptrval(void)
{
	x_obj_t *p_obj;
	x_char_t *p_ptr, *ptr = X_TEST_PTR_VALUE;

	p_obj = x_mksatom(NULL, ptr);

	p_ptr = x_ptrval(p_obj);
	_it_should("return the Pointer's value", ptr == p_ptr);

	x_sys_free(p_obj);

	return NULL;
}

static char *test_mkptr(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t i = rand();

	p_obj = x_mkptr(NULL, (void *)i);
	_it_should("make a Pointer object and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_isptr(NULL, p_obj)
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj)
		&& (void *)i == x_ptrval(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkptr(NULL, (void *)i);
	_it_should("make a Pointer object, attach it to the Base object, and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_isptr(NULL, p_obj)
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj)
		&& (void *)i == x_ptrval(p_obj)
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_mkfptr(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t i = rand();
	x_obj_flag_t flags = rand();

	p_obj = x_mkfptr(NULL, flags, (void *)i);
	_it_should("make a Pointer object and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_isptr(NULL, p_obj)
		&& flags == x_obj_flags(p_obj)
		&& (void *)i == x_ptrval(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkfptr(p_base, flags, (void *)i);
	_it_should("make a Pointer object, attach it to the Base object, and set its value",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_isptr(p_base, p_obj)
		&& flags == x_obj_flags(p_obj)
		&& (void *)i == x_ptrval(p_obj)
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_mkptrown(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t i = rand();

	p_obj = x_mkptrown(NULL, (void *)i);
	_it_should("make a Pointer object and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_isptr(NULL, p_obj)
		&& X_OBJ_FLAG_OWN == x_obj_flags(p_obj)
		&& (void *)i == x_ptrval(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkptrown(p_base, (void *)i);
	_it_should("make a Pointer object, attach it to the Base object, and set its value",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_isptr(p_base, p_obj)
		&& X_OBJ_FLAG_OWN == x_obj_flags(p_obj)
		&& (void *)i == x_ptrval(p_obj)
	);

	x_sys_free(p_obj);

	return NULL;
}

static char *test_mkfptrown(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t i = rand();
	x_obj_flag_t flags = rand();

	p_obj = x_mkfptrown(NULL, flags, (void *)i);
	_it_should("make a Pointer object and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_isptr(NULL, p_obj)
		&& (X_OBJ_FLAG_OWN | flags) == x_obj_flags(p_obj)
		&& (void *)i == x_ptrval(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkfptrown(p_base, flags, (void *)i);
	_it_should("make a Pointer object, attach it to the Base object, and set its value",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_isptr(p_base, p_obj)
		&& (X_OBJ_FLAG_OWN | flags) == x_obj_flags(p_obj)
		&& (void *)i == x_ptrval(p_obj)
	);

	x_sys_free(p_obj);

	return NULL;
}

static char *test_make_ptr(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t i = rand();
	x_obj_flag_t flags = rand();

	p_obj = x_make_ptr(NULL, flags, (void *)i);
	_it_should("make a Pointer object and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_isptr(NULL, p_obj)
		&& flags == x_obj_flags(p_obj)
		&& (void *)i == x_ptrval(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_make_ptr(p_base, flags, (void *)i);
	_it_should("make a Pointer object, attach it to the Base object, and set its value",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_isptr(p_base, p_obj)
		&& flags == x_obj_flags(p_obj)
		&& (void *)i == x_ptrval(p_obj)
	);

	x_sys_free(p_obj);

	return NULL;
}

static char *test_type_ptr_struct(void)
{
	x_obj_t *p_base, *p_type;

	helper_alloc_reset();

	p_base = x_mksatom(NULL, 0);
	p_type = x_type_ptr_struct(p_base, p_base);
	_it_should("return Pointer Type list",
		! x_obj_isnil(p_base, p_type)
		&& 0 == strcmp(X_TYPE_PTR_NAME, x_atomstr(x_type_field_name(p_type)))
	);

	_it_should("contain the Name object",
		x_type_ptr_name == x_type_field_name(p_type)
	);

	_it_should("set the Data object to nil",
		NULL == x_type_field_data(p_type)
	);

	_it_should("set the Make primitive",
		x_type_ptr_make_prim == x_type_field_make(p_type)
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

	_it_should("not set the Convert primitive",
		NULL == x_type_field_convert(p_type)
	);

	_it_should("not set the Analyse primitive",
		NULL == x_type_field_analyse(p_type)
	);

	_it_should("not set the Delimit primitive",
		NULL == x_type_field_delimit(p_type)
	);

	_it_should("not set the Write primitive",
		NULL == x_type_field_write(p_type)
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_type_ptr_register(void)
{
	x_obj_t *p_base, *p_type;

	p_base = x_base_make(NULL, NULL);

	p_type = x_type_ptr_register(p_base, p_base);
	_it_should("return the Pointer type object",
		0 == x_lib_strcmp(X_TYPE_PTR_NAME, x_strval(x_type_field_name(p_type)))
	);
	_it_should("add the Pointer type to the Type alist",
		p_type == x_firstobj(x_base_field_type_alist(p_base))
	);

	x_sys_free(p_base);

	return NULL;
}

static char *test_base_alist_assoc(void)
{
	x_obj_t *p_base, *p_type;

	helper_alloc_reset();

	p_base = x_base_make(NULL, NULL);
	x_base_type_alist_extend(p_base, x_mkspair(p_base, x_type_ptr_name, atom(1)));

	p_type = x_base_type_alist_assoc(p_base, x_mkspair(p_base, x_type_ptr_name, NULL));
	_it_should("find the type in the Type alist and return its properties",
		x_type_ptr_name == x_firstobj(p_type)
	);

	return NULL;
}

static char *test_type_ptr_make(void)
{
	x_obj_t *p_base, *p_args, *p_ptr, *p_obj[2];
	x_char_t *value = X_TEST_PTR_VALUE;

	helper_alloc_reset();

	/* NULL p_base object */
	p_ptr = x_mksatom(NULL, value);
	p_args = x_mkspair(NULL, p_ptr, NULL);
	p_obj[0] = x_type_ptr_make(NULL, p_args);
	_it_should("make a Pointer object",
		! x_obj_isnil(NULL, p_obj[0])
		&& x_obj_type_isptr(NULL, p_obj[0])
		&& value == x_ptrval(p_obj[0])
	);

	p_obj[1] = x_type_ptr_make(NULL, p_args);
	_it_should("make a second Pointer object",
		! x_obj_isnil(NULL, p_obj[1])
		&& x_obj_type_isptr(NULL, p_obj[1])
		&& value == x_ptrval(p_obj[1])
	);

	_it_should("have returned a different Type object for both objects",
		x_obj_type(p_obj[0]) != x_obj_type(p_obj[1])
	);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(p_args);
	x_sys_free(p_ptr);


	helper_alloc_reset();

	/* Empty p_base object */
	p_base = x_mksatom(NULL, NULL);
	p_ptr = x_mksatom(p_base, value);
	p_args = x_mkspair(p_base, p_ptr, NULL);

	p_obj[0] = x_type_ptr_make(p_base, p_args);
	_it_should("make a Pointer object",
		! x_obj_isnil(NULL, p_obj[0])
		&& x_obj_type_isptr(p_base, p_obj[0])
		&& value == x_ptrval(p_obj[0])
	);

	p_obj[1] = x_type_ptr_make(p_base, p_args);
	_it_should("make a second pointer object",
		! x_obj_isnil(p_base, p_obj[1])
		&& x_obj_type_isptr(p_base, p_obj[1])
		&& value == x_ptrval(p_obj[1])
	);

	_it_should("have returned a different Type object for both objects",
		x_obj_type(p_obj[0]) != x_obj_type(p_obj[1])
	);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(p_args);
	x_sys_free(p_ptr);
	x_sys_free(p_base);


	helper_alloc_reset();

	/* With p_base object */
	p_base = x_mksatom(NULL, NULL);
	x_atomobj(p_base) = pair(
		pair(nil, nil),
		pair(
			pair(atom(STDIN_FILENO),
			pair(atom(STDOUT_FILENO),
			pair(atom(STDERR_FILENO),
			nil))),
		nil));
	p_ptr = x_mksatom(p_base, value);
	p_args = x_mkspair(p_base, p_ptr, NULL);

	p_obj[0] = x_type_ptr_make(p_base, p_args);
	_it_should("make a Pointer object with a base object",
		! x_obj_isnil(p_base, p_obj[0])
		&& x_obj_type_isptr(p_base, p_obj[0])
	);

	p_obj[1] = x_type_ptr_make(p_base, p_args);
	_it_should("make a second Pointer object a base object",
		! x_obj_isnil(p_base, p_obj[1])
		&& x_obj_type_isptr(p_base, p_obj[1])
	);
	_it_should("have returned the same type object for both objects",
		x_obj_type(p_obj[0]) == x_obj_type(p_obj[1])
	);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(p_args);
	x_sys_free(p_ptr);
	x_sys_free(p_base);


	return NULL;
}

static char *run_tests() {
	_run_test(test_obj_type_isptr);
	_run_test(test_ptrval);
	_run_test(test_mkptr);
	_run_test(test_mkfptr);
	_run_test(test_mkptrown);
	_run_test(test_mkfptrown);
	_run_test(test_make_ptr);
	_run_test(test_type_ptr_struct);
	_run_test(test_type_ptr_register);
	_run_test(test_base_alist_assoc);
	_run_test(test_type_ptr_make);

	return NULL;
}
