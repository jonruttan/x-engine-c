/*
 * # Unit Tests: *x-obj*
 */

#include "test-runner.h"

/* Include Garbage Collection structures. */
#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "src/x-sys.c"
#include "src/x-lib.c"
#include "src/x.c"
#include "src/x-obj.c"
#include "src/x-obj/prim.c"

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
 * ## Test Helpers
 */
#define nil			p_base
#define pair(X,Y)	(x_mkspair(p_base, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, (X)))


/*
 * ## Test Runners
 */
x_obj_t *_test_make_type(x_obj_t *p_base)
{
	return
		pair(atom("ONE"),
		pair(pair(atom(NULL), atom(NULL)),
		pair(pair(atom(NULL), pair(atom(NULL), pair(atom(NULL), atom(NULL)))),
		pair(pair(atom(NULL), pair(atom(NULL), atom(NULL))),
		pair(pair(atom(NULL), pair(atom(NULL), atom(NULL))),
	nil)))));
}

static char *test_obj_prim_make(void)
{
	x_obj_t *p_base, *p_obj, *p_flags, *p_vals[2], *p_args[4], *p_ret,
		*p_type;

	helper_alloc_reset();

	p_ret = x_obj_prim_make(NULL, NULL);
	_it_should("return NULL when the arguments are NULL",
		NULL == p_ret
	);


	p_obj = x_mksatom(NULL, 0);
	p_flags = x_mksatom(NULL, 0xf0);
	p_vals[0] = x_mksatom(NULL, 10);
	p_args[2] = x_mkspair(NULL, p_vals[0], NULL);
	p_args[1] = x_mkspair(NULL, p_flags, p_args[2]);
	p_args[0] = x_mkspair(NULL, p_obj, p_args[1]);
	p_ret = x_obj_prim_make(NULL, p_args[0]);
	_it_should("return a new object with the same type as the atom "
		"and flags and value from the arguments when base is NULL",
		p_ret != p_obj
		&& x_obj_type(p_ret) == x_obj_type(p_obj)
		&& x_atomint(p_flags) == x_obj_flags(p_ret)
		&& x_atomint(p_vals[0]) == x_atomint(p_ret)
	);
	x_obj_free(p_obj);
	x_obj_free(p_flags);
	x_obj_free(p_vals[0]);
	x_obj_free(p_args[2]);
	x_obj_free(p_args[1]);
	x_obj_free(p_args[0]);
	x_obj_free(p_ret);

	p_obj = x_mkspair(NULL, 0, 0);
	p_flags = x_mksatom(NULL, 0xf0);
	p_vals[0] = x_mksatom(NULL, 10);
	p_vals[1] = x_mksatom(NULL, 20);
	p_args[3] = x_mkspair(NULL, p_vals[1], NULL);
	p_args[2] = x_mkspair(NULL, p_vals[0], p_args[3]);
	p_args[1] = x_mkspair(NULL, p_flags, p_args[2]);
	p_args[0] = x_mkspair(NULL, p_obj, p_args[1]);
	p_ret = x_obj_prim_make(NULL, p_args[0]);
	_it_should("return a new object with the same type as the pair "
		"and flags and values from the arguments when base is NULL",
		p_ret != p_obj
		&& x_obj_type(p_ret) == x_obj_type(p_obj)
		&& x_atomint(p_flags) == x_obj_flags(p_ret)
		&& x_atomint(p_vals[0]) == x_firstint(p_ret)
		&& x_atomint(p_vals[1]) == x_restint(p_ret)
	);
	x_obj_free(p_obj);
	x_obj_free(p_flags);
	x_obj_free(p_vals[1]);
	x_obj_free(p_vals[0]);
	x_obj_free(p_args[3]);
	x_obj_free(p_args[2]);
	x_obj_free(p_args[1]);
	x_obj_free(p_args[0]);
	x_obj_free(p_ret);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_mksatom(p_base, 0);
	p_flags = x_mksatom(p_base, 0xf0);
	p_vals[0] = x_mksatom(p_base, 10);
	p_args[2] = x_mkspair(p_base, p_vals[0], p_base);
	p_args[1] = x_mkspair(p_base, p_flags, p_args[2]);
	p_args[0] = x_mkspair(p_base, p_obj, p_args[1]);
	p_ret = x_obj_prim_make(p_base, p_args[0]);
	p_ret = x_obj_prim_make(p_base, p_args[0]);
	_it_should("return a new object with the same type as the atom "
		"and flags and value from the arguments when base is empty",
		p_ret != p_obj
		&& x_obj_type(p_ret) == x_obj_type(p_obj)
		&& x_atomint(p_flags) == x_obj_flags(p_ret)
		&& x_atomint(p_vals[0]) == x_atomint(p_ret)
	);
	x_obj_free(p_obj);
	x_obj_free(p_flags);
	x_obj_free(p_vals[0]);
	x_obj_free(p_args[2]);
	x_obj_free(p_args[1]);
	x_obj_free(p_args[0]);
	x_obj_free(p_ret);
	x_obj_free(p_base);

	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkspair(NULL, 0, 0);
	p_flags = x_mksatom(p_base, 0xf0);
	p_vals[0] = x_mksatom(p_base, 10);
	p_vals[1] = x_mksatom(p_base, 20);
	p_args[3] = x_mkspair(p_base, p_vals[1], p_base);
	p_args[2] = x_mkspair(p_base, p_vals[0], p_args[3]);
	p_args[1] = x_mkspair(p_base, p_flags, p_args[2]);
	p_args[0] = x_mkspair(p_base, p_obj, p_args[1]);
	p_ret = x_obj_prim_make(p_base, p_args[0]);
	_it_should("return a new object with the same type as the pair "
		"and flags and values from the arguments when base is empty",
		p_ret != p_obj
		&& x_obj_type(p_ret) == x_obj_type(p_obj)
		&& x_atomint(p_flags) == x_obj_flags(p_ret)
		&& x_atomint(p_vals[0]) == x_firstint(p_ret)
		&& x_atomint(p_vals[1]) == x_restint(p_ret)
	);
	x_obj_free(p_obj);
	x_obj_free(p_flags);
	x_obj_free(p_vals[1]);
	x_obj_free(p_vals[0]);
	x_obj_free(p_args[3]);
	x_obj_free(p_args[2]);
	x_obj_free(p_args[1]);
	x_obj_free(p_args[0]);
	x_obj_free(p_ret);
	x_obj_free(p_base);

	_mark_incomplete();

	p_base = x_mksatom(NULL, 0);
	p_type = _test_make_type(p_base);
	p_obj = x_obj_alloc(p_base, p_type, 0, 0);
	p_flags = x_mksatom(p_base, 0xf0);
	p_vals[0] = x_mksatom(p_base, 10);
	p_vals[1] = x_mksatom(p_base, 20);
	p_args[3] = x_mkspair(p_base, p_vals[1], p_base);
	p_args[2] = x_mkspair(p_base, p_vals[0], p_args[3]);
	p_args[1] = x_mkspair(p_base, p_flags, p_args[2]);
	p_args[0] = x_mkspair(p_base, p_obj, p_args[1]);
	p_ret = x_obj_prim_make(p_base, p_args[0]);
	_it_should("return a new object with the same type as the object "
		"when type object is set",
		p_obj != p_ret
		&& x_obj_type(p_obj) == x_obj_type(p_ret)
	);
	x_obj_free(p_obj);
	x_obj_free(p_flags);
	x_obj_free(p_vals[1]);
	x_obj_free(p_vals[0]);
	x_obj_free(p_args[3]);
	x_obj_free(p_args[2]);
	x_obj_free(p_args[1]);
	x_obj_free(p_args[0]);
	x_obj_free(p_ret);
	x_obj_free(p_base);

	return NULL;
}

