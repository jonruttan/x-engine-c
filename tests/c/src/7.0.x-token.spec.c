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
#include "src/x-type/buffer.c"
#include "src/x-type/atom.c"
#include "src/x-type/pair.c"
#include "src/x-type/iter.c"

/* Stub sexp list symbols to avoid pulling in x-sexp dependencies. */
#include "x-token/sexp/list.h"
x_satom_t x_sexp_list_analyse_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = NULL }),
	x_sexp_list_delimit_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = NULL }),
	x_sexp_list_read_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = NULL });

#include "src/x-eval.c"
#include "src/x-type/list.c"
#include "src/x-token.c"

#define STUB_X_PRIM
#define STUB_X_PROCEDURE
#define STUB_X_OPERATIVE
#define STUB_X_HEAP
#define STUB_X_OBJ_OBJ
#define STUB_X_STR
#define STUB_X_PRIM_SHADOW
#define STUB_X_PROCEDURE_APPLY
#define STUB_X_SYMBOL
#include "helper-stubs.c"



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
 * ## Test Helpers
 */

/* Whitespace type: matches runs of whitespace, discards them. */
x_obj_t *test_token_read_analyse_whitespace(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *test_token_read_read_whitespace(x_obj_t *p_base, x_obj_t *p_args);

x_satom_t test_token_read_analyse_whitespace_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_analyse_whitespace });

x_obj_t *test_token_read_analyse_whitespace(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_score = x_token_read_arg_score(p_args);

	if (x_lib_strchr(" \t\r\n", x_bufferlastchar(p_buffer))) {
		return test_token_read_analyse_whitespace_prim;
	}

	if (x_bufferlen(p_buffer) > 1) {
		x_firstint(p_score) = x_bufferlen(p_buffer) - 1;
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

	return x_mksatom(p_base, X_OBJ_FLAG_NONE, x_bufferval(p_buffer)[0]);
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
		return NULL;
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

	return NULL;
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

	return NULL;
}

x_obj_t *test_token_read_read2(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	return x_mksatom(p_base, X_OBJ_FLAG_NONE, x_bufferlastchar(p_buffer));
}


/* NullReader type: scores successfully but reader returns NULL. */
x_obj_t *test_token_read_read_null(x_obj_t *p_base, x_obj_t *p_args)
{
	return NULL;
}

x_satom_t test_token_read_read_null_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_read_null });

x_obj_t *test_token_read_analyse_null(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_score = x_token_read_arg_score(p_args);

	/* Score with positive value but reader returns NULL */
	x_firstint(p_score) = x_bufferlen(p_buffer);
	x_restobj(p_score) = test_token_read_read_null_prim;
	return p_score;
}

/* ContinueType: keeps returning buffer_args (continue), never scores.
 * Used for RO EOF auto-score testing. */
x_obj_t *test_token_read_analyse_continue(x_obj_t *p_base, x_obj_t *p_args)
{
	return p_args;
}

/* AutoScoreType: sets reader on score as side-effect, then continues.
 * Used to exercise the EOF auto-score path (lines 175-184). */
x_obj_t *test_token_read_analyse_autoscore(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_score = x_token_read_arg_score(p_args);

	/* Set the reader on the score via side-effect, then return
	 * buffer_args (continue) to keep consuming chars. */
	x_restobj(p_score) = test_token_read_read_catchall_prim;
	x_firstint(p_score) = -1;

	return p_args;
}


/* Static handler atoms — must use x_type_atom_obj for x_callable_apply dispatch */
static x_satom_t
	analyse_whitespace_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_analyse_whitespace }),
	delimit_whitespace_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_delimit_whitespace }),
	analyse1_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_analyse1 }),
	analyse_catchall_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_analyse_catchall }),
	read_catchall_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_read_catchall }),
	read1_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_read1 }),
	analyse2_1_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_analyse2_1 }),
	read2_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_read2 }),
	analyse_null_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_analyse_null }),
	read_null_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_read_null }),
	analyse_autoscore_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_token_read_analyse_autoscore });

/*
 * ## Test Runners
 */
