/*
 * # Unit Tests: *x-sexp/int*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"
#include "x-type/buffer.h"
#include <stdio.h>

/* We need the GC structures for cleanup. */
#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-lib.c"
#include "ext/x-expr/src/x.c"
#include "src/x-obj.c"
#include "src/x-alist.c"
#include "src/x-base.c"
#include "src/x-type.c"
#include "src/x-type/prim.c"
#include "src/x-type/char.c"
#include "src/x-token/sexp/char.c"
#include "src/x-type/buffer.c"
#include "src/x-type/int.c"
#include "src/x-token/sexp/int.c"
#include "src/x-type/str.c"
#include "src/x-token/sexp/str.c"
#include "src/x-type/atom.c"
#include "src/x-token/sexp/atom.c"
#include "src/x-token/sexp/pair.c"
#include "src/x-type/iter.c"
#include "src/x-eval.c"
#include "src/x-type/list.c"
#include "src/x-token/sexp/list.c"
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

#define X_TEST_INT_VALUE		'@'

#define nil			p_base
#define pair(X,Y)	(x_mkspair(p_base, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, (X)))

static char *test_sexp_int_analyse_digits(void)
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
	p_obj = x_sexp_int_analyse_digits(p_base, p_args);
	_it_should("return the Base", p_base == p_obj);

	test_cleanup(p_base);


	s = "9a";
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
		p_obj = x_sexp_int_analyse_digits(p_base, p_args);
		_it_should("return the analyse digits primitive",
			x_sexp_int_analyse_digits_prim == p_obj
		);

		p_obj = x_type_buffer_read(p_base, p_args);
		p_obj = x_sexp_int_analyse_digits(p_base, p_args);
		_it_should("return the score", p_score == p_obj);
		_it_should("set the score to 1", 1 == x_firstint(p_score));
		_it_should("set the read prim", x_sexp_int_read_prim == x_restobj(p_score));
	}

	test_cleanup(p_base);

	return NULL;
}

static char *test_sexp_int_analyse_xdigits(void)
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
	p_obj = x_sexp_int_analyse_xdigits(p_base, p_args);
	_it_should("return the Base", p_base == p_obj);

	test_cleanup(p_base);


	s = "0xa@";
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
		p_obj = x_type_buffer_read(p_base, p_args);
		p_obj = x_type_buffer_read(p_base, p_args);
		p_obj = x_sexp_int_analyse_xdigits(p_base, p_args);
		_it_should("return the analyse xdigits primitive",
			x_sexp_int_analyse_xdigits_prim == p_obj
		);

		p_obj = x_type_buffer_read(p_base, p_args);
		p_obj = x_sexp_int_analyse_xdigits(p_base, p_args);
		_it_should("return the score", p_score == p_obj);
		_it_should("set the score to 3", 3 == x_firstint(p_score));
		_it_should("set the read prim", x_sexp_int_read_prim == x_restobj(p_score));
	}

	test_cleanup(p_base);

	return NULL;
}

static char *test_sexp_int_analyse_base(void)
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
	p_obj = x_sexp_int_analyse_base(p_base, p_args);
	_it_should("return the Base", p_base == p_obj);

	test_cleanup(p_base);


	s = "X";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);
	p_obj = x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_int_analyse_base(p_base, p_args);
	_it_should("return the analyse xdigits primitive",
		x_sexp_int_analyse_xdigits_prim == p_obj
	);

	test_cleanup(p_base);


	s = "x";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);
	p_obj = x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_int_analyse_base(p_base, p_args);
	_it_should("return the analyse xdigits primitive",
		x_sexp_int_analyse_xdigits_prim == p_obj
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_sexp_int_analyse_prefix(void)
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
	p_obj = x_sexp_int_analyse_prefix(p_base, p_args);
	_it_should("return the Base", p_base == p_obj);

	test_cleanup(p_base);


	s = "0";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);
	p_obj = x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_int_analyse_prefix(p_base, p_args);
	_it_should("return the analyse base primitive",
		x_sexp_int_analyse_base_prim == p_obj
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_sexp_int_analyse_sign(void)
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
	p_obj = x_sexp_int_analyse_sign(p_base, p_args);
	_it_should("return the Base", p_base == p_obj);

	test_cleanup(p_base);


	s = "+";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);
	p_obj = x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_int_analyse_sign(p_base, p_args);
	_it_should("return the analyse prefix primitive",
		x_sexp_int_analyse_prefix_prim == p_obj
	);

	test_cleanup(p_base);


	s = "-";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);
	p_obj = x_type_buffer_read(p_base, p_args);
	p_obj = x_sexp_int_analyse_sign(p_base, p_args);
	_it_should("return the analyse prefix primitive",
		x_sexp_int_analyse_prefix_prim == p_obj
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_sexp_int_read(void)
{
	x_obj_t *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];

	s = "9@";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_buffer = x_mkbuffer(NULL, buffer);
	p_args = x_mkspair(NULL, p_buffer, NULL);
	p_obj = x_sexp_int_read(NULL, p_args);
	_it_should("return the Base", x_obj_isnil(NULL, p_obj));

	p_obj = x_type_buffer_read(NULL, p_args);
	p_obj = x_sexp_int_read(NULL, p_args);
	_it_should("return a Integer object with the value set",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_isint(NULL, p_obj)
		&& 9 == x_intval(p_obj)
	);

	x_sys_free(p_obj);
	x_sys_free(p_args);
	x_sys_free(p_buffer);

	return NULL;
}

