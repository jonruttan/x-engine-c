/*
 * # Unit Tests: *x-obj*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

/* Include Garbage Collection structures. */
#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-lib.c"
#include "ext/x-expr/src/x.c"
#include "ext/x-expr/src/x-obj.c"
#include "src/x-obj/prim.c"

#define STUB_X_PROCEDURE
#include "helper-stubs.c"

#include "ext/x-expr/tests/src/test-helper-system.c"

/*
 * ## Test Overhead
 */
static void _setup(void)
{
	helper_set_alloc(MEM_GUARANTEED);
	_buffer_index = -1;
}

static void _teardown(void)
{
}

/*
 * ## Test Helpers
 */
#define nil			p_base
#define pair(X,Y)	(x_mkspair(p_base, X_OBJ_FLAG_NONE, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, X_OBJ_FLAG_NONE, (X)))


/*
 * ## Test Runners
 */
x_obj_t *_test_make_type(x_obj_t *p_base)
{
	return
		/* name-stack */
		pair(pair(atom("ONE"), NULL),
		/* data-stack */
		pair(pair(atom(NULL), NULL),
		/* heap: (mark make free clone units length) — each stack-wrapped */
		pair(pair(pair(atom(NULL), NULL), pair(pair(atom(NULL), NULL),
			pair(pair(atom(NULL), NULL), pair(pair(atom(NULL), NULL),
			pair(pair(atom(NULL), NULL), pair(pair(atom(NULL), NULL),
			NULL)))))),
		/* proc: (call eval) */
		pair(pair(pair(atom(NULL), NULL), pair(pair(atom(NULL), NULL),
			NULL)),
		/* cvt: (from to) */
		pair(pair(pair(atom(NULL), NULL), pair(pair(atom(NULL), NULL),
			NULL)),
		/* io: (analyse delimit write display error) */
		pair(pair(pair(atom(NULL), NULL), pair(pair(atom(NULL), NULL),
			pair(pair(atom(NULL), NULL), pair(pair(atom(NULL), NULL),
			pair(pair(atom(NULL), NULL),
			NULL))))),
		NULL))))));
}

