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

#include "ext/x-expr/tests/src/test-helper-system.c"

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-lib.c"
#include "ext/x-expr/src/x.c"
#include "ext/x-expr/src/x-obj.c"
#include "src/x-alist.c"
#include "ext/x-expr/src/x-base.c"
#include "src/x-base.c"
#include "ext/x-expr/src/x-heap.c"
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
#define STUB_X_PRIM_SHADOW
#define STUB_X_PROCEDURE_APPLY
#include "helper-stubs.c"


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
#define TEST_TYPE_STRUCT_READ		TEST_TYPE_MK_ENTRY(0xE)
#define TEST_TYPE_STRUCT_WRITE		TEST_TYPE_MK_ENTRY(0xF)
#define TEST_TYPE_STRUCT_DISPLAY	TEST_TYPE_MK_ENTRY(0x10)


/*
 * ## Test Overhead
 */

static void _setup(void)
{
	helper_set_alloc(MEM_GUARANTEED);
	helper_sys_funcs.exit = mock_exit;
	helper_sys_funcs.malloc = helper_malloc;
	helper_sys_funcs.free = helper_free;
	_buffer_index = -1;
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
#define pair(X,Y)	(x_mkspair(p_base, X_OBJ_FLAG_NONE, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, X_OBJ_FLAG_NONE, (X)))

static char *test_type_types(void)
{
	x_obj_t *p_base, *p_obj;

	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);
	p_obj = x_mksatom(p_base, X_OBJ_FLAG_NONE, 0);

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

	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);

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
	p_obj = x_mksatom(p_base, X_OBJ_FLAG_NONE, NULL);
	units = x_obj_units(p_base, p_obj);
	_it_should("have returned X_OBJ_UNITS_ATOM units", X_OBJ_UNITS_ATOM == units);

	p_obj = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, NULL);
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

	return mock_obj = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, mock_str), NULL), p_base);
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
		TEST_TYPE_STRUCT_READ,
		TEST_TYPE_STRUCT_WRITE,
		TEST_TYPE_STRUCT_DISPLAY,
		NULL	/* p_error */
	};

	helper_alloc_reset();

	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);
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

	_it_should("set the Read primitive",
		TEST_TYPE_STRUCT_READ == x_type_field_read(p_type)
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

	p_obj[3] = x_mksatom(NULL, X_OBJ_FLAG_NONE, mock_fn);
	p_obj[2] = x_mksatom(NULL, X_OBJ_FLAG_NONE, mock_str);
	p_obj[1] = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_obj[3], NULL);

	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE,
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


	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, NULL);
	p_obj[3] = x_mksatom(p_base, X_OBJ_FLAG_NONE, mock_fn);
	p_obj[2] = x_mksatom(p_base, X_OBJ_FLAG_NONE, mock_str);
	p_obj[1] = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj[3], p_base);

	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE,
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


	p_base = x_base_ts_make(NULL, NULL);

	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE,
		x_mksatom(p_base, X_OBJ_FLAG_NONE, mock_str),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, mock_fn), p_base)
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
	p_fn = x_mksatom(NULL, X_OBJ_FLAG_NONE, type_write);
	x_type_field_write(x_obj_type(p_obj)) = p_fn;

	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_obj, NULL);

	p_ret = x_type_write(NULL, p_args);

	_it_should("call the object type's write primitive",
		p_obj == p_ret
		&& 1 == type_write_call_count
	);

	return NULL;
}

static char *test_type_write_null(void)
{
	x_obj_t *p_obj, *p_args, *p_ret;

	p_obj = x_mkatom(NULL, NULL);
	/* Explicitly clear the write field to test the NULL path */
	x_type_field_write(x_obj_type(p_obj)) = NULL;
	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_obj, NULL);

	p_ret = x_type_write(NULL, p_args);

	_it_should("return NULL when write fn is nil",
		p_ret == NULL
	);

	return NULL;
}

static int type_error_call_count = 0;
x_obj_t *type_error_fn(x_obj_t *p_base, x_obj_t *p_args)
{
	++type_error_call_count;

	return x_firstobj(p_args);
}

