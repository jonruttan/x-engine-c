/*
 * # Unit Tests: *x-type*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"
#include "x-obj.h"
#include "x-type/prim.h"

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
#include "src/x-type/atom.c"
#include "src/x-type/prim.c"

#define STUB_X_PRIM
#define STUB_X_PROCEDURE
#define STUB_X_OPERATIVE
#define STUB_X_EVAL
#define STUB_X_TOKEN
#define STUB_X_HEAP
#define STUB_X_OBJ_OBJ
#define STUB_X_STR
#define STUB_X_PRIM_REGISTER
#include "helper-stubs.c"

#include "ext/x-expr/tests/src/helper-system-functions.c"

/*
 * ## Test Macros
 */
#define TEST_TYPE_MK_ENTRY(X)		(x_obj_t *)(X)
#define TEST_TYPE_STRUCT_NAME		TEST_TYPE_MK_ENTRY(0x1)
#define TEST_TYPE_STRUCT_DATA		TEST_TYPE_MK_ENTRY(0x2)
#define TEST_TYPE_STRUCT_MAKE		TEST_TYPE_MK_ENTRY(0x3)
#define TEST_TYPE_STRUCT_FREE		TEST_TYPE_MK_ENTRY(0x4)
#define TEST_TYPE_STRUCT_CLONE		TEST_TYPE_MK_ENTRY(0x5)
#define TEST_TYPE_STRUCT_UNITS		TEST_TYPE_MK_ENTRY(0x6)
#define TEST_TYPE_STRUCT_LENGTH		TEST_TYPE_MK_ENTRY(0x7)
#define TEST_TYPE_STRUCT_CALL		TEST_TYPE_MK_ENTRY(0x8)
#define TEST_TYPE_STRUCT_EVAL		TEST_TYPE_MK_ENTRY(0x9)
#define TEST_TYPE_STRUCT_FROM		TEST_TYPE_MK_ENTRY(0xA)
#define TEST_TYPE_STRUCT_TO		TEST_TYPE_MK_ENTRY(0xB)
#define TEST_TYPE_STRUCT_ANALYSE	TEST_TYPE_MK_ENTRY(0xC)
#define TEST_TYPE_STRUCT_DELIMIT	TEST_TYPE_MK_ENTRY(0xD)
#define TEST_TYPE_STRUCT_WRITE		TEST_TYPE_MK_ENTRY(0xE)


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

