/*
 * # Unit Tests: *x-token*
 */

#define X_TOKEN_SIZE_MAX 3

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

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
#include "src/x-type/atom.c"
#include "src/x-type/pair.c"
#include "src/x-type/iter.c"

/* Stub sexp list symbols to avoid pulling in x-sexp dependencies. */
#include "x-sexp/list.h"
x_satom_t x_sexp_list_analyse_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = NULL }),
	x_sexp_list_delimit_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = NULL }),
	x_sexp_list_read_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = NULL }),
	x_sexp_list_write_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = NULL });

#include "src/x-type/list.c"
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
 * ## Test Helpers
 */

/* Whitespace type: matches runs of whitespace, discards them. */
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


/* Catchall type: matches any single char with negative score. */
x_obj_t *test_token_read_read_catchall(x_obj_t *p_base, x_obj_t *p_args);

x_satom_t test_token_read_read_catchall_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_read_catchall });

x_obj_t *test_token_read_analyse_catchall(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_score = x_token_read_arg_score(p_args);

	x_firstint(p_score) = -x_bufferlen(p_buffer);
	x_restobj(p_score) = test_token_read_read_catchall_prim;
	return p_score;
}

x_obj_t *test_token_read_read_catchall(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	return x_mksatom(p_base, x_bufferval(p_buffer)[0]);
}


/* TYPE1: matches sequences of @/A chars, returns as owned atom. */
x_obj_t *test_token_read_read1(x_obj_t *p_base, x_obj_t *p_args);

x_satom_t test_token_read_read1_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_read1 });

x_obj_t *test_token_read_analyse1(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_score = x_token_read_arg_score(p_args);

	if (x_lib_strchr("@A", x_bufferlastchar(p_buffer))) {
		return p_args;
	}

	x_bufferread(p_buffer)--;

	if (x_bufferlen(p_buffer) < 1) {
		return p_base;
	}

	x_firstint(p_score) = x_bufferlen(p_buffer);
	x_restobj(p_score) = test_token_read_read1_prim;
	return p_score;
}

x_obj_t *test_token_read_read1(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);
	x_char_t *s = x_lib_strndup(x_bufferval(p_buffer), x_bufferlen(p_buffer));

	return x_obj_make(p_base, x_type_atom_obj, X_OBJ_FLAG_OWN, X_OBJ_LENGTH_ATOM, (x_obj_t *)s);
}


/* TYPE2: matches @A (two-stage analyser), returns as satom. */
x_obj_t *test_token_read_analyse2_2(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *test_token_read_read2(x_obj_t *p_base, x_obj_t *p_args);

x_satom_t test_token_read_analyse2_2_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_analyse2_2 }),
	test_token_read_read2_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_read2 });

x_obj_t *test_token_read_analyse2_1(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	if ('@' == x_bufferlastchar(p_buffer)) {
		return test_token_read_analyse2_2_prim;
	}

	return p_base;
}

x_obj_t *test_token_read_analyse2_2(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_score = x_token_read_arg_score(p_args);

	if ('A' == x_bufferlastchar(p_buffer)) {
		x_firstint(p_score) = x_bufferlen(p_buffer);
		x_restobj(p_score) = test_token_read_read2_prim;
		return p_score;
	}

	return p_base;
}

x_obj_t *test_token_read_read2(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	return x_mksatom(p_base, x_bufferlastchar(p_buffer));
}


/*
 * ## Test Runners
 */
static char *test_token_delimit(void)
{
	return NULL;
}


static char *test_token_read(void)
{
	x_obj_t *p_base = x_base_make(NULL, NULL),
		*p_base2 = x_base_make(NULL, NULL),
		*p_type, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];
	struct x_type_t type_whitespace = {
		.p_name = x_mkatom(p_base, (void *)"WHITESPACE"),
		.p_analyse = x_mkatom(p_base, test_token_read_analyse_whitespace),
		.p_delimit = x_mkatom(p_base, test_token_read_delimit_whitespace)
	}, type_catchall = {
		.p_name = x_mkatom(p_base, (void *)"CATCHALL"),
		.p_analyse = x_mkatom(p_base, test_token_read_analyse_catchall)
	}, type1 = {
		.p_name = x_mkatom(p_base, (void *)"TYPE1"),
		.p_analyse = x_mkatom(p_base, test_token_read_analyse1)
	}, type2 = {
		.p_name = x_mkatom(p_base, (void *)"TYPE2"),
		.p_analyse = x_mkatom(p_base, test_token_read_analyse2_1)
	};

	s = "@AAB  @AA ";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_type = x_type_struct_make(p_base, type_whitespace);
	x_base_type_alist_extend(p_base2, p_type);

	p_type = x_type_struct_make(p_base, type_catchall);
	x_base_type_alist_extend(p_base2, p_type);

	p_type = x_type_struct_make(p_base, type1);
	x_base_type_alist_extend(p_base2, p_type);

	p_type = x_type_struct_make(p_base, type2);
	x_base_type_alist_extend(p_base2, p_type);

	p_buffer = x_mkbufferown(p_base, buffer);
	p_args = x_mkpair(p_base, p_buffer, p_base);

	p_obj = x_token_read(p_base2, p_args);
  	_it_should("return a TYPE1 token",
		x_obj_type_issatom(p_obj)
		&& 0 == strcmp("@AA", x_atomstr(p_obj)));

	p_obj = x_token_read(p_base2, p_args);
  	_it_should("return a CATCHALL token",
		x_obj_type_issatom(p_obj)
		&& 'B' == x_atomchar(p_obj));

	p_obj = x_token_read(p_base2, p_args);
  	_it_should("return a TYPE1 token",
		x_obj_type_issatom(p_obj)
		&& 0 == strcmp("@AA", x_atomstr(p_obj)));


	test_cleanup(p_base);
	test_cleanup(p_base2);


	return NULL;
}

static char *run_tests() {
	_run_test(test_token_delimit);
	_run_test(test_token_read);

	return NULL;
}
