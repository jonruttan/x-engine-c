/*
 * # Unit Tests: *x-sexp/pair*
 */

#include "test-runner.h"

#include "src/x-sys.c"
#include "src/x-lib.c"
#include "src/x-obj.c"
#include "src/x.c"
#include "src/x-alist.c"
#include "src/x-base.c"
#include "src/x-sexp/pair.c"

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

static int x_sexp_write_call_count = 0;
x_obj_t *x_sexp_write(x_obj_t *p_base, x_obj_t *p_obj)
{
	char s[16];
	int fd = x_base_isset(p_base) ? x_atomint(x_base_field_fileout(p_base)) : STDOUT_FILENO;

	sprintf(s, "<%d>", ++x_sexp_write_call_count);
	x_sys_write(fd, s, strlen(s));

	return p_base;
}


/*
 * ## Test Runners
 */
static char *test_sexp_pair_write(void)
{
	x_obj_t *p_atoms[2], *p_pairs[4], *p_args;
	x_char_t buffer[4096], *expected, *s;

	helper_file_buffer_ptr[TEST_HELPER_FILE_STDOUT] = buffer;

	p_atoms[0] = x_mksatom(NULL, 0);
	p_atoms[1] = x_mksatom(NULL, 1);
	p_pairs[0] = x_mkspair(NULL, NULL, NULL);
	p_pairs[1] = x_mkspair(NULL, p_atoms[0], NULL);
	p_pairs[2] = x_mkspair(NULL, p_atoms[0], p_atoms[1]);
	p_pairs[3] = x_mkspair(NULL, p_atoms[1], p_pairs[1]);

	p_args = x_mkspair(NULL, NULL, NULL);

	helper_file_reset();
	x_firstobj(p_args) = p_pairs[0];
	x_sexp_pair_write(NULL, p_args);
	expected = "()";
	s = helper_file_str(TEST_HELPER_FILE_STDOUT);
	_it_should("write empty pair s-exp to stdout",
		0 == strncmp(expected, s, strlen(expected))
		&& 0 == x_sexp_write_call_count
	);


	helper_file_reset();
	x_firstobj(p_args) = p_pairs[1];
	x_sexp_pair_write(NULL, p_args);
	expected = "(<1>)";
	s = helper_file_str(TEST_HELPER_FILE_STDOUT);
	_it_should("write pair s-exp to stdout",
		0 == strncmp(expected, s, strlen(expected))
		&& 1 == x_sexp_write_call_count
	);


	helper_file_reset();
	x_firstobj(p_args) = p_pairs[2];
	x_sexp_pair_write(NULL, p_args);
	expected = "(<2> . <3>)";
	s = helper_file_str(TEST_HELPER_FILE_STDOUT);
	_it_should("write broken pair s-exp to stdout",
		0 == strncmp(expected, s, strlen(expected))
		&& 3 == x_sexp_write_call_count
	);


	helper_file_reset();
	x_firstobj(p_args) = p_pairs[3];
	x_sexp_pair_write(NULL, p_args);
	expected = "(<4> <5>)";
	s = helper_file_str(TEST_HELPER_FILE_STDOUT);
	_it_should("write list s-exp to stdout",
		0 == strncmp(expected, s, strlen(expected))
		&& 5 == x_sexp_write_call_count
	);


	x_sys_free(p_pairs[3]);
	x_sys_free(p_pairs[2]);
	x_sys_free(p_pairs[1]);
	x_sys_free(p_pairs[0]);
	x_sys_free(p_atoms[1]);
	x_sys_free(p_atoms[0]);

	return NULL;
}

static char *run_tests() {
	_run_test(test_sexp_pair_write);

	return NULL;
}