static char *test_type_error(void)
{
	x_obj_t *p_obj, *p_fn, *p_args, *p_ret;

	/* With error fn set */
	p_obj = x_mkatom(NULL, NULL);
	p_fn = x_mksatom(NULL, X_OBJ_FLAG_NONE, type_error_fn);
	x_type_field_error(x_obj_type(p_obj)) = p_fn;

	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_obj, NULL);

	type_error_call_count = 0;
	p_ret = x_type_error(NULL, p_args);

	_it_should("call the error fn",
		p_obj == p_ret
		&& 1 == type_error_call_count
	);

	/* Without error fn (NULL) */
	p_obj = x_mkatom(NULL, NULL);
	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_obj, NULL);

	p_ret = x_type_error(NULL, p_args);

	_it_should("return NULL when error fn is nil",
		p_ret == NULL
	);

	return NULL;
}

static char *test_type_prim_type_name(void)
{
	x_obj_t *p_base, *p_obj, *p_args, *p_ret;

	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);

	/* nil args returns NULL */
	p_ret = x_type_prim_type_name(p_base, NULL);
	_it_should("return NULL for nil args", p_ret == NULL);

	/* nil object returns NULL */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, NULL);
	p_ret = x_type_prim_type_name(p_base, p_args);
	_it_should("return NULL for nil object", p_ret == NULL);

	/* satom returns its type directly */
	p_obj = x_mksatom(p_base, X_OBJ_FLAG_NONE, 42);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj, NULL);
	p_ret = x_type_prim_type_name(p_base, p_args);
	_it_should("return type pointer for satom",
		p_ret == x_obj_type(p_obj));

	/* spair returns its type directly */
	p_obj = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, NULL);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj, NULL);
	p_ret = x_type_prim_type_name(p_base, p_args);
	_it_should("return type pointer for spair",
		p_ret == x_obj_type(p_obj));

	/* heap atom with NULL type returns type (NULL) */
	p_obj = x_mkatom(p_base, NULL);
	x_obj_type(p_obj) = NULL;
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj, NULL);
	p_ret = x_type_prim_type_name(p_base, p_args);
	_it_should("return NULL type for obj with NULL type",
		p_ret == NULL);

	/* typed object returns the name field */
	p_obj = x_mkatom(p_base, NULL);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj, NULL);
	p_ret = x_type_prim_type_name(p_base, p_args);
	_it_should("return name field for typed obj",
		p_ret == x_type_field_name(x_obj_type(p_obj)));

	/* typed object with nil name returns NULL */
	{
		struct x_type_t type_desc;
		x_obj_t *p_type;

		memset(&type_desc, 0, sizeof(type_desc));
		p_type = x_type_struct_make(p_base, type_desc);
		p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE,
			X_OBJ_LENGTH_ATOM, 0);
		p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj, NULL);
		p_ret = x_type_prim_type_name(p_base, p_args);
		_it_should("return NULL for typed obj with nil name",
			p_ret == NULL);
	}

	test_cleanup(p_base);

	return NULL;
}

static char *test_type_prim_units(void)
{
	x_obj_t *p_base, *p_obj, *p_args, *p_ret;

	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);

	/* nil args returns NULL */
	p_ret = x_type_prim_units(p_base, NULL);
	_it_should("return NULL for nil args", p_ret == NULL);

	/* nil object returns NULL */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, NULL);
	p_ret = x_type_prim_units(p_base, p_args);
	_it_should("return NULL for nil object", p_ret == NULL);

	/* spair goes to pair units */
	p_obj = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, NULL);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj, NULL);
	p_ret = x_type_prim_units(p_base, p_args);
	_it_should("return pair units for spair",
		p_ret != NULL && x_atomint(p_ret) == X_OBJ_UNITS_PAIR);

	/* satom goes to atom units */
	p_obj = x_mksatom(p_base, X_OBJ_FLAG_NONE, 42);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj, NULL);
	p_ret = x_type_prim_units(p_base, p_args);
	_it_should("return atom units for satom",
		p_ret != NULL && x_atomint(p_ret) == X_OBJ_UNITS_ATOM);

	/* typed object with nil units returns NULL */
	{
		struct x_type_t type_desc;
		x_obj_t *p_type;

		memset(&type_desc, 0, sizeof(type_desc));
		type_desc.p_name = x_mksatom(p_base, X_OBJ_FLAG_NONE, "TEST");
		p_type = x_type_struct_make(p_base, type_desc);
		p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE,
			X_OBJ_LENGTH_ATOM, 0);
		p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj, NULL);
		p_ret = x_type_prim_units(p_base, p_args);
		_it_should("return NULL for typed obj with nil units",
			p_ret == NULL);
	}

	/* typed object with units function */
	{
		struct x_type_t type_desc;
		x_obj_t *p_type;

		memset(&type_desc, 0, sizeof(type_desc));
		type_desc.p_name = x_mksatom(p_base, X_OBJ_FLAG_NONE, "TEST");
		type_desc.p_units = x_mksatom(p_base, X_OBJ_FLAG_NONE, mock_fn);
		p_type = x_type_struct_make(p_base, type_desc);
		p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE,
			X_OBJ_LENGTH_ATOM, 0);
		p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj, NULL);
		mock_fn_calls = 0;
		p_ret = x_type_prim_units(p_base, p_args);
		_it_should("call units fn for typed obj",
			1 == mock_fn_calls);
	}

	test_cleanup(p_base);

	return NULL;
}

