/*
 * # Unit Tests: *x-sexp/whitespace*
 */

#include "test-runner.h"
#include "x-sexp/whitespace.h"
#include "x-type/buffer.h"

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
#include "src/x-type/whitespace.c"
#include "src/x-sexp/whitespace.c"
#include "src/x-sexp.c"
#include "src/x-type/atom.c"
#include "src/x-sexp/atom.c"
#include "src/x-sexp/pair.c"
#include "src/x-token.c"

#include "helper-system-functions.c"

/*
 * ## Test Overhead
 */

static void setup(void)
{
	helper_set_alloc(MEM_GUARANTEED);
}

static void teardown(void)
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
	_it_should("return the Base", p_base == p_obj);


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
return NULL;
	p_obj = x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_whitespace_analyse2(p_base, p_args);
	_it_should("return the buffer object", p_buffer == p_obj);
	_it_should("reposition the read pointer",
		x_bufferread(p_buffer) == x_bufferval(p_buffer) + strlen(X_SEXP_WHITESPACE_CHARS_STR)
		&& x_bufferwrite(p_buffer) == x_bufferread(p_buffer) + 1
	);


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
	_it_should("return the base object", p_base == p_obj);


	test_cleanup(p_base);

	return NULL;
}

static char *test_sexp_whitespace_read(void)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];


	s = X_SEXP_WHITESPACE_CHARS_STR"@";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);
	p_obj = x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_whitespace_read(p_base, p_args);
	_it_should("return the arguments", p_args == p_obj);


	test_cleanup(p_base);

	return NULL;
}

x_obj_t *test_token_read_analyse_all(x_obj_t *p_base, x_obj_t *p_args)
{
	return p_args;
}

x_obj_t *test_token_read_read_all(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	return x_mkstrown(p_base, x_lib_strndup(x_bufferval(p_buffer), x_bufferlen(p_buffer)));
}

static char *test_sexp_whitespace_read_token(void)
{
	x_obj_t *p_base = x_base_make(NULL, NULL),
		*p_type, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];
	struct x_type_t type_all = {
		.p_name = x_mkstr(p_base, "ALL"),
		.p_analyse = x_mkatom(p_base, test_token_read_analyse_all)
	};


	s = X_SEXP_WHITESPACE_CHARS_STR "@ABC" X_SEXP_WHITESPACE_CHARS_STR "DEF";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_type = x_type_struct_make(p_base, type_all);
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
	_run_test(test_sexp_whitespace_read);
	_run_test(test_sexp_whitespace_read_token);

	return NULL;
}
