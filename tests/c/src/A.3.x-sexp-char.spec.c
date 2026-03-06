/*
 * # Unit Tests: *x-sexp/char*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

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
#include "src/x-type/char.c"
#include "src/x-sexp/char.c"
#include "src/x-type/str.c"
#include "src/x-sexp/str.c"
#include "src/x-type/buffer.c"
#include "src/x-sexp.c"
#include "src/x-type/atom.c"
#include "src/x-sexp/atom.c"
#include "src/x-sexp/pair.c"
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

#define X_TEST_CHAR_VALUE		'@'

#define nil			p_base
#define pair(X,Y)	(x_mkspair(p_base, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, (X)))

static char *test_sexp_char_analyse1(void)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];

	s = "@";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);
	p_obj = x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_char_analyse1(p_base, p_args);
	_it_should("return the base", p_base == p_obj);

	test_cleanup(p_base);


 	s = X_SEXP_CHAR_PRE_STR;
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);
	p_obj = x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_char_analyse1(p_base, p_args);
	_it_should("return the analyse2 primitive",
		x_sexp_char_analyse2_prim == p_obj
	);

	test_cleanup(p_base);


	return NULL;
}

static char *test_sexp_char_analyse2(void)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];

	s = "@";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);
	p_obj = x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_char_analyse2(p_base, p_args);
	_it_should("return the base", p_base == p_obj);

	test_cleanup(p_base);


 	s = X_SEXP_CHAR_PRE_STR "@";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s + 1;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);
	p_obj = x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_char_analyse2(p_base, p_args);
	_it_should("return the buffer", p_buffer == p_obj);

	test_cleanup(p_base);


	return NULL;
}

static char *test_sexp_char_read(void)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_analyser, *p_obj;
	x_char_t *s, buffer[32];

 	s = X_SEXP_CHAR_PRE_STR "@";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s + 1;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_analyser = x_mkspair(p_base, x_sexp_char_analyse2_prim, p_base);
	p_args = x_mkspair(p_base, p_buffer,
		x_mkspair(p_base, p_analyser, p_base));
	p_obj = x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_char_read(p_base, p_args);
	_it_should("return a char object", x_obj_type_ischar(p_base, p_obj));
	_it_should("set the char object to '@'", '@' == x_charval(p_obj));

	test_cleanup(p_base);


	return NULL;
}

x_obj_t *test_token_read_analyse_any(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	return p_buffer;
}

x_obj_t *test_token_read_read_any(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	return x_mksatom(p_base, x_bufferval(p_buffer)[0]);
}

static char *test_sexp_char_read_token(void)
{
	x_obj_t *p_base = x_base_make(NULL, NULL),
		*p_type, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];
	struct x_type_t type_any = {
		.p_name = x_mkstr(p_base, "ANY"),
		.p_analyse = x_mkatom(p_base, test_token_read_analyse_any),
	};


	s = X_SEXP_CHAR_PRE_STR "@ " X_SEXP_CHAR_PRE_STR "A";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	x_type_char_register(p_base, p_base);
	p_type = x_type_struct_make(p_base, type_any);
	x_base_type_alist_extend(p_base, p_type);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);

	p_obj = x_token_read(p_base, p_args);
	_it_should("return the @ character",
		x_obj_type_ischar(p_base, p_obj)
		&& '@' == x_charval(p_obj)
	);

	p_obj = x_token_read(p_base, p_args);
	_it_should("return an atom containing a space",
		x_obj_type_issatom(p_obj)
		&& ' ' == x_charval(p_obj)
	);

	p_obj = x_token_read(p_base, p_args);
	_it_should("return the A character",
		x_obj_type_ischar(p_base, p_obj)
		&& 'A' == x_charval(p_obj)
	);


	test_cleanup(p_base);

	return NULL;
}

static char *test_sexp_char_write(void)
{
	x_obj_t *p_args, *p_obj, *p_ret;
	x_char_t c, buffer[8];

	helper_file_buffer_ptr[TEST_HELPER_FILE_STDOUT] = buffer;
	helper_file_reset();

	c = '@';
	p_obj = x_mkchar(NULL, c);
	p_args = x_mkspair(NULL, p_obj, NULL);
	p_ret = x_sexp_char_write(NULL, p_args);
	_it_should("write the value of the Character object",
		! x_obj_isnil(NULL, p_ret)
		&& p_obj == p_ret
		&& buffer[0] == c
	);

	x_sys_free(p_args);
	x_sys_free(p_obj);

	return NULL;
}

static char *run_tests() {
	_run_test(test_sexp_char_analyse1);
	_run_test(test_sexp_char_analyse2);
	_run_test(test_sexp_char_read);
	_run_test(test_sexp_char_read_token);
	_run_test(test_sexp_char_write);

	return NULL;
}
