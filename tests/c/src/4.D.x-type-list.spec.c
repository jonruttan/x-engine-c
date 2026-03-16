/*
 * # Unit Tests: *x-type/list*
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
#include "ext/x-expr/src/x-obj.c"
#include "src/x-alist.c"
#include "src/x-base.c"
#include "src/x-eval.c"
#include "src/x-type.c"
#include "src/x-type/buffer.c"
#include "src/x-type/iter.c"
#include "src/x-type/list.c"
#include "src/x-type/prim.c"
#include "src/x-token/sexp/atom.c"
#include "src/x-token/sexp/pair.c"
#include "src/x-token/sexp/list.c"
#include "src/x-token.c"

#define STUB_X_PRIM
#define STUB_X_PROCEDURE
#define STUB_X_OPERATIVE
#define STUB_X_HEAP
#define STUB_X_OBJ_OBJ
#define STUB_X_STR
#define STUB_X_INT
#define STUB_X_SYMBOL
#define STUB_X_PRIM_REGISTER
#include "helper-stubs.c"

#include "ext/x-expr/tests/src/helper-system-functions.c"

/*
 * ## Test Overhead
 */

static void _setup(void)
{
	_buffer_index = -1;
	helper_set_alloc(MEM_GUARANTEED);
	x_obj_hook_type_name = x_type_prim_type_name;
	x_obj_hook_units = x_type_prim_units;
	x_obj_hook_length = x_type_prim_length;
	x_obj_hook_error = x_base_error;
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

#define nil     NULL
#define pair(X,Y) (x_mkspair(p_base, (X), (Y)))
#define atom(X)   (x_mksatom(p_base, (X)))

static char *test_obj_type_islist(void)
{
	x_obj_t *p_obj;

	helper_alloc_reset();

	p_obj = x_mklist(NULL, 0, 0);
	_it_should("return true when object is a list",
		1 == x_obj_type_islist(NULL, p_obj)
	);
	x_obj_free(p_obj);

	p_obj = x_mksatom(NULL, 0);
	_it_should("return false when object is not a list",
		0 == x_obj_type_islist(NULL, p_obj)
	);
	x_obj_free(p_obj);

	return NULL;
}

static char *test_mklist(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t i1 = rand(), i2 = rand();

	p_obj = x_mklist(NULL, (void *)i1, (void *)i2);
	_it_should("make a List object and set its first and rest values",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_islist(NULL, p_obj)
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj)
		&& i1 == x_firstint(p_obj)
		&& i2 == x_restint(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_mklist(p_base, (void *)i1, (void *)i2);
	_it_should("make a List object, attach it to the Base object, and set its first and rest values",
		! x_obj_isnil(p_base, p_obj)
		&& p_obj == x_obj_heap(p_base)
		&& x_obj_type_islist(p_base, p_obj)
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj)
		&& p_obj == x_obj_heap(p_base)
		&& i1 == x_firstint(p_obj)
		&& i2 == x_restint(p_obj)
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_mkflist(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t i1 = rand(), i2 = rand();
	x_obj_flag_t flags = rand();

	p_obj = x_mkflist(NULL, flags, (void *)i1, (void *)i2);
	_it_should("make a List object and set its first and rest values",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_islist(NULL, p_obj)
		&& flags == (x_obj_flag_t)x_obj_flags(p_obj)
		&& i1 == x_firstint(p_obj)
		&& i2 == x_restint(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_mkflist(p_base, flags, (void *)i1, (void *)i2);
	_it_should("make a List object, attach it to the Base object, and set its first and rest values",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_islist(p_base, p_obj)
		&& flags == (x_obj_flag_t)x_obj_flags(p_obj)
		&& p_obj == x_obj_heap(p_base)
		&& i1 == x_firstint(p_obj)
		&& i2 == x_restint(p_obj)
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_make_list(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t i1 = rand(), i2 = rand();
	x_obj_flag_t flags = rand();

	p_obj = x_make_list(NULL, flags, (void *)i1, (void *)i2);
	_it_should("make a List object and set its first and rest values",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_islist(NULL, p_obj)
		&& flags == (x_obj_flag_t)x_obj_flags(p_obj)
		&& i1 == x_firstint(p_obj)
		&& i2 == x_restint(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, 0);
	p_obj = x_make_list(p_base, flags, (void *)i1, (void *)i2);
	_it_should("make a List object, attach it to the Base object, and set its first and rest values",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_islist(p_base, p_obj)
		&& flags == (x_obj_flag_t)x_obj_flags(p_obj)
		&& p_obj == x_obj_heap(p_base)
		&& i1 == x_firstint(p_obj)
		&& i2 == x_restint(p_obj)
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_type_list_register(void)
{
	x_obj_t *p_base, *p_type;

	p_base = x_base_make(NULL, NULL);

	p_type = x_type_list_register(p_base, p_base);
	_it_should("return the List type object",
		0 == x_lib_strcmp(X_TYPE_LIST_NAME, x_atomstr(x_type_field_name(p_type)))
	);
	_it_should("add the List type to the Type alist",
		p_type == x_restobj(x_firstobj(x_base_field_type_alist(p_base)))
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_type_list_struct(void)
{
	x_obj_t *p_base, *p_type;

	helper_alloc_reset();

	p_base = x_mklist(NULL, NULL, NULL);
	p_type = x_type_list_struct(p_base, p_base);
	_it_should("return List Type list",
		! x_obj_isnil(p_base, p_type)
		&& 0 == strcmp(X_TYPE_LIST_NAME, x_atomstr(x_type_field_name(p_type)))
	);

	_it_should("contain the Name object",
		x_type_list_name == x_type_field_name(p_type)
	);

	_it_should("set the Data object to nil",
		NULL == x_type_field_data(p_type)
	);

	_it_should("set the Make primitive",
		x_type_list_make_prim == x_type_field_make(p_type)
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
		x_type_list_length_prim == x_type_field_length(p_type)
	);

	_it_should("set the Call primitive",
		x_type_list_call_prim == x_type_field_call(p_type)
	);

	/* TODO: Eval */
	_it_should("set the Eval primitive",
		x_type_list_eval_prim == x_type_field_eval(p_type)
	);

	_it_should("not set the From alist",
		NULL == x_type_field_from(p_type)
	);

	_it_should("not set the To alist",
		NULL == x_type_field_to(p_type)
	);

	_it_should("set the Analyse primitive",
		x_sexp_list_analyse_prim == x_type_field_analyse(p_type)
	);

	_it_should("set the Delimit primitive",
		x_sexp_list_delimit_prim == x_type_field_delimit(p_type)
	);

	_it_should("set the Write primitive",
		x_sexp_list_write_prim == x_type_field_write(p_type)
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_base_alist_assoc(void)
{
	x_obj_t *p_base, *p_type;

	helper_alloc_reset();

	p_base = x_base_make(NULL, NULL);
	x_base_type_alist_extend(p_base, x_mkspair(p_base, x_mkspair(p_base, x_type_list_name, NULL), atom(1)));

	p_type = x_base_type_alist_assoc(p_base, x_mkspair(p_base, x_type_list_name, NULL));
	_it_should("find the type in the Type alist and return its properties",
		x_type_list_name == x_type_field_name(p_type)
	);

	return NULL;
}

static char *test_type_list_make(void)
{
	x_obj_t *p_base, *p_list, *p_flags, *p_args, *p_obj[2];

	helper_alloc_reset();

	/* NULL p_base object */
	p_list = x_mkspair(NULL, rand(), rand());
	p_args = x_mkspair(NULL, p_list, NULL);

	p_obj[0] = x_type_list_make(NULL, p_args);
	_it_should("make a List object and set its value",
		! x_obj_isnil(NULL, p_obj[0])
		&& x_obj_type_islist(NULL, p_obj[0])
		&& x_firstobj(p_list) == x_firstobj(p_obj[0])
		&& x_restobj(p_list) == x_restobj(p_obj[0])
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj[0])
	);

	/* w/flags */
	x_firstint(p_list) = rand();
	x_restint(p_list) = rand();
	p_flags = x_mksatom(NULL, rand());
	x_restobj(p_args) = x_mkspair(NULL, p_flags, NULL);

	p_obj[1] = x_type_list_make(NULL, p_args);
	_it_should("make a second List object and set its value and flags",
		! x_obj_isnil(NULL, p_obj[1])
		&& x_obj_type_islist(NULL, p_obj[1])
		&& x_firstint(p_list) == x_firstint(p_obj[1])
		&& x_restint(p_list) == x_restint(p_obj[1])
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
	x_sys_free(p_list);


	/* Empty p_base object */
	p_base = x_mksatom(NULL, NULL);
	p_list = x_mkspair(p_base, rand(), rand());
	p_args = x_mkspair(p_base, p_list, NULL);

	p_obj[0] = x_type_list_make(p_base, p_args);
	_it_should("make a List object with an empty base and set its value",
		! x_obj_isnil(NULL, p_obj[0])
		&& x_obj_type_islist(p_base, p_obj[0])
	);

	/* w/flags */
	x_firstint(p_list) = rand();
	x_restint(p_list) = rand();
	p_flags = x_mksatom(NULL, rand());
	x_restobj(p_args) = x_mkspair(NULL, p_flags, NULL);

	p_obj[1] = x_type_list_make(p_base, p_args);
	_it_should("make a second List object with an empty base and set its value",
		! x_obj_isnil(p_base, p_obj[1])
		&& x_obj_type_islist(p_base, p_obj[1])
		&& x_firstint(p_list) == x_firstint(p_obj[1])
		&& x_restint(p_list) == x_restint(p_obj[1])
		&& x_atomint(p_flags) == x_obj_flags(p_obj[1])
		&& p_obj[0] != p_obj[1]
	);

	_it_should("not have returned the same type object for both objects",
		x_obj_type(p_obj[0]) != x_obj_type(p_obj[1])
	);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(x_restobj(p_args));
	x_sys_free(p_args);
	x_sys_free(p_flags);
	x_sys_free(p_list);
	x_sys_free(p_base);


	/* With p_base object */
	p_base = x_base_make(NULL, NULL);
	p_list = x_mkspair(p_base, rand(), rand());
	p_args = x_mkspair(p_base, p_list, NULL);

	p_obj[0] = x_type_list_make(p_base, p_args);
	_it_should("make an List object with a base object and set its value",
		! x_obj_isnil(p_base, p_obj[0])
		&& x_obj_type_islist(p_base, p_obj[0])
		&& x_firstint(p_list) == x_firstint(p_obj[0])
		&& x_restint(p_list) == x_restint(p_obj[0])
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj[0])
	);

	/* w/flags */
	x_firstint(p_list) = rand();
	x_restint(p_list) = rand();
	p_flags = x_mksatom(NULL, rand());
	x_restobj(p_args) = x_mkspair(NULL, p_flags, NULL);

	p_obj[1] = x_type_list_make(p_base, p_args);
	_it_should("make a second List object a base object and set its value and flags",
		! x_obj_isnil(p_base, p_obj[1])
		&& x_obj_type_islist(p_base, p_obj[1])
		&& x_firstint(p_list) == x_firstint(p_obj[1])
		&& x_restint(p_list) == x_restint(p_obj[1])
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
	x_sys_free(p_list);
	x_sys_free(p_base);


	return NULL;
}


x_obj_t *test_prim(x_obj_t *p_base, x_obj_t *p_args)
{
	/* Hello, World! */
	/* puts(x_firststr(x_0(p_args))); */

	return x_0(p_args);
}

x_satom_t test_type_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = "PRIM" });

static char *test_type_list_eval(void)
{
	x_obj_t *p_base, *p_type, *p_list, *p_prim, *p_atom, *p_args, *p_ret;
	struct x_type_t type = {
			.p_name = test_type_name,
			.p_call = x_type_prim_call_prim
		};

	helper_alloc_reset();

	p_base = x_mksatom(NULL, NULL);
	p_type = x_type_struct_make(p_base, type);
	p_prim = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE, X_OBJ_LENGTH_ATOM, test_prim);
	p_atom = x_mksatom(p_base, "Hello, World!");
	p_list = x_mklist(p_base, p_prim, x_mkspair(p_base, p_atom, NULL));
	p_args = x_mkspair(p_base, x_mkspair(p_base, p_list, NULL), NULL);
	p_ret = x_type_list_eval(p_base, p_args);
	_it_should("return the first argument", p_atom == p_ret);
	p_ret = x_eval(p_base, p_args);
	_it_should("return the first argument", p_atom == p_ret);

	test_cleanup(p_base);

	return NULL;
}

static char *test_type_list_eval_nil_operator(void)
{
	x_obj_t *p_base, *p_list, *p_args, *p_ret;

	helper_alloc_reset();

	/* List whose first element evaluates to nil -> returns p_exp */
	p_base = x_base_make(NULL, NULL);
	p_list = x_mklist(p_base, NULL, pair(atom(42), nil));
	p_args = pair(pair(p_list, nil), nil);
	p_ret = x_type_list_eval(p_base, p_args);
	_it_should("return exp when operator is nil",
		p_ret == p_list);
	test_cleanup(p_base);

	return NULL;
}

static char *test_type_list_eval_no_call(void)
{
	x_obj_t *p_base, *p_type, *p_obj, *p_list, *p_args, *p_ret;
	struct x_type_t type_desc;

	helper_alloc_reset();

	/* Type with no call field -> returns p_exp */
	p_base = x_base_make(NULL, NULL);

	x_lib_memset(&type_desc, 0, sizeof(type_desc));
	type_desc.p_name = x_mksatom(p_base, "NOCALL");
	type_desc.p_units = (x_obj_t *)&x_type_units_pair_obj;

	p_type = x_type_struct_make(p_base, type_desc);
	p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE,
		X_OBJ_LENGTH_PAIR, NULL, NULL);
	p_list = x_mklist(p_base, p_obj, pair(atom(42), nil));
	p_args = pair(pair(p_list, nil), nil);
	p_ret = x_type_list_eval(p_base, p_args);
	_it_should("return exp when operator type has no call field",
		p_ret == p_list);
	test_cleanup(p_base);

	return NULL;
}

static char *test_type_list_iter(void)
{
	x_obj_t *p_base, *p_list, *p_iter, *p_args, *p_ret;

	helper_alloc_reset();

	/* Make a simple base to help with cleanup. */
	p_base = x_mksatom(NULL, NULL);
	p_list = x_mkspair(p_base, x_mksatom(p_base, 0),
		x_mkspair(p_base, x_mksatom(p_base, 1),
		x_mkspair(p_base, x_mksatom(p_base, 2),
		NULL)));
	p_iter = x_mkiter(p_base, x_type_list_iter_prim, p_list);
	p_args = x_mkspair(p_base, p_iter, NULL);
	p_ret = x_type_list_iter(p_base, p_args);
	_it_should("return the first item in the list",
		x_0(p_list) == p_ret
	);
	p_ret = x_type_list_iter(p_base, p_args);
	_it_should("return the next item in the list",
		x_01(p_list) == p_ret
	);
	p_ret = x_type_list_iter(p_base, p_args);
	_it_should("return the last item in the list",
		x_011(p_list) == p_ret
	);
	p_ret = x_type_list_iter(p_base, p_args);
	_it_should("return nil",
		x_obj_isnil(p_base, p_ret)
	);
	p_ret = x_type_list_iter(p_base, p_args);
	_it_should("return nil",
		x_obj_isnil(p_base, p_ret)
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_type_list_length(void)
{
	x_obj_t *p_base, *p_list, *p_args, *p_ret;

	/* empty list */
	p_base = x_base_make(NULL, NULL);
	p_args = pair(nil, nil);
	p_ret = x_type_list_length(p_base, p_args);
	_it_should("return 0 for nil list", x_atomint(p_ret) == 0);
	test_cleanup(p_base);

	/* single element */
	p_base = x_base_make(NULL, NULL);
	p_list = x_mklist(p_base, atom(1), nil);
	p_args = pair(p_list, nil);
	p_ret = x_type_list_length(p_base, p_args);
	_it_should("return 1 for single-element list", x_atomint(p_ret) == 1);
	test_cleanup(p_base);

	/* three elements */
	p_base = x_base_make(NULL, NULL);
	p_list = x_mklist(p_base, atom(1),
		x_mklist(p_base, atom(2),
		x_mklist(p_base, atom(3), nil)));
	p_args = pair(p_list, nil);
	p_ret = x_type_list_length(p_base, p_args);
	_it_should("return 3 for three-element list", x_atomint(p_ret) == 3);
	test_cleanup(p_base);

	return NULL;
}

static char *test_type_list_call(void)
{
	x_obj_t *p_base, *p_list, *p_args, *p_ret;

	/* no args returns nil */
	p_base = x_base_make(NULL, NULL);
	p_list = x_mklist(p_base, atom(10), x_mklist(p_base, atom(20), nil));
	p_args = pair(p_list, nil);
	p_ret = x_type_list_call(p_base, p_args);
	_it_should("return nil with no args", p_ret == NULL);
	test_cleanup(p_base);

	/* single index 0 */
	p_base = x_base_make(NULL, NULL);
	p_list = x_mklist(p_base, atom(10),
		x_mklist(p_base, atom(20),
		x_mklist(p_base, atom(30), nil)));
	p_args = pair(p_list, pair(atom(0), nil));
	p_ret = x_type_list_call(p_base, p_args);
	_it_should("index 0 returns first element",
		p_ret != NULL && x_atomint(p_ret) == 10);
	test_cleanup(p_base);

	/* single index 2 */
	p_base = x_base_make(NULL, NULL);
	p_list = x_mklist(p_base, atom(10),
		x_mklist(p_base, atom(20),
		x_mklist(p_base, atom(30), nil)));
	p_args = pair(p_list, pair(atom(2), nil));
	p_ret = x_type_list_call(p_base, p_args);
	_it_should("index 2 returns third element",
		p_ret != NULL && x_atomint(p_ret) == 30);
	test_cleanup(p_base);

	/* out of bounds index */
	p_base = x_base_make(NULL, NULL);
	p_list = x_mklist(p_base, atom(10), nil);
	p_args = pair(p_list, pair(atom(5), nil));
	p_ret = x_type_list_call(p_base, p_args);
	_it_should("out of bounds returns nil", p_ret == NULL);
	test_cleanup(p_base);

	/* negative index -1 */
	p_base = x_base_make(NULL, NULL);
	p_list = x_mklist(p_base, atom(10),
		x_mklist(p_base, atom(20),
		x_mklist(p_base, atom(30), nil)));
	p_args = pair(p_list, pair(atom((x_int_t)-1), nil));
	p_ret = x_type_list_call(p_base, p_args);
	_it_should("index -1 returns last element",
		p_ret != NULL && x_atomint(p_ret) == 30);
	test_cleanup(p_base);

	/* negative index -2 */
	p_base = x_base_make(NULL, NULL);
	p_list = x_mklist(p_base, atom(10),
		x_mklist(p_base, atom(20),
		x_mklist(p_base, atom(30), nil)));
	p_args = pair(p_list, pair(atom((x_int_t)-2), nil));
	p_ret = x_type_list_call(p_base, p_args);
	_it_should("index -2 returns second-to-last",
		p_ret != NULL && x_atomint(p_ret) == 20);
	test_cleanup(p_base);

	/* slice: (list 1 2) from (10 20 30 40) */
	p_base = x_base_make(NULL, NULL);
	p_list = x_mklist(p_base, atom(10),
		x_mklist(p_base, atom(20),
		x_mklist(p_base, atom(30),
		x_mklist(p_base, atom(40), nil))));
	p_args = pair(p_list, pair(atom(1), pair(atom(2), nil)));
	p_ret = x_type_list_call(p_base, p_args);
	_it_should("slice returns first element 20",
		p_ret != NULL && x_atomint(x_firstobj(p_ret)) == 20);
	_it_should("slice returns second element 30",
		x_atomint(x_firstobj(x_restobj(p_ret))) == 30);
	_it_should("slice has two elements",
		x_obj_isnil(p_base, x_restobj(x_restobj(p_ret))));
	test_cleanup(p_base);

	/* slice: start=0 len=0 returns nil */
	p_base = x_base_make(NULL, NULL);
	p_list = x_mklist(p_base, atom(10), nil);
	p_args = pair(p_list, pair(atom(0), pair(atom(0), nil)));
	p_ret = x_type_list_call(p_base, p_args);
	_it_should("slice with len=0 returns nil", p_ret == NULL);
	test_cleanup(p_base);

	return NULL;
}

static char *run_tests() {
	_run_test(test_obj_type_islist);
	_run_test(test_mklist);
	_run_test(test_mkflist);
	_run_test(test_make_list);
	_run_test(test_type_list_register);
	_run_test(test_type_list_struct);
	_run_test(test_base_alist_assoc);
	_run_test(test_type_list_make);
	_run_test(test_type_list_eval);
	_run_test(test_type_list_eval_nil_operator);
	_run_test(test_type_list_eval_no_call);
	_run_test(test_type_list_iter);
	_run_test(test_type_list_length);
	_run_test(test_type_list_call);

	return NULL;
}
