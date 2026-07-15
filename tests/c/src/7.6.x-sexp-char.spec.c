/*
 * # Unit Tests: *x-sexp/char*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

/* We need the GC structures for cleanup. */
#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "ext/x-expr/tests/src/test-helper-system.c"

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-lib.c"
#include "ext/x-expr/src/x.c"
#include "ext/x-expr/src/x-obj.c"
#include "src/x-alist.c"
#include "ext/x-expr/src/x-base.c"
#include "ext/x-expr/src/x-heap.c"
#include "src/x-type.c"
#include "src/x-type/prim.c"
#include "src/x-type/iter.c"
#include "src/x-type/buffer.c"
#include "src/x-type/char.c"
#include "src/x-token/sexp/char.c"
#include "src/x-type/str.c"
#include "src/x-token/sexp/str.c"
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
#define STUB_X_PRIM_SHADOW
#define STUB_X_PROCEDURE_APPLY
#include "helper-stubs.c"

/*
 * Minimal int/symbol stubs that allocate real objects (needed for
 * x_type_char_struct's named-character data alist).
 */
x_obj_t *x_make_int(x_obj_t *p_base, x_obj_flag_t flags, x_int_t i)
{
	x_obj_t *p_type = NULL;
	return x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_ATOM, i);
}

x_obj_t *x_make_symbol(x_obj_t *p_base, x_obj_flag_t flags, x_char_t *s)
{
	x_obj_t *p_type = NULL;
	return x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_ATOM,
		x_lib_strndup(s, x_lib_strlen(s)));
}


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

#define X_TEST_CHAR_VALUE		'@'

#define nil			p_base
#define pair(X,Y)	(x_mkspair(p_base, X_OBJ_FLAG_NONE, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, X_OBJ_FLAG_NONE, (X)))

static char *test_sexp_char_analyse1(void)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];

	s = "@";
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
		x_spair_t self_args = x_obj_set(NULL, X_OBJ_FLAG_NONE,
			{ (x_obj_t *)&x_sexp_char_analyse1_prim }, { (x_obj_t *)buffer_args });

		p_args = (x_obj_t *)buffer_args;
		p_obj = x_type_buffer_read(p_base, p_args);
		p_obj = x_sexp_char_analyse1(p_base, (x_obj_t *)&self_args);
		_it_should("return NULL", NULL == p_obj);
	}

	test_cleanup(p_base);


 	s = X_SEXP_CHAR_PRE_STR;
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
		x_spair_t self_args = x_obj_set(NULL, X_OBJ_FLAG_NONE,
			{ (x_obj_t *)&x_sexp_char_analyse1_prim }, { (x_obj_t *)buffer_args });

		p_args = (x_obj_t *)buffer_args;
		p_obj = x_type_buffer_read(p_base, p_args);
		p_obj = x_sexp_char_analyse1(p_base, (x_obj_t *)&self_args);
	}
	_it_should("return the analyse2 primitive",
		(x_obj_t *)&x_sexp_char_analyse2_prim == p_obj
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

	p_base = x_eval_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	{
		x_spair_t score = x_obj_set(NULL, X_OBJ_FLAG_NONE, {});
		x_spair_t buffer_args[3] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_buffer }, { (x_obj_t *)(buffer_args + 1) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { score }, { (x_obj_t *)(buffer_args + 2) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL }),
		};
		x_spair_t self_args = x_obj_set(NULL, X_OBJ_FLAG_NONE,
			{ (x_obj_t *)&x_sexp_char_analyse2_prim }, { (x_obj_t *)buffer_args });

		p_args = (x_obj_t *)buffer_args;
		p_obj = x_type_buffer_read(p_base, p_args);
		p_obj = x_sexp_char_analyse2(p_base, (x_obj_t *)&self_args);
		_it_should("return NULL", NULL == p_obj);
	}

	test_cleanup(p_base);


 	s = X_SEXP_CHAR_PRE_STR "@";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s + 1;
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
		x_spair_t self_args = x_obj_set(NULL, X_OBJ_FLAG_NONE,
			{ (x_obj_t *)&x_sexp_char_analyse2_prim }, { (x_obj_t *)buffer_args });

		p_args = (x_obj_t *)buffer_args;
		p_obj = x_type_buffer_read(p_base, p_args);
		p_obj = x_sexp_char_analyse2(p_base, (x_obj_t *)&self_args);
		_it_should("return the analyse3 primitive",
			(x_obj_t *)&x_sexp_char_analyse3_prim == p_obj
		);
	}

	test_cleanup(p_base);


	return NULL;
}

static char *test_sexp_char_read(void)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];

 	s = X_SEXP_CHAR_PRE_STR "@";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_eval_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_buffer, p_base);

	/* Read all chars into buffer. */
	while ( ! x_obj_isnil(p_base, x_type_buffer_read_text(p_base, p_args))) {}
	/* Back up to before the null. */
	x_bufferread(p_buffer)--;

	p_obj = x_sexp_char_read(p_base, p_args);
	_it_should("return a char object", x_obj_type_ischar(p_base, p_obj));
	_it_should("set the char object to '@'", '@' == x_charval(p_obj));

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

