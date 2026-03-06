/*
 * # Unit Tests: *x-obj*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

/* Include Garbage Collection structures. */
#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "src/x-sys.c"
#include "src/x-lib.c"
#include "src/x.c"
#include "src/x-obj.c"

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


#define TEST_TYPE_ID		0x1d
#define TEST_TYPE_UNITS		0x1e
#define TEST_TYPE_LENGTH	0x1f
#define TEST_TYPE_STR_NAME	"NAME"

x_obj_t * (test_prim_fn)(x_obj_t *p_base, x_obj_t *p_args)
{
	return NULL;
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
static char *test_obj_sys_alloc(void)
{
	x_obj_t *p_obj[3];

	helper_alloc_reset();

	p_obj[0] = x_obj_alloc(NULL, x_type_atom_obj, 2, 3);
	_it_should("make a new object", p_obj[0] != NULL);
	_it_should("set the new object's gc pointer to NULL", x_obj_gc(p_obj[0]) == NULL);
	_it_should("set the new object's type to the value given", x_obj_type(p_obj[0]) == x_type_atom_obj);
	_it_should("set the new object's flags to the value given", x_obj_flags(p_obj[0]) == 2);

	p_obj[1] = x_obj_alloc(p_obj[0], x_type_atom_obj, 3, 4);
	_it_should("make a new object", p_obj[1] != NULL);
	_it_should("set the new object's gc pointer to NULL", x_obj_gc(p_obj[1]) == NULL);
	_it_should("set the new object's type to the value given", x_obj_type(p_obj[1]) == x_type_atom_obj);
	_it_should("set the new object's flags to the value given", x_obj_flags(p_obj[1]) == 3);
	_it_should("set the p_base object's gc pointer to the new object", x_obj_gc(p_obj[0]) == p_obj[1]);

	p_obj[2] = x_obj_alloc(p_obj[0], x_type_atom_obj, 4, 5);
	_it_should("make a new object", p_obj[2] != NULL);
	_it_should("set the new object's gc pointer to obj1", x_obj_gc(p_obj[2]) == p_obj[1]);
	_it_should("set the new object's type to the value given", x_obj_type(p_obj[2]) == x_type_atom_obj);
	_it_should("set the new object's flags to the value given", x_obj_flags(p_obj[2]) == 4);
	_it_should("set the p_base object's gc pointer to the new object", x_obj_gc(p_obj[0]) == p_obj[2]);

	x_sys_free(p_obj[2]);
	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);

	return NULL;
}

static char *test_obj_sys_make(void)
{
	x_obj_t *p_obj;

	helper_alloc_reset();

	p_obj = x_obj_make(NULL, x_type_atom_obj, 2, 3, (void *)4, (void *)5);
	_it_should("make a new object", p_obj != NULL);
	_it_should("set the new object's gc pointer to NULL", x_obj_gc(p_obj) == NULL);
	_it_should("set the new object's type to the value given", x_obj_type(p_obj) == x_type_atom_obj);
	_it_should("set the new object's flags to the value given", x_obj_flags(p_obj) == 2);
	_it_should("set the new object's first element", x_obj_data_ptr(p_obj)[0].p == (void *)4);
	_it_should("set the new object's second element", x_obj_data_ptr(p_obj)[1].p == (void *)5);

	x_sys_free(p_obj);

	return NULL;
}

static char *test_obj_sys_free(void)
{
	x_obj_t *p_obj, *p_own;

	helper_alloc_reset();

	p_obj = x_obj_alloc(NULL, x_type_atom_obj, 0, 0);
	_it_should("allocate an object", helper_alloc_count() == 1);
	x_obj_free(p_obj);
	_it_should("free the object", helper_free_count() == 1);

	helper_alloc_reset();

	p_own = x_sys_malloc(16);
	_it_should("allocate some memory", helper_alloc_count() == 1);
	p_obj = x_obj_make(NULL, x_type_atom_obj, X_OBJ_FLAG_OWN, X_OBJ_LENGTH_ATOM, p_own);
	_it_should("allocate an object", helper_alloc_count() == 2);
	x_obj_free(p_obj);
	_it_should("free the object and OWNed data", helper_free_count() == 2);

	return NULL;
}

x_obj_t *test_make_type(x_obj_t *p_base)
{
	return
		/* name */
		pair(atom(TEST_TYPE_STR_NAME),
		/* data */
		pair(NULL,
		/* Heap: '(make free clone units length) */
		pair(pair(NULL,
			pair(NULL,
			pair(NULL,
			pair(NULL,
			pair(NULL,
			nil))))),
		/* Proc: '(call eval convert) */
		pair(pair(NULL,
			pair(NULL,
			pair(NULL,
			nil))),
		/* IO: '(analyse delimit write) */
		pair(pair(NULL,
			pair(NULL,
			pair(NULL,
			nil))),
		nil)))));
}

