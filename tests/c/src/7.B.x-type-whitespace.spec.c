/*
 * # Unit Tests: *x-type/whitespace*
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
#include "src/x-type.c"
#include "src/x-type/prim.c"
#include "src/x-type/buffer.c"
#include "src/x-type/char.c"
#include "src/x-sexp/char.c"
#include "src/x-type/str.c"
#include "src/x-sexp/str.c"
#include "src/x-type/whitespace.c"
#include "src/x-sexp/whitespace.c"

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

#define X_TEST_WHITESPACE_VALUE		"TEST"

#define nil			p_base
#define pair(X,Y)	(x_mkspair(p_base, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, (X)))

static char *test_obj_type_iswhitespace(void)
{
	x_obj_t *p_obj, *p_type;

	helper_alloc_reset();

	p_type = x_type_whitespace_register(NULL, NULL);
	p_obj = x_obj_make(NULL, p_type, X_OBJ_FLAG_NONE, X_OBJ_LENGTH_ATOM, NULL);

	_it_should("return true when object is Whitespace",
		1 == x_obj_type_iswhitespace(NULL, p_obj)
	);
	x_obj_free(p_obj);

	p_obj = x_mksatom(NULL, 0);
	_it_should("return false when object is not Whitespace",
		0 == x_obj_type_iswhitespace(NULL, p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}

static char *test_whitespaceval(void)
{
	x_obj_t *p_obj;
	x_char_t *str = X_TEST_WHITESPACE_VALUE;

	p_obj = x_mksatom(NULL, str);

	_it_should("return the Whitespace's value", str == x_whitespaceval(p_obj));

	x_sys_free(p_obj);

	return NULL;
}

static char *test_whitespacelen(void)
{
	x_obj_t *p_obj;
	x_char_t *str = X_TEST_WHITESPACE_VALUE;

	p_obj = x_mksatom(NULL, str);

	_it_should("return the Whitespace's length",
		x_lib_strlen(str) == x_whitespacelen(p_obj)
	);

	x_sys_free(p_obj);

	return NULL;
}

static char *test_type_whitespace_struct(void)
{
	x_obj_t *p_base, *p_type;

	helper_alloc_reset();

	p_base = x_mksatom(NULL, 0);
	p_type = x_type_whitespace_struct(p_base, p_base);
	_it_should("return Whitespace Type list",
		! x_obj_isnil(p_base, p_type)
		&& 0 == strcmp(X_TYPE_WHITESPACE_NAME, x_atomstr(x_type_field_name(p_type)))
	);

	_it_should("contain the Name object",
		x_type_whitespace_name == x_type_field_name(p_type)
	);

	_it_should("set the Data object to nil",
		NULL == x_type_field_data(p_type)
	);

	_it_should("not set the Make primitive",
		NULL == x_type_field_make(p_type)
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
		x_sexp_whitespace_analyse1_prim == x_type_field_analyse(p_type)
	);

	_it_should("not set the Delimit primitive",
		x_sexp_whitespace_delimit_prim == x_type_field_delimit(p_type)
	);

	_it_should("not set the Write primitive",
		NULL == x_type_field_write(p_type)
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_type_whitespace_register(void)
{
	x_obj_t *p_base, *p_type;

	p_base = x_base_make(NULL, NULL);

	p_type = x_type_whitespace_register(p_base, p_base);
	_it_should("return the Whitespace type object",
		0 == x_lib_strcmp(X_TYPE_WHITESPACE_NAME, x_strval(x_type_field_name(p_type)))
	);
	_it_should("add the Whitespace type to the Type alist",
		p_type == x_firstobj(x_base_field_type_alist(p_base))
	);

	x_sys_free(p_base);

	return NULL;
}

static char *run_tests() {
	_run_test(test_obj_type_iswhitespace);
	_run_test(test_whitespaceval);
	_run_test(test_whitespacelen);
	_run_test(test_type_whitespace_struct);
	_run_test(test_type_whitespace_register);

	return NULL;
}