static char *test_obj_prim_free(void)
{
	return NULL;
}

static char *test_obj_prim_clone(void)
{
	return NULL;
}

static char *test_obj_prim_dump(void)
{
	return NULL;
}


unsigned int _test_prim_fn_calls = 0;
x_obj_t *_test_prim_fn(x_obj_t *p_base, x_obj_t *p_args)
{
	_test_prim_fn_calls++;

	return p_base;
}

x_obj_t *_test_type_prim_call(x_obj_t *p_base, x_obj_t *p_args)
{
	return (*x_atomfn(x_firstobj(p_args)))(p_base, x_restobj(p_args));
}

static char *test_obj_prim_call(void)
{
	x_obj_t *p_base, *p_type, *p_obj, *p_args, *p_ret;

	helper_alloc_reset();

	p_base = x_mksatom(NULL, 0);
	p_type = _test_make_type(p_base);
	p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE, X_OBJ_LENGTH_ATOM, _test_prim_fn);
	p_args = x_mkspair(p_base, p_obj, p_base);

	_test_prim_fn_calls = 0;
	p_ret = x_obj_prim_call(p_base, p_args);
	_it_should("not call the test function and return p_base",
		p_base == p_ret
		&& 0 == _test_prim_fn_calls
	);

	x_firstptr(x_type_field_call(p_type)) = _test_type_prim_call;

	_test_prim_fn_calls = 0;
	p_ret = x_obj_prim_call(p_base, p_args);
	_it_should("call the test function and return p_base",
		p_base == p_ret
		&& 1 == _test_prim_fn_calls
	);


	return NULL;
}

static char *test_obj_prim_eval(void)
{
	return NULL;
}

static char *test_obj_prim_convert(void)
{
	return NULL;
}


static char *test_obj_prim_identify(void)
{
	return NULL;
}

static char *test_obj_prim_read(void)
{
	return NULL;
}

static char *test_obj_prim_write(void)
{
	return NULL;
}

static char *run_tests() {
	_run_test(test_obj_prim_make);
	_run_test(test_obj_prim_free);
	_run_test(test_obj_prim_clone);
	_run_test(test_obj_prim_dump);

	_run_test(test_obj_prim_call);
	_run_test(test_obj_prim_eval);
	_run_test(test_obj_prim_convert);

	_run_test(test_obj_prim_identify);
	_run_test(test_obj_prim_read);
	_run_test(test_obj_prim_write);

	return NULL;
}