static char *test_obj_prim_type_name(void)
{
	x_obj_t *p_base, *p_obj, *p_args, *p_type, *p_ret;

	helper_alloc_reset();

	p_ret = x_obj_prim_type_name(NULL, NULL);
	_it_should("return NULL when args is NULL",
		x_obj_isnil(NULL, p_ret)
	);

	p_base = x_mksatom(NULL, 0);
	p_ret = x_obj_prim_type_name(p_base, p_base);
	_it_should("return nil when args is nil",
		x_obj_isnil(p_base, p_ret)
	);
	x_obj_free(p_base);


	p_args = x_mkspair(NULL, NULL, NULL);
	p_ret = x_obj_prim_type_name(NULL, p_args);
	_it_should("return NULL when first element of args is NULL",
		x_obj_isnil(NULL, p_ret)
	);

	p_base = x_mksatom(NULL, 0);
	p_args = x_mkspair(p_base, p_base, p_base);
	p_ret = x_obj_prim_type_name(p_base, p_args);
	_it_should("return nil when args is nil",
		x_obj_isnil(p_base, p_ret)
	);
	x_obj_free(p_args);
	x_obj_free(p_base);


	p_obj = x_mksatom(NULL, 0);
	p_args = x_mkspair(NULL, p_obj, NULL);
	p_ret = x_obj_prim_type_name(NULL, p_args);
	_it_should("return atom's type name when base is NULL",
		 x_type_atom_obj == p_ret
	);
	x_obj_free(p_args);
	x_obj_free(p_obj);

	p_obj = x_mkspair(NULL, 0, 0);
	p_args = x_mkspair(NULL, p_obj, NULL);
	p_ret = x_obj_prim_type_name(NULL, p_args);
	_it_should("return pair's type name when base is NULL",
		x_type_pair_obj == p_ret
	);
	x_obj_free(p_obj);
	x_obj_free(p_args);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_mksatom(p_base, 0);
	p_args = x_mkspair(p_base, p_obj, p_base);
	p_ret = x_obj_prim_type_name(p_base, p_args);
	_it_should("return atom's type name when base is empty",
		x_type_atom_obj == p_ret
	);
	x_obj_free(p_obj);
	x_obj_free(p_base);

	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkspair(p_base, 0, 0);
	p_args = x_mkspair(p_base, p_obj, p_base);
	p_ret = x_obj_prim_type_name(p_base, p_args);
	_it_should("return pair's type name when base is empty",
		x_type_pair_obj == p_ret
	);
	x_obj_free(p_obj);
	x_obj_free(p_args);
	x_obj_free(p_base);


	p_base = x_mksatom(NULL, 0);
	p_type = test_make_type(p_base);
	p_obj = x_obj_alloc(p_base, p_type, 0, 0);
	p_args = x_mkspair(p_base, p_obj, p_base);
	p_ret = x_obj_prim_type_name(p_base, p_args);
	_it_should("return the object's type name when type object is set",
		0 == strcmp(TEST_TYPE_STR_NAME, x_atomstr(p_ret))
	);
	x_obj_free(p_obj);
	x_obj_free(p_args);
	x_obj_free(p_type);
	x_obj_free(p_base);

	return NULL;
}

static char *test_obj_type_name(void)
{
	x_obj_t *p_base, *p_obj, *p_type;
	x_char_t *s;

	helper_alloc_reset();

	p_obj = x_mksatom(NULL, 0);
	s = x_obj_type_name(NULL, p_obj);
	_it_should("return atom's type name when base is NULL",
		0 == strcmp(X_TYPE_ATOM_NAME, s)
	);
	x_obj_free(p_obj);

	p_obj = x_mkspair(NULL, 0, 0);
	s = x_obj_type_name(NULL, p_obj);
	_it_should("return pair's type name when base is NULL",
		0 == strcmp(X_TYPE_PAIR_NAME, s)
	);
	x_obj_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_mksatom(p_base, 0);
	s = x_obj_type_name(p_base, p_obj);
	_it_should("return atom's type name when base is empty",
		0 == strcmp(X_TYPE_ATOM_NAME, s)
	);
	x_obj_free(p_obj);
	x_obj_free(p_base);

	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkspair(p_base, 0, 0);
	s = x_obj_type_name(p_base, p_obj);
	_it_should("return pair's type name when base is empty",
		0 == strcmp(X_TYPE_PAIR_NAME, s)
	);
	x_obj_free(p_obj);
	x_obj_free(p_base);


	p_base = x_mksatom(NULL, 0);
	p_type = test_make_type(p_base);
	p_obj = x_obj_alloc(p_base, p_type, 0, 0);
	s = x_obj_type_name(p_base, p_obj);
	_it_should("return the object's type name when type object is set",
		0 == strcmp(TEST_TYPE_STR_NAME, s)
	);
	x_obj_free(p_obj);
	x_obj_free(p_type);
	x_obj_free(p_base);

	return NULL;
}

x_obj_t *test_units(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_firstobj(p_args);
}

x_satom_t test_units_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_units });


