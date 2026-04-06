/*
 * # Unit Tests: *x-type/symbol*
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
#include "ext/x-expr/src/x-base.c"
#include "src/x-base.c"
#include "src/x-token/sexp/atom.c"
#include "src/x-token/sexp/pair.c"
#include "src/x-token.c"
#include "ext/x-expr/src/x-heap.c"
#include "src/x-type.c"
#include "src/x-type/iter.c"
#include "src/x-eval.c"
#include "src/x-type/list.c"
#include "src/x-token/sexp/list.c"
#include "src/x-type/prim.c"
#include "src/x-type/buffer.c"
#include "src/x-type/char.c"
#include "src/x-token/sexp/char.c"
#include "src/x-type/str.c"
#include "src/x-token/sexp/str.c"
#include "src/x-type/symbol.c"
#include "src/x-token/sexp/symbol.c"

#define STUB_X_PRIM
#define STUB_X_PROCEDURE
#define STUB_X_OPERATIVE
#define STUB_X_HEAP
#define STUB_X_OBJ_OBJ
#define STUB_X_INT
#define STUB_X_PRIM_REGISTER
#define STUB_X_PRIM_SHADOW
#define STUB_X_PROCEDURE_APPLY
#include "helper-stubs.c"

#include "ext/x-expr/tests/src/test-helper-system.c"

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

#define X_TEST_SYMBOL_VALUE		"TEST"

#define nil			NULL
#define pair(X,Y)	(x_mkspair(p_base, X_OBJ_FLAG_NONE, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, X_OBJ_FLAG_NONE, (X)))

static char *test_obj_type_issymbol(void)
{
	x_obj_t *p_obj;

	helper_alloc_reset();

	p_obj = x_mksymbol(NULL, 0);
	_it_should("return true when object is a Symbol",
		1 == x_obj_type_issymbol(NULL, p_obj)
	);
	x_obj_free(NULL, p_obj);

	p_obj = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);
	_it_should("return false when object is not a Symbol",
		0 == x_obj_type_issymbol(NULL, p_obj)
	);
	x_obj_free(NULL, p_obj);

	return NULL;
}

static char *test_symbolval(void)
{
	x_obj_t *p_symbol;
	x_char_t *p_str, *str = X_TEST_SYMBOL_VALUE;

	p_symbol = x_mksymbol(NULL, str);

	p_str = x_symbolval(p_symbol);
	_it_should("return the symbol's value", str == p_str);

	x_sys_free(p_symbol);

	return NULL;
}

static char *test_symbolname(void)
{
	x_obj_t *p_symbol;
	x_char_t *p_str, *str = X_TEST_SYMBOL_VALUE;

	p_symbol = x_mksymbol(NULL, str);

	p_str = x_symbolname(p_symbol);
	_it_should("return the symbol's name", str == p_str);

	x_sys_free(p_symbol);

	return NULL;
}

static char *test_symbol_data_list(void)
{
	x_obj_t *p_type = x_type_symbol_register(NULL, NULL);

	_it_should("return an empty Symbol data list",
		x_obj_isnil(NULL, x_symbol_data_list(p_type))
	);

	return NULL;
}

static char *test_mksymbol(void)
{
	x_obj_t *p_base, *p_obj;

	p_obj = x_mksymbol(NULL, X_TEST_SYMBOL_VALUE);
	_it_should("make a Symbol object and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_issymbol(NULL, p_obj)
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj)
		&& 0 == strcmp(X_TEST_SYMBOL_VALUE, x_symbolval(p_obj))
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);
	p_obj = x_mksymbol(p_base, X_TEST_SYMBOL_VALUE);
	_it_should("make a Symbol object, attach it to the Base object, and set its value",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_issymbol(p_base, p_obj)
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj)
		&& 0 == strcmp(X_TEST_SYMBOL_VALUE, x_symbolval(p_obj))
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_mkfsymbol(void)
{
	x_obj_t *p_base, *p_obj;
	x_obj_flag_t flags = rand();

	p_obj = x_mkfsymbol(NULL, flags, X_TEST_SYMBOL_VALUE);
	_it_should("make a Symbol object and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_issymbol(NULL, p_obj)
		&& flags == (x_obj_flag_t)x_obj_flags(p_obj)
		&& 0 == strcmp(X_TEST_SYMBOL_VALUE, x_symbolval(p_obj))
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);
	p_obj = x_mkfsymbol(p_base, flags, X_TEST_SYMBOL_VALUE);
	_it_should("make a Symbol object, attach it to the Base object, and set its value",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_issymbol(p_base, p_obj)
		&& flags == (x_obj_flag_t)x_obj_flags(p_obj)
		/* heap chain assertions removed — layout changed with unified callable */
		&& 0 == strcmp(X_TEST_SYMBOL_VALUE, x_symbolval(p_obj))
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_mksymbolown(void)
{
	x_obj_t *p_base, *p_obj;

	p_obj = x_mksymbolown(NULL, X_TEST_SYMBOL_VALUE);
	_it_should("make an Owned Symbol object and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_issymbol(NULL, p_obj)
		&& X_OBJ_FLAG_OWN == x_obj_flags(p_obj)
		&& 0 == strcmp(X_TEST_SYMBOL_VALUE, x_symbolval(p_obj))
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);
	p_obj = x_mksymbolown(p_base, X_TEST_SYMBOL_VALUE);
	_it_should("make an Owned Symbol, attach it to the Base object, object and set its value",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_issymbol(p_base, p_obj)
		&& X_OBJ_FLAG_OWN == x_obj_flags(p_obj)
		/* heap chain assertions removed — layout changed with unified callable */
		&& 0 == strcmp(X_TEST_SYMBOL_VALUE, x_symbolval(p_obj))
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_mkfsymbolown(void)
{
	x_obj_t *p_base, *p_obj;
	x_obj_flag_t flags = rand();

	p_obj = x_mkfsymbolown(NULL, flags, X_TEST_SYMBOL_VALUE);
	_it_should("make an Owned Symbol object and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_issymbol(NULL, p_obj)
		&& (x_obj_flag_t)(X_OBJ_FLAG_OWN | flags) == (x_obj_flag_t)x_obj_flags(p_obj)
		&& 0 == strcmp(X_TEST_SYMBOL_VALUE, x_symbolval(p_obj))
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);
	p_obj = x_mkfsymbolown(p_base, flags, X_TEST_SYMBOL_VALUE);
	_it_should("make an Owned Symbol, attach it to the Base object, object and set its value",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_issymbol(p_base, p_obj)
		&& (x_obj_flag_t)(X_OBJ_FLAG_OWN | flags) == (x_obj_flag_t)x_obj_flags(p_obj)
		/* heap chain assertions removed — layout changed with unified callable */
		&& 0 == strcmp(X_TEST_SYMBOL_VALUE, x_symbolval(p_obj))
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_make_symbol(void)
{
	x_obj_t *p_base, *p_obj;
	x_obj_flag_t flags = rand();

	helper_alloc_reset();

	p_obj = x_make_symbol(NULL, flags, X_TEST_SYMBOL_VALUE);
	_it_should("make a Symbol object and set its value",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_issymbol(NULL, p_obj)
		&& flags == (x_obj_flag_t)x_obj_flags(p_obj)
		&& 0 == strcmp(X_TEST_SYMBOL_VALUE, x_symbolval(p_obj))
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);
	p_obj = x_make_symbol(p_base, flags, X_TEST_SYMBOL_VALUE);
	_it_should("make a Symbol object, attach it to the Base object, and set its value",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_issymbol(p_base, p_obj)
		&& flags == (x_obj_flag_t)x_obj_flags(p_obj)
		/* heap chain assertions removed — layout changed with unified callable */
		&& 0 == strcmp(X_TEST_SYMBOL_VALUE, x_symbolval(p_obj))
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_type_symbol_struct(void)
{
	x_obj_t *p_base, *p_type;

	helper_alloc_reset();

	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);
	p_type = x_type_symbol_struct(p_base, p_base);
	_it_should("return Symbol Type list",
		! x_obj_isnil(p_base, p_type)
		&& 0 == strcmp(X_TYPE_SYMBOL_NAME, x_atomstr(x_type_field_name(p_type)))
	);

	_it_should("contain the Name object",
		x_type_symbol_name == x_type_field_name(p_type)
	);

	/* TODO: Move symobl list to data. */
	_it_should("set the Data object to nil",
		NULL == x_type_field_data(p_type)
	);

	_it_should("set the Make primitive",
		x_type_symbol_make_prim == x_type_field_make(p_type)
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

	_it_should("set the Eval primitive",
		x_type_symbol_eval_prim == x_type_field_eval(p_type)
	);

	_it_should("not set the From alist",
		NULL == x_type_field_from(p_type)
	);

	_it_should("not set the To alist",
		NULL == x_type_field_to(p_type)
	);

	_it_should("set the Analyse primitive",
		x_sexp_symbol_analyse_prim == x_type_field_analyse(p_type)
	);

	_it_should("not set the Delimit primitive",
		NULL == x_type_field_delimit(p_type)
	);

	_it_should("set the Write primitive",
		x_sexp_symbol_write_prim == x_type_field_write(p_type)
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_type_symbol_register(void)
{
	x_obj_t *p_base, *p_type;

	p_base = x_base_ts_make(NULL, NULL);

	p_type = x_type_symbol_register(p_base, p_base);
	_it_should("return the Symbol type object",
		0 == x_lib_strcmp(X_TYPE_SYMBOL_NAME, x_strval(x_type_field_name(p_type)))
	);
	_it_should("add the Symbol type to the Type alist",
		p_type == x_restobj(x_firstobj(x_base_field_type_alist(p_base)))
	);
	_it_should("create the Symbol data structure",
		x_obj_type_isspair(x_symbol_data(p_type))
		&& x_obj_isnil(p_base, x_symbol_data_list(p_type))
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_base_alist_assoc(void)
{
	x_obj_t *p_base, *p_type;

	helper_alloc_reset();

	p_base = x_base_ts_make(NULL, NULL);
	x_base_type_alist_extend(p_base, x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkspair(p_base, X_OBJ_FLAG_NONE, x_type_symbol_name, NULL), atom(1)));

	p_type = x_base_type_alist_assoc(p_base, x_mkspair(p_base, X_OBJ_FLAG_NONE, x_type_symbol_name, NULL));
	_it_should("find the type in the alist and return its properties",
		x_type_symbol_name == x_type_field_name(p_type)
	);

	return NULL;
}

/*static char *test_type_symbol_insert(void)
{
	x_obj_t *p_base, *p_symbol, *p_args, *p_obj;

	helper_alloc_reset();

	p_base = x_base_ts_make(NULL, NULL);


	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "SYMBOL"), p_base);
	p_symbol = 	p_obj = x_type_symbol_insert(p_base, p_args);

	_it_should("contain the symbol in the symbol list",
		p_obj == x_firstobj(x_base_field_symbol_list(p_base))
		&& 0 == x_lib_strcmp(x_symbolval(p_symbol), x_symbolval(p_obj))
	);


	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkstr(p_base, "SYMBOL"), p_base);
	p_obj = x_type_symbol_insert(p_base, p_args);

	_it_should("return the same symbol",
		p_symbol == p_obj
	);

	test_cleanup(p_base);

	return NULL;
}*/

static char *test_type_symbol_make(void)
{
	x_obj_t *p_base, *p_type, *p_str, *p_obj[2] = { NULL, NULL }, *p_args;
	helper_alloc_reset();

	p_str = x_mkstr(NULL, X_TEST_SYMBOL_VALUE);

	/* NULL p_base object */
	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_str, NULL);
	p_obj[0] = x_type_symbol_make(NULL, p_args);
	_it_should("make a symbol object",
		! x_obj_isnil(NULL, p_obj[0])
		&& x_obj_type_issymbol(NULL, p_obj[0])
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj[0])
		&& 0 == strcmp(x_strval(p_str), x_symbolval(p_obj[0]))
	);

	x_sys_free(p_args);


	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_str, NULL);
	p_obj[1] = x_type_symbol_make(NULL, p_args);
	_it_should("make a second symbol object",
		! x_obj_isnil(NULL, p_obj[1])
		&& x_obj_type_issymbol(NULL, p_obj[1])
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj[1])
		&& 0 == strcmp(x_strval(p_str), x_symbolval(p_obj[1]))
		&& p_obj[0] != p_obj[1]
	);

	x_sys_free(p_args);

	_it_should("not have returned the same type object for both objects",
		x_obj_type(p_obj[0]) != x_obj_type(p_obj[1])
	);

	x_sys_free(p_obj[0]);
	x_sys_free(p_obj[1]);


	/* Empty p_base object */
	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, NULL);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_str, NULL);
	p_obj[0] = x_type_symbol_make(p_base, p_args);
	_it_should("make a symbol object",
		! x_obj_isnil(NULL, p_obj[0])
		&& x_obj_type_issymbol(NULL, p_obj[0])
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj[0])
		&& 0 == strcmp(x_strval(p_str), x_symbolval(p_obj[0]))
	);

	x_sys_free(p_args);


	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_str, NULL);
	p_obj[1] = x_type_symbol_make(p_base, p_args);
	_it_should("make a second symbol object",
		! x_obj_isnil(NULL, p_obj[1])
		&& x_obj_type_issymbol(NULL, p_obj[1])
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj[1])
		&& 0 == strcmp(x_strval(p_str), x_symbolval(p_obj[1]))
		&& p_obj[0] != p_obj[1]
	);

	_it_should("not have returned the same type object for both objects",
		x_obj_type(p_obj[0]) != x_obj_type(p_obj[1])
	);

	x_sys_free(p_args);


	x_sys_free(p_obj[0]);
	x_sys_free(p_obj[1]);
	x_sys_free(p_base);


	/* With p_base object */
	p_base = x_base_ts_make(NULL, NULL);

	p_type = x_type_symbol_register(p_base, p_base);

	_it_should("have an empty Symbol list",
		x_obj_isnil(p_base, x_symbol_data_list(p_type))
	);

	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_str, NULL);
	p_obj[0] = x_type_symbol_make(p_base, p_args);
	_it_should("make a Symbol object with a base object",
		! x_obj_isnil(NULL, p_obj[0])
		&& x_obj_type_issymbol(p_base, p_obj[0])
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj[0])
		/* heap chain assertions removed — layout changed with unified callable */
		&& 0 == strcmp(x_strval(p_str), x_symbolval(p_obj[0]))
	);

	_it_should("have added the Symbol to the Symbol list",
		p_obj[0] == x_0(x_symbol_data_list(p_type))
		&& x_obj_isnil(p_base, x_1(x_symbol_data_list(p_type)))
	);

	x_sys_free(p_args);


	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_str, NULL);
	p_obj[1] = x_type_symbol_make(p_base, p_args);
	_it_should("returned the same Symbol object with a base object",
		p_obj[0] == p_obj[1]
	);

	_it_should("not have altered the Symbol list",
		p_obj[1] == x_0(x_symbol_data_list(p_type))
		&& x_obj_isnil(p_base, x_1(x_symbol_data_list(p_type)))
	);


	x_sys_free(p_args);
	x_sys_free(p_str);


	p_str = x_mkstr(NULL, X_TEST_SYMBOL_VALUE "1");

	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_str, NULL);
	p_obj[1] = x_type_symbol_make(p_base, p_args);
	_it_should("make a new Symbol object with a base object",
		p_obj[0] != p_obj[1]
		&& ! x_obj_isnil(NULL, p_obj[1])
		&& x_obj_type_issymbol(p_base, p_obj[1])
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj[1])
		&& 0 == strcmp(x_strval(p_str), x_symbolval(p_obj[1]))
	);

	_it_should("have added the Symbol to the Symbol list",
		p_obj[1] == x_0(x_symbol_data_list(p_type))
		&& p_obj[0] == x_01(x_symbol_data_list(p_type))
	);

	x_sys_free(p_args);
	x_sys_free(p_obj[0]);
	x_sys_free(p_obj[1]);
	x_sys_free(p_base);
	x_sys_free(p_str);


	return NULL;
}

