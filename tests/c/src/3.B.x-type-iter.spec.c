/*
 * # Unit Tests: *x-type/pair*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"
#include "x-obj.h"
#include "x-type/iter.h"
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
#include "src/x-type/prim.c"
#include "src/x-type/atom.c"
#include "src/x-type/pair.c"
#include "src/x-type/iter.c"

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
#define STUB_X_SEXP_PAIR_WRITE
#define STUB_X_PRIM_SHADOW
#define STUB_X_PROCEDURE_APPLY
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

x_obj_t *list_iter_prim(x_obj_t *p_base, x_obj_t *p_args)
{
	/* Spair callable: p_args = (self . actual_args).
	 * Skip self to get the iterator. */
	x_obj_t *p_iter = x_firstobj(x_restobj(p_args)),
		*p_obj = x_firstobj(x_iterval(p_iter));

	x_iterval(p_iter) = x_restobj(x_iterval(p_iter));

	return p_obj;
}


/*
 * ## Test Runners
 */

#define nil			NULL
#define pair(X,Y)	(x_mkspair(p_base, X_OBJ_FLAG_NONE, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, X_OBJ_FLAG_NONE, (X)))

static char *test_obj_type_isiter(void)
{
	x_obj_t *p_base, *p_obj;

	p_base = x_base_ts_make(NULL, NULL);

	p_obj = x_mkiter(p_base, 0, 0);
	_it_should("return true when object is an Iter",
		1 == x_obj_type_isiter(p_base, p_obj)
	);

	p_obj = x_mksatom(p_base, X_OBJ_FLAG_NONE, 0);
	_it_should("return false when object is not an Iter",
		0 == x_obj_type_isiter(p_base, p_obj)
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_iterprim(void)
{
	x_obj_t *p_obj;
	x_int_t i = rand(), j = rand();

	p_obj = x_mkiter(NULL, (void *)i, (void *)j);

	_it_should("return the Iter's Prim value", (void *)i == x_iterprim(p_obj));

	x_sys_free(p_obj);

	return NULL;
}

static char *test_iterval(void)
{
	x_obj_t *p_obj;
	x_int_t i = rand(), j = rand();

	p_obj = x_mkiter(NULL, (void *)i, (void *)j);

	_it_should("return the Iter's value", (void *)j == x_iterval(p_obj));

	x_sys_free(p_obj);

	return NULL;
}

static char *test_iterempty(void)
{
	x_obj_t *p_obj;

	helper_alloc_reset();

	p_obj = x_mkiter(NULL, NULL, NULL);
	_it_should("return true when Iter is empty",
		1 == x_iterempty(NULL, p_obj)
	);
	x_obj_free(NULL, p_obj);

	p_obj = x_mkiter(NULL, NULL, (void *)1);
	_it_should("return false when Iter is not empty",
		0 == x_iterempty(NULL, p_obj)
	);
	x_obj_free(NULL, p_obj);

	return NULL;
}

static char *test_mkiter(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t i = rand(), j = rand();

	p_base = x_base_ts_make(NULL, NULL);

	p_obj = x_mkiter(p_base, (void *)i, (void *)j);
	_it_should("make an Iter object and set its first and rest values",
		p_obj != NULL
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj)
		&& (void *)i == x_iterprim(p_obj)
		&& (void *)j == x_iterval(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_base_ts_make(NULL, NULL);
	p_obj = x_mkiter(p_base, (void *)i, (void *)j);
	_it_should("make an Iter object and set its first and rest values",
		p_obj != NULL
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj)
		&& (void *)i == x_iterprim(p_obj)
		&& (void *)j == x_iterval(p_obj)
	);

	x_sys_free(p_obj);

	return NULL;
}

static char *test_mkfiter(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t i = rand(), j = rand();
	x_obj_flag_t flags = rand();

	p_base = x_base_ts_make(NULL, NULL);

	p_obj = x_mkfiter(p_base, flags, (void *)i, (void *)j);
	_it_should("make an Iter object and set its first and rest values",
		p_obj != NULL
		&& flags == (x_obj_flag_t)x_obj_flags(p_obj)
		&& (void *)i == x_iterprim(p_obj)
		&& (void *)j == x_iterval(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_base_ts_make(NULL, NULL);
	p_obj = x_mkfiter(p_base, flags, (void *)i, (void *)j);
	_it_should("make an Iter object and set its first and rest values",
		p_obj != NULL
		&& flags == (x_obj_flag_t)x_obj_flags(p_obj)
		&& (void *)i == x_iterprim(p_obj)
		&& (void *)j == x_iterval(p_obj)
	);

	x_sys_free(p_obj);

	return NULL;
}

static char *test_make_iter(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t i = rand(), j = rand();
	x_obj_flag_t flags = rand();

	p_base = x_base_ts_make(NULL, NULL);

	p_obj = x_make_iter(p_base, flags, (void *)i, (void *)j);
	_it_should("make an Iter object and set its first and rest values",
		p_obj != NULL
		&& flags == (x_obj_flag_t)x_obj_flags(p_obj)
		&& (void *)i == x_iterprim(p_obj)
		&& (void *)j == x_iterval(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_base_ts_make(NULL, NULL);
	p_obj = x_make_iter(p_base, flags, (void *)i, (void *)j);
	_it_should("make an Iter object and set its first and rest values",
		p_obj != NULL
		&& flags == (x_obj_flag_t)x_obj_flags(p_obj)
		&& (void *)i == x_iterprim(p_obj)
		&& (void *)j == x_iterval(p_obj)
	);

	x_sys_free(p_obj);

	return NULL;
}

static char *test_type_iter_register(void)
{
	x_obj_t *p_base, *p_type;

	p_base = x_base_ts_make(NULL, NULL);

	p_type = x_type_iter_register(p_base, p_base);
	_it_should("return the Iter type object",
		0 == x_lib_strcmp(X_TYPE_ITER_NAME, x_atomstr(x_type_field_name(p_type)))
	);
	_it_should("add the Iter type to the Type alist",
		p_type == x_restobj(x_firstobj(x_firstobj(x_base_field_type_alist(p_base))))
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_type_iter_struct(void)
{
	x_obj_t *p_base, *p_type;

	helper_alloc_reset();

	p_base = x_base_ts_make(NULL, NULL);
	p_type = x_type_iter_struct(p_base, p_base);
	_it_should("return Iter Type list",
		! x_obj_isnil(p_base, p_type)
		&& 0 == strcmp(X_TYPE_ITER_NAME, x_atomstr(x_type_field_name(p_type)))
	);

	_it_should("contain the Name object",
		x_type_iter_name == x_type_field_name(p_type)
	);

	_it_should("set the Data object to nil",
		NULL == x_type_field_data(p_type)
	);

	_it_should("set the Make primitive",
		x_type_iter_make_prim == x_type_field_make(p_type)
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
		x_type_iter_write_prim == x_type_field_write(p_type)
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_base_alist_assoc(void)
{
	x_obj_t *p_base, *p_type;

	helper_alloc_reset();

	p_base = x_base_ts_make(NULL, NULL);
	x_base_type_alist_extend(p_base, x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkspair(p_base, X_OBJ_FLAG_NONE, x_type_iter_name, NULL), atom(1)));

	p_type = x_base_type_alist_assoc(p_base, x_mkspair(p_base, X_OBJ_FLAG_NONE, x_type_iter_name, NULL));
	_it_should("find the type in the Type alist and return its properties",
		x_type_iter_name == x_type_field_name(p_type)
	);

	return NULL;
}

static char *test_type_iter_make(void)
{
	x_obj_t *p_base, *p_iter, *p_args, *p_obj[2];

	helper_alloc_reset();

	/* NULL p_base object */
	p_iter = x_mkspair(NULL, X_OBJ_FLAG_NONE, rand(), rand());
	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_iter, NULL);

	p_obj[0] = x_type_iter_make(NULL, p_args);
	_it_should("make a Iter object and set its value",
		p_obj[0] != NULL
		&& x_firstobj(p_iter) == x_firstobj(p_obj[0])
		&& x_restobj(p_iter) == x_restobj(p_obj[0])
	);

	p_obj[1] = x_type_iter_make(NULL, p_args);
	_it_should("make a second Iter object",
		p_obj[1] != NULL
	);

	_it_should("have not have returned the same type object for both objects",
		x_obj_type(p_obj[0]) != x_obj_type(p_obj[1])
	);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(p_args);
	x_sys_free(p_iter);


	/* Empty p_base object */
	p_base = x_base_ts_make(NULL, NULL);
	p_iter = x_mkspair(p_base, X_OBJ_FLAG_NONE, rand(), rand());
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_iter, NULL);

	p_obj[0] = x_type_iter_make(p_base, p_args);
	_it_should("make a Iter object",
		p_obj[0] != NULL
	);

	p_obj[1] = x_type_iter_make(p_base, p_args);
	_it_should("make a second Iter object",
		p_obj[1] != NULL
	);

	_it_should("not have returned the same type object for both objects",
		x_obj_type(p_obj[0]) == x_obj_type(p_obj[1])
	);

	test_cleanup(p_base);


	/* With p_base object */
	p_base = x_base_ts_make(NULL, NULL);
	p_iter = x_mkspair(p_base, X_OBJ_FLAG_NONE, rand(), rand());
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_iter, NULL);

	p_obj[0] = x_type_iter_make(p_base, p_args);
	_it_should("make an Iter object with a base object",
		p_obj[0] != NULL
	);

	p_obj[1] = x_type_iter_make(p_base, p_args);
	_it_should("make a second Iter object a base object",
		p_obj[1] != NULL
	);
	_it_should("have returned the same type object for both objects",
		x_obj_type(p_obj[0]) == x_obj_type(p_obj[1])
	);

	test_cleanup(p_base);


	return NULL;
}

static char *test_type_iter_next(void)
{
	x_obj_t *p_base, *p_list, *p_iter, *p_args, *p_obj;

	helper_alloc_reset();

	p_base = x_base_ts_make(NULL, NULL);
	p_list = pair(atom(1), pair(nil, pair(atom(3), nil)));
	p_iter = x_mkiter(p_base, x_mkprim(p_base, list_iter_prim), p_list);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_iter, NULL);

	p_obj = x_type_iter_next(p_base, p_args);
	_it_should("return the first item in the list", x_0(p_list) == p_obj);
	_it_should("not be empty", ! x_iterempty(p_base, p_iter));

	p_obj = x_type_iter_next(p_base, p_args);
	_it_should("return the next item in the list", x_01(p_list) == p_obj);
	_it_should("not be empty", ! x_iterempty(p_base, p_iter));

	p_obj = x_type_iter_next(p_base, p_args);
	_it_should("return the last item in the list", x_011(p_list) == p_obj);
	_it_should("be empty", x_iterempty(p_base, p_iter));

	return NULL;
}

static char *test_type_iter_write(void)
{
	x_obj_t *p_base, *p_iter, *p_args, *p_ret;
	x_char_t buf[64];

	helper_alloc_reset();
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDOUT] = buf;
	helper_file_reset();

	p_base = x_base_ts_make(NULL, NULL);
	p_iter = x_mkiter(p_base, NULL, NULL);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_iter, NULL);

	p_ret = x_type_iter_write(p_base, p_args);
	_it_should("return the iter object", p_iter == p_ret);
	_it_should("write iter representation",
		0 == strncmp(X_TYPE_ITER_WRITE_STR, buf, X_TYPE_ITER_WRITE_LEN));

	return NULL;
}

static char *test_type_iter_isempty_fn(void)
{
	x_obj_t *p_base, *p_iter, *p_args, *p_ret;

	helper_alloc_reset();

	p_base = x_base_ts_make(NULL, NULL);

	p_iter = x_mkiter(p_base, NULL, NULL);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_iter, NULL);
	p_ret = x_type_iter_isempty(p_base, p_args);
	_it_should("return base for empty iter", p_ret == p_base);

	p_iter = x_mkiter(p_base, NULL, (void *)1);
	x_firstobj(p_args) = p_iter;
	p_ret = x_type_iter_isempty(p_base, p_args);
	_it_should("return args for non-empty iter", p_ret == p_args);

	return NULL;
}

static char *run_tests() {
	_run_test(test_obj_type_isiter);
	_run_test(test_iterprim);
	_run_test(test_iterval);
	_run_test(test_iterempty);
	_run_test(test_mkiter);
	_run_test(test_mkfiter);
	_run_test(test_make_iter);
	_run_test(test_type_iter_register);
	_run_test(test_type_iter_struct);
	_run_test(test_base_alist_assoc);
	_run_test(test_type_iter_make);
	_run_test(test_type_iter_next);
	_run_test(test_type_iter_write);
	_run_test(test_type_iter_isempty_fn);

	return NULL;
}