static char *test_obj_prim_units(void)
{
	x_obj_t *p_base, *p_obj, *p_args, *p_type, *p_ret;

	helper_alloc_reset();

	p_ret = x_obj_prim_units(NULL, NULL);
	_it_should("return NULL when args is NULL",
		x_obj_isnil(NULL, p_ret)
	);

	p_base = x_mksatom(NULL, 0);
	p_ret = x_obj_prim_units(p_base, p_base);
	_it_should("return nil when args is nil",
		x_obj_isnil(p_base, p_ret)
	);
	x_obj_free(p_base);


	p_args = x_mkspair(NULL, NULL, NULL);
	p_ret = x_obj_prim_units(NULL, p_args);
	_it_should("return NULL when first element of args is NULL",
		x_obj_isnil(NULL, p_ret)
	);

	p_base = x_mksatom(NULL, 0);
	p_args = x_mkspair(p_base, p_base, p_base);
	p_ret = x_obj_prim_units(p_base, p_args);
	_it_should("return nil when args is nil",
		x_obj_isnil(p_base, p_ret)
	);
	x_obj_free(p_args);
	x_obj_free(p_base);


	p_obj = x_mksatom(NULL, 0);
	p_args = x_mkspair(NULL, p_obj, NULL);
	p_ret = x_obj_prim_units(NULL, p_args);
	_it_should("return atom's size in units when base is NULL",
		 x_type_units_atom_obj == p_ret
	);
	x_obj_free(p_args);
	x_obj_free(p_obj);

	p_obj = x_mkspair(NULL, 0, 0);
	p_args = x_mkspair(NULL, p_obj, NULL);
	p_ret = x_obj_prim_units(NULL, p_args);
	_it_should("return pair's size in units when base is NULL",
		x_type_units_pair_obj == p_ret
	);
	x_obj_free(p_obj);
	x_obj_free(p_args);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_mksatom(p_base, 0);
	p_args = x_mkspair(p_base, p_obj, p_base);
	p_ret = x_obj_prim_units(p_base, p_args);
	_it_should("return atom's size in units when base is empty",
		x_type_units_atom_obj == p_ret
	);
	x_obj_free(p_obj);
	x_obj_free(p_base);

	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkspair(p_base, 0, 0);
	p_args = x_mkspair(p_base, p_obj, p_base);
	p_ret = x_obj_prim_units(p_base, p_args);
	_it_should("return pair's size in units when base is empty",
		x_type_units_pair_obj == p_ret
	);
	x_obj_free(p_obj);
	x_obj_free(p_args);
	x_obj_free(p_base);


	p_base = x_mksatom(NULL, 0);
	p_type = test_make_type(p_base);
	p_obj = x_obj_make(p_base, p_type, 0, 1, TEST_TYPE_LENGTH);
	p_args = x_mkspair(p_base, p_obj, p_base);
	p_ret = x_obj_prim_units(p_base, p_args);
	_it_should("return the base object when type object is set and "
		"byte function is nil",
		p_base == p_ret
	);

	x_type_field_units(p_type) = test_units_prim;
	p_ret = x_obj_prim_units(p_base, p_args);
	_it_should("return the object's size in units when type object is set",
		TEST_TYPE_LENGTH ==  x_atomint(p_ret)
	);
	x_obj_free(p_obj);
	x_obj_free(p_args);
	x_obj_free(p_type);
	x_obj_free(p_base);

	return NULL;
}

static char *test_obj_units(void)
{
	x_obj_t *p_base, *p_obj, *p_type;
	int i;

	p_obj = x_mksatom(NULL, 0);
	i = x_obj_units(NULL, p_obj);
	_it_should("return atoms's size in units when base is NULL",
		X_OBJ_UNITS_ATOM == i
	);
	x_obj_free(p_obj);

	p_obj = x_mkspair(NULL, 0, 0);
	i = x_obj_units(NULL, p_obj);
	_it_should("return pair's size in units when base is NULL",
		X_OBJ_UNITS_PAIR == i
	);
	x_obj_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_mksatom(p_base, 0);
	i = x_obj_units(p_base, p_obj);
	_it_should("return atom's size in units when base is empty",
		X_OBJ_UNITS_ATOM == i
	);
	x_obj_free(p_obj);
	x_obj_free(p_base);

	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkspair(p_base, 0, 0);
	i = x_obj_units(p_base, p_obj);
	_it_should("return pair's size in units when base is empty",
		X_OBJ_UNITS_PAIR == i
	);
	x_obj_free(p_obj);
	x_obj_free(p_base);


	p_base = x_mksatom(NULL, 0);
	p_type = test_make_type(p_base);
	p_obj = x_obj_alloc(p_base, p_type, 0, 0);
	i = x_obj_units(p_base, p_obj);
	_it_should("return the object's size in units when type object is set "
		"and primitive is nil",
		X_OBJ_UNITS_ATOM == i
	);
	x_obj_free(p_obj);
	x_obj_free(p_type);
	x_obj_free(p_base);


	p_base = x_mksatom(NULL, 0);
	p_type = test_make_type(p_base);
	p_obj = x_obj_make(p_base, p_type, 15, 1, TEST_TYPE_UNITS);
	i = x_obj_units(p_base, p_obj);
	_it_should("return an atom's size in units when the type object is set and "
		"byte size function is nil",
		X_OBJ_UNITS_ATOM == i
	);

	x_type_field_units(p_type) = test_units_prim;
	i = x_obj_units(p_base, p_obj);
	_it_should("return the units returned from the type units primitive "
		"when the type object is set",
		TEST_TYPE_UNITS == i
	);
	x_obj_free(p_obj);
	x_obj_free(p_type);
	x_obj_free(p_base);

	return NULL;
}

x_obj_t *test_length(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_firstobj(p_args);
}

x_satom_t test_length_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_length });

