/*
 * # Unit Tests: *x-sexp/list*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"
#include "x-type/buffer.h"
#include "x-token/sexp/list.h"

/* The truncation tests install a guard-shaped jmp error handler. */
#include <setjmp.h>

/* We need the GC structures for cleanup. */
#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "ext/x-expr/tests/src/test-helper-system.c"

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-stdlib.c"
#include "ext/x-expr/src/x-lib.c"
#include "ext/x-expr/src/x.c"
#include "ext/x-expr/src/x-obj.c"
#include "src/x-alist.c"
#include "ext/x-expr/src/x-base.c"
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
x_obj_t *x_prim_callcc_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_binding_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_closure_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_control_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_quote_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }



/*
 * ## Test Overhead
 */

static void _setup(void)
{
	helper_alloc_reset();
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

static char *test_sexp_list_analyse(void)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];

	/* Non-list char returns NULL */
	s = "a";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_eval_make(NULL, NULL);
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

	p_base = x_eval_make(NULL, NULL);
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

	p_base = x_eval_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_buffer, NULL);
	x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_list_delimit(p_base, p_args);
	_it_should("return NULL for non-list char", NULL == p_obj);
	test_cleanup(p_base);

	/* ')' returns buffer and unreads */
	s = ")";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_eval_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_buffer, NULL);
	x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_list_delimit(p_base, p_args);
	_it_should("return buffer for ')'", p_buffer == p_obj);
	_it_should("unread the character", 0 == x_bufferlen(p_buffer));
	test_cleanup(p_base);

	return NULL;
}

static char *test_sexp_list_read(void)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[64];

	/* Use a single base for all read sub-tests to conserve allocations. */
	s = "(42) (1 2 3) (1 . 2) ()";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_buffer_remaining[TEST_HELPER_FILE_STDIN] = x_lib_strlen(s);
	helper_file_reset();

	p_base = x_eval_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Register sexp types so the tokenizer has types to iterate. */
	x_type_whitespace_register(p_base, p_base);
	x_type_comment_register(p_base, p_base);
	x_type_char_register(p_base, p_base);
	x_type_int_register(p_base, p_base);
	x_type_symbol_register(p_base, p_base);
	x_type_str_register(p_base, p_base);
	x_type_list_register(p_base, p_base);

	p_buffer = x_mkbufferown(p_base, buffer);
	p_args = x_mkpair(p_base, p_buffer, p_base);

	/* Read (42) */
	p_obj = x_token_read(p_base, p_args);
	_it_should("return a list for (42)",
		x_obj_type_islist(p_base, p_obj)
	);
	_it_should("first element is 42",
		42 == x_intval(x_firstobj(p_obj))
	);
	_it_should("rest is nil",
		x_obj_isnil(p_base, x_restobj(p_obj))
	);

	/* Read (1 2 3) — multi-element list */
	p_obj = x_token_read(p_base, p_args);
	_it_should("return a list for (1 2 3)",
		x_obj_type_islist(p_base, p_obj)
	);
	_it_should("first is 1",
		1 == x_intval(x_firstobj(p_obj))
	);
	_it_should("second is 2",
		2 == x_intval(x_firstobj(x_restobj(p_obj)))
	);
	_it_should("third is 3",
		3 == x_intval(x_firstobj(x_restobj(x_restobj(p_obj))))
	);
	_it_should("rest of third is nil",
		x_obj_isnil(p_base, x_restobj(x_restobj(x_restobj(p_obj))))
	);

	/* Read (1 . 2) — dotted pair */
	p_obj = x_token_read(p_base, p_args);
	_it_should("return a list for (1 . 2)",
		x_obj_type_islist(p_base, p_obj)
	);
	_it_should("first is 1",
		1 == x_intval(x_firstobj(p_obj))
	);
	_it_should("rest is 2 (dotted)",
		2 == x_intval(x_restobj(p_obj))
	);

	/* Read () — empty list */
	p_obj = x_token_read(p_base, p_args);
	_it_should("return nil for ()",
		x_obj_isnil(p_base, p_obj)
	);

	/* Read past the end — clean EOF is the SENTINEL, not nil (#156):
	 * nil is a VALUE (the () just read); conflating the two made the
	 * list loop spin and () terminate loads. */
	p_obj = x_token_read(p_base, p_args);
	_it_should("return the EOF sentinel at end of input",
		(x_obj_t *)x_token_eof_prim == p_obj
	);

	helper_file_buffer_remaining[TEST_HELPER_FILE_STDIN] = TEST_HELPER_FILE_UNDEFINED;
	test_cleanup(p_base);

	return NULL;
}

/* End-of-input INSIDE an open list is truncation: the reader must raise
 * ("Unterminated input"), never return a partial list and never spin
 * (#156).  The raise needs a live error handler or x_eval_error exits;
 * install the guard-shaped jmp handler by hand (x-syntax/control.c). */
static char *test_sexp_list_read_truncated_one(const char *s)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_handler;
	x_char_t buffer[64];
	jmp_buf jmp;
	int caught;

	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = (x_char_t *)s;
	helper_file_buffer_remaining[TEST_HELPER_FILE_STDIN] = x_lib_strlen((x_char_t *)s);
	helper_file_reset();

	p_base = x_eval_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	x_type_whitespace_register(p_base, p_base);
	x_type_comment_register(p_base, p_base);
	x_type_char_register(p_base, p_base);
	x_type_int_register(p_base, p_base);
	x_type_symbol_register(p_base, p_base);
	x_type_str_register(p_base, p_base);
	x_type_list_register(p_base, p_base);

	p_buffer = x_mkbufferown(p_base, buffer);
	p_args = x_mkpair(p_base, p_buffer, p_base);

	/* Handler: (jmp-ptr (saved-env . saved-boundary) (error . line)) */
	p_handler = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkptr(p_base, &jmp),
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mkspair(p_base, X_OBJ_FLAG_NONE,
				x_firstobj(x_eval_field_env_alist(p_base)),
				x_eval_field_env_local_boundary(p_base)),
			x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, NULL)));
	x_firstobj(x_eval_field_error_handler(p_base)) = p_handler;

	caught = 0;
	if (setjmp(jmp) == 0) {
		x_token_read(p_base, p_args);
	} else {
		caught = 1;
	}

	_it_should("raise on end of input inside an open list", 1 == caught);
	_it_should("report Unterminated input",
		caught && 0 == x_lib_strncmp(
			x_atomstr(x_error_handler_error(p_handler)),
			(x_char_t *)"Unterminated input", 18)
	);

	helper_file_buffer_remaining[TEST_HELPER_FILE_STDIN] = TEST_HELPER_FILE_UNDEFINED;
	test_cleanup(p_base);

	return NULL;
}

static char *test_sexp_list_read_truncated(void)
{
	char *p_result;

	/* Element loop, dotted-tail read, and dotted close-paren read. */
	p_result = test_sexp_list_read_truncated_one("(a b");
	if (p_result != NULL) return p_result;
	p_result = test_sexp_list_read_truncated_one("(1 . ");
	if (p_result != NULL) return p_result;
	p_result = test_sexp_list_read_truncated_one("(1 . 2");
	if (p_result != NULL) return p_result;

	return NULL;
}

static char *run_tests() {
	_run_test(test_sexp_list_analyse);
	_run_test(test_sexp_list_delimit);
	_run_test(test_sexp_list_read);
	_run_test(test_sexp_list_read_truncated);

	return NULL;
}