static char *test_obj_prim_make(void)
{
	x_obj_t *p_base, *p_obj, *p_flags, *p_vals[2], *p_args[4], *p_ret;

	helper_alloc_reset();

	p_ret = x_obj_prim_make(NULL, NULL);
	_it_should("return NULL when the arguments are NULL",
		NULL == p_ret
	);


	p_obj = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);
	p_flags = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0xf0);
	p_vals[0] = x_mksatom(NULL, X_OBJ_FLAG_NONE, 10);
	p_args[2] = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_vals[0], NULL);
	p_args[1] = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_flags, p_args[2]);
	p_args[0] = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_obj, p_args[1]);
	p_ret = x_obj_prim_make(NULL, p_args[0]);
	_it_should("return a new object with the same type as the atom "
		"and flags and value from the arguments when base is NULL",
		p_ret != p_obj
		&& x_obj_type(p_ret) == x_obj_type(p_obj)
		&& x_atomint(p_flags) == x_obj_flags(p_ret)
		&& x_atomint(p_vals[0]) == x_atomint(p_ret)
	);
	x_obj_free(NULL, p_obj);
	x_obj_free(NULL, p_flags);
	x_obj_free(NULL, p_vals[0]);
	x_obj_free(NULL, p_args[2]);
	x_obj_free(NULL, p_args[1]);
	x_obj_free(NULL, p_args[0]);
	x_obj_free(NULL, p_ret);

	p_obj = x_mkspair(NULL, X_OBJ_FLAG_NONE, 0, 0);
	p_flags = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0xf0);
	p_vals[0] = x_mksatom(NULL, X_OBJ_FLAG_NONE, 10);
	p_vals[1] = x_mksatom(NULL, X_OBJ_FLAG_NONE, 20);
	p_args[3] = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_vals[1], NULL);
	p_args[2] = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_vals[0], p_args[3]);
	p_args[1] = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_flags, p_args[2]);
	p_args[0] = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_obj, p_args[1]);
	p_ret = x_obj_prim_make(NULL, p_args[0]);
	_it_should("return a new object with the same type as the pair "
		"and flags and values from the arguments when base is NULL",
		p_ret != p_obj
		&& x_obj_type(p_ret) == x_obj_type(p_obj)
		&& x_atomint(p_flags) == x_obj_flags(p_ret)
		&& x_atomint(p_vals[0]) == x_firstint(p_ret)
		&& x_atomint(p_vals[1]) == x_restint(p_ret)
	);
	x_obj_free(NULL, p_obj);
	x_obj_free(NULL, p_flags);
	x_obj_free(NULL, p_vals[1]);
	x_obj_free(NULL, p_vals[0]);
	x_obj_free(NULL, p_args[3]);
	x_obj_free(NULL, p_args[2]);
	x_obj_free(NULL, p_args[1]);
	x_obj_free(NULL, p_args[0]);
	x_obj_free(NULL, p_ret);


	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);
	p_obj = x_mksatom(p_base, X_OBJ_FLAG_NONE, 0);
	p_flags = x_mksatom(p_base, X_OBJ_FLAG_NONE, 0xf0);
	p_vals[0] = x_mksatom(p_base, X_OBJ_FLAG_NONE, 10);
	p_args[2] = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_vals[0], p_base);
	p_args[1] = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_flags, p_args[2]);
	p_args[0] = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj, p_args[1]);
	p_ret = x_obj_prim_make(p_base, p_args[0]);
	p_ret = x_obj_prim_make(p_base, p_args[0]);
	_it_should("return a new object with the same type as the atom "
		"and flags and value from the arguments when base is empty",
		p_ret != p_obj
		&& x_obj_type(p_ret) == x_obj_type(p_obj)
		&& x_atomint(p_flags) == x_obj_flags(p_ret)
		&& x_atomint(p_vals[0]) == x_atomint(p_ret)
	);
	x_obj_free(NULL, p_obj);
	x_obj_free(NULL, p_flags);
	x_obj_free(NULL, p_vals[0]);
	x_obj_free(NULL, p_args[2]);
	x_obj_free(NULL, p_args[1]);
	x_obj_free(NULL, p_args[0]);
	x_obj_free(NULL, p_ret);
	x_obj_free(NULL, p_base);

	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);
	p_obj = x_mkspair(NULL, X_OBJ_FLAG_NONE, 0, 0);
	p_flags = x_mksatom(p_base, X_OBJ_FLAG_NONE, 0xf0);
	p_vals[0] = x_mksatom(p_base, X_OBJ_FLAG_NONE, 10);
	p_vals[1] = x_mksatom(p_base, X_OBJ_FLAG_NONE, 20);
	p_args[3] = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_vals[1], p_base);
	p_args[2] = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_vals[0], p_args[3]);
	p_args[1] = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_flags, p_args[2]);
	p_args[0] = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj, p_args[1]);
	p_ret = x_obj_prim_make(p_base, p_args[0]);
	_it_should("return a new object with the same type as the pair "
		"and flags and values from the arguments when base is empty",
		p_ret != p_obj
		&& x_obj_type(p_ret) == x_obj_type(p_obj)
		&& x_atomint(p_flags) == x_obj_flags(p_ret)
		&& x_atomint(p_vals[0]) == x_firstint(p_ret)
		&& x_atomint(p_vals[1]) == x_restint(p_ret)
	);
	x_obj_free(NULL, p_obj);
	x_obj_free(NULL, p_flags);
	x_obj_free(NULL, p_vals[1]);
	x_obj_free(NULL, p_vals[0]);
	x_obj_free(NULL, p_args[3]);
	x_obj_free(NULL, p_args[2]);
	x_obj_free(NULL, p_args[1]);
	x_obj_free(NULL, p_args[0]);
	x_obj_free(NULL, p_ret);
	x_obj_free(NULL, p_base);

	return NULL;
}

static char *test_obj_prim_make_null_type(void)
{
	x_obj_t *p_obj, *p_args, *p_ret;

	helper_alloc_reset();

	/* Object whose type field is NULL */
	p_obj = x_mksatom(NULL, X_OBJ_FLAG_NONE, 42);
	x_obj_type(p_obj) = NULL;
	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_obj, NULL);
	p_ret = x_obj_prim_make(NULL, p_args);
	_it_should("return NULL when object type is nil",
		NULL == p_ret);
	x_obj_free(NULL, p_args);
	x_obj_free(NULL, p_obj);

	return NULL;
}

static char *test_obj_prim_make_custom_nil_name(void)
{
	x_obj_t *p_base, *p_type, *p_obj, *p_args, *p_ret;

	helper_alloc_reset();

	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);
	p_type = _test_make_type(p_base);
	p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE, X_OBJ_LENGTH_ATOM, 0);

	/* Clear name field to NULL */
	x_type_field_name(p_type) = NULL;

	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_obj, NULL);
	p_ret = x_obj_prim_make(p_base, p_args);
	_it_should("return NULL when custom type name is nil",
		NULL == p_ret);

	return NULL;
}

unsigned int _test_prim_fn_calls = 0;
x_obj_t *_test_prim_fn(x_obj_t *p_base, x_obj_t *p_args)
{
	_test_prim_fn_calls++;

	return p_base;
}