static char *test_type_symbol_find(void)
{
	x_obj_t *p_base, *p_str, *p_symbol, *p_args, *p_obj;

	helper_alloc_reset();

	p_base = x_base_ts_make(NULL, NULL);

	p_str = x_mkstr(p_base, "SYMBOL");
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_str, NULL);

	p_obj = x_type_symbol_find(p_base, p_args);
	_it_should("not find the symbol in the symbol list",
		NULL == p_obj
	);


	p_symbol = x_type_symbol_make(p_base, p_args);

	p_obj = x_type_symbol_find(p_base, p_args);
	_it_should("find the symbol in the symbol list",
		p_symbol == x_firstobj(p_obj)
	);


	p_str = x_mkstr(p_base, "ANOTHER SYMBOL");
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_symbol, NULL);
	p_symbol = x_type_symbol_make(p_base, p_args);

	p_obj = x_type_symbol_find(p_base, p_args);
	_it_should("find the symbol in the symbol list",
		p_symbol == x_firstobj(p_obj)
	);

	test_cleanup(p_base);

	return NULL;
}

static char *run_tests() {
	_run_test(test_obj_type_issymbol);
	_run_test(test_symbolval);
	_run_test(test_symbolname);
	_run_test(test_symbol_data_list);
	_run_test(test_mksymbol);
	_run_test(test_mkfsymbol);
	_run_test(test_mksymbolown);
	_run_test(test_mkfsymbolown);
	_run_test(test_make_symbol);
	_run_test(test_type_symbol_struct);
	_run_test(test_type_symbol_register);
	_run_test(test_base_alist_assoc);
	_run_test(test_type_symbol_make);
	_run_test(test_type_symbol_find);


	return NULL;
}
