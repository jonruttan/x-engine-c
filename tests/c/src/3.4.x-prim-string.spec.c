/*
 * # Unit Tests: *x-prim/string*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-lib.c"
#include "ext/x-expr/src/x-obj.c"
#include "src/x-obj/obj.c"
#include "src/x-obj/prim.c"
#include "ext/x-expr/src/x.c"
#include "src/x-alist.c"
#include "src/x-base.c"
#include "src/x-eval.c"
#include "src/x-type.c"
#include "src/x-type/atom.c"
#include "src/x-token/sexp/atom.c"
#include "src/x-type/pair.c"
#include "src/x-token/sexp/pair.c"
#include "src/x-type/prim.c"
#include "src/x-type/symbol.c"
#include "src/x-token/sexp/symbol.c"
#include "src/x-type/procedure.c"
#include "src/x-type/operative.c"
#include "src/x-type/list.c"
#include "src/x-token/sexp/list.c"
#include "src/x-type/str.c"
#include "src/x-token/sexp/str.c"
#include "src/x-type/int.c"
#include "src/x-token/sexp/int.c"
#include "src/x-type/char.c"
#include "src/x-token/sexp/char.c"
#include "src/x-type/ptr.c"
#include "src/x-type/whitespace.c"
#include "src/x-token/sexp/whitespace.c"
#include "src/x-type/comment.c"
#include "src/x-token/sexp/comment.c"
#include "src/x-type/buffer.c"
#include "src/x-type/iter.c"
#include "ext/x-expr/src/x-heap.c"
#include "src/x-token.c"
#include "src/x-prim.c"
#include "src/x-prim/string.c"

/* Stubs for primitives not under test. */
x_obj_t *x_prim_core_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_arith_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_pred_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_io_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_type_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_ffi_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }

#include "ext/x-expr/tests/src/helper-system-functions.c"


/*
 * ## Test Overhead
 */

static void _setup(void)
{
	_buffer_index = -1;
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

static char *test_string_append(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (string-append "foo" "bar") -> "foobar" */
	p_args = x_mkspair(p_base,
		x_mkstr(p_base, "foo"),
		x_mkspair(p_base, x_mkstr(p_base, "bar"), NULL));
	p_result = x_prim_string_append(p_base, p_args);
	_it_should("(string-append \"foo\" \"bar\") = \"foobar\"",
		x_lib_strcmp(x_strval(p_result), "foobar") == 0);

	/* (string-append "" "x") -> "x" */
	p_args = x_mkspair(p_base,
		x_mkstr(p_base, ""),
		x_mkspair(p_base, x_mkstr(p_base, "x"), NULL));
	p_result = x_prim_string_append(p_base, p_args);
	_it_should("(string-append \"\" \"x\") = \"x\"",
		x_lib_strcmp(x_strval(p_result), "x") == 0);

	test_cleanup(p_base);
	return NULL;
}

static char *test_string_symbol_convert(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (string->symbol "foo") -> foo symbol */
	p_args = x_mkspair(p_base,
		x_mkstr(p_base, "foo"), NULL);
	p_result = x_prim_string_to_symbol(p_base, p_args);
	_it_should("(string->symbol \"foo\") is a symbol",
		x_lib_strcmp(x_symbolval(p_result), "foo") == 0);

	/* (symbol->string 't) -> "t" (t is already bound in env) */
	p_args = x_mkspair(p_base,
		x_base_field_true(p_base), NULL);
	p_result = x_prim_symbol_to_string(p_base, p_args);
	_it_should("(symbol->string t) = \"t\"",
		x_lib_strcmp(x_strval(p_result), "t") == 0);

	test_cleanup(p_base);
	return NULL;
}

static char *test_number_string_convert(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (number->string 42) -> "42" */
	p_args = x_mkspair(p_base,
		x_mksatom(p_base, (x_int_t)42), NULL);
	p_result = x_prim_number_to_string(p_base, p_args);
	_it_should("(number->string 42) = \"42\"",
		x_lib_strcmp(x_strval(p_result), "42") == 0);

	/* (number->string 255 16) -> "ff" */
	p_args = x_mkspair(p_base,
		x_mksatom(p_base, (x_int_t)255),
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)16), NULL));
	p_result = x_prim_number_to_string(p_base, p_args);
	_it_should("(number->string 255 16) = \"ff\"",
		x_lib_strcmp(x_strval(p_result), "ff") == 0);

	/* (string->number "42") -> 42 */
	p_args = x_mkspair(p_base,
		x_mkstr(p_base, "42"), NULL);
	p_result = x_prim_string_to_number(p_base, p_args);
	_it_should("(string->number \"42\") = 42",
		x_intval(p_result) == 42);

	/* (string->number "ff" 16) -> 255 */
	p_args = x_mkspair(p_base,
		x_mkstr(p_base, "ff"),
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)16), NULL));
	p_result = x_prim_string_to_number(p_base, p_args);
	_it_should("(string->number \"ff\" 16) = 255",
		x_intval(p_result) == 255);

	test_cleanup(p_base);
	return NULL;
}