static char *test_obj_prim_length(void)
{
	x_obj_t *p_base, *p_obj, *p_args, *p_type, *p_ret;

	helper_alloc_reset();

	p_ret = x_obj_prim_length(NULL, NULL);
	_it_should("return NULL when args is NULL",
		x_obj_isnil(NULL, p_ret)
	);

	p_base = x_mksatom(NULL, 0);
	p_ret = x_obj_prim_length(p_base, p_base);
	_it_should("return nil when args is nil",
		x_obj_isnil(p_base, p_ret)
	);
	x_obj_free(p_base);


	p_args = x_mkspair(NULL, NULL, NULL);
	p_ret = x_obj_prim_length(NULL, p_args);
	_it_should("return NULL when first element of args is NULL",
		x_obj_isnil(NULL, p_ret)
	);

	p_base = x_mksatom(NULL, 0);
	p_args = x_mkspair(p_base, p_base, p_base);
	p_ret = x_obj_prim_length(p_base, p_args);
	_it_should("return nil when args is nil",
		x_obj_isnil(p_base, p_ret)
	);
	x_obj_free(p_args);
	x_obj_free(p_base);


	p_obj = x_mksatom(NULL, 0);
	p_args = x_mkspair(NULL, p_obj, NULL);
	p_ret = x_obj_prim_length(NULL, p_args);
	_it_should("return atom's length when base is NULL",
		 x_type_length_atom_obj == p_ret
	);
	x_obj_free(p_args);
	x_obj_free(p_obj);

	p_obj = x_mkspair(NULL, 0, 0);
	p_args = x_mkspair(NULL, p_obj, NULL);
	p_ret = x_obj_prim_length(NULL, p_args);
	_it_should("return pair's length when base is NULL",
		x_type_length_pair_obj == p_ret
	);
	x_obj_free(p_obj);
	x_obj_free(p_args);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_mksatom(p_base, 0);
	p_args = x_mkspair(p_base, p_obj, p_base);
	p_ret = x_obj_prim_length(p_base, p_args);
	_it_should("return atom's length when base is empty",
		x_type_length_atom_obj == p_ret
	);
	x_obj_free(p_obj);
	x_obj_free(p_base);

	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkspair(p_base, 0, 0);
	p_args = x_mkspair(p_base, p_obj, p_base);
	p_ret = x_obj_prim_length(p_base, p_args);
	_it_should("return pair's length when base is empty",
		x_type_length_pair_obj == p_ret
	);
	x_obj_free(p_obj);
	x_obj_free(p_args);
	x_obj_free(p_base);


	p_base = x_mksatom(NULL, 0);
	p_type = test_make_type(p_base);
	p_obj = x_obj_make(p_base, p_type, 0, 1, TEST_TYPE_LENGTH);
	p_args = x_mkspair(p_base, p_obj, p_base);
	p_ret = x_obj_prim_length(p_base, p_args);
	_it_should("return the base when type object is set and "
		"byte function is nil",
		p_base == p_ret
	);

	x_type_field_length(p_type) = test_length_prim;
	p_ret = x_obj_prim_length(p_base, p_args);
	_it_should("return the object's length when type object is set",
		TEST_TYPE_LENGTH ==  x_atomint(p_ret)
	);
	x_obj_free(p_obj);
	x_obj_free(p_args);
	x_obj_free(p_type);
	x_obj_free(p_base);

	return NULL;
}

static char *test_obj_length(void)
{
	x_obj_t *p_base, *p_obj, *p_type;
	int i;

	helper_alloc_reset();

	p_obj = x_mksatom(NULL, 0);
	i = x_obj_length(NULL, p_obj);
	_it_should("return atoms's length when base is NULL and type is an integer",
		X_OBJ_LENGTH_ATOM == i
	);
	x_obj_free(p_obj);

	p_obj = x_mkspair(NULL, 0, 0);
	i = x_obj_length(NULL, p_obj);
	_it_should("return pair's length when base is NULL and type is an integer",
		X_OBJ_LENGTH_PAIR == i
	);
	x_obj_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_mksatom(p_base, 0);
	i = x_obj_length(p_base, p_obj);
	_it_should("return atom's length when base is empty and type is an integer",
		X_OBJ_LENGTH_ATOM == i
	);
	x_obj_free(p_obj);
	x_obj_free(p_base);

	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkspair(p_base, 0, 0);
	i = x_obj_length(p_base, p_obj);
	_it_should("return pair's length when base is empty and type is an integer",
		X_OBJ_LENGTH_PAIR == i
	);
	x_obj_free(p_obj);
	x_obj_free(p_base);


	p_base = x_mksatom(NULL, 0);
	p_type = test_make_type(p_base);
	p_obj = x_obj_alloc(p_base, p_type, 0, 0);
	i = x_obj_length(p_base, p_obj);
	_it_should("return zero when type object is set "
		"and primitive is nil",
		0 == i
	);
	x_obj_free(p_obj);
	x_obj_free(p_type);
	x_obj_free(p_base);

	p_base = x_mksatom(NULL, 0);
	p_type = test_make_type(p_base);
	x_type_field_length(p_type) = test_length_prim;
	p_obj = x_obj_make(p_base, p_type, 15, 1, TEST_TYPE_LENGTH);
	i = x_obj_length(p_base, p_obj);
	_it_should("return the length returned from the type length primitive "
		"when the type object is set",
		TEST_TYPE_LENGTH == i
	);
	x_obj_free(p_obj);
	x_obj_free(p_type);
	x_obj_free(p_base);

	return NULL;
}


static char *test_obj_is_nil(void)
{
	x_obj_t *p_base, *p_obj;

	helper_alloc_reset();

	_it_should("return true when base is NULL and value is NULL",
		1 == x_obj_isnil(NULL, NULL)
	);

	p_obj = x_mksatom(NULL, 0);
	_it_should("return false when base is NULL and value is an object",
		0 == x_obj_isnil(NULL, p_obj)
	);
	x_obj_free(p_obj);

	p_base = x_mksatom(NULL, 0);
	_it_should("return true when base is an object and value is base",
		1 == x_obj_isnil(p_base, p_base)
	);

	_it_should("return true when base is an object and value is NULL",
		1 == x_obj_isnil(p_base, NULL)
	);

	p_obj = x_mksatom(p_base, 0);
	_it_should("return false when base is and object and value is another object",
		0 == x_obj_isnil(p_base, p_obj)
	);

	x_obj_free(p_obj);
	x_obj_free(p_base);

	return NULL;
}


