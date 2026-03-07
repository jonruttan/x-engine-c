/*
 * # Unit Tests: *x-type/prim*
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
#include "src/x-type/atom.c"
/*#include "src/x-type/pair.c"*/
#include "src/x-type/char.c"
#include "src/x-sexp/char.c"
#include "src/x-type/prim.c"
#include "src/x-type/str.c"
#include "src/x-sexp/str.c"
#include "src/x-type/buffer.c"

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

#define nil			p_base
#define pair(X,Y)	(x_mkspair(p_base, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, (X)))

x_obj_t *test_prim_fn(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_count = x_firstobj(p_args);

	x_firstint(p_count)++;

	return p_count;
}

static char *test_obj_type_isprim(void)
{
	x_obj_t *p_obj;

	helper_alloc_reset();

	p_obj = x_mkprim(NULL, 0);
	_it_should("return true when object is a primitive",
		1 == x_obj_type_isprim(NULL, p_obj)
	);
	x_obj_free(p_obj);

	p_obj = x_mkatom(NULL, 0);
	_it_should("return false when object is not a primative",
		0 == x_obj_type_isprim(NULL, p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}

static char *test_primval(void)
{
	x_obj_t *p_call;
	x_prim_fn p_fn;

	p_call = x_mkprim(NULL, test_prim_fn);

	p_fn = x_primval(p_call);
	_it_should("return the Primitive's value", p_fn == test_prim_fn);

	x_sys_free(p_call);

	return NULL;
}

static char *test_mkprim(void)
{
	x_obj_t *p_base, *p_obj;

	p_obj = x_mkprim(NULL, test_prim_fn);
	_it_should("make a Primitive object and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_isprim(NULL, p_obj)
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj)
		&& test_prim_fn == x_primval(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkprim(p_base, test_prim_fn);
	_it_should("make a Primitive object, attach it to the Base object, and set its value",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_isprim(p_base, p_obj)
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj)
		&& p_obj == x_obj_gc(p_base)
		&& test_prim_fn == x_primval(p_obj)
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_mkfprim(void)
{
	x_obj_t *p_base, *p_obj;
	x_obj_flag_t flags = rand();

	p_obj = x_mkfprim(NULL, flags, test_prim_fn);
	_it_should("make a Primitive object and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_isprim(NULL, p_obj)
		&& flags == x_obj_flags(p_obj)
		&& test_prim_fn == x_primval(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkfprim(p_base, flags, test_prim_fn);
	_it_should("make a Primitive object, attach it to the Base object, and set its value",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_isprim(p_base, p_obj)
		&& flags == x_obj_flags(p_obj)
		&& p_obj == x_obj_gc(p_base)
		&& test_prim_fn == x_primval(p_obj)
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_make_prim(void)
{
	x_obj_t *p_base, *p_obj;
	x_obj_flag_t flags = rand();

	p_obj = x_make_prim(NULL, flags, test_prim_fn);
	_it_should("make a Primitive object and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_isprim(NULL, p_obj)
		&& flags == x_obj_flags(p_obj)
		&& test_prim_fn == x_primval(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_make_prim(p_base, flags, test_prim_fn);
	_it_should("make a Primitive object, attach it to the Base object, and set its value",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_isprim(p_base, p_obj)
		&& flags == x_obj_flags(p_obj)
		&& p_obj == x_obj_gc(p_base)
		&& test_prim_fn == x_primval(p_obj)
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_type_prim_register(void)
{
	x_obj_t *p_base, *p_type;

	p_base = x_base_make(NULL, NULL);

	p_type = x_type_prim_register(p_base, p_base);
	_it_should("return the Primitive type object",
		0 == x_lib_strcmp(X_TYPE_PRIM_NAME, x_strval(x_type_field_name(p_type)))
	);
	_it_should("add the Primitive type to the Type alist",
		p_type == x_firstobj(x_base_field_type_alist(p_base))
	);

	x_sys_free(p_base);

	return NULL;
}

static char *test_type_prim_struct(void)
{
	x_obj_t *p_base, *p_type;

	helper_alloc_reset();

	p_base = x_mksatom(NULL, 0);
	p_type = x_type_prim_struct(p_base, p_base);
	_it_should("return Primitive Type list",
		! x_obj_isnil(p_base, p_type)
		&& 0 == strcmp(X_TYPE_PRIM_NAME, x_atomstr(x_type_field_name(p_type)))
	);

	_it_should("contain the Name object",
		x_type_prim_name == x_type_field_name(p_type)
	);

	_it_should("set the Data object to nil",
		NULL == x_type_field_data(p_type)
	);

	_it_should("set the Make primitive",
		x_type_prim_make == x_primval(x_type_field_make(p_type))
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

	_it_should("set the Call primitive",
		x_type_prim_call == x_primval(x_type_field_call(p_type))
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

static char *test_base_alist_assoc(void)
{
	x_obj_t *p_base, *p_type;

	helper_alloc_reset();

	p_base = x_base_make(NULL, NULL);
	x_base_type_alist_extend(p_base, x_mkspair(p_base, x_type_prim_name, atom(1)));

	p_type = x_base_type_alist_assoc(p_base, x_mkspair(p_base, x_type_prim_name, NULL));
	_it_should("find the type in the Type alist and return its properties",
		x_type_prim_name == x_firstobj(p_type)
	);

	return NULL;
}

static char *test_type_prim_make(void)
{
	x_obj_t *p_base, *p_fn, *p_args, *p_obj[2];

	helper_alloc_reset();

	/* NULL p_base object */
	p_fn = x_mksatom(NULL, test_prim_fn);
	p_args = x_mkspair(NULL, p_fn, NULL);

	p_obj[0] = x_type_prim_make(NULL, p_args);
	_it_should("make a Primitive object and set its value",
		! x_obj_isnil(NULL, p_obj[0])
		&& x_obj_type_isprim(NULL, p_obj[0])
		&& test_prim_fn == x_primval(p_obj[0])
	);

	p_obj[1] = x_type_prim_make(NULL, p_args);
	_it_should("make a second Primitive object",
		! x_obj_isnil(NULL, p_obj[1])
		&& x_obj_type_isprim(NULL, p_obj[1])
	);

	_it_should("have not have returned the same type object for both objects",
		x_obj_type(p_obj[0]) != x_obj_type(p_obj[1])
	);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(p_args);
	x_sys_free(p_fn);


	/* Empty p_base object */
	p_base = x_mksatom(NULL, NULL);
	p_fn = x_mksatom(p_base, test_prim_fn);
	p_args = x_mkspair(p_base, p_fn, NULL);

	p_obj[0] = x_type_prim_make(p_base, p_args);
	_it_should("make a Primitive object",
		! x_obj_isnil(NULL, p_obj[0])
		&& x_obj_type_isprim(p_base, p_obj[0])
	);

	p_obj[1] = x_type_prim_make(p_base, p_args);
	_it_should("make a second Primitive object",
		! x_obj_isnil(p_base, p_obj[1])
		&& x_obj_type_isprim(p_base, p_obj[1])
	);

	_it_should("not have returned the same type object for both objects",
		x_obj_type(p_obj[0]) != x_obj_type(p_obj[1])
	);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(p_args);
	x_sys_free(p_fn);
	x_sys_free(p_base);


	/* With p_base object */
	p_base = x_base_make(NULL, NULL);
	p_fn = x_mksatom(p_base, test_prim_fn);
	p_args = x_mkspair(p_base, p_fn, NULL);

	p_obj[0] = x_type_prim_make(p_base, p_args);
	_it_should("make a Primitive object with a base object",
		! x_obj_isnil(NULL, p_obj[0])
		&& x_obj_type_isprim(p_base, p_obj[0])
	);

	p_obj[1] = x_type_prim_make(p_base, p_args);
	_it_should("make a second Primitive object a base object",
		! x_obj_isnil(p_base, p_obj[1])
		&& x_obj_type_isprim(p_base, p_obj[1])
	);
	_it_should("have returned the same type object for both objects",
		x_obj_type(p_obj[0]) == x_obj_type(p_obj[1])
	);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(p_args);
	x_sys_free(p_fn);
	x_sys_free(p_base);


	return NULL;
}

static char *test_type_prim_call(void)
{
	x_obj_t *p_call, *p_count, *p_args, *p_obj;

	helper_alloc_reset();

	p_call = x_mksatom(NULL, test_prim_fn);
	p_count = x_mksatom(NULL, 0);
	p_args = x_mkspair(NULL, p_call, x_mkspair(NULL, p_count, NULL));

	p_obj = x_type_prim_call(NULL, p_args);
	_it_should("call the test function and return the incremented argument",
		p_count == p_obj
		&& 1 == x_firstint(p_count)
	);

	x_sys_free(x_1(p_args));
	x_sys_free(p_args);
	x_sys_free(p_count);
	x_sys_free(p_call);

	return NULL;
}

static char *run_tests() {
	_run_test(test_obj_type_isprim);
	_run_test(test_primval);
	_run_test(test_mkprim);
	_run_test(test_mkfprim);
	_run_test(test_make_prim);
	_run_test(test_type_prim_register);
	_run_test(test_type_prim_struct);
	_run_test(test_base_alist_assoc);
	_run_test(test_type_prim_make);
	_run_test(test_type_prim_call);

	return NULL;
}
