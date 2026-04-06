/*
 * # Unit Tests: *x-prim/io*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"
#include "x-type/buffer.h"

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
#include "ext/x-expr/src/x-base.c"
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

#include "src/x-prim/io.c"

/* Stubs for primitives not under test. */
x_obj_t *x_prim_core_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_arith_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_pred_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_string_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_type_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_ffi_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_callcc_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_binding_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_closure_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_control_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_quote_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }

#include "ext/x-expr/tests/src/test-helper-system.c"


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

static char *test_io_display(void)
{
	x_obj_t *p_base, *p_args;
	x_char_t s[64];

	p_base = x_base_ts_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	helper_file_buffer_ptr[TEST_HELPER_FILE_STDOUT] = s;
	helper_file_buffer_length[TEST_HELPER_FILE_STDOUT] = 64;
	helper_file_reset();
	s[0] = '\0';

	/* (display "hello") -> writes "hello" without quotes */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkstr(p_base, "hello"), NULL));
	x_prim_display(p_base, p_args);
	_it_should("display writes string without quotes",
		s[0] == 'h');

	/* (display 42) -> writes "42" */
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDOUT] = s;
	helper_file_buffer_length[TEST_HELPER_FILE_STDOUT] = 64;
	helper_file_reset();
	s[0] = '\0';

	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkint(p_base, (x_int_t)42), NULL));
	x_prim_display(p_base, p_args);
	_it_should("display writes integer",
		s[0] == '4');

	test_cleanup(p_base);
	return NULL;
}

static char *test_io_write(void)
{
	x_obj_t *p_base, *p_args;
	x_char_t s[64];

	p_base = x_base_ts_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	helper_file_buffer_ptr[TEST_HELPER_FILE_STDOUT] = s;
	helper_file_buffer_length[TEST_HELPER_FILE_STDOUT] = 64;
	helper_file_reset();
	s[0] = '\0';

	/* (write 42) */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkint(p_base, (x_int_t)42), NULL));
	x_prim_write(p_base, p_args);
	_it_should("write outputs integer",
		s[0] == '4');

	test_cleanup(p_base);
	return NULL;
}

static char *test_io_heap_count(void)
{
	x_obj_t *p_base, *p_result;

	p_base = x_base_ts_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_result = x_prim_heap_count(p_base, NULL);
	_it_should("heap-count returns an integer",
		p_result != NULL);
	_it_should("heap count is positive after setup",
		x_intval(p_result) > 0);

	test_cleanup(p_base);
	return NULL;
}

static char *test_io_read_char(void)
{
	x_obj_t *p_base, *p_result;
	x_char_t buffer[32];

	p_base = x_base_ts_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Set up stdin with 'A' */
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = "A";
	helper_file_reset();

	/* Create and set buffer */
	x_base_buffer_push(p_base, x_mkbuffer(p_base, buffer));

	p_result = x_prim_read_char(p_base, NULL);
	_it_should("read-char returns a char",
		p_result != NULL);
	_it_should("read-char reads 'A'",
		x_charval(p_result) == 'A');

	test_cleanup(p_base);
	return NULL;
}

static char *test_io_write_to_string(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	helper_set_alloc(MEM_SYSTEM);
	p_base = x_base_ts_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* (write-to-string 42) -> "42" */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkint(p_base, (x_int_t)42), NULL));
	p_result = x_prim_write_to_string(p_base, p_args);
	_it_should("write-to-string returns a string",
		p_result != NULL);
	_it_should("write-to-string of 42 is \"42\"",
		x_lib_strcmp(x_strval(p_result), "42") == 0);

	/* (write-to-string "hello") -> "\"hello\"" */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkstr(p_base, "hello"), NULL));
	p_result = x_prim_write_to_string(p_base, p_args);
	_it_should("write-to-string of string includes quotes",
		x_strval(p_result)[0] == '"');

	/* (write-to-string ()) -> "" */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, NULL));
	p_result = x_prim_write_to_string(p_base, p_args);
	_it_should("write-to-string of nil is \"()\"",
		x_lib_strcmp(x_strval(p_result), "()") == 0);

	/* write-buf is restored after write-to-string */
	_it_should("write-buf restored to nil after write-to-string",
		x_obj_isnil(p_base, x_base_field_write_buf(p_base)));

	test_cleanup(p_base);
	helper_set_alloc(MEM_GUARANTEED);
	return NULL;
}