static char *test_obj_prim_make_custom_with_make_fn(void)
{
	x_obj_t *p_base, *p_type, *p_obj, *p_args, *p_ret;

	helper_alloc_reset();

	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);
	p_type = _test_make_type(p_base);
	p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE, X_OBJ_LENGTH_ATOM, 0);

	/* Set the make field (heap[1]) to a real function pointer */
	x_type_field_make(p_type) = x_mksatom(NULL, X_OBJ_FLAG_NONE, (x_int_t)_test_prim_fn);

	_test_prim_fn_calls = 0;
	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_obj, NULL);
	p_ret = x_obj_prim_make(p_base, p_args);
	_it_should("custom type with make fn calls the make function",
		_test_prim_fn_calls == 1);
	_it_should("custom type with make fn returns make result",
		p_ret == p_base);

	return NULL;
}

x_obj_t *_test_type_prim_call(x_obj_t *p_base, x_obj_t *p_args)
{
	return (*x_atomfn(x_firstobj(p_args)))(p_base, x_restobj(p_args));
}

static char *test_obj_prim_call(void)
{
	x_obj_t *p_base, *p_type, *p_obj, *p_args, *p_ret;

	/* NULL args returns NULL */
	p_ret = x_obj_prim_call(NULL, NULL);
	_it_should("return NULL for nil args",
		NULL == p_ret);

	/* nil object returns NULL */
	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE, NULL, NULL);
	p_ret = x_obj_prim_call(NULL, p_args);
	_it_should("return NULL for nil object",
		NULL == p_ret);

	/* Object with NULL type returns NULL */
	p_obj = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);
	x_obj_type(p_obj) = NULL;
	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_obj, NULL);
	p_ret = x_obj_prim_call(NULL, p_args);
	_it_should("return NULL for NULL type",
		NULL == p_ret);

	/* Typed object with NULL call field returns NULL */
	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);
	p_type = _test_make_type(p_base);
	p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE, X_OBJ_LENGTH_ATOM, _test_prim_fn);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj, NULL);

	_test_prim_fn_calls = 0;
	p_ret = x_obj_prim_call(p_base, p_args);
	_it_should("return NULL when call field is nil",
		NULL == p_ret
		&& 0 == _test_prim_fn_calls
	);

	/* Set call field and call succeeds */
	x_firstptr(x_type_field_call(p_type)) = _test_type_prim_call;

	_test_prim_fn_calls = 0;
	p_ret = x_obj_prim_call(p_base, p_args);
	_it_should("call the fn and return result",
		p_base == p_ret
		&& 1 == _test_prim_fn_calls
	);

	return NULL;
}

static char *test_obj_prim_type_name_nil(void)
{
	x_obj_t *p_ret;

	/* nil args -> NULL */
	p_ret = x_obj_prim_type_name(NULL, NULL);
	_it_should("type_name with nil args returns NULL",
		NULL == p_ret);

	return NULL;
}

static char *test_obj_prim_units_nil(void)
{
	x_obj_t *p_ret;
	x_int_t units;

	/* nil args -> NULL */
	p_ret = x_obj_prim_units(NULL, NULL);
	_it_should("prim_units with nil args returns NULL",
		NULL == p_ret);

	/* x_obj_units with nil -> default ATOM */
	units = x_obj_units(NULL, NULL);
	_it_should("obj_units with nil returns ATOM units",
		units == X_OBJ_UNITS_ATOM);

	return NULL;
}

static char *test_obj_prim_units_typed(void)
{
	x_obj_t *p_base, *p_type, *p_obj;
	x_obj_t *p_ret;

	helper_alloc_reset();
	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);
	p_type = _test_make_type(p_base);
	p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE,
		X_OBJ_LENGTH_ATOM, 42);

	{
		x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_obj }, { p_base });
		p_ret = x_obj_prim_units(p_base, (x_obj_t *)args);
		(void)p_ret;
		_it_should("prim_units with typed obj exercises hook path", 1);
	}

	return NULL;
}

static char *test_obj_prim_length_nil(void)
{
	x_obj_t *p_ret;
	x_int_t len;

	/* nil args -> NULL */
	p_ret = x_obj_prim_length(NULL, NULL);
	_it_should("prim_length with nil args returns NULL",
		NULL == p_ret);

	/* x_obj_length with nil -> 0 */
	len = x_obj_length(NULL, NULL);
	_it_should("obj_length with nil returns 0",
		len == 0);

	return NULL;
}

static char *test_obj_prim_length_satom(void)
{
	x_obj_t *p_base, *p_obj;
	x_obj_t *p_ret;
	x_int_t len;

	helper_alloc_reset();
	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);
	p_obj = x_mksatom(p_base, X_OBJ_FLAG_NONE, 42);

	{
		x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_obj }, { p_base });
		/* satom -> atom length */
		p_ret = x_obj_prim_length(p_base, (x_obj_t *)args);
		_it_should("prim_length of satom returns atom length",
			p_ret != NULL
			&& x_atomint(p_ret) == X_OBJ_LENGTH_ATOM);
	}

	len = x_obj_length(p_base, p_obj);
	_it_should("obj_length of satom returns ATOM length",
		len == X_OBJ_LENGTH_ATOM);

	return NULL;
}

