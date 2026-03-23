/*
 * # Unit Tests: *x-type/symbol*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"
#include "x-obj.h"
#include "x-token.h"
#include "x-type/symbol.h"

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
#include "src/x-type/iter.c"
#include "src/x-type/buffer.c"
#include "src/x-type/char.c"
#include "src/x-token/sexp/char.c"
#include "src/x-type/str.c"
#include "src/x-token/sexp/str.c"
#include "src/x-type/symbol.c"
#include "src/x-token/sexp/symbol.c"
#include "src/x-type/atom.c"
#include "src/x-token/sexp/atom.c"
#include "src/x-token/sexp/pair.c"
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

#define X_TEST_SYMBOL_VALUE		"TEST"

#define nil			p_base
#define pair(X,Y)	(x_mkspair(p_base, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, (X)))


static char *test_sexp_symbol_analyse(void)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];

	s = "@ABC";
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
		p_obj = x_sexp_symbol_analyse(p_base, p_args);
		_it_should("return the arguments", p_args == p_obj);
	}


	test_cleanup(p_base);

	return NULL;
}

static char *test_sexp_symbol_read(void)
{
x_obj_t *p_base, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];

	s = "@ABC";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);

	while ( ! x_obj_isnil(p_base, x_type_buffer_read_text(p_base, p_args))) {}
	/* Back up to before the null. */
	x_bufferread(p_buffer)--;

	p_obj = x_sexp_symbol_read(p_base, p_args);
	_it_should("return a Symbol object with the value set",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_issymbol(p_base, p_obj)
		&& 0 == strcmp(s, x_strval(p_obj))
	);


	test_cleanup(p_base);

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

	return NULL;
}

x_obj_t *test_token_read_delimit_whitespace(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	if (x_lib_strchr(" \t\r\n", x_bufferlastchar(p_buffer))) {
		x_bufferread(p_buffer)--;
		return p_buffer;
	}

	return NULL;
}

x_obj_t *test_token_read_read_whitespace(x_obj_t *p_base, x_obj_t *p_args)
{
	return p_args;
}


static char *test_sexp_symbol_read_token(void)
{
	x_obj_t *p_base = x_base_make(NULL, NULL),
		*p_type, *p_args, *p_buffer, *p_obj, *p_symbol;
	x_char_t *s, buffer[32];
	struct x_type_t type_whitespace = {
		.p_name = x_mkstr(p_base, "WHITESPACE"),
		.p_analyse = test_token_read_analyse_whitespace_prim,
		.p_delimit = x_mkatom(p_base, test_token_read_delimit_whitespace)
	};


	s = "@ABC @ABC DEF\n";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_buffer_remaining[TEST_HELPER_FILE_STDIN] = strlen(s);
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);
	x_type_symbol_register(p_base, p_base);
	p_type = x_type_struct_make(p_base, type_whitespace);
	x_base_type_alist_extend(p_base, p_type);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, p_buffer, p_base);

	p_obj = x_token_read(p_base, p_args);
	_it_should("return a Symbol object with the value set",
		x_obj_type_issymbol(p_base, p_obj)
		&& 0 == x_lib_strcmp("@ABC", x_symbolval(p_obj))
	);

	p_symbol = p_obj;
	p_obj = x_token_read(p_base, p_args);
	_it_should("return the same Symbol object with the value set",
		x_obj_type_issymbol(p_base, p_obj)
		&& 0 == x_lib_strcmp("@ABC", x_symbolval(p_obj))
		&& p_symbol == p_obj
	);

	p_obj = x_token_read(p_base, p_args);
	_it_should("return a second Symbol object with the value set",
		x_obj_type_issymbol(p_base, p_obj)
		&& 0 == x_lib_strcmp("DEF", x_symbolval(p_obj))
		&& p_symbol != p_obj
	);

	test_cleanup(p_base);

	return NULL;
}


static char *test_sexp_symbol_write(void)
{
	x_obj_t *p_args, *p_obj, *p_ret;
	x_char_t *s, buffer[8] = "\0\0\0\0\0\0\0\0";

	helper_file_buffer_ptr[TEST_HELPER_FILE_STDOUT] = buffer;
	helper_file_reset();

	s = "@ABC";
	p_obj = x_mkstr(NULL, s);
	p_args = x_mkspair(NULL, p_obj, NULL);
	p_ret = x_sexp_symbol_write(NULL, p_args);
	_it_should("write the value of the string object",
		! x_obj_isnil(NULL, p_ret)
		&& p_obj == p_ret
		&& 0 == strncmp(s, buffer, strlen(s))
	);

	x_sys_free(p_args);
	x_sys_free(p_obj);

	return NULL;
}

static char *run_tests() {
	_run_test(test_sexp_symbol_analyse);
	_run_test(test_sexp_symbol_read);
	_run_test(test_sexp_symbol_read_token);
	_run_test(test_sexp_symbol_write);

	return NULL;
}
