/*
 * # Unit Tests: *x-sexp*
 */

#define X_TOKEN_SIZE_MAX 3

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

#include "src/x-sys.c"
#include "src/x-lib.c"
#include "src/x.c"
#include "src/x-obj.c"
#include "src/x-alist.c"
#include "src/x-base.c"
#include "src/x-sexp.c"
#include "src/x-token.c"
#include "src/x-type.c"
#include "src/x-type/atom.c"
#include "src/x-sexp/atom.c"
#include "src/x-type/pair.c"
#include "src/x-sexp/pair.c"
#include "src/x-type/char.c"
#include "src/x-sexp/char.c"
#include "src/x-type/str.c"
#include "src/x-sexp/str.c"
#include "src/x-type/buffer.c"
#include "src/x-type/prim.c"
#include "src/x-type/list.c"
#include "src/x-sexp/list.c"
#include "src/x-type/iter.c"

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
static char *test_token_get(void)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];

	s = "  ;123\n  (ABC)";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_base = x_base_make(NULL, NULL);

	p_buffer = x_base_field_buffer(p_base) = x_mkbufferown(p_base, buffer);
	/*x_sexp_write(p_base, x_base(p_base));*/
	p_args = x_mkspair(p_base, p_buffer, p_base);
	p_obj = x_token_get(p_base, p_base);
	printf("\"%s\"\n", x_bufferval(p_obj));
	p_obj = x_token_get(p_base, p_base);
	printf("\"%s\"\n", x_bufferval(p_obj));
/*	_it_should("return the Base", x_obj_isnil(p_base, p_obj));
*/
	x_sys_free(p_args);
	x_sys_free(p_buffer);

	return NULL;
}

static char *test_token_delimit(void)
{
	return NULL;
}


x_obj_t *test_token_read_analyse_whitespace(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	printf("Analysing whitespace...");

	if (x_lib_strchr(" \t\r\n", x_bufferlastchar(p_buffer))) {
		printf("match.\n");
		return p_args;
	}

	if (x_bufferlen(p_buffer) > 1) {
		x_bufferread(p_buffer)--;
		return p_buffer;
	}

	printf("unrecognized ('%c').\n", x_bufferlastchar(p_buffer));

	return p_base;
}

x_obj_t *test_token_read_delimit_whitespace(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	printf("Delmiting whitespace...");

	if (x_lib_strchr(" \t\r\n", x_bufferlastchar(p_buffer))) {
		printf("match.\n");
		x_bufferread(p_buffer)--;
		return p_buffer;
	}

	printf("unrecognized ('%c').\n", x_bufferlastchar(p_buffer));

	return p_base;
}

x_obj_t *test_token_read_read_whitespace(x_obj_t *p_base, x_obj_t *p_args)
{
	printf("Consuming whitespace.");

	return p_args;
}


x_obj_t *test_token_read_analyse_any(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	printf("Analysing any...match.\n");

	return p_buffer;
}

x_obj_t *test_token_read_read_any(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	printf("Reading any.");

	return x_mksatom(p_base, x_bufferval(p_buffer)[0]);
}


x_obj_t *test_token_read_analyse1(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	printf("Analysing1...");

	if (x_lib_strchr("@A ", x_bufferlastchar(p_buffer))) {
		printf("match.\n");
		return p_args;
	}

	if (x_bufferlen(p_buffer) > 1) {
		printf("unrecognized ('%c'), returning recognized.\n", x_bufferlastchar(p_buffer));
		x_bufferread(p_buffer)--;
		return p_buffer;
	}

	printf("unrecognized ('%c').\n", x_bufferlastchar(p_buffer));

	return p_base;
}

x_obj_t *test_token_read_read1(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);
	x_char_t *s = x_lib_strndup(x_bufferval(p_buffer), x_bufferlen(p_buffer));

	printf("Reading1.");

	return x_mkstrown(p_base, s);
	/*return x_mksatom(p_base, x_bufferval(p_buffer)[0]);*/
}