static char *test_sexp_char_read_token(void)
{
	x_obj_t *p_base = x_eval_make(NULL, NULL),
		*p_type, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];
	struct x_type_t type_whitespace = {
		.p_name = x_mkstr(p_base, "WHITESPACE"),
		.p_analyse = test_token_read_analyse_whitespace_prim,
		.p_delimit = x_mkatom(p_base, test_token_read_delimit_whitespace)
	};


	s = X_SEXP_CHAR_PRE_STR "@ " X_SEXP_CHAR_PRE_STR "A";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_eval_make(NULL, NULL);
	x_type_char_register(p_base, p_base);
	p_type = x_type_struct_make(p_base, type_whitespace);
	x_eval_type_alist_extend(p_base, p_type);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_buffer, p_base);

	p_obj = x_token_read(p_base, p_args);
	_it_should("return the @ character",
		x_obj_type_ischar(p_base, p_obj)
		&& '@' == x_charval(p_obj)
	);

	p_obj = x_token_read(p_base, p_args);
	_it_should("return the A character",
		x_obj_type_ischar(p_base, p_obj)
		&& 'A' == x_charval(p_obj)
	);


	test_cleanup(p_base);

	return NULL;
}

static char *test_sexp_char_read_named(void)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_obj;
	x_char_t buffer[64];
	x_int_t len;

	/* #\newline -> '\n' */
	p_base = x_eval_make(NULL, NULL);
	x_type_char_register(p_base, p_base);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_buffer, p_base);

	len = x_lib_strlen("#\\newline");
	x_lib_memcpy(x_bufferval(p_buffer), "#\\newline", len);
	x_bufferread(p_buffer) = x_bufferval(p_buffer) + len;

	p_obj = x_sexp_char_read(p_base, p_args);
	_it_should("return a char for #\\newline",
		x_obj_type_ischar(p_base, p_obj)
	);
	_it_should("set the char to '\\n'", '\n' == x_charval(p_obj));

	test_cleanup(p_base);


	/* #\space -> ' ' */
	p_base = x_eval_make(NULL, NULL);
	x_type_char_register(p_base, p_base);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_buffer, p_base);

	len = x_lib_strlen("#\\space");
	x_lib_memcpy(x_bufferval(p_buffer), "#\\space", len);
	x_bufferread(p_buffer) = x_bufferval(p_buffer) + len;

	p_obj = x_sexp_char_read(p_base, p_args);
	_it_should("return a char for #\\space",
		x_obj_type_ischar(p_base, p_obj)
	);
	_it_should("set the char to ' '", ' ' == x_charval(p_obj));

	test_cleanup(p_base);


	/* #\tab -> '\t' */
	p_base = x_eval_make(NULL, NULL);
	x_type_char_register(p_base, p_base);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_buffer, p_base);

	len = x_lib_strlen("#\\tab");
	x_lib_memcpy(x_bufferval(p_buffer), "#\\tab", len);
	x_bufferread(p_buffer) = x_bufferval(p_buffer) + len;

	p_obj = x_sexp_char_read(p_base, p_args);
	_it_should("return a char for #\\tab",
		x_obj_type_ischar(p_base, p_obj)
	);
	_it_should("set the char to '\\t'", '\t' == x_charval(p_obj));

	test_cleanup(p_base);


	return NULL;
}

static char *test_sexp_char_read_named_token(void)
{
	x_obj_t *p_base, *p_type, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[64];
	struct x_type_t type_whitespace;

	/* Read #\newline through the full tokenizer (exercises analyse4) */
	s = X_SEXP_CHAR_PRE_STR "newline ";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_eval_make(NULL, NULL);
	x_type_char_register(p_base, p_base);
	x_lib_memset(&type_whitespace, 0, sizeof(type_whitespace));
	type_whitespace.p_name = x_mkstr(p_base, "WHITESPACE");
	type_whitespace.p_analyse = test_token_read_analyse_whitespace_prim;
	type_whitespace.p_delimit = x_mkatom(p_base, test_token_read_delimit_whitespace);
	p_type = x_type_struct_make(p_base, type_whitespace);
	x_eval_type_alist_extend(p_base, p_type);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_buffer, p_base);

	p_obj = x_token_read(p_base, p_args);
	_it_should("return a char for tokenized #\\newline",
		x_obj_type_ischar(p_base, p_obj)
	);
	_it_should("set the char to '\\n'", '\n' == x_charval(p_obj));

	test_cleanup(p_base);


	return NULL;
}

static int test_error_called = 0;
static void test_error_hook(x_obj_t *p_base, x_char_t *msg, x_obj_t *p_obj)
{
	test_error_called = 1;
}
static x_satom_t test_error_hook_atom = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .v = (void *)test_error_hook });

static char *test_sexp_char_read_unknown(void)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_obj;
	x_char_t buffer[64];
	x_int_t len;

	/* #\xyzzy — unknown named character */
	p_base = x_eval_make(NULL, NULL);
	x_type_char_register(p_base, p_base);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_buffer, p_base);

	len = x_lib_strlen("#\\xyzzy");
	x_lib_memcpy(x_bufferval(p_buffer), "#\\xyzzy", len);
	x_bufferread(p_buffer) = x_bufferval(p_buffer) + len;

	/* Install error hook to prevent exit */
	x_firstobj(x_base_field_hook_error(p_base)) = (x_obj_t *)test_error_hook_atom;
	test_error_called = 0;

	p_obj = x_sexp_char_read(p_base, p_args);
	_it_should("return NULL for unknown named char",
		x_obj_isnil(p_base, p_obj));
	_it_should("trigger error for unknown named char",
		test_error_called == 1);

	test_cleanup(p_base);

	return NULL;
}

static char *run_tests() {
	_run_test(test_sexp_char_analyse1);
	_run_test(test_sexp_char_analyse2);
	_run_test(test_sexp_char_read);
	_run_test(test_sexp_char_read_token);
	_run_test(test_sexp_char_read_named);
	_run_test(test_sexp_char_read_named_token);
	_run_test(test_sexp_char_read_unknown);

	return NULL;
}