static char *test_token_delimit(void)
{
	x_obj_t *p_base = x_eval_make(NULL, NULL),
		*p_base2 = x_eval_make(NULL, NULL),
		*p_type, *p_buffer, *p_args, *p_obj;
	x_char_t buffer[32];
	struct x_type_t type_whitespace = {
		.p_name = x_mkatom(p_base, (void *)"WHITESPACE"),
		.p_analyse = (x_obj_t *)analyse_whitespace_prim,
		.p_delimit = (x_obj_t *)delimit_whitespace_prim
	}, type1 = {
		.p_name = x_mkatom(p_base, (void *)"TYPE1"),
		.p_analyse = (x_obj_t *)analyse1_prim
	};

	p_type = x_type_struct_make(p_base, type_whitespace);
	x_eval_type_alist_extend(p_base2, p_type);

	p_type = x_type_struct_make(p_base, type1);
	x_eval_type_alist_extend(p_base2, p_type);

	/* Space is a whitespace delimiter.
	 * x_token_delimit expects (buffer, self-type, ...) where self-type
	 * is the type to skip.  Pass NULL as self-type so no type is skipped. */
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = " ";
	helper_file_reset();
	p_buffer = x_mkbufferown(p_base, buffer);
	x_type_buffer_read(p_base, x_mkspair(p_base, X_OBJ_FLAG_NONE, p_buffer, NULL));
	{
		x_spair_t delimit_args[2] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_buffer }, { (x_obj_t *)(delimit_args + 1) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL }),
		};
		p_args = (x_obj_t *)delimit_args;

		p_obj = x_token_delimit(p_base2, p_args);
		_it_should("delimit returns buffer for space",
			p_obj == p_buffer);

		/* Non-delimiter char */
		helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = "A";
		helper_file_reset();
		x_type_buffer_reset(p_base, x_mkspair(p_base, X_OBJ_FLAG_NONE, p_buffer, NULL));
		x_type_buffer_read(p_base, x_mkspair(p_base, X_OBJ_FLAG_NONE, p_buffer, NULL));

		p_obj = x_token_delimit(p_base2, p_args);
		_it_should("delimit returns NULL for non-delimiter",
			x_obj_isnil(p_base, p_obj));
	}

	test_cleanup(p_base);
	test_cleanup(p_base2);

	return NULL;
}