x_obj_t *test_token_read_analyse2_2(x_obj_t *p_base, x_obj_t *p_args);
x_satom_t test_token_read_analyse2_1_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_analyse2_2 });

x_obj_t *test_token_read_analyse2_1(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	printf("Analysing2_1...");

	if ('@' == x_bufferlastchar(p_buffer)) {
		printf("match.\n");
		return test_token_read_analyse2_1_prim;
	}

	printf("unrecognized ('%c').\n", x_bufferlastchar(p_buffer));

	return p_base;
}

x_obj_t *test_token_read_analyse2_2(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	printf("Analysing2_2...");

	if ('A' == x_bufferlastchar(p_buffer)) {
		printf("match.\n");
		return p_buffer;
	}

	printf("unrecognized ('%c').\n", x_bufferlastchar(p_buffer));

	return p_base;
}

x_obj_t *test_token_read_read2(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_obj = x_mksatom(p_base, x_bufferlastchar(p_buffer));

	printf("Reading2.");

	return p_obj;
}

static char *test_token_read(void)
{
	x_obj_t *p_base = x_base_make(NULL, NULL),
		*p_base2 = x_base_make(NULL, NULL),
		*p_type, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];
	struct x_type_t type_whitespace = {
		.p_name = x_mkstr(p_base, "WHITESPACE"),
		.p_analyse = x_mkatom(p_base, test_token_read_analyse_whitespace),
		.p_delimit = x_mkatom(p_base, test_token_read_delimit_whitespace)
	}, type_any = {
		.p_name = x_mkstr(p_base, "ANY"),
		.p_analyse = x_mkatom(p_base, test_token_read_analyse_any)
	}, type1 = {
		.p_name = x_mkstr(p_base, "TYPE1"),
		.p_analyse = x_mkatom(p_base, test_token_read_analyse1)
	}, type2 = {
		.p_name = x_mkstr(p_base, "TYPE2"),
		.p_analyse = x_mkatom(p_base, test_token_read_analyse2_1)
	};

	s = "@AAB  @AA ";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_type = x_type_struct_make(p_base, type_whitespace);
	x_base_type_alist_extend(p_base2, p_type);

	p_type = x_type_struct_make(p_base, type_any);
	x_base_type_alist_extend(p_base2, p_type);

	p_type = x_type_struct_make(p_base, type1);
	x_base_type_alist_extend(p_base2, p_type);

	p_type = x_type_struct_make(p_base, type2);
	x_base_type_alist_extend(p_base2, p_type);

	p_buffer = x_mkbufferown(p_base, buffer);

/*	x_type_buffer_read(p_base, x_mkspair(p_base, p_buffer, x_mkspair(p_base, x_mkchar(p_base, '\0'), p_base)));
	printf("C:'%c'\n", x_bufferlastchar(p_buffer));*/
	/*x_sexp_write(p_base, x_base(p_base));*/
	p_args = x_mkpair(p_base, p_buffer, p_base);

	p_obj = x_token_read(p_base2, p_args);
	printf("'%s'\n", x_strval(p_obj));
  	_it_should("return a token",
		x_obj_type_isstr(p_base2, p_obj)
		&& 0 == strcmp("@AA", x_strval(p_obj)));

	p_obj = x_token_read(p_base2, p_args);
	printf("'%c'\n", x_charval(p_obj));
  	_it_should("return a token",
		x_obj_type_issatom(p_obj)
		&& 'B' == x_charval(p_obj));

	p_obj = x_token_read(p_base2, p_args);
	printf("'%s'\n", x_strval(p_obj));
  	_it_should("return a token",
		x_obj_type_isstr(p_base2, p_obj)
		&& 0 == strcmp("@AA", x_strval(p_obj)));


	test_cleanup(p_base);
	test_cleanup(p_base2);


	return NULL;
}

static char *run_tests() {
	_xrun_test(test_token_get);
	_run_test(test_token_delimit);
	_xrun_test(test_token_read);

	return NULL;
}
