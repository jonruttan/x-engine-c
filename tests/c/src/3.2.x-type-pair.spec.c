/*
 * # Unit Tests: *x-type/pair*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

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
#define STUB_X_EVAL
#include "src/x-eval.c"
#include "ext/x-expr/src/x-heap.c"
#include "src/x-type.c"
#include "src/x-type/prim.c"
#include "src/x-type/atom.c"
#include "src/x-type/pair.c"

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

/*
 * ## Test Runners
 */

#define nil			p_base
#define pair(X,Y)	(x_mkspair(p_base, X_OBJ_FLAG_NONE, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, X_OBJ_FLAG_NONE, (X)))

static char *test_obj_type_ispair(void)
{
	x_obj_t *p_base, *p_obj;

	p_base = x_eval_make(NULL, NULL);

	p_obj = x_mkpair(p_base, 0, 0);
	_it_should("return true when object is a pair",
		1 == x_obj_type_ispair(p_base, p_obj)
	);

	p_obj = x_mkspair(p_base, X_OBJ_FLAG_NONE, 0, 0);
	_it_should("return true when object is a statically registered pair",
		1 == x_obj_type_ispair(p_base, p_obj)
	);

	p_obj = x_mkprim(p_base, 0);
	_it_should("return false when object is not a pair",
		0 == x_obj_type_ispair(p_base, p_obj)
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_mkpair(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t i1 = rand(), i2 = rand();

	p_base = x_eval_make(NULL, NULL);

	p_obj = x_mkpair(p_base, (void *)i1, (void *)i2);
	_it_should("make a Pair object and set its first and rest values",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_ispair(p_base, p_obj)
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj)
		&& i1 == x_firstint(p_obj)
		&& i2 == x_restint(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_eval_make(NULL, NULL);
	p_obj = x_mkpair(p_base, (void *)i1, (void *)i2);
	_it_should("make a Pair object with a real base and set its first and rest values",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_ispair(p_base, p_obj)
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj)
		&& i1 == x_firstint(p_obj)
		&& i2 == x_restint(p_obj)
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_mkfpair(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t i1 = rand(), i2 = rand();
	x_obj_flag_t flags = rand();

	p_base = x_eval_make(NULL, NULL);

	p_obj = x_mkfpair(p_base, flags, (void *)i1, (void *)i2);
	_it_should("make a Pair object and set its first and rest values",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_ispair(p_base, p_obj)
		&& flags == (x_obj_flag_t)x_obj_flags(p_obj)
		&& i1 == x_firstint(p_obj)
		&& i2 == x_restint(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_eval_make(NULL, NULL);
	p_obj = x_mkfpair(p_base, flags, (void *)i1, (void *)i2);
	_it_should("make a Pair object with a real base and set its first and rest values",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_ispair(p_base, p_obj)
		&& flags == (x_obj_flag_t)x_obj_flags(p_obj)
		&& i1 == x_firstint(p_obj)
		&& i2 == x_restint(p_obj)
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_make_pair(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t i1 = rand(), i2 = rand();
	x_obj_flag_t flags = rand();

	p_base = x_eval_make(NULL, NULL);

	p_obj = x_make_pair(p_base, flags, (void *)i1, (void *)i2);
	_it_should("make a Pair object and set its first and rest values",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_ispair(p_base, p_obj)
		&& flags == (x_obj_flag_t)x_obj_flags(p_obj)
		&& i1 == x_firstint(p_obj)
		&& i2 == x_restint(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_eval_make(NULL, NULL);
	p_obj = x_make_pair(p_base, flags, (void *)i1, (void *)i2);
	_it_should("make a Pair object with a real base and set its first and rest values",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_ispair(p_base, p_obj)
		&& flags == (x_obj_flag_t)x_obj_flags(p_obj)
		&& i1 == x_firstint(p_obj)
		&& i2 == x_restint(p_obj)
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_type_pair_register(void)
{
	x_obj_t *p_base, *p_type;

	p_base = x_eval_make(NULL, NULL);

	p_type = x_type_pair_register(p_base, p_base);
	_it_should("return the Pair type object",
		0 == x_lib_strcmp(X_TYPE_PAIR_SYMBOL, x_atomstr(x_type_field_name(p_type)))
	);
	_it_should("add the Pair type to the Type alist",
		p_type == x_restobj(x_firstobj(x_firstobj(x_eval_field_type_alist(p_base))))
	);

	x_sys_free(p_base);

	return NULL;
}

static char *test_type_pair_struct(void)
{
	x_obj_t *p_base, *p_type;

	helper_alloc_reset();

	p_base = x_eval_make(NULL, NULL);
	p_type = x_type_pair_struct(p_base, p_base);
	_it_should("return Pair Type list",
		! x_obj_isnil(p_base, p_type)
		&& 0 == strcmp(X_TYPE_PAIR_SYMBOL, x_atomstr(x_type_field_name(p_type)))
	);

	_it_should("contain the Name object",
		x_type_pair_name == x_type_field_name(p_type)
	);

	_it_should("set the Data object to nil",
		NULL == x_type_field_data(p_type)
	);

	_it_should("set the Make primitive",
		x_type_pair_make == x_atomfn(x_type_field_make(p_type))
	);

	_it_should("not set the Free primitive",
		NULL == x_type_field_free(p_type)
	);

	_it_should("not set the Clone primitive",
		NULL == x_type_field_clone(p_type)
	);

	_it_should("set the Units primitive",
		(x_obj_t *)&x_type_units_pair_obj == x_type_field_units(p_type)
	);

	_it_should("set the Length primitive",
		x_type_pair_length_prim == x_type_field_length(p_type)
	);

	_it_should("set the Call primitive",
		NULL == x_type_field_call(p_type)
	);

	/* TODO: Eval */
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

	_it_should("leaves the Write cell empty (the printer is x-lang)",
		x_type_field_write(p_type) == NULL
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_base_alist_assoc(void)
{
	x_obj_t *p_base, *p_type;

	helper_alloc_reset();

	p_base = x_eval_make(NULL, NULL);
	x_eval_type_alist_extend(p_base, x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkspair(p_base, X_OBJ_FLAG_NONE, x_type_pair_name, NULL), atom(1)));

	p_type = x_eval_type_alist_assoc(p_base, x_mkspair(p_base, X_OBJ_FLAG_NONE, x_type_pair_name, NULL));
	_it_should("find the type in the Type alist and return its properties",
		x_type_pair_name == x_type_field_name(p_type)
	);

	return NULL;
}

static char *test_type_pair_make(void)
{
	x_obj_t *p_base, *p_pair, *p_flags, *p_args, *p_obj[2];

	helper_alloc_reset();

	/* NULL p_base object */
	p_pair = x_mkspair(NULL, X_OBJ_FLAG_NONE, rand(), rand());
	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_pair, NULL);

	p_obj[0] = x_type_pair_make(NULL, p_args);
	_it_should("make a Pair object and set its value",
		p_obj[0] != NULL
		&& x_firstobj(p_pair) == x_firstobj(p_obj[0])
		&& x_restobj(p_pair) == x_restobj(p_obj[0])
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj[0])
	);

	/* w/flags */
	x_firstint(p_pair) = rand();
	x_restint(p_pair) = rand();
	p_flags = x_mksatom(NULL, X_OBJ_FLAG_NONE, rand());
	x_restobj(p_args) = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_flags, NULL);

	p_obj[1] = x_type_pair_make(NULL, p_args);
	_it_should("make a second Pair object and set its value and flags",
		p_obj[1] != NULL
		&& x_firstint(p_pair) == x_firstint(p_obj[1])
		&& x_restint(p_pair) == x_restint(p_obj[1])
		&& x_atomint(p_flags) == x_obj_flags(p_obj[1])
		&& p_obj[0] != p_obj[1]
	);

	_it_should("have not have returned the same type object for both objects",
		x_obj_type(p_obj[0]) != x_obj_type(p_obj[1])
	);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(x_restobj(p_args));
	x_sys_free(p_args);
	x_sys_free(p_flags);
	x_sys_free(p_pair);


	/* Real base object */
	p_base = x_eval_make(NULL, NULL);
	p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE, rand(), rand());
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_pair, NULL);

	p_obj[0] = x_type_pair_make(p_base, p_args);
	_it_should("make a Pair object with a real base and set its value",
		! x_obj_isnil(p_base, p_obj[0])
		&& x_obj_type_ispair(p_base, p_obj[0])
	);

	/* w/flags */
	x_firstint(p_pair) = rand();
	x_restint(p_pair) = rand();
	p_flags = x_mksatom(NULL, X_OBJ_FLAG_NONE, rand());
	x_restobj(p_args) = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_flags, NULL);

	p_obj[1] = x_type_pair_make(p_base, p_args);
	_it_should("make a second Pair object with an empty base and set its value",
		! x_obj_isnil(p_base, p_obj[1])
		&& x_obj_type_ispair(p_base, p_obj[1])
		&& x_firstint(p_pair) == x_firstint(p_obj[1])
		&& x_restint(p_pair) == x_restint(p_obj[1])
		&& x_atomint(p_flags) == x_obj_flags(p_obj[1])
		&& p_obj[0] != p_obj[1]
	);

	_it_should("have returned the same registered type for both objects",
		x_obj_type(p_obj[0]) == x_obj_type(p_obj[1])
	);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(x_restobj(p_args));
	x_sys_free(p_args);
	x_sys_free(p_flags);
	x_sys_free(p_pair);
	x_sys_free(p_base);


	/* With p_base object */
	p_base = x_eval_make(NULL, NULL);
	p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE, rand(), rand());
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_pair, NULL);

	p_obj[0] = x_type_pair_make(p_base, p_args);
	_it_should("make an Pair object with a base object and set its value",
		! x_obj_isnil(p_base, p_obj[0])
		&& x_obj_type_ispair(p_base, p_obj[0])
		&& x_firstint(p_pair) == x_firstint(p_obj[0])
		&& x_restint(p_pair) == x_restint(p_obj[0])
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj[0])
	);

	/* w/flags */
	x_firstint(p_pair) = rand();
	x_restint(p_pair) = rand();
	p_flags = x_mksatom(NULL, X_OBJ_FLAG_NONE, rand());
	x_restobj(p_args) = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_flags, NULL);

	p_obj[1] = x_type_pair_make(p_base, p_args);
	_it_should("make a second Pair object a base object and set its value and flags",
		! x_obj_isnil(p_base, p_obj[1])
		&& x_obj_type_ispair(p_base, p_obj[1])
		&& x_firstint(p_pair) == x_firstint(p_obj[1])
		&& x_restint(p_pair) == x_restint(p_obj[1])
		&& x_atomint(p_flags) == x_obj_flags(p_obj[1])
		&& p_obj[0] != p_obj[1]
	);
	_it_should("have returned the same type object for both objects",
		x_obj_type(p_obj[0]) == x_obj_type(p_obj[1])
	);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(x_restobj(p_args));
	x_sys_free(p_args);
	x_sys_free(p_flags);
	x_sys_free(p_pair);
	x_sys_free(p_base);


	return NULL;
}

static char *test_type_pair_length(void)
{
	x_obj_t *p_base, *p_list, *p_args, *p_ret;

	helper_alloc_reset();

	p_base = x_eval_make(NULL, NULL);

	/* Empty list */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, NULL);
	p_ret = x_type_pair_length(p_base, p_args);
	_it_should("return 0 for an empty list",
		0 == x_atomint(p_ret));

	/* 3-element list */
	p_list = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 1),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 2),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 3), NULL)));
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_list, NULL);
	p_ret = x_type_pair_length(p_base, p_args);
	_it_should("return 3 for a 3-element list",
		3 == x_atomint(p_ret));

	return NULL;
}

static char *run_tests() {
	_run_test(test_obj_type_ispair);
	_run_test(test_mkpair);
	_run_test(test_mkfpair);
	_run_test(test_make_pair);
	_run_test(test_type_pair_register);
	_run_test(test_type_pair_struct);
	_run_test(test_base_alist_assoc);
	_run_test(test_type_pair_make);
	_run_test(test_type_pair_length);

	return NULL;
}