static char *test_list_to_string(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_list;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (list->string '(#\a #\b #\c)) -> "abc" */
	p_list = x_mkspair(p_base, x_mkchar(p_base, 'a'),
		x_mkspair(p_base, x_mkchar(p_base, 'b'),
		x_mkspair(p_base, x_mkchar(p_base, 'c'), NULL)));
	p_args = x_mkspair(p_base, p_list, NULL);
	p_result = x_prim_list_to_string(p_base, p_args);
	_it_should("(list->string '(a b c)) = \"abc\"",
		x_lib_strcmp(x_strval(p_result), "abc") == 0);

	/* Empty list -> "" */
	p_args = x_mkspair(p_base, NULL, NULL);
	p_result = x_prim_list_to_string(p_base, p_args);
	_it_should("(list->string '()) = \"\"",
		x_lib_strcmp(x_strval(p_result), "") == 0);

	test_cleanup(p_base);
	return NULL;
}

static char *test_make_string(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (make-string 3) -> "   " (3 spaces) */
	p_args = x_mkspair(p_base,
		x_mksatom(p_base, (x_int_t)3), NULL);
	p_result = x_prim_make_string(p_base, p_args);
	_it_should("(make-string 3) = \"   \"",
		x_lib_strcmp(x_strval(p_result), "   ") == 0);

	/* (make-string 3 #\x) -> "xxx" */
	p_args = x_mkspair(p_base,
		x_mksatom(p_base, (x_int_t)3),
		x_mkspair(p_base, x_mkchar(p_base, 'x'), NULL));
	p_result = x_prim_make_string(p_base, p_args);
	_it_should("(make-string 3 x) = \"xxx\"",
		x_lib_strcmp(x_strval(p_result), "xxx") == 0);

	/* (make-string 0) -> "" */
	p_args = x_mkspair(p_base,
		x_mksatom(p_base, (x_int_t)0), NULL);
	p_result = x_prim_make_string(p_base, p_args);
	_it_should("(make-string 0) = \"\"",
		x_lib_strcmp(x_strval(p_result), "") == 0);

	test_cleanup(p_base);
	return NULL;
}

static char *test_str_call_negative_index(void)
{
	x_obj_t *p_base, *p_str, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* ("hello" -1) -> last char 'o' */
	p_str = x_mkstr(p_base, "hello");
	p_args = x_mkspair(p_base, p_str,
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)-1), NULL));
	p_result = x_type_str_call(p_base, p_args);
	_it_should("(\"hello\" -1) returns 'o'",
		p_result != NULL && x_atomchar(p_result) == 'o');

	/* ("hello" 1) -> 'e' */
	p_str = x_mkstr(p_base, "hello");
	p_args = x_mkspair(p_base, p_str,
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)1), NULL));
	p_result = x_type_str_call(p_base, p_args);
	_it_should("(\"hello\" 1) returns 'e'",
		p_result != NULL && x_atomchar(p_result) == 'e');

	test_cleanup(p_base);
	return NULL;
}

static char *run_tests() {
	_run_test(test_string_append);
	_run_test(test_string_symbol_convert);
	_run_test(test_number_string_convert);
	_run_test(test_list_to_string);
	_run_test(test_make_string);
	_run_test(test_str_call_negative_index);

	return NULL;
}
