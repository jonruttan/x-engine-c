/*
 * # Unit Tests: *x-sexp/str*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"
#include "x-lib.h"
#include "x-obj.h"
#include "x-type/buffer.h"
#include "x-type/str.h"

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

#define X_TEST_STR_VALUE		"TEST"

#define nil			p_base
#define pair(X,Y)	(x_mkspair(p_base, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, (X)))

static char *test_sexp_str_analyse1(void)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];


	s = X_SEXP_STR_PRE_STR;
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);
	p_obj = x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_str_analyse1(p_base, p_args);
	_it_should("return the analyse2 primitive object",
		x_sexp_str_analyse2_prim == p_obj
	);


	s = " ";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();
	x_type_buffer_reset(p_base, p_args);

	p_obj = x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_str_analyse1(p_base, p_args);
	_it_should("return the Base", p_base == p_obj);


	test_cleanup(p_base);

	return NULL;
}

static char *test_sexp_str_analyse2(void)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];


	s = X_SEXP_STR_POST_STR;
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);
	p_obj = x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_str_analyse2(p_base, p_args);
	_it_should("return the buffer", p_buffer == p_obj);


	s = " ";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();
	x_type_buffer_reset(p_base, p_args);

	p_obj = x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_str_analyse2(p_base, p_args);
	_it_should("return the analyse2 primitive object",
		x_sexp_str_analyse2_prim == p_obj
	);


	test_cleanup(p_base);

	return NULL;
}

static char *test_sexp_str_read(void)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32], tmp[32];

	s = X_SEXP_STR_PRE_STR "@ABC" X_SEXP_STR_POST_STR;
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);

	while ( ! x_obj_isnil(p_base, x_type_buffer_read_text(p_base, p_args))) {}
	/* Back up to before the null. */
	x_bufferread(p_buffer)--;

	p_obj = x_sexp_str_read(p_base, p_args);
	_it_should("return a String object with the value set",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_isstr(p_base, p_obj)
		&& sprintf(tmp, X_SEXP_STR_PRE_STR "%s" X_SEXP_STR_POST_STR, x_strval(p_obj))
		&& 0 == strcmp(s, tmp)
	);


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

static char *test_sexp_str_read_token(void)
{
	x_obj_t *p_base = x_base_make(NULL, NULL),
		*p_type, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];
	struct x_type_t type_any = {
		.p_name = x_mkstr(p_base, "ANY"),
		.p_analyse = x_mkatom(p_base, test_token_read_analyse_any)
	};


	s = X_SEXP_STR_PRE_STR "@ABC" X_SEXP_STR_POST_STR " "
		X_SEXP_STR_PRE_STR "DEF" X_SEXP_STR_POST_STR;
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	x_type_str_register(p_base, p_base);
	p_type = x_type_struct_make(p_base, type_any);
	x_base_type_alist_extend(p_base, p_type);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);

	p_obj = x_token_read(p_base, p_args);
	_it_should("return a String object with the value set",
		x_obj_type_isstr(p_base, p_obj)
		&& 0 == x_lib_strcmp("@ABC", x_strval(p_obj))
	);

	p_obj = x_token_read(p_base, p_args);
	_it_should("return an atom containing a space",
		x_obj_type_issatom(p_obj)
		&& ' ' == x_charval(p_obj)
	);

	p_obj = x_token_read(p_base, p_args);
	_it_should("return a second String object with the value set",
		x_obj_type_isstr(p_base, p_obj)
		&& 0 == x_lib_strcmp("DEF", x_strval(p_obj))
	);

	test_cleanup(p_base);

	return NULL;
}


static char *test_sexp_str_write(void)
{
	x_obj_t *p_args, *p_obj, *p_ret;
	x_char_t *s, buffer[8] = "\0\0\0\0\0\0\0\0", tmp[32];

	helper_file_buffer_ptr[TEST_HELPER_FILE_STDOUT] = buffer;
	helper_file_reset();

	s = "@ABC";

	sprintf(tmp, X_SEXP_STR_PRE_STR "%s" X_SEXP_STR_POST_STR, s);
	p_obj = x_mkstr(NULL, s);
	p_args = x_mkspair(NULL, p_obj, NULL);
	p_ret = x_sexp_str_write(NULL, p_args);
	_it_should("write the value of the string object",
		! x_obj_isnil(NULL, p_ret)
		&& p_obj == p_ret
		&& 0 == strncmp(tmp, buffer, strlen(tmp))
	);

	x_sys_free(p_args);
	x_sys_free(p_obj);

	return NULL;
}


static char *run_tests() {
	_run_test(test_sexp_str_analyse1);
	_run_test(test_sexp_str_analyse2);
	_run_test(test_sexp_str_read);
	_run_test(test_sexp_str_read_token);
	_run_test(test_sexp_str_write);

	return NULL;
}
