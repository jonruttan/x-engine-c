/*
 * # Unit Tests: *x-sexp/list*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"
#include "x-type/buffer.h"
#include "x-token/sexp/list.h"

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
#include "src/x-type/prim.c"
#include "src/x-type/atom.c"
#include "src/x-token/sexp/atom.c"
#include "src/x-type/pair.c"
#include "src/x-token/sexp/pair.c"
#include "src/x-type/char.c"
#include "src/x-token/sexp/char.c"
#include "src/x-type/buffer.c"
#include "src/x-type/int.c"
#include "src/x-token/sexp/int.c"
#include "src/x-type/str.c"
#include "src/x-token/sexp/str.c"
#include "src/x-type/symbol.c"
#include "src/x-token/sexp/symbol.c"
#include "src/x-type/whitespace.c"
#include "src/x-token/sexp/whitespace.c"
#include "src/x-type/comment.c"
#include "src/x-token/sexp/comment.c"
#include "src/x-type/list.c"
#include "src/x-token/sexp/list.c"
#include "src/x-type/iter.c"
#include "ext/x-expr/src/x-heap.c"
#include "src/x-token.c"
#include "src/x-prim.c"
#include "src/x-obj/obj.c"
#include "src/x-obj/prim.c"
#include "src/x-type/ptr.c"
#include "src/x-type/procedure.c"
#include "src/x-type/operative.c"

/* Stubs for primitives not under test. */
x_obj_t *x_prim_core_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_arith_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_pred_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_string_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
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

static char *test_sexp_list_analyse(void)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];

	/* Non-list char returns NULL */
	s = "a";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	{
		x_spair_t score = x_obj_set(NULL, X_OBJ_FLAG_NONE, {});
		x_spair_t buffer_args[3] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_buffer }, { (x_obj_t *)(buffer_args + 1) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { score }, { (x_obj_t *)(buffer_args + 2) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL }),
		};

		p_args = (x_obj_t *)buffer_args;
		p_obj = x_type_buffer_read(p_base, p_args);
		p_obj = x_sexp_list_analyse(p_base, p_args);
		_it_should("return NULL for non-list char", NULL == p_obj);
	}
	test_cleanup(p_base);

	/* '(' returns score */
	s = "(";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	{
		x_spair_t score = x_obj_set(NULL, X_OBJ_FLAG_NONE, {});
		x_spair_t buffer_args[3] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_buffer }, { (x_obj_t *)(buffer_args + 1) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { score }, { (x_obj_t *)(buffer_args + 2) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL }),
		};
		x_obj_t *p_score = (x_obj_t *)score;

		p_args = (x_obj_t *)buffer_args;
		p_obj = x_type_buffer_read(p_base, p_args);
		p_obj = x_sexp_list_analyse(p_base, p_args);
		_it_should("return score for '('", p_score == p_obj);
		_it_should("set score to buffer len", 1 == x_firstint(p_score));
		_it_should("set read prim", x_sexp_list_read_prim == x_restobj(p_score));
	}
	test_cleanup(p_base);

	return NULL;
}

static char *test_sexp_list_delimit(void)
{
	x_obj_t *p_base, *p_buffer, *p_args, *p_obj;
	x_char_t *s, buffer[32];

	/* Non-list char returns NULL */
	s = "a";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, NULL);
	x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_list_delimit(p_base, p_args);
	_it_should("return NULL for non-list char", NULL == p_obj);
	test_cleanup(p_base);

	/* ')' returns buffer and unreads */
	s = ")";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, NULL);
	x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_list_delimit(p_base, p_args);
	_it_should("return buffer for ')'", p_buffer == p_obj);
	_it_should("unread the character", 0 == x_bufferlen(p_buffer));
	test_cleanup(p_base);

	return NULL;
}

static char *test_sexp_list_write(void)
{
	x_obj_t *p_base, *p_list, *p_args;
	x_char_t s[64];

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Write empty list: () */
	p_list = x_mklist(p_base, NULL, NULL);

	helper_file_buffer_ptr[TEST_HELPER_FILE_STDOUT] = s;
	helper_file_buffer_length[TEST_HELPER_FILE_STDOUT] = 64;
	helper_file_reset();
	s[0] = '\0';

	p_args = x_mkspair(p_base, p_list, NULL);
	x_sexp_list_write(p_base, p_args);

	_it_should("write something for empty list",
		s[0] == '(');

	/* Write single-element list: (42) */
	p_list = x_mklist(p_base,
		x_mksatom(p_base, 42),
		NULL);

	helper_file_buffer_ptr[TEST_HELPER_FILE_STDOUT] = s;
	helper_file_buffer_length[TEST_HELPER_FILE_STDOUT] = 64;
	helper_file_reset();
	s[0] = '\0';

	p_args = x_mkspair(p_base, p_list, NULL);
	x_sexp_list_write(p_base, p_args);

	_it_should("write open paren for single-element list",
		s[0] == '(');

	test_cleanup(p_base);

	return NULL;
}

static char *run_tests() {
	_run_test(test_sexp_list_analyse);
	_run_test(test_sexp_list_delimit);
	_run_test(test_sexp_list_write);

	return NULL;
}