static char *test_io_read_expr(void)
{
	x_obj_t *p_base, *p_result;
	x_char_t buffer[32];

	p_base = x_base_ts_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Register types needed by the tokenizer */
	x_type_int_register(p_base, p_base);
	x_type_whitespace_register(p_base, p_base);

	/* Provide "42\n" on stdin — read should parse integer 42.
	 * remaining limits total bytes; returns 0 (EOF) when exhausted. */
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = "42\n";
	helper_file_buffer_remaining[TEST_HELPER_FILE_STDIN] = 3;
	helper_file_reset();
	x_base_buffer_push(p_base, x_mkbuffer(p_base, buffer));

	p_result = x_prim_read_expr(p_base, NULL);
	_it_should("read returns an integer",
		p_result != NULL && x_obj_type_isint(p_base, p_result));
	_it_should("read returns 42",
		x_intval(p_result) == 42);

	helper_file_buffer_remaining[TEST_HELPER_FILE_STDIN] = TEST_HELPER_FILE_UNDEFINED;
	test_cleanup(p_base);
	return NULL;
}

static char *test_io_read_char_eof(void)
{
	x_obj_t *p_base, *p_result;
	x_char_t buffer[32];

	p_base = x_base_ts_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Empty stdin — read-char should return NULL (EOF) */
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = "";
	helper_file_buffer_remaining[TEST_HELPER_FILE_STDIN] = 0;
	helper_file_reset();
	x_base_buffer_push(p_base, x_mkbuffer(p_base, buffer));

	p_result = x_prim_read_char(p_base, NULL);
	_it_should("read-char returns NULL on EOF", NULL == p_result);

	helper_file_buffer_remaining[TEST_HELPER_FILE_STDIN] = TEST_HELPER_FILE_UNDEFINED;
	test_cleanup(p_base);
	return NULL;
}

static char *test_io_clock(void)
{
	x_obj_t *p_base, *p_result;

	p_base = x_base_ts_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_result = x_prim_clock(p_base, NULL);
	_it_should("clock returns an integer",
		p_result != NULL && x_obj_type_isint(p_base, p_result));
	_it_should("clock returns non-negative value",
		x_intval(p_result) >= 0);

	test_cleanup(p_base);
	return NULL;
}

static char *test_io_heap_mark_sweep_collect(void)
{
	x_obj_t *p_base;
	long count1, count2;

	p_base = x_base_ts_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Allocate some unreachable objects */
	x_mkint(p_base, 111);
	x_mkint(p_base, 222);
	x_mkint(p_base, 333);

	count1 = x_intval(x_prim_heap_count(p_base, NULL));
	_it_should("heap has objects before collect", count1 > 0);

	/* Mark reachable objects, then sweep unmarked */
	x_prim_heap_mark(p_base, NULL);
	x_prim_heap_sweep(p_base, NULL);

	count2 = x_intval(x_prim_heap_count(p_base, NULL));
	_it_should("heap shrank after mark+sweep", count2 < count1);

	test_cleanup(p_base);
	return NULL;
}

static char *test_io_repl(void)
{
	x_obj_t *p_base;
	x_char_t buffer[64], out[64];

	p_base = x_base_ts_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Register types needed by the tokenizer */
	x_type_int_register(p_base, p_base);
	x_type_whitespace_register(p_base, p_base);

	/* Provide a simple expression then EOF — repl evals it and returns */
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = "42\n";
	helper_file_buffer_remaining[TEST_HELPER_FILE_STDIN] = 3;
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDOUT] = out;
	helper_file_reset();
	x_base_buffer_push(p_base, x_mkbuffer(p_base, buffer));

	x_prim_repl(p_base, NULL);
	_it_should("repl returns without error on EOF", 1);

	helper_file_buffer_remaining[TEST_HELPER_FILE_STDIN] = TEST_HELPER_FILE_UNDEFINED;
	test_cleanup(p_base);
	return NULL;
}

static char *run_tests() {
	_run_test(test_io_display);
	_run_test(test_io_write);
	_run_test(test_io_heap_count);
	_run_test(test_io_read_char);
	_run_test(test_io_read_char_eof);
	_run_test(test_io_write_to_string);
	_run_test(test_io_clock);
	_run_test(test_io_heap_mark_sweep_collect);
	_run_test(test_io_read_expr);
	_run_test(test_io_repl);

	return NULL;
}