static char *test_obj_prim_length_spair(void)
{
	x_obj_t *p_base, *p_obj;
	x_obj_t *p_ret;
	x_int_t len;

	helper_alloc_reset();
	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);
	p_obj = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, NULL);

	{
		x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_obj }, { p_base });
		/* spair -> pair length */
		p_ret = x_obj_prim_length(p_base, (x_obj_t *)args);
		_it_should("prim_length of spair returns pair length",
			p_ret != NULL
			&& x_atomint(p_ret) == X_OBJ_LENGTH_PAIR);
	}

	len = x_obj_length(p_base, p_obj);
	_it_should("obj_length of spair returns PAIR length",
		len == X_OBJ_LENGTH_PAIR);

	return NULL;
}

static char *test_obj_prim_length_typed(void)
{
	x_obj_t *p_base, *p_type, *p_obj;
	x_obj_t *p_ret;

	helper_alloc_reset();
	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);
	p_type = _test_make_type(p_base);
	p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE,
		X_OBJ_LENGTH_ATOM, 42);

	{
		x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_obj }, { p_base });
		p_ret = x_obj_prim_length(p_base, (x_obj_t *)args);
		(void)p_ret;
		_it_should("prim_length with typed obj exercises hook path", 1);
	}

	return NULL;
}

static char *test_obj_alloc_fail(void)
{
	x_obj_t *p_ret;

	/* Set allocator to always fail */
	helper_set_alloc(MEM_ERROR);

	p_ret = x_obj_alloc(NULL, NULL, X_OBJ_FLAG_NONE, 1);
	_it_should("alloc returns NULL on malloc failure",
		NULL == p_ret);

	helper_set_alloc(MEM_GUARANTEED);

	return NULL;
}

static char *test_lib_inttostr(void)
{
	x_char_t buf[64];
	x_char_t *p_ret;

	/* Invalid base returns NULL */
	p_ret = x_lib_inttostr(42, buf, 1);
	_it_should("inttostr base 1 returns NULL", p_ret == NULL);

	p_ret = x_lib_inttostr(42, buf, 37);
	_it_should("inttostr base 37 returns NULL", p_ret == NULL);

	/* Negative number */
	p_ret = x_lib_inttostr(-42, buf, 10);
	_it_should("inttostr -42 returns '-42'",
		p_ret != NULL && x_lib_strcmp(p_ret, "-42") == 0);

	return NULL;
}

static char *test_lib_strtoint(void)
{
	x_char_t *end;
	x_int_t val;

	/* Leading whitespace */
	val = x_lib_strtoint((x_char_t *)"  42", &end, 10);
	_it_should("strtoint skips leading whitespace",
		val == 42);

	/* Auto-detect octal */
	val = x_lib_strtoint((x_char_t *)"010", NULL, 0);
	_it_should("strtoint auto-detects octal",
		val == 8);

	/* Auto-detect hex */
	val = x_lib_strtoint((x_char_t *)"0xff", NULL, 0);
	_it_should("strtoint auto-detects hex",
		val == 255);

	/* Break on invalid digit for base */
	val = x_lib_strtoint((x_char_t *)"19", &end, 8);
	_it_should("strtoint breaks on digit >= base",
		val == 1 && *end == '9');

	/* Break on invalid hex letter for base */
	val = x_lib_strtoint((x_char_t *)"1g", &end, 16);
	_it_should("strtoint breaks on hex letter >= base",
		val == 1 && *end == 'g');

	/* Hex letter in value */
	val = x_lib_strtoint((x_char_t *)"ff", NULL, 16);
	_it_should("strtoint parses hex letters",
		val == 255);

	return NULL;
}

static char *run_tests() {
	_run_test(test_obj_prim_make);
	_run_test(test_obj_prim_make_null_type);
	_run_test(test_obj_prim_make_custom_nil_name);
	_run_test(test_obj_prim_make_custom_with_make_fn);
	_run_test(test_obj_prim_call);
	_run_test(test_obj_prim_type_name_nil);
	_run_test(test_obj_prim_units_nil);
	_run_test(test_obj_prim_units_typed);
	_run_test(test_obj_prim_length_nil);
	_run_test(test_obj_prim_length_satom);
	_run_test(test_obj_prim_length_spair);
	_run_test(test_obj_prim_length_typed);
	_run_test(test_obj_alloc_fail);
	_run_test(test_lib_inttostr);
	_run_test(test_lib_strtoint);

	return NULL;
}