x_obj_t *test_token_read_analyse_whitespace(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *test_token_read_read_whitespace(x_obj_t *p_base, x_obj_t *p_args);

x_satom_t test_token_read_analyse_whitespace_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_analyse_whitespace }),
	test_token_read_read_whitespace_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_read_whitespace });

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

static char *test_sexp_int_read_token(void)
{
	x_obj_t *p_base = x_base_make(NULL, NULL),
		*p_type, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];
	struct x_type_t type_whitespace = {
		.p_name = x_mkstr(p_base, "WHITESPACE"),
		.p_analyse = test_token_read_analyse_whitespace_prim,
		.p_delimit = x_mkatom(p_base, test_token_read_delimit_whitespace)
	};


	s = "1 +2 -3 0xE -0XF";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	/* NOTE: Type registration order matters. */
	p_base = x_base_make(NULL, NULL);
	x_type_int_register(p_base, p_base);
	p_type = x_type_struct_make(p_base, type_whitespace);
	x_base_type_alist_extend(p_base, p_type);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);

	p_obj = x_token_read(p_base, p_args);
	_it_should("return 1",
		x_obj_type_isint(p_base, p_obj)
		&& 1 == x_intval(p_obj)
	);

	p_obj = x_token_read(p_base, p_args);
	_it_should("return 2",
		x_obj_type_isint(p_base, p_obj)
		&& 2 == x_intval(p_obj)
	);

	p_obj = x_token_read(p_base, p_args);
	_it_should("return -3",
		x_obj_type_isint(p_base, p_obj)
		&& -3 == x_intval(p_obj)
	);

	p_obj = x_token_read(p_base, p_args);
	_it_should("return 14",
		x_obj_type_isint(p_base, p_obj)
		&& 14 == x_intval(p_obj)
	);

	p_obj = x_token_read(p_base, p_args);
	_it_should("return -15",
		x_obj_type_isint(p_base, p_obj)
		&& -15 == x_intval(p_obj)
	);


	test_cleanup(p_base);

	return NULL;
}

static char *test_sexp_int_write(void)
{
	x_obj_t *p_args, *p_obj, *p_ret;
	x_char_t *s, buffer[8];
	x_int_t i;

	helper_file_buffer_ptr[TEST_HELPER_FILE_STDOUT] = buffer;
	helper_file_reset();

	i = 123;
	s = "123";
	p_obj = x_mkint(NULL, i);
	p_args = x_mkspair(NULL, p_obj, NULL);
	p_ret = x_sexp_int_write(NULL, p_args);
	_it_should("write the value of the Integer object",
		! x_obj_isnil(NULL, p_ret)
		&& p_obj == p_ret
		&& 0 == strncmp(s, buffer, strlen(s))
	);

	return NULL;
}

static char *run_tests() {
	_run_test(test_sexp_int_analyse_digits);
	_run_test(test_sexp_int_analyse_xdigits);
	_run_test(test_sexp_int_analyse_base);
	_run_test(test_sexp_int_analyse_prefix);
	_run_test(test_sexp_int_analyse_sign);
	_run_test(test_sexp_int_read);
	_run_test(test_sexp_int_read_token);
	_run_test(test_sexp_int_write);

	return NULL;
}