static char *test_token_read(void)
{
	x_obj_t *p_base = x_eval_make(NULL, NULL),
		*p_base2 = x_eval_make(NULL, NULL),
		*p_type, *p_args, *p_buffer, *p_obj;
	x_char_t *s, buffer[32];
	struct x_type_t type_whitespace = {
		.p_name = x_mkatom(p_base, (void *)"WHITESPACE"),
		.p_analyse = (x_obj_t *)analyse_whitespace_prim,
		.p_delimit = (x_obj_t *)delimit_whitespace_prim
	}, type_catchall = {
		.p_name = x_mkatom(p_base, (void *)"CATCHALL"),
		.p_analyse = (x_obj_t *)analyse_catchall_prim,
		.p_read = (x_obj_t *)read_catchall_prim
	}, type1 = {
		.p_name = x_mkatom(p_base, (void *)"TYPE1"),
		.p_analyse = (x_obj_t *)analyse1_prim,
		.p_read = (x_obj_t *)read1_prim
	}, type2 = {
		.p_name = x_mkatom(p_base, (void *)"TYPE2"),
		.p_analyse = (x_obj_t *)analyse2_1_prim,
		.p_read = (x_obj_t *)read2_prim
	};

	s = "@AAB  @AA ";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_type = x_type_struct_make(p_base, type_whitespace);
	x_eval_type_alist_extend(p_base2, p_type);

	p_type = x_type_struct_make(p_base, type_catchall);
	x_eval_type_alist_extend(p_base2, p_type);

	p_type = x_type_struct_make(p_base, type1);
	x_eval_type_alist_extend(p_base2, p_type);

	p_type = x_type_struct_make(p_base, type2);
	x_eval_type_alist_extend(p_base2, p_type);

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

static char *test_token_read_eof(void)
{
	x_obj_t *p_base = x_eval_make(NULL, NULL),
		*p_base2 = x_eval_make(NULL, NULL),
		*p_type, *p_args, *p_buffer, *p_obj;
	x_char_t buffer[32];
	struct x_type_t type_catchall = {
		.p_name = x_mkatom(p_base, (void *)"CATCHALL"),
		.p_analyse = (x_obj_t *)analyse_catchall_prim,
		.p_read = (x_obj_t *)read_catchall_prim
	};

	p_type = x_type_struct_make(p_base, type_catchall);
	x_eval_type_alist_extend(p_base2, p_type);

	/* Empty input — analyse returns NULL → read returns NULL */
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = "";
	helper_file_buffer_remaining[TEST_HELPER_FILE_STDIN] = 0;
	helper_file_reset();

	p_buffer = x_mkbufferown(p_base, buffer);
	p_args = x_mkpair(p_base, p_buffer, p_base);

	p_obj = x_token_read(p_base2, p_args);
	_it_should("return NULL on empty input",
		x_obj_isnil(p_base, p_obj));

	/* RO buffer with data — exercises the RO EOF branch in analyse */
	{
		x_char_t ro_buf[] = "A";
		x_obj_t *p_ro_buffer, *p_ro_args;

		p_ro_buffer = x_mkbufferro(p_base, ro_buf);
		x_bufferwrite(p_ro_buffer) = x_bufferval(p_ro_buffer) + 1;

		p_ro_args = x_mkpair(p_base, p_ro_buffer, p_base);

		p_obj = x_token_read(p_base2, p_ro_args);
		_it_should("return token from RO buffer",
			! x_obj_isnil(p_base, p_obj));
	}

	helper_file_buffer_remaining[TEST_HELPER_FILE_STDIN] = TEST_HELPER_FILE_UNDEFINED;
	test_cleanup(p_base);
	test_cleanup(p_base2);

	return NULL;
}

static char *test_token_read_null_reader(void)
{
	x_obj_t *p_base = x_eval_make(NULL, NULL),
		*p_base2 = x_eval_make(NULL, NULL),
		*p_type, *p_args, *p_buffer, *p_obj;
	x_char_t buffer[32];
	struct x_type_t type_null = {
		.p_name = x_mkatom(p_base, (void *)"NULL_READER"),
		.p_analyse = (x_obj_t *)analyse_null_prim,
		.p_read = (x_obj_t *)read_null_prim
	};

	p_type = x_type_struct_make(p_base, type_null);
	x_eval_type_alist_extend(p_base2, p_type);

	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = "X";
	helper_file_buffer_remaining[TEST_HELPER_FILE_STDIN] = 1;
	helper_file_reset();

	p_buffer = x_mkbufferown(p_base, buffer);
	p_args = x_mkpair(p_base, p_buffer, p_base);

	/* Reader returns NULL → x_token_read returns NULL */
	p_obj = x_token_read(p_base2, p_args);
	_it_should("read returns NULL when reader returns NULL",
		x_obj_isnil(p_base, p_obj));

	helper_file_buffer_remaining[TEST_HELPER_FILE_STDIN] = TEST_HELPER_FILE_UNDEFINED;
	test_cleanup(p_base);
	test_cleanup(p_base2);

	return NULL;
}

static char *test_token_read_ro_eof(void)
{
	x_obj_t *p_base = x_eval_make(NULL, NULL),
		*p_base2 = x_eval_make(NULL, NULL),
		*p_type, *p_args, *p_obj;
	x_char_t ro_buf[] = "AB";
	x_obj_t *p_ro_buffer;
	struct x_type_t type_autoscore = {
		.p_name = x_mkatom(p_base, (void *)"AUTOSCORE"),
		.p_analyse = (x_obj_t *)analyse_autoscore_prim,
		.p_read = (x_obj_t *)read_catchall_prim
	};

	/* AutoScore type: sets reader via side-effect, returns continue.
	 * When RO EOF is hit, auto-score path computes score from consumed
	 * chars and the sign from the partial score. */
	p_type = x_type_struct_make(p_base, type_autoscore);
	x_eval_type_alist_extend(p_base2, p_type);

	/* RO buffer with 2 bytes of data */
	p_ro_buffer = x_mkbufferro(p_base, ro_buf);
	x_bufferwrite(p_ro_buffer) = x_bufferval(p_ro_buffer) + 2;

	p_args = x_mkpair(p_base, p_ro_buffer, p_base);

	/* This exercises RO EOF break (line 129) and catchall auto-score. */
	p_obj = x_token_read(p_base2, p_args);
	_it_should("RO EOF read returns a token",
		! x_obj_isnil(p_base, p_obj));

	test_cleanup(p_base);
	test_cleanup(p_base2);

	return NULL;
}

static char *test_alist_iter_nil(void)
{
	x_obj_t *p_base, *p_iter, *p_args, *p_obj;

	p_base = x_eval_make(NULL, NULL);

	/* Iterator over empty list — x_type_alist_iter returns nil */
	p_iter = x_mkiter(p_base, (x_obj_t *)&x_type_alist_iter_prim, NULL);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_iter, NULL);
	p_obj = x_type_alist_iter(p_base, p_args);
	_it_should("alist_iter returns nil for empty list",
		x_obj_isnil(p_base, p_obj));

	test_cleanup(p_base);
	return NULL;
}

static char *run_tests() {
	_run_test(test_token_delimit);
	_run_test(test_token_read);
	_run_test(test_token_read_eof);
	_run_test(test_token_read_null_reader);
	_run_test(test_token_read_ro_eof);
	_run_test(test_alist_iter_nil);

	return NULL;
}