static char *test_type_prim_length(void)
{
	x_obj_t *p_base, *p_obj, *p_args, *p_ret;

	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);

	/* nil args returns NULL */
	p_ret = x_type_prim_length(p_base, NULL);
	_it_should("return NULL for nil args", p_ret == NULL);

	/* nil object returns NULL */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, NULL);
	p_ret = x_type_prim_length(p_base, p_args);
	_it_should("return NULL for nil object", p_ret == NULL);

	/* spair goes to pair length */
	p_obj = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 1), x_mksatom(p_base, X_OBJ_FLAG_NONE, 2));
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj, NULL);
	p_ret = x_type_prim_length(p_base, p_args);
	_it_should("return pair length for spair",
		p_ret != NULL && x_atomint(p_ret) == X_OBJ_UNITS_PAIR);

	/* satom goes to atom length */
	p_obj = x_mksatom(p_base, X_OBJ_FLAG_NONE, 42);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj, NULL);
	p_ret = x_type_prim_length(p_base, p_args);
	_it_should("return atom length for satom",
		p_ret != NULL && x_atomint(p_ret) == X_OBJ_UNITS_ATOM);

	/* typed object with nil length returns NULL */
	{
		struct x_type_t type_desc;
		x_obj_t *p_type;

		memset(&type_desc, 0, sizeof(type_desc));
		type_desc.p_name = x_mksatom(p_base, X_OBJ_FLAG_NONE, "TEST");
		p_type = x_type_struct_make(p_base, type_desc);
		p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE,
			X_OBJ_LENGTH_ATOM, 0);
		p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj, NULL);
		p_ret = x_type_prim_length(p_base, p_args);
		_it_should("return NULL for typed obj with nil length",
			p_ret == NULL);
	}

	/* typed object with length function */
	{
		struct x_type_t type_desc;
		x_obj_t *p_type;

		memset(&type_desc, 0, sizeof(type_desc));
		type_desc.p_name = x_mksatom(p_base, X_OBJ_FLAG_NONE, "TEST");
		type_desc.p_length = x_mksatom(p_base, X_OBJ_FLAG_NONE, mock_fn);
		p_type = x_type_struct_make(p_base, type_desc);
		p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE,
			X_OBJ_LENGTH_ATOM, 0);
		p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj, NULL);
		mock_fn_calls = 0;
		p_ret = x_type_prim_length(p_base, p_args);
		_it_should("call length fn for typed obj",
			1 == mock_fn_calls);
	}

	test_cleanup(p_base);

	return NULL;
}