static char *test_obj_type(void)
{
	x_obj_t *p_obj;

	helper_alloc_reset();

	p_obj = x_mksatom(NULL, 0);
	_it_should("return atom type object when object is an atom",
		x_type_atom_obj == x_obj_type(p_obj)
	);
	x_obj_free(p_obj);

	p_obj = x_mkspair(NULL, 0, 0);
	_it_should("return pair type object when object is a pair",
		x_type_pair_obj == x_obj_type(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_obj_flags(void)
{
	x_obj_t *p_obj;
	enum x_obj_flag_enum flags = rand();

	helper_alloc_reset();

	p_obj = x_mkfsatom(NULL, flags, 0);
	_it_should("return object flags",
		flags == x_obj_flags(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_obj_data(void)
{
	x_obj_t *p_obj;
	union x_datum_union data = { .i = rand() };

	helper_alloc_reset();

	p_obj = x_mksatom(NULL, data);
	_it_should("return object data",
		0 == memcmp(&data, &x_obj_data(p_obj), sizeof(data))
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_obj_data_ptr(void)
{
	x_obj_t *p_obj;

	helper_alloc_reset();

	p_obj = x_mksatom(NULL, 0);
	_it_should("return object data pointer",
		p_obj + X_OBJ_META_LEN == &x_obj_data(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_obj_type_issatom(void)
{
	x_obj_t *p_obj;

	helper_alloc_reset();

	p_obj = x_mksatom(NULL, 0);
	_it_should("return true when object is an atom",
		1 == x_obj_type_issatom(p_obj)
	);
	x_obj_free(p_obj);

	p_obj = x_mkspair(NULL, 0, 0);
	_it_should("return false when object is not an atom",
		0 == x_obj_type_issatom(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_obj_type_isspair(void)
{
	x_obj_t *p_obj;

	helper_alloc_reset();

	p_obj = x_mkspair(NULL, 0, 0);
	_it_should("return true when object is a pair",
		1 == x_obj_type_isspair(p_obj)
	);
	x_obj_free(p_obj);

	p_obj = x_mksatom(NULL, 0);
	_it_should("return false when object is not a pair",
		0 == x_obj_type_isspair(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_obj_type_isnil(void)
{
	x_obj_t *p_base, *p_obj;

	helper_alloc_reset();

	p_obj = x_obj_make(NULL, NULL, 0, X_OBJ_LENGTH_ATOM, 0);
	_it_should("return true when object type is NULL",
		1 == x_obj_type_isnil(NULL, p_obj)
	);
	x_obj_free(p_obj);

	p_base = x_mksatom(NULL, 0);
	p_obj = x_obj_make(p_base, NULL, 0, X_OBJ_LENGTH_ATOM, 0);
	_it_should("return true when object type is base",
		1 == x_obj_type_isnil(p_base, p_obj)
	);
	x_obj_free(p_obj);
	x_obj_free(p_base);

	p_obj = x_mksatom(NULL, 0);
	_it_should("return false when object type is not NULL",
		0 == x_obj_type_isnil(NULL, p_obj)
	);
	x_obj_free(p_obj);

	p_base = x_mksatom(NULL, 0);
	p_obj = x_mksatom(p_base, 0);
	_it_should("return false when object type is not base",
		0 == x_obj_type_isnil(p_base, p_obj)
	);
	x_obj_free(p_obj);
	x_obj_free(p_base);

	return NULL;
}


static char *test_obj_is_type(void)
{
	x_obj_t *p_base, *p_obj, *p_type;

	helper_alloc_reset();

	p_obj = x_mksatom(NULL, 0);
	_it_should("return true atom's type name when base is NULL",
		x_obj_is_type(NULL, p_obj, X_TYPE_ATOM_NAME)
	);
	x_obj_free(p_obj);

	p_obj = x_mkspair(NULL, 0, 0);
	_it_should("return pair's type name when base is NULL",
		x_obj_is_type(NULL, p_obj, X_TYPE_PAIR_NAME)
	);
	x_obj_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_mksatom(p_base, 0);
	_it_should("return atom's type name when base is empty",
		x_obj_is_type(p_base, p_obj, X_TYPE_ATOM_NAME)
	);
	x_obj_free(p_obj);
	x_obj_free(p_base);

	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkspair(p_base, 0, 0);
	_it_should("return pair's type name when base is empty",
		x_obj_is_type(p_base, p_obj, X_TYPE_PAIR_NAME)
	);
	x_obj_free(p_obj);
	x_obj_free(p_base);


	p_base = x_mksatom(NULL, 0);
	p_type = test_make_type(p_base);
	p_obj = x_obj_alloc(p_base, p_type, 0, 0);
	_it_should("return the object's type name when type object is set",
		x_obj_is_type(p_base, p_obj, TEST_TYPE_STR_NAME)
	);
	x_obj_free(p_obj);
	x_obj_free(p_type);
	x_obj_free(p_base);

	return NULL;
}


static char *test_obj_set(void)
{
	enum x_obj_flag_enum flags = rand() * (1 << sizeof(flags) << 3) / RAND_MAX;
	union x_datum_union data[2] = { { .i = rand() }, { .i = rand() } };
	x_satom_t atom_obj = x_obj_set(NULL, flags, data[0]);
	x_spair_t pair_obj = x_obj_set(NULL, flags, data[0], data[1]);

	_it_should("return NULL type when object type is NULL",
		NULL == x_obj_type(atom_obj)
	);
	_it_should("return object flags",
		flags == x_obj_flags(atom_obj)
	);
	_it_should("return object data",
		data[0].i == x_obj_data_i(atom_obj, 0).i
	);


	_it_should("return NULL type when object type is NULL",
		NULL == x_obj_type(pair_obj)
	);
	_it_should("return object flags",
		flags == x_obj_flags(pair_obj)
	);
	_it_should("return object data",
		data[0].i == x_obj_data_i(pair_obj, 0).i
		&& data[1].i == x_obj_data_i(pair_obj, 1).i
	);

	return NULL;
}


static char *test_make_atom(void)
{
	x_obj_t *p_base, *p_obj;
	unsigned int flags = rand();
	x_int_t value = rand();

	helper_alloc_reset();

	p_obj = x_mkfsatom(NULL, flags, value);
	_it_should("make an atom with the supplied flags and value",
		x_obj_type_issatom(p_obj)
		&& flags == x_obj_flags(p_obj)
		&& value == x_int(x_obj_data(p_obj))
	);
	x_obj_free(p_obj);

	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkfsatom(p_base, flags, value);
	_it_should("make an atom with the supplied base, flags and value",
		x_obj_type_issatom(p_obj)
		&& p_obj == x_obj_gc(p_base)
		&& flags == x_obj_flags(p_obj)
		&& value == x_int(x_obj_data(p_obj))
	);
	x_obj_free(p_obj);
	x_obj_free(p_base);

	return NULL;
}


static char *test_make_atom_own(void)
{
	x_obj_t *p_base, *p_obj;
	unsigned int flags = rand();
	x_int_t value = rand();

	helper_alloc_reset();

	p_obj = x_mkfsatomown(NULL, flags, value);
	_it_should("make an atom with the supplied flags and value",
		x_obj_type_issatom(p_obj)
		&& (flags | X_OBJ_FLAG_OWN) == x_obj_flags(p_obj)
		&& value == x_int(x_obj_data(p_obj))
	);
	x_obj_free(p_obj);

	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkfsatomown(p_base, flags, value);
	_it_should("make an atom with the supplied base, flags and value",
		x_obj_type_issatom(p_obj)
		&& p_obj == x_obj_gc(p_base)
		&& (flags | X_OBJ_FLAG_OWN) == x_obj_flags(p_obj)
		&& value == x_int(x_obj_data(p_obj))
	);
	x_obj_free(p_obj);
	x_obj_free(p_base);

	return NULL;
}


static char *test_make_pair(void)
{
	x_obj_t *p_base, *p_obj;
	unsigned int flags = rand();
	x_int_t values[2] = { rand(), rand() };

	helper_alloc_reset();

	p_obj = x_mkfspair(NULL, flags, values[0], values[1]);
	_it_should("make a pair with the supplied flags and values",
		x_obj_type_isspair(p_obj)
		&& flags == x_obj_flags(p_obj)
		&& values[0] == x_int(x_obj_data(p_obj))
		&& values[1] == x_int(x_obj_data_ptr(p_obj)[1])
	);
	x_obj_free(p_obj);

	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkfspair(p_base, flags, values[0], values[1]);
	_it_should("make a pair with the supplied base, flags and values",
		x_obj_type_isspair(p_obj)
		&& p_obj == x_obj_gc(p_base)
		&& flags == x_obj_flags(p_obj)
		&& values[0] == x_int(x_obj_data(p_obj))
		&& values[1] == x_int(x_obj_data_ptr(p_obj)[1])
	);
	x_obj_free(p_obj);
	x_obj_free(p_base);

	return NULL;
}


static char *test_mkatom(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t value = rand();

	helper_alloc_reset();

	p_obj = x_mksatom(NULL, value);
	_it_should("make an atom with the supplied value",
		x_obj_type_issatom(p_obj)
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj)
		&& value == x_int(x_obj_data(p_obj))
	);
	x_obj_free(p_obj);

	p_base = x_mksatom(NULL, 0);
	p_obj = x_mksatom(p_base, value);
	_it_should("make an atom with the supplied base and value",
		x_obj_type_issatom(p_obj)
		&& p_obj == x_obj_gc(p_base)
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj)
		&& value == x_int(x_obj_data(p_obj))
	);
	x_obj_free(p_obj);
	x_obj_free(p_base);

	return NULL;
}


static char *test_mkatomown(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t value = rand();

	helper_alloc_reset();

	p_obj = x_mksatomown(NULL, value);
	_it_should("make an atom with the supplied flags and value",
		x_obj_type_issatom(p_obj)
		&& X_OBJ_FLAG_OWN == x_obj_flags(p_obj)
		&& value == x_int(x_obj_data(p_obj))
	);
	x_obj_free(p_obj);

	p_base = x_mksatom(NULL, 0);
	p_obj = x_mksatomown(p_base, value);
	_it_should("make an atom with the supplied base, flags and value",
		x_obj_type_issatom(p_obj)
		&& p_obj == x_obj_gc(p_base)
		&& X_OBJ_FLAG_OWN == x_obj_flags(p_obj)
		&& value == x_int(x_obj_data(p_obj))
	);
	x_obj_free(p_obj);
	x_obj_free(p_base);

	return NULL;
}


static char *test_mkpair(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t values[2] = { rand(), rand() };

	helper_alloc_reset();

	p_obj = x_mkspair(NULL, values[0], values[1]);
	_it_should("make a pair with the supplied flags and values",
		x_obj_type_isspair(p_obj)
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj)
		&& values[0] == x_int(x_obj_data(p_obj))
		&& values[1] == x_int(x_obj_data_ptr(p_obj)[1])
	);
	x_obj_free(p_obj);

	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkspair(p_base, values[0], values[1]);
	_it_should("make a pair with the supplied base, flags and values",
		x_obj_type_isspair(p_obj)
		&& p_obj == x_obj_gc(p_base)
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj)
		&& values[0] == x_int(x_obj_data(p_obj))
		&& values[1] == x_int(x_obj_data_ptr(p_obj)[1])
	);
	x_obj_free(p_obj);
	x_obj_free(p_base);

	return NULL;
}


static char *test_ptr(void)
{
	x_obj_t *p_obj = NULL;
	void *ptr = (void *)&p_obj;

	helper_alloc_reset();

	p_obj = x_mksatom(NULL, ptr);
	_it_should("return the void pointer value",
		ptr == x_ptr(x_obj_data(p_obj))
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_obj(void)
{
	x_obj_t *p_obj[2];

	helper_alloc_reset();

	p_obj[0] = x_mksatom(NULL, NULL);
	p_obj[1] = x_mksatom(NULL, p_obj[0]);
	_it_should("return the object pointer value",
		p_obj[0] == x_obj(x_obj_data(p_obj[1]))
	);
	x_obj_free(p_obj[1]);
	x_obj_free(p_obj[0]);

	return NULL;
}


static char *test_int(void)
{
	x_obj_t *p_obj;
	x_int_t i = rand();

	helper_alloc_reset();

	p_obj = x_mksatom(NULL, i);
	_it_should("return the integer value",
		i == x_int(x_obj_data(p_obj))
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_char(void)
{
	x_obj_t *p_obj;
	x_char_t c = rand();

	helper_alloc_reset();

	p_obj = x_mksatom(NULL, c);
	_it_should("return the character value",
		c == x_char(x_obj_data(p_obj))
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_str(void)
{
	x_obj_t *p_obj;
	char *s = "test";

	helper_alloc_reset();

	p_obj = x_mksatom(NULL, s);
	_it_should("return the string value",
		s == x_str(x_obj_data(p_obj))
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_fn(void)
{
	x_obj_t *p_obj;

	helper_alloc_reset();

	p_obj = x_mksatom(NULL, test_prim_fn);
	_it_should("return the function pointer value",
		test_prim_fn == x_fn(x_obj_data(p_obj))
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_first(void)
{
	x_obj_t *p_obj;

	helper_alloc_reset();

	p_obj = x_mksatom(NULL, 0);
	_it_should("return the first data element",
		x_obj_data_ptr(p_obj) == &x_first(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_second(void)
{
	x_obj_t *p_obj;

	helper_alloc_reset();

	p_obj = x_mkspair(NULL, 0, 0);
	_it_should("return the second data element",
		x_obj_data_ptr(p_obj) + 1 == &x_second(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_rest(void)
{
	x_obj_t *p_obj;

	helper_alloc_reset();

	p_obj = x_mkspair(NULL, 0, 0);
	_it_should("return the rest data element",
		x_obj_data_ptr(p_obj) + 1 == &x_rest(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_firstptr(void)
{
	x_obj_t *p_obj = NULL;
	void *ptr = (void *)&p_obj;

	helper_alloc_reset();

	p_obj = x_mksatom(NULL, ptr);
	_it_should("return the void pointer value",
		ptr == x_firstptr(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_firstobj(void)
{
	x_obj_t *p_obj[2];

	helper_alloc_reset();

	p_obj[0] = x_mksatom(NULL, NULL);
	p_obj[1] = x_mksatom(NULL, p_obj[0]);
	_it_should("return the object pointer value",
		p_obj[0] == x_firstobj(p_obj[1])
	);
	x_obj_free(p_obj[1]);
	x_obj_free(p_obj[0]);

	return NULL;
}


static char *test_firstint(void)
{
	x_obj_t *p_obj;
	x_int_t i = rand();

	helper_alloc_reset();

	p_obj = x_mksatom(NULL, i);
	_it_should("return the integer value",
		i == x_firstint(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_firstchar(void)
{
	x_obj_t *p_obj;
	x_char_t c = rand();

	helper_alloc_reset();

	p_obj = x_mksatom(NULL, c);
	_it_should("return the character value",
		c == x_firstchar(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_firststr(void)
{
	x_obj_t *p_obj;
	char *s = "test";

	helper_alloc_reset();

	p_obj = x_mksatom(NULL, s);
	_it_should("return the string value",
		s == x_firststr(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}

static char *test_firstfn(void)
{
	x_obj_t *p_obj;

	helper_alloc_reset();

	p_obj = x_mksatom(NULL, test_prim_fn);
	_it_should("return the function pointer value",
		test_prim_fn == x_firstfn(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_secondptr(void)
{
	x_obj_t *p_obj = NULL;
	void *ptr = (void *)&p_obj;

	helper_alloc_reset();

	p_obj = x_mkspair(NULL, 0, ptr);
	_it_should("return the void pointer value",
		ptr == x_secondptr(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_secondobj(void)
{
	x_obj_t *p_obj[2];

	helper_alloc_reset();

	p_obj[0] = x_mkspair(NULL, 0, NULL);
	p_obj[1] = x_mkspair(NULL, 0, p_obj[0]);
	_it_should("return the object pointer value",
		p_obj[0] == x_secondobj(p_obj[1])
	);
	x_obj_free(p_obj[1]);
	x_obj_free(p_obj[0]);

	return NULL;
}


static char *test_secondint(void)
{
	x_obj_t *p_obj;
	x_int_t i = rand();

	helper_alloc_reset();

	p_obj = x_mkspair(NULL, 0, i);
	_it_should("return the integer value",
		i == x_secondint(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_secondchar(void)
{
	x_obj_t *p_obj;
	x_char_t c = rand();

	helper_alloc_reset();

	p_obj = x_mkspair(NULL, 0, c);
	_it_should("return the character value",
		c == x_secondchar(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_secondstr(void)
{
	x_obj_t *p_obj;
	char *s = "test";

	helper_alloc_reset();

	p_obj = x_mkspair(NULL, 0, s);
	_it_should("return the string value",
		s == x_secondstr(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}

static char *test_secondfn(void)
{
	x_obj_t *p_obj;

	helper_alloc_reset();

	p_obj = x_mkspair(NULL, 0, test_prim_fn);
	_it_should("return the function pointer value",
		test_prim_fn == x_secondfn(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_atomptr(void)
{
	x_obj_t *p_obj = NULL;
	void *ptr = (void *)&p_obj;

	helper_alloc_reset();

	p_obj = x_mksatom(NULL, ptr);
	_it_should("return the void pointer value",
		ptr == x_atomptr(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_atomobj(void)
{
	x_obj_t *p_obj[2];

	helper_alloc_reset();

	p_obj[0] = x_mksatom(NULL, NULL);
	p_obj[1] = x_mksatom(NULL, p_obj[0]);
	_it_should("return the object pointer value",
		p_obj[0] == x_atomobj(p_obj[1])
	);
	x_obj_free(p_obj[1]);
	x_obj_free(p_obj[0]);

	return NULL;
}


static char *test_atomint(void)
{
	x_obj_t *p_obj;
	x_int_t i = rand();

	helper_alloc_reset();

	p_obj = x_mksatom(NULL, i);
	_it_should("return the integer value",
		i == x_atomint(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_atomchar(void)
{
	x_obj_t *p_obj;
	x_char_t c = rand();

	helper_alloc_reset();

	p_obj = x_mksatom(NULL, c);
	_it_should("return the character value",
		c == x_atomchar(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_atomstr(void)
{
	x_obj_t *p_obj;
	char *s = "test";

	helper_alloc_reset();

	p_obj = x_mksatom(NULL, s);
	_it_should("return the string value",
		s == x_atomstr(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}

static char *test_atomfn(void)
{
	x_obj_t *p_obj;

	helper_alloc_reset();

	p_obj = x_mksatom(NULL, test_prim_fn);
	_it_should("return the function pointer value",
		test_prim_fn == x_atomfn(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_restptr(void)
{
	x_obj_t *p_obj = NULL;
	void *ptr = (void *)&p_obj;

	helper_alloc_reset();

	p_obj = x_mkspair(NULL, 0, ptr);
	_it_should("return the void pointer value",
		ptr == x_restptr(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_restobj(void)
{
	x_obj_t *p_obj[2];

	helper_alloc_reset();

	p_obj[0] = x_mkspair(NULL, 0, NULL);
	p_obj[1] = x_mkspair(NULL, 0, p_obj[0]);
	_it_should("return the object pointer value",
		p_obj[0] == x_restobj(p_obj[1])
	);
	x_obj_free(p_obj[1]);
	x_obj_free(p_obj[0]);

	return NULL;
}


static char *test_restint(void)
{
	x_obj_t *p_obj;
	x_int_t i = rand();

	helper_alloc_reset();

	p_obj = x_mkspair(NULL, 0, i);
	_it_should("return the integer value",
		i == x_restint(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_restchar(void)
{
	x_obj_t *p_obj;
	x_char_t c = rand();

	helper_alloc_reset();

	p_obj = x_mkspair(NULL, 0, c);
	_it_should("return the character value",
		c == x_restchar(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *test_reststr(void)
{
	x_obj_t *p_obj;
	char *s = "test";

	helper_alloc_reset();

	p_obj = x_mkspair(NULL, 0, s);
	_it_should("return the string value",
		s == x_reststr(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}

static char *test_restfn(void)
{
	x_obj_t *p_obj;

	helper_alloc_reset();

	p_obj = x_mkspair(NULL, 0, test_prim_fn);
	_it_should("return the function pointer value",
		test_prim_fn == x_restfn(p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}


static char *run_tests()
{
	_run_test(test_obj_sys_alloc);
	_run_test(test_obj_sys_make);
	_run_test(test_obj_sys_free);

	_run_test(test_obj_prim_type_name);
	_run_test(test_obj_type_name);

	_run_test(test_obj_prim_units);
	_run_test(test_obj_units);
	_run_test(test_obj_prim_length);
	_run_test(test_obj_length);

	_run_test(test_obj_is_nil);

	_run_test(test_obj_type);
	_run_test(test_obj_flags);
	_run_test(test_obj_data);
	_run_test(test_obj_data_ptr);
	_run_test(test_obj_type_issatom);
	_run_test(test_obj_type_isspair);
	_run_test(test_obj_type_isnil);
	_run_test(test_obj_is_type);

	_run_test(test_obj_set);

	_run_test(test_make_atom);
	_run_test(test_make_atom_own);
	_run_test(test_make_pair);

	_run_test(test_mkatom);
	_run_test(test_mkatomown);
	_run_test(test_mkpair);

	_run_test(test_ptr);
	_run_test(test_obj);
	_run_test(test_int);
	_run_test(test_char);
	_run_test(test_str);
	_run_test(test_fn);

	_run_test(test_first);
	_run_test(test_second);
	_run_test(test_rest);

	_run_test(test_firstptr);
	_run_test(test_firstobj);
	_run_test(test_firstint);
	_run_test(test_firstchar);
	_run_test(test_firststr);
	_run_test(test_firstfn);

	_run_test(test_secondptr);
	_run_test(test_secondobj);
	_run_test(test_secondint);
	_run_test(test_secondchar);
	_run_test(test_secondstr);
	_run_test(test_secondfn);

	_run_test(test_atomptr);
	_run_test(test_atomobj);
	_run_test(test_atomint);
	_run_test(test_atomchar);
	_run_test(test_atomstr);
	_run_test(test_atomfn);

	_run_test(test_restptr);
	_run_test(test_restobj);
	_run_test(test_restint);
	_run_test(test_restchar);
	_run_test(test_reststr);
	_run_test(test_restfn);

	return NULL;
}
