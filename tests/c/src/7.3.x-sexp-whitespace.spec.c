/*
 * # Unit Tests: *x-sexp/whitespace*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"
#include "x-token/sexp/whitespace.h"
#include "x-type/buffer.h"

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
#include "src/x-type.c"
#include "src/x-type/prim.c"
#include "src/x-type/buffer.c"
#include "src/x-type/char.c"
#include "src/x-token/sexp/char.c"
#include "src/x-type/str.c"
#include "src/x-token/sexp/str.c"
#include "src/x-type/whitespace.c"
#include "src/x-token/sexp/whitespace.c"
#include "src/x-type/atom.c"
#include "src/x-token/sexp/atom.c"
#include "src/x-token/sexp/pair.c"
#include "src/x-type/iter.c"
#include "src/x-eval.c"
#include "src/x-type/list.c"
#include "src/x-token/sexp/list.c"
#include "src/x-token.c"

#define STUB_X_PRIM
#define STUB_X_PROCEDURE
#define STUB_X_OPERATIVE
#define STUB_X_HEAP
#define STUB_X_OBJ_OBJ
#define STUB_X_INT
#define STUB_X_SYMBOL
#define STUB_X_PRIM_SHADOW
#define STUB_X_PROCEDURE_APPLY
#include "helper-stubs.c"

#include "ext/x-expr/tests/src/helper-system-functions.c"

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

#define X_TEST_WHITESPACE_VALUE		"TEST"

#define nil			p_base
#define pair(X,Y)	(x_mkspair(p_base, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, (X)))

static char *test_sexp_whitespace_analyse1(void)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];
	size_t i;


	s = X_SEXP_WHITESPACE_CHARS_STR "@";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);

	for (i = 0; i < strlen(X_SEXP_WHITESPACE_CHARS_STR); i++) {
		p_obj = x_type_buffer_read(p_base, p_args);
		p_obj = x_sexp_whitespace_analyse1(p_base, p_args);
		_it_should("return the analyse2 primitive",
			x_sexp_whitespace_analyse2_prim == p_obj
		);
	}

	p_obj = x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_whitespace_analyse1(p_base, p_args);
	_it_should("return NULL", NULL == p_obj);


	test_cleanup(p_base);

	return NULL;
}

static char *test_sexp_whitespace_analyse2(void)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];
	size_t i;


	s = X_SEXP_WHITESPACE_CHARS_STR "@";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);

	for (i = 0; i < strlen(X_SEXP_WHITESPACE_CHARS_STR); i++) {
		p_obj = x_type_buffer_read(p_base, p_args);
		p_obj = x_sexp_whitespace_analyse2(p_base, p_args);
		_it_should("return the analyse2 primitive",
			x_sexp_whitespace_analyse2_prim == p_obj
		);
	}
	{
		x_spair_t score = x_obj_set(NULL, X_OBJ_FLAG_NONE, {});
		x_spair_t buffer_args2[3] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_buffer }, { (x_obj_t *)(buffer_args2 + 1) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { score }, { (x_obj_t *)(buffer_args2 + 2) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL }),
		};
		x_obj_t *p_score = (x_obj_t *)score;

		p_obj = x_type_buffer_read(p_base, (x_obj_t *)buffer_args2);
		p_obj = x_sexp_whitespace_analyse2(p_base, (x_obj_t *)buffer_args2);
		_it_should("return the score", p_score == p_obj);
		_it_should("set the score",
			x_intval(p_score) == (x_int_t)(strlen(X_SEXP_WHITESPACE_CHARS_STR))
		);
	}


	test_cleanup(p_base);

	return NULL;
}

static char *test_sexp_whitespace_delimit(void)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];
	size_t i;


	s = X_SEXP_WHITESPACE_CHARS_STR "@";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);

	for (i = 0; i < strlen(X_SEXP_WHITESPACE_CHARS_STR); i++) {
		p_obj = x_type_buffer_read(p_base, p_args);
		p_obj = x_sexp_whitespace_delimit(p_base, p_args);
		_it_should("return the buffer object", p_buffer == p_obj);
		_it_should("reposition the read pointer",
			x_bufferread(p_buffer) == x_bufferval(p_buffer) + i
			&& x_bufferwrite(p_buffer) == x_bufferread(p_buffer) + 1
		);
		/* Move the pointer back for the next test. */
		x_bufferread(p_buffer)++;
	}

	p_obj = x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_whitespace_delimit(p_base, p_args);
	_it_should("return NULL", NULL == p_obj);


	test_cleanup(p_base);

	return NULL;
}