static char *test_type_types(void)
{
	x_obj_t *p_base, *p_obj;

	p_base = x_mksatom(NULL, -1);
	p_obj = x_mksatom(p_base, 0);

	x_type_settypes(p_base, p_obj);

	_it_should("have set the types", x_type_types(p_base) == p_obj);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_type_units(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t units;

	p_base = x_mksatom(NULL, -1);

/*	x_atomobj(p_base) = pair(
		pair(nil, nil),
		pair(
			pair(atom(STDIN_FILENO),
			pair(atom(STDOUT_FILENO),
			pair(atom(STDERR_FILENO),
			nil))),
		nil));
*/
	/*x_sexp_write(p_base, x_firstobj(p_base));*/

	/* TODO: GC version. */
	p_obj = x_mksatom(p_base, NULL);
	units = x_obj_units(p_base, p_obj);
	_it_should("have returned X_OBJ_UNITS_ATOM units", X_OBJ_UNITS_ATOM == units);

	p_obj = x_mkspair(p_base, NULL, NULL);
	units = x_obj_units(p_base, p_obj);
	_it_should("have returned X_OBJ_UNITS_PAIR units", X_OBJ_UNITS_PAIR == units);


	test_cleanup(p_base);

	return NULL;
}

x_char_t *mock_str = "MOCK";
unsigned int mock_fn_calls = 0;
x_obj_t *mock_obj = NULL;
x_obj_t *mock_fn(x_obj_t *p_base, x_obj_t *p_args)
{
	mock_fn_calls++;

	return mock_obj = x_mkspair(p_base, x_mksatom(p_base, mock_str), p_base);
}


static char *test_type_struct_make(void)
{
	x_obj_t *p_base = NULL, *p_type;
	struct x_type_t type = {
		TEST_TYPE_STRUCT_NAME,
		TEST_TYPE_STRUCT_DATA,
		NULL,	/* p_mark */
		TEST_TYPE_STRUCT_MAKE,
		TEST_TYPE_STRUCT_FREE,
		TEST_TYPE_STRUCT_CLONE,
		TEST_TYPE_STRUCT_UNITS,
		TEST_TYPE_STRUCT_LENGTH,
		TEST_TYPE_STRUCT_CALL,
		TEST_TYPE_STRUCT_EVAL,
		TEST_TYPE_STRUCT_FROM,
		TEST_TYPE_STRUCT_TO,
		TEST_TYPE_STRUCT_ANALYSE,
		TEST_TYPE_STRUCT_DELIMIT,
		TEST_TYPE_STRUCT_WRITE,
		NULL	/* p_error */
	};

	helper_alloc_reset();

	p_base = x_mksatom(NULL, 0);
	p_type = x_type_struct_make(p_base, type);
	_it_should("return a new Type list",
		! x_obj_isnil(p_base, p_type)
	);

	_it_should("set the Name object",
		TEST_TYPE_STRUCT_NAME == x_type_field_name(p_type)
	);

	_it_should("set the Data object",
		TEST_TYPE_STRUCT_DATA == x_type_field_data(p_type)
	);

	_it_should("set the Make primitive",
		TEST_TYPE_STRUCT_MAKE == x_type_field_make(p_type)
	);

	_it_should("set the Free primitive",
		TEST_TYPE_STRUCT_FREE == x_type_field_free(p_type)
	);

	_it_should("set the Clone primitive",
		TEST_TYPE_STRUCT_CLONE == x_type_field_clone(p_type)
	);

	_it_should("set the Units primitive",
		TEST_TYPE_STRUCT_UNITS == x_type_field_units(p_type)
	);

	_it_should("set the Length primitive",
		TEST_TYPE_STRUCT_LENGTH == x_type_field_length(p_type)
	);

	_it_should("set the Call primitive",
		TEST_TYPE_STRUCT_CALL == x_type_field_call(p_type)
	);

	_it_should("set the Eval primitive",
		TEST_TYPE_STRUCT_EVAL == x_type_field_eval(p_type)
	);

	_it_should("set the From alist",
		TEST_TYPE_STRUCT_FROM == x_type_field_from(p_type)
	);

	_it_should("set the To alist",
		TEST_TYPE_STRUCT_TO == x_type_field_to(p_type)
	);

	_it_should("set the Analyse primitive",
		TEST_TYPE_STRUCT_ANALYSE == x_type_field_analyse(p_type)
	);

	_it_should("set the Delimit primitive",
		TEST_TYPE_STRUCT_DELIMIT == x_type_field_delimit(p_type)
	);

	_it_should("set the Write primitive",
		TEST_TYPE_STRUCT_WRITE == x_type_field_write(p_type)
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_type_struct_get(void)
{
	x_obj_t *p_base, *p_obj[4], *p_args;

	p_obj[3] = x_mksatom(NULL, mock_fn);
	p_obj[2] = x_mksatom(NULL, mock_str);
	p_obj[1] = x_mkspair(NULL, p_obj[3], NULL);

	p_args = x_mkspair(NULL,
		p_obj[2],
		p_obj[1]
	);

	mock_fn_calls = 0;
	p_obj[0] = x_type_struct_get(NULL, p_args);
	_it_should("return mock_obj when base is NULL",
		mock_obj == p_obj[0]
		&& 1 == mock_fn_calls
	);

	x_sys_free(p_obj[3]);
	x_sys_free(p_obj[2]);
	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(p_args);


	p_base = x_mksatom(NULL, NULL);
	p_obj[3] = x_mksatom(p_base, mock_fn);
	p_obj[2] = x_mksatom(p_base, mock_str);
	p_obj[1] = x_mkspair(p_base, p_obj[3], p_base);

	p_args = x_mkspair(p_base,
		p_obj[2],
		p_obj[1]
	);

	mock_fn_calls = 0;
	p_obj[0] = x_type_struct_get(p_base, p_args);
	_it_should("return mock_obj when base is not set",
		mock_obj == p_obj[0]
		&& 1 == mock_fn_calls
	);

	x_sys_free(p_obj[3]);
	x_sys_free(p_obj[2]);
	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(p_args);
	x_sys_free(p_base);


	p_base = x_base_make(NULL, NULL);

	p_args = x_mkspair(NULL,
		x_mksatom(p_base, mock_str),
		x_mkspair(p_base, x_mksatom(p_base, mock_fn), p_base)
	);

	mock_fn_calls = 0;
	p_obj[0] = x_type_struct_get(p_base, p_args);
	_it_should("return mock_obj when base is set",
		mock_obj == p_obj[0]
		&& 1 == mock_fn_calls
	);

	p_obj[1] = x_type_struct_get(p_base, p_args);
	_it_should("return same mock_obj when base is set",
		mock_obj == p_obj[1]
		&& 1 == mock_fn_calls
		&& p_obj[0] == p_obj[1]
	);

	test_cleanup(p_base);

	return NULL;
}

static int type_write_call_count = 0;
x_obj_t *type_write(x_obj_t *p_base, x_obj_t *p_args)
{
	++type_write_call_count;

	return x_firstobj(p_args);
}


static char *test_type_write(void)
{
	x_obj_t *p_obj, *p_fn, *p_args, *p_ret;

	p_obj = x_mkatom(NULL, NULL);
	p_fn = x_mksatom(NULL, type_write);
	x_type_field_write(x_obj_type(p_obj)) = p_fn;

	p_args = x_mkspair(NULL, p_obj, NULL);

	p_ret = x_type_write(NULL, p_args);

	_it_should("call the object type's write primitive",
		p_obj == p_ret
		&& 1 == type_write_call_count
	);

	return NULL;
}

static char *run_tests() {
	_run_test(test_type_types);
	_run_test(test_type_units);
	_run_test(test_type_struct_make);
	_run_test(test_type_struct_get);
	_run_test(test_type_write);

	return NULL;
}
