/*
 * # Unit Tests: *x-type/str*
 */

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
#include "src/x-type.c"
#include "src/x-type/prim.c"
#include "src/x-type/buffer.c"
#include "src/x-type/char.c"
#include "src/x-sexp/char.c"
#include "src/x-type/str.c"
#include "src/x-sexp/str.c"

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

#define X_TEST_STR_VALUE		"TEST"

#define nil			p_base
#define pair(X,Y)	(x_mkspair(p_base, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, (X)))

static char *test_obj_type_isstr(void)
{
	x_obj_t *p_obj;

	helper_alloc_reset();

	p_obj = x_mkstr(NULL, X_TEST_STR_VALUE);
	_it_should("return true when object is a String",
		1 == x_obj_type_isstr(NULL, p_obj)
	);
	x_obj_free(p_obj);

	p_obj = x_mksatom(NULL, 0);
	_it_should("return false when object is not a String",
		0 == x_obj_type_isstr(NULL, p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}

static char *test_strval(void)
{
	x_obj_t *p_obj;
	x_char_t *p_str, *str = X_TEST_STR_VALUE;

	p_obj = x_mksatom(NULL, str);

	p_str = x_strval(p_obj);
	_it_should("return the String's value", str == p_str);

	x_sys_free(p_obj);

	return NULL;
}

static char *test_mkstr(void)
{
	x_obj_t *p_base, *p_obj;

	p_obj = x_mkstr(NULL, X_TEST_STR_VALUE);
	_it_should("make a String object and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_isstr(NULL, p_obj)
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj)
		&& 0 == strcmp(X_TEST_STR_VALUE, x_strval(p_obj))
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkstr(p_base, X_TEST_STR_VALUE);
	_it_should("make an String object, attach it to the Base object, and set its value",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_isstr(NULL, p_obj)
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj)
		&& p_obj == x_obj_gc(p_base)
		&& 0 == strcmp(X_TEST_STR_VALUE, x_strval(p_obj))
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_mkfstr(void)
{
	x_obj_t *p_base, *p_obj;
	x_obj_flag_t flags = rand();

	p_obj = x_mkfstr(NULL, flags, X_TEST_STR_VALUE);
	_it_should("make a String object and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_isstr(NULL, p_obj)
		&& flags == x_obj_flags(p_obj)
		&& 0 == strcmp(X_TEST_STR_VALUE, x_strval(p_obj))
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkfstr(p_base, flags, X_TEST_STR_VALUE);
	_it_should("make an String object, attach it to the Base object, and set its value",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_isstr(NULL, p_obj)
		&& flags == x_obj_flags(p_obj)
		&& p_obj == x_obj_gc(p_base)
		&& 0 == strcmp(X_TEST_STR_VALUE, x_strval(p_obj))
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_mkstrown(void)
{
	x_obj_t *p_base, *p_obj;

	p_obj = x_mkstrown(NULL, X_TEST_STR_VALUE);
	_it_should("make an Owned String object and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_isstr(NULL, p_obj)
		&& X_OBJ_FLAG_OWN == x_obj_flags(p_obj)
		&& 0 == strcmp(X_TEST_STR_VALUE, x_strval(p_obj))
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkstrown(p_base, X_TEST_STR_VALUE);
	_it_should("make an Owned String object, attach it to the Base object, and set its value",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_isstr(NULL, p_obj)
		&& X_OBJ_FLAG_OWN == x_obj_flags(p_obj)
		&& p_obj == x_obj_gc(p_base)
		&& 0 == strcmp(X_TEST_STR_VALUE, x_strval(p_obj))
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_mkfstrown(void)
{
	x_obj_t *p_base, *p_obj;
	x_obj_flag_t flags = rand();

	p_obj = x_mkfstrown(NULL, flags, X_TEST_STR_VALUE);
	_it_should("make an Owned String object and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_isstr(NULL, p_obj)
		&& (X_OBJ_FLAG_OWN | flags) == x_obj_flags(p_obj)
		&& 0 == strcmp(X_TEST_STR_VALUE, x_strval(p_obj))
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkfstr(p_base, flags, X_TEST_STR_VALUE);
	_it_should("make an Owned String object, attach it to the Base object, and set its value",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_isstr(p_base, p_obj)
		&& flags == x_obj_flags(p_obj)
		&& p_obj == x_obj_gc(p_base)
		&& 0 == strcmp(X_TEST_STR_VALUE, x_strval(p_obj))
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_make_str(void)
{
	x_obj_t *p_base, *p_obj;
	x_obj_flag_t flags = rand();

	p_obj = x_make_str(NULL, flags, X_TEST_STR_VALUE);
	_it_should("make an Owned String object and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_isstr(NULL, p_obj)
		&& flags == x_obj_flags(p_obj)
		&& 0 == strcmp(X_TEST_STR_VALUE, x_strval(p_obj))
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_make_str(p_base, flags, X_TEST_STR_VALUE);
	_it_should("make an Owned String object, attach it to the Base object, and set its value",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_isstr(p_base, p_obj)
		&& flags == x_obj_flags(p_obj)
		&& p_obj == x_obj_gc(p_base)
		&& 0 == strcmp(X_TEST_STR_VALUE, x_strval(p_obj))
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_type_str_struct(void)
{
	x_obj_t *p_base, *p_type;

	helper_alloc_reset();

	p_base = x_mksatom(NULL, 0);
	p_type = x_type_str_struct(p_base, p_base);
	_it_should("return String Type list",
		! x_obj_isnil(p_base, p_type)
		&& 0 == strcmp(X_TYPE_STR_NAME, x_atomstr(x_type_field_name(p_type)))
	);

	_it_should("contain the Name object",
		x_type_str_name == x_type_field_name(p_type)
	);

	_it_should("set the Data object to nil",
		NULL == x_type_field_data(p_type)
	);

	_it_should("set the Make primitive",
		x_type_str_make_prim == x_type_field_make(p_type)
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

	_it_should("set the Length primitive",
		x_type_str_length_prim == x_type_field_length(p_type)
	);

	_it_should("set the Call primitive",
		x_type_str_call_prim == x_type_field_call(p_type)
	);

	_it_should("not set the Eval primitive",
		NULL == x_type_field_eval(p_type)
	);

	_it_should("not set the Convert primitive",
		NULL == x_type_field_convert(p_type)
	);

	_it_should("not set the Analyse primitive",
		NULL == x_type_field_delimit(p_type)
	);

	_it_should("set the Write primitive",
		x_sexp_str_write_prim == x_type_field_write(p_type)
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_type_str_register(void)
{
	x_obj_t *p_base, *p_type;

	p_base = x_base_make(NULL, NULL);

	p_type = x_type_str_register(p_base, p_base);
	_it_should("return the String type object",
		0 == x_lib_strcmp(X_TYPE_STR_NAME, x_strval(x_type_field_name(p_type)))
	);
	_it_should("add the String type to the Type alist",
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
	x_base_type_alist_extend(p_base, x_mkspair(p_base, x_type_str_name, atom(1)));

	p_type = x_base_type_alist_assoc(p_base, x_mkspair(p_base, x_type_str_name, NULL));
	_it_should("find the type in the Type alist and return its properties",
		x_type_str_name == x_firstobj(p_type)
	);

	return NULL;
}

static char *test_type_str_make(void)
{
	x_obj_t *p_base, *p_args, *p_str, *p_obj[2];
	x_char_t *value = X_TEST_STR_VALUE;

	helper_alloc_reset();

	/* NULL p_base object */
	p_str = x_mksatom(NULL, value);
	p_args = x_mkspair(NULL, p_str, NULL);
	p_obj[0] = x_type_str_make(NULL, p_args);
	_it_should("make a String object",
		! x_obj_isnil(NULL, p_obj[0])
		&& x_obj_type_isstr(NULL, p_obj[0])
		&& 0 == strcmp(X_TEST_STR_VALUE, x_strval(p_obj[0]))
	);

	p_obj[1] = x_type_str_make(NULL, p_args);
	_it_should("make a second String object",
		! x_obj_isnil(NULL, p_obj[1])
		&& x_obj_type_isstr(NULL, p_obj[1])
		&& 0 == strcmp(X_TEST_STR_VALUE, x_strval(p_obj[1]))
	);

	_it_should("have returned a different Type object for both objects",
		x_obj_type(p_obj[0]) != x_obj_type(p_obj[1])
	);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(p_args);
	x_sys_free(p_str);


	helper_alloc_reset();

	/* Empty p_base object */
	p_base = x_mksatom(NULL, NULL);
	p_str = x_mksatom(p_base, value);
	p_args = x_mkspair(p_base, p_str, NULL);

	p_obj[0] = x_type_str_make(p_base, p_args);
	_it_should("make a String object",
		! x_obj_isnil(NULL, p_obj[0])
		&& x_obj_type_isstr(p_base, p_obj[0])
		&& 0 == strcmp(X_TEST_STR_VALUE, x_strval(p_obj[0]))
	);

	p_obj[1] = x_type_str_make(p_base, p_args);
	_it_should("make a second String object",
		! x_obj_isnil(p_base, p_obj[1])
		&& x_obj_type_isstr(p_base, p_obj[1])
		&& 0 == strcmp(X_TEST_STR_VALUE, x_strval(p_obj[1]))
	);

	_it_should("have returned a different Type object for both objects",
		x_obj_type(p_obj[0]) != x_obj_type(p_obj[1])
	);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(p_args);
	x_sys_free(p_str);
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
	p_str = x_mksatom(p_base, value);
	p_args = x_mkspair(p_base, p_str, NULL);

	p_obj[0] = x_type_str_make(p_base, p_args);
	_it_should("make a String object with a base object",
		! x_obj_isnil(p_base, p_obj[0])
		&& x_obj_type_isstr(p_base, p_obj[0])
	);

	p_obj[1] = x_type_str_make(p_base, p_args);
	_it_should("make a second String object a base object",
		! x_obj_isnil(p_base, p_obj[1])
		&& x_obj_type_isstr(p_base, p_obj[1])
	);
	_it_should("have returned the same type object for both objects",
		x_obj_type(p_obj[0]) == x_obj_type(p_obj[1])
	);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(p_args);
	x_sys_free(p_str);
	x_sys_free(p_base);


	return NULL;
}

static char *test_type_str_call(void)
{
	x_obj_t *p_base, *p_str, *p_args, *p_obj;

	helper_alloc_reset();

	/* With p_base object */
	p_base = x_mksatom(NULL, NULL);
	x_atomobj(p_base) =
		pair(
			pair(nil, nil),
		pair(
			pair(atom(STDIN_FILENO),
			pair(atom(STDOUT_FILENO),
			pair(atom(STDERR_FILENO),
			nil))),
		pair(
			pair(nil, nil),
		nil)));

	p_str = x_mksatom(p_base, X_TYPE_STR_NAME);
	p_args = x_mkspair(p_base, p_str, p_base);

	p_obj = x_type_str_call(p_base, p_args);
	_it_should("call the test function and return p_base",
		X_TYPE_STR_NAME[0] == x_charval(p_obj)
	);

	x_sys_free(p_args);

	_mark_incomplete();

	return NULL;
}

static char *run_tests() {
	_run_test(test_obj_type_isstr);
	_run_test(test_strval);
	_run_test(test_mkstr);
	_run_test(test_mkfstr);
	_run_test(test_mkstrown);
	_run_test(test_mkfstrown);
	_run_test(test_make_str);
	_run_test(test_type_str_struct);
	_run_test(test_type_str_register);
	_run_test(test_base_alist_assoc);
	_run_test(test_type_str_make);
	_run_test(test_type_str_call);

	return NULL;
}