static char *test_sexp_whitespace_null_read(void)
{
	x_obj_t *p_base, *p_type;

	/* Whitespace type now uses NULL p_read (discard mechanism).
	 * Verify it registers with NULL p_read. */
	p_base = x_base_make(NULL, NULL);
	x_type_whitespace_register(p_base, p_base);

	/* Find whitespace type on type alist */
	p_type = x_base_field_type_alist(p_base);
	_it_should("whitespace type is registered",
		! x_obj_isnil(p_base, p_type));
	_it_should("whitespace type has NULL p_read (discard)",
		x_obj_isnil(p_base,
			x_type_field_read(x_restobj(x_firstobj(p_type)))));

	test_cleanup(p_base);

	return NULL;
}

x_obj_t *test_token_read_analyse_catchall(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *test_token_read_read_catchall(x_obj_t *p_base, x_obj_t *p_args);

x_satom_t test_token_read_analyse_catchall_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_analyse_catchall }),
	test_token_read_read_catchall_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_read_catchall });

x_obj_t *test_token_read_analyse_catchall(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_score = x_token_read_arg_score(p_args);

	/* Stop at whitespace or null. */
	if (!x_bufferlastchar(p_buffer)
		|| x_lib_strchr(X_SEXP_WHITESPACE_CHARS_STR, x_bufferlastchar(p_buffer))) {
		x_bufferread(p_buffer)--;
		if (x_bufferlen(p_buffer) < 1) {
			return NULL;
		}
		x_firstint(p_score) = -x_bufferlen(p_buffer);
		x_restobj(p_score) = test_token_read_read_catchall_prim;
		return p_score;
	}

	return test_token_read_analyse_catchall_prim;
}

x_obj_t *test_token_read_read_catchall(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	return x_mkstrown(p_base, x_lib_strndup(x_bufferval(p_buffer), x_bufferlen(p_buffer)));
}

static char *test_sexp_whitespace_read_token(void)
{
	x_obj_t *p_base = x_base_make(NULL, NULL),
		*p_type, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];
	struct x_type_t type_catchall = {
		.p_name = x_mkstr(p_base, "CATCHALL"),
		.p_analyse = test_token_read_analyse_catchall_prim,
		.p_read = test_token_read_read_catchall_prim,
	};


	s = X_SEXP_WHITESPACE_CHARS_STR "@ABC" X_SEXP_WHITESPACE_CHARS_STR "DEF";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_type = x_type_struct_make(p_base, type_catchall);
	x_base_type_alist_extend(p_base, p_type);
	x_type_whitespace_register(p_base, p_base);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);

	p_obj = x_token_read(p_base, p_args);
	_it_should("return a String object with the value set",
		x_obj_type_isstr(p_base, p_obj)
		&& 0 == x_lib_strcmp("@ABC", x_strval(p_obj))
	);

	p_obj = x_token_read(p_base, p_args);
	_it_should("return a second String object with the value set",
		x_obj_type_isstr(p_base, p_obj)
		&& 0 == x_lib_strcmp("DEF", x_strval(p_obj))
	);

	test_cleanup(p_base);

	return NULL;
}


static char *run_tests() {
	_run_test(test_sexp_whitespace_analyse1);
	_run_test(test_sexp_whitespace_analyse2);
	_run_test(test_sexp_whitespace_delimit);
	_run_test(test_sexp_whitespace_null_read);
	_run_test(test_sexp_whitespace_read_token);

	return NULL;
}