static char *test_type_heap_mark(void)
{
	x_obj_t *p_base, *p_obj, *p_ret;

	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);

	/* Path 1: base type object returns x_atomobj */
	p_obj = x_mksatom(p_base, X_OBJ_FLAG_NONE, 99);
	x_obj_type(p_obj) = (x_obj_t *)&x_type_base_obj;
	p_ret = x_type_heap_mark(p_base, p_obj, 0);
	_it_should("base type returns atomobj",
		p_ret == x_atomobj(p_obj));

	/* Path 2: NULL type returns NULL */
	p_obj = x_mksatom(p_base, X_OBJ_FLAG_NONE, 0);
	x_obj_type(p_obj) = NULL;
	p_ret = x_type_heap_mark(p_base, p_obj, 0);
	_it_should("NULL type returns NULL", p_ret == NULL);

	/* Path 3: satom type (not pair) returns NULL */
	p_obj = x_mksatom(p_base, X_OBJ_FLAG_NONE, 0);
	/* type is already x_type_atom_obj which is a satom */
	p_ret = x_type_heap_mark(p_base, p_obj, 0);
	_it_should("non-pair type returns NULL", p_ret == NULL);

	test_cleanup(p_base);

	/* Path 4: typed object with custom mark fn */
	{
		x_obj_t *p_type;
		struct x_type_t type_desc;

		p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);

		memset(&type_desc, 0, sizeof(type_desc));
		type_desc.p_mark = x_mksatom(p_base, X_OBJ_FLAG_NONE, mock_fn);
		type_desc.p_units = x_mksatom(p_base, X_OBJ_FLAG_NONE, 2);

		p_type = x_type_struct_make(p_base, type_desc);
		p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE, 2,
			x_mksatom(p_base, X_OBJ_FLAG_NONE, 10), x_mksatom(p_base, X_OBJ_FLAG_NONE, 20));

		mock_fn_calls = 0;
		p_ret = x_type_heap_mark(p_base, p_obj, 0);
		_it_should("custom mark: calls mark fn and returns NULL",
			p_ret == NULL && mock_fn_calls == 1);

		test_cleanup(p_base);
	}

	/* Path 5: typed object with units but no mark fn — generic traversal */
	{
		x_obj_t *p_type, *p_slot0, *p_slot1;
		struct x_type_t type_desc;

		p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);

		memset(&type_desc, 0, sizeof(type_desc));
		type_desc.p_units = x_mksatom(p_base, X_OBJ_FLAG_NONE, 2);

		p_type = x_type_struct_make(p_base, type_desc);
		p_slot0 = x_mksatom(p_base, X_OBJ_FLAG_NONE, 10);
		p_slot1 = x_mksatom(p_base, X_OBJ_FLAG_NONE, 20);
		p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE, 2,
			p_slot0, p_slot1);

		p_ret = x_type_heap_mark(p_base, p_obj, 0);
		_it_should("generic traversal: returns last slot",
			p_ret == p_slot1);

		test_cleanup(p_base);
	}

	return NULL;
}

static int type_free_call_count = 0;
x_obj_t *type_free_fn(x_obj_t *p_base, x_obj_t *p_args)
{
	++type_free_call_count;

	return NULL;
}

static char *test_type_heap_free(void)
{
	x_obj_t *p_base, *p_obj, *p_type;
	struct x_type_t type_desc;

	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);

	/* With free fn */
	memset(&type_desc, 0, sizeof(type_desc));
	type_desc.p_free = x_mksatom(p_base, X_OBJ_FLAG_NONE, type_free_fn);

	p_type = x_type_struct_make(p_base, type_desc);
	p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE, 1, NULL);

	type_free_call_count = 0;
	x_type_heap_free(p_base, p_obj);
	_it_should("call the free fn",
		1 == type_free_call_count);

	/* Without free fn */
	memset(&type_desc, 0, sizeof(type_desc));
	p_type = x_type_struct_make(p_base, type_desc);
	p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE, 1, NULL);

	type_free_call_count = 0;
	x_type_heap_free(p_base, p_obj);
	_it_should("not call free fn when NULL",
		0 == type_free_call_count);

	/* NULL type — no crash */
	p_obj = x_mksatom(p_base, X_OBJ_FLAG_NONE, 0);
	x_obj_type(p_obj) = NULL;
	x_type_heap_free(p_base, p_obj);
	_it_should("handle NULL type gracefully", 1);

	/* satom type (not pair) — no crash */
	p_obj = x_mksatom(p_base, X_OBJ_FLAG_NONE, 0);
	x_type_heap_free(p_base, p_obj);
	_it_should("handle non-pair type gracefully", 1);

	test_cleanup(p_base);

	return NULL;
}

static char *run_tests() {
	_run_test(test_type_types);
	_run_test(test_type_units);
	_run_test(test_type_struct_make);
	_run_test(test_type_struct_get);
	_run_test(test_type_write);
	_run_test(test_type_write_null);
	_run_test(test_type_error);
	_run_test(test_type_prim_type_name);
	_run_test(test_type_prim_units);
	_run_test(test_type_prim_length);
	_run_test(test_type_heap_mark);
	_run_test(test_type_heap_free);

	return NULL;
}
