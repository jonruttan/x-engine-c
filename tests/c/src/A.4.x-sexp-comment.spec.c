/*
 * # Unit Tests: *x-sexp/comment*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"
#include "x-obj.h"

/* We need the GC structures for cleanup. */
#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "src/x-sys.c"
#include "src/x-lib.c"
#include "src/x.c"
#include "src/x-obj.c"
#include "src/x-alist.c"
#include "src/x-base.c"
#include "src/x-type.c"
#include "src/x-type/prim.c"
#include "src/x-type/buffer.c"
#include "src/x-type/char.c"
#include "src/x-sexp/char.c"
#include "src/x-type/str.c"
#include "src/x-sexp/str.c"
#include "src/x-type/comment.c"
#include "src/x-sexp/comment.c"
#include "src/x-sexp.c"
#include "src/x-type/atom.c"
#include "src/x-sexp/atom.c"
#include "src/x-sexp/pair.c"
#include "src/x-type/iter.c"
#include "src/x-eval.c"
#include "src/x-type/list.c"
#include "src/x-sexp/list.c"
#include "src/x-token.c"

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

void test_cleanup(x_obj_t *p_base)
{
	x_obj_t *p_gc = p_base, *p_tmp;

	while (p_gc) {
		p_tmp = x_obj_gc(p_gc);
		x_sys_free(p_gc);
		p_gc = p_tmp;
	}
}

/*
 * ## Test Runners
 */

#define X_TEST_COMMENT_VALUE		"TEST"

#define nil			p_base
#define pair(X,Y)	(x_mkspair(p_base, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, (X)))

static char *test_sexp_comment_analyse1(void)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];


	s = X_SEXP_COMMENT_PRE_STR;
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);
	p_obj = x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_comment_analyse1(p_base, p_args);
	_it_should("return the analyse2 primitive object",
		x_sexp_comment_analyse2_prim == p_obj
	);


	s = " ";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();
	x_type_buffer_reset(p_base, p_args);

	p_obj = x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_comment_analyse1(p_base, p_args);
	_it_should("return the Base", p_base == p_obj);


	test_cleanup(p_base);

	return NULL;
}

static char *test_sexp_comment_analyse2(void)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];


	s = X_SEXP_COMMENT_POST_STR;
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
		p_obj = x_sexp_comment_analyse2(p_base, p_args);
		_it_should("return the score", p_score == p_obj);
	}


	s = " ";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();
	x_type_buffer_reset(p_base, p_args);

	p_obj = x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_comment_analyse2(p_base, p_args);
	_it_should("return the analyse2 primitive object",
		x_sexp_comment_analyse2_prim == p_obj
	);


	test_cleanup(p_base);

	return NULL;
}

static char *test_sexp_comment_delimit(void)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];


	s = X_SEXP_COMMENT_CHARS_STR;
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);
	p_obj = x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_comment_delimit(p_base, p_args);
	_it_should("return the buffer object", p_buffer == p_obj);


	s = " ";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();
	x_type_buffer_reset(p_base, p_args);

	p_obj = x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_comment_delimit(p_base, p_args);
	_it_should("return the Base", p_base == p_obj);


	test_cleanup(p_base);

	return NULL;
}

static char *test_sexp_comment_read(void)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];


	s = "@" X_SEXP_COMMENT_POST_STR;
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);
	p_obj = x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_comment_read(p_base, p_args);
	_it_should("return the arguments", p_args == p_obj);


	test_cleanup(p_base);

	return NULL;
}

x_obj_t *test_token_read_analyse_whitespace(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *test_token_read_read_whitespace(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *test_token_read_analyse_catchall(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *test_token_read_read_catchall(x_obj_t *p_base, x_obj_t *p_args);

x_satom_t test_token_read_analyse_whitespace_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_analyse_whitespace }),
	test_token_read_read_whitespace_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_read_whitespace }),
	test_token_read_analyse_catchall_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_analyse_catchall }),
	test_token_read_read_catchall_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_read_catchall });

x_obj_t *test_token_read_analyse_whitespace(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_score = x_token_read_arg_score(p_args);

	if (x_lib_strchr(" \t\r\n", x_bufferlastchar(p_buffer))) {
		return test_token_read_analyse_whitespace_prim;
	}

	if (x_bufferlen(p_buffer) > 1) {
		x_firstint(p_score) = x_bufferlen(p_buffer) - 1;
		x_restobj(p_score) = test_token_read_read_whitespace_prim;
		return p_score;
	}

	return p_base;
}

x_obj_t *test_token_read_delimit_whitespace(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	if (x_lib_strchr(" \t\r\n", x_bufferlastchar(p_buffer))) {
		x_bufferread(p_buffer)--;
		return p_buffer;
	}

	return p_base;
}

x_obj_t *test_token_read_read_whitespace(x_obj_t *p_base, x_obj_t *p_args)
{
	return p_args;
}

x_obj_t *test_token_read_analyse_catchall(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_score = x_token_read_arg_score(p_args);

	/* Immediately return a negative score (fallback, lowest priority). */
	x_firstint(p_score) = -x_bufferlen(p_buffer);
	x_restobj(p_score) = test_token_read_read_catchall_prim;
	return p_score;
}

x_obj_t *test_token_read_read_catchall(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	return x_mksatom(p_base, x_bufferval(p_buffer)[0]);
}

static char *test_sexp_comment_read_token(void)
{
	x_obj_t *p_base = x_base_make(NULL, NULL),
		*p_type, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];
	struct x_type_t type_whitespace = {
		.p_name = x_mkstr(p_base, "WHITESPACE"),
		.p_analyse = test_token_read_analyse_whitespace_prim,
		.p_delimit = x_mkatom(p_base, test_token_read_delimit_whitespace)
	};
	struct x_type_t type_catchall = {
		.p_name = x_mkstr(p_base, "CATCHALL"),
		.p_analyse = test_token_read_analyse_catchall_prim,
	};


	s = X_SEXP_COMMENT_PRE_STR "ABCDEF" X_SEXP_COMMENT_POST_STR "@";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	x_type_comment_register(p_base, p_base);
	p_type = x_type_struct_make(p_base, type_whitespace);
	x_base_type_alist_extend(p_base, p_type);
	p_type = x_type_struct_make(p_base, type_catchall);
	x_base_type_alist_extend(p_base, p_type);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);

	p_obj = x_token_read(p_base, p_args);
	_it_should("return an atom",
		x_obj_type_issatom(p_obj)
		&& '@' == x_charval(p_obj)
	);


	test_cleanup(p_base);

	return NULL;
}

static char *run_tests() {
	_run_test(test_sexp_comment_analyse1);
	_run_test(test_sexp_comment_analyse2);
	_run_test(test_sexp_comment_delimit);
	_run_test(test_sexp_comment_read);
	_run_test(test_sexp_comment_read_token);

	return NULL;
}
