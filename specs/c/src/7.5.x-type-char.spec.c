/*
 * # Unit Tests: *x-type/char*
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
#include "src/x-type/char.c"
#include "src/x-sexp/char.c"
#include "src/x-type/str.c"
#include "src/x-sexp/str.c"
#include "src/x-type/buffer.c"

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

#define X_TEST_CHAR_VALUE		'@'

#define nil			p_base
#define pair(X,Y)	(x_mkspair(p_base, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, (X)))

static char *test_obj_type_ischar(void)
{
	x_obj_t *p_obj;

	helper_alloc_reset();

	p_obj = x_mkchar(NULL, 0);
	_it_should("return true when object is a char",
		1 == x_obj_type_ischar(NULL, p_obj)
	);
	x_obj_free(p_obj);

	p_obj = x_mksatom(NULL, 0);
	_it_should("return false when object is not a char",
		0 == x_obj_type_ischar(NULL, p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}

static char *test_charval(void)
{
	x_obj_t *p_obj;
	x_char_t c, value = X_TEST_CHAR_VALUE;

	p_obj = x_mksatom(NULL, value);

	c = x_charval(p_obj);
	_it_should("return the Character's value", value == c);

	x_sys_free(p_obj);

	return NULL;
}

static char *test_mkchar(void)
{
	x_obj_t *p_base, *p_obj;
	x_char_t c = rand();

	p_obj = x_mkchar(NULL, c);
	_it_should("make a Character object and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_ischar(NULL, p_obj)
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj)
		&& c == x_charval(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkchar(p_base, c);
	_it_should("make a Character object, attach it to the Base object, and set its value",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_ischar(p_base, p_obj)
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj)
		&& p_obj == x_obj_gc(p_base)
		&& c == x_charval(p_obj)
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_mkfchar(void)
{
	x_obj_t *p_base, *p_obj;
	x_char_t c = rand();
	x_obj_flag_t flags = rand();

	p_obj = x_mkfchar(NULL, flags, c);
	_it_should("make a Character object and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_ischar(NULL, p_obj)
		&& flags == x_obj_flags(p_obj)
		&& c == x_charval(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkfchar(p_base, flags, c);
	_it_should("make a Character object, attach it to the Base object, and set its value",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_ischar(p_base, p_obj)
		&& flags == x_obj_flags(p_obj)
		&& p_obj == x_obj_gc(p_base)
		&& c == x_charval(p_obj)
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_make_char(void)
{
	x_obj_t *p_base, *p_obj;
	x_char_t c = rand();
	x_obj_flag_t flags = rand();

	p_obj = x_make_char(NULL, flags, c);
	_it_should("make a Character object and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_ischar(NULL, p_obj)
		&& flags == x_obj_flags(p_obj)
		&& c == x_charval(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_make_char(p_base, flags, c);
	_it_should("make a Character object, attach it to the Base object, and set its value",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_ischar(p_base, p_obj)
		&& flags == x_obj_flags(p_obj)
		&& p_obj == x_obj_gc(p_base)
		&& c == x_charval(p_obj)
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_type_char_struct(void)
{
	x_obj_t *p_base, *p_type;

	helper_alloc_reset();

	p_base = x_mksatom(NULL, 0);
	p_type = x_type_char_struct(p_base, p_base);
	_it_should("return Character Type list",
		! x_obj_isnil(p_base, p_type)
		&& 0 == strcmp(X_TYPE_CHAR_NAME, x_atomstr(x_type_field_name(p_type)))
	);

	_it_should("contain the Name object",
		x_type_char_name == x_type_field_name(p_type)
	);

	_it_should("set the Data object to nil",
		NULL == x_type_field_data(p_type)
	);

	_it_should("set the Make primitive",
		x_type_char_make == x_primval(x_type_field_make(p_type))
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

	_it_should("set the Analyse primitive",
		x_sexp_char_analyse1_prim == x_type_field_analyse(p_type)
	);

	_it_should("not set the Delimit primitive",
		NULL == x_type_field_delimit(p_type)
	);

	_it_should("set the Write primitive",
		x_sexp_char_write_prim == x_type_field_write(p_type)
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_type_char_register(void)
{
	x_obj_t *p_base, *p_type;

	p_base = x_base_make(NULL, NULL);

	p_type = x_type_char_register(p_base, p_base);
	_it_should("return the Character type object",
		0 == x_lib_strcmp(X_TYPE_CHAR_NAME, x_strval(x_type_field_name(p_type)))
	);
	_it_should("add the Character type to the Type alist",
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
	x_base_type_alist_extend(p_base, x_mkspair(p_base, x_type_char_name, atom(1)));

	p_type = x_base_type_alist_assoc(p_base, x_mkspair(p_base, x_type_char_name, NULL));
	_it_should("find the type in the Type alist and return its properties",
		x_type_char_name == x_firstobj(p_type)
	);

	return NULL;
}

static char *test_type_char_make(void)
{
	x_obj_t *p_base, *p_args, *p_char, *p_obj[2];
	x_char_t value = X_TEST_CHAR_VALUE;

	helper_alloc_reset();

	/* NULL p_base object */
	p_char = x_mksatom(NULL, value);
	p_args = x_mkspair(NULL, p_char, NULL);

	p_obj[0] = x_type_char_make(NULL, p_args);
	_it_should("make a Character object",
		! x_obj_isnil(NULL, p_obj[0])
		&& x_obj_type_ischar(NULL, p_obj[0])
		&& value == x_charval(p_obj[0])
	);

	p_obj[1] = x_type_char_make(NULL, p_args);
	_it_should("make a second Character object",
		! x_obj_isnil(NULL, p_obj[1])
		&& x_obj_type_ischar(NULL, p_obj[1])
		&& value == x_charval(p_obj[1])
	);

	_it_should("have returned a different Type object for both objects",
		x_obj_type(p_obj[0]) != x_obj_type(p_obj[1])
	);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(p_args);
	x_sys_free(p_char);


	helper_alloc_reset();

	/* Empty p_base object */
	p_base = x_mksatom(NULL, NULL);
	p_char = x_mksatom(p_base, value);
	p_args = x_mkspair(p_base, p_char, NULL);

	p_obj[0] = x_type_char_make(p_base, p_args);
	_it_should("make a Character object",
		! x_obj_isnil(NULL, p_obj[0])
		&& x_obj_type_ischar(p_base, p_obj[0])
		&& value == x_charval(p_obj[0])
	);

	p_obj[1] = x_type_char_make(p_base, p_args);
	_it_should("make a second Character object",
		! x_obj_isnil(p_base, p_obj[1])
		&& x_obj_type_ischar(p_base, p_obj[1])
		&& value == x_charval(p_obj[1])
	);

	_it_should("have returned a different Type object for both objects",
		x_obj_type(p_obj[0]) != x_obj_type(p_obj[1])
	);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(p_args);
	x_sys_free(p_char);
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
	p_char = x_mksatom(p_base, value);
	p_args = x_mkspair(p_base, p_char, NULL);

	p_obj[0] = x_type_char_make(p_base, p_args);
	_it_should("make a Character object with a base object",
		! x_obj_isnil(p_base, p_obj[0])
		&& x_obj_type_ischar(p_base, p_obj[0])
	);

	p_obj[1] = x_type_char_make(p_base, p_args);
	_it_should("make a second Character object a base object",
		! x_obj_isnil(p_base, p_obj[1])
		&& x_obj_type_ischar(p_base, p_obj[1])
	);
	_it_should("have returned the same type object for both objects",
		x_obj_type(p_obj[0]) == x_obj_type(p_obj[1])
	);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(p_args);
	x_sys_free(p_char);
	x_sys_free(p_base);


	return NULL;
}

static char *run_tests() {
	_run_test(test_obj_type_ischar);
	_run_test(test_charval);
	_run_test(test_mkchar);
	_run_test(test_mkfchar);
	_run_test(test_make_char);
	_run_test(test_type_char_struct);
	_run_test(test_type_char_register);
	_run_test(test_base_alist_assoc);
	_run_test(test_type_char_make);

	return NULL;
}
