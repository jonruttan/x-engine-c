/*
 * # Unit Tests: *x-base*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "src/x-sys.c"
#include "src/x-lib.c"
#include "src/x-obj.c"
#include "src/x.c"
#include "src/x-alist.c"
#include "src/x-base.c"

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

static char *test_base_make(void)
{
	x_obj_t *p_base = NULL, *p_obj;

	_it_should("return that the Base object is not set",
		! x_base_isset(p_base)
	);

	p_base = x_mksatom(NULL, NULL);
	_it_should("return that the Base object is not set",
		! x_base_isset(p_base)
	);

	x_obj_free(p_base);


	p_base = x_base_make(NULL, NULL);
	_it_should("return a new Base object",
		NULL != p_base
		&& x_obj_type_issatom(p_base)
	);

	_it_should("return that the Base object is set", x_base_isset(p_base));

	p_obj = x_base(p_base);
	_it_should("return the Base object db",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_isspair(p_obj)
	);

/*	p_obj = x_base_field_type_alist(p_base);
	_it_should("return the Base object type alist",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_isspair(p_obj)
		&& x_obj_isnil(p_base, x_car(p_obj))
	);
*/

	p_obj = x_base_field_files(p_base);
	_it_should("return the Base object file descriptor list",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_isspair(p_obj)
	);

	p_obj = x_base_field_filein(p_base);
	_it_should("return the Base object input file descriptor",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_issatom(p_obj)
		&& x_atomint(p_obj) == STDIN_FILENO
	);

	p_obj = x_base_field_fileout(p_base);
	_it_should("return the Base object output file descriptor",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_issatom(p_obj)
		&& x_atomint(p_obj) == STDOUT_FILENO
	);

	p_obj = x_base_field_fileerr(p_base);
	_it_should("return the Base object error file descriptor",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_issatom(p_obj)
		&& x_atomint(p_obj) == STDERR_FILENO
	);


	p_obj = x_base_field_env_alist(p_base);
	_it_should("return the Base object environment list",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_isspair(p_obj)
	);

	p_obj = x_base_field_eval_list(p_base);
	_it_should("return the Base object expression list",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_isspair(p_obj)
		&& x_obj_isnil(p_base, x_car(p_obj))
	);

	p_obj = x_base_field_buffer(p_base);
	_it_should("return the Base object buffer",
		x_obj_isnil(p_base, p_obj)
	);

	p_obj = x_base_field_token_cache(p_base);
	_it_should("return the Base object token cache",
		x_obj_isnil(p_base, p_obj)
	);

	x_sys_free(p_base);

	return NULL;
}

static char *test_base_type_alist_extend(void)
{
	x_obj_t *p_base, *p_alist, *p_atoms[3], *p_args;

	p_base = x_mksatom(NULL, 0);

	p_atoms[0] = x_mksatom(p_base, 1);
	p_atoms[1] = x_mksatom(p_base, 2);
	p_atoms[2] = x_mksatom(p_base, 3);


	p_alist = p_base;
	p_args = x_mkspair(NULL, p_atoms[0], p_atoms[1]);
	p_alist = x_base_type_alist_extend(NULL, p_args);
	_it_should("return NULL when base is NULL",
		NULL == p_alist
	);


	p_base = x_mksatom(NULL, NULL);
	p_args = x_mkspair(p_base, p_atoms[0], p_atoms[1]);
	p_alist = x_base_type_alist_extend(p_base, p_args);
	_it_should("return nil when base is not set",
		p_base == p_alist
	);

	x_sys_free(p_base);


	p_base = x_base_make(NULL, NULL);
	p_args = x_mkspair(p_base, p_atoms[0], p_atoms[1]);
	p_alist = x_base_type_alist_extend(p_base, p_args);
	_it_should("extend alist with (#<0x1:0x0> . #<0x1:0x1>)",
		x_obj_type_isspair(p_alist)
		&& x_obj_type_isspair(x_car(p_alist))
		&& x_caar(p_alist) == p_atoms[0]
		&& x_cdar(p_alist) == p_atoms[1]
	);


	p_args = x_mkspair(p_base, p_atoms[1], p_atoms[2]);
	p_alist = x_base_type_alist_extend(p_base, p_args);
	_it_should("extend alist with (#<0x1:0x2> . #<0x1:0x3>)",
		x_obj_type_isspair(p_alist)
		&& x_obj_type_isspair(x_car(p_alist))
		&& x_caar(p_alist) == p_atoms[1]
		&& x_cdar(p_alist) == p_atoms[2]
		&& x_obj_type_isspair(x_cadr(p_alist))
		&& x_caadr(p_alist) == p_atoms[0]
		&& x_cdadr(p_alist) == p_atoms[1]
	);

	x_sys_free(p_base);

	return NULL;
}

static char *test_base_type_alist_assoc(void)
{
	x_obj_t *p_base, *p_obj[2], *p_args, *p_assoc[2];
	x_char_t *s[3] = {
		"item1",
		"item2",
		"item3"
	};

	p_args = x_mkspair(NULL,
		x_mksatom(NULL, s[0]),
		NULL
	);

	p_obj[0] = x_base_type_alist_assoc(NULL, p_args);
	_it_should("return NULL when base is NULL",
		NULL == p_obj[0]
	);

	p_base = x_mksatom(NULL, NULL);
	p_obj[0] = x_base_type_alist_assoc(p_base, p_args);
	_it_should("return nil when base is not set",
		p_base == p_obj[0]
	);

	x_sys_free(p_base);


	p_base = x_base_make(NULL, NULL);

	p_args = x_mkspair(p_base, x_mksatom(p_base, s[0]), p_base);
	p_obj[0] = x_base_type_alist_assoc(p_base, p_args);
	_it_should("return nil when item is not found",
		p_base == p_obj[0]
	);


	p_assoc[0] = x_mkspair(p_base, x_mksatom(p_base, s[0]), p_base);
	p_assoc[1] = x_mkspair(p_base, x_mksatom(p_base, s[1]), p_base);
	x_base_type_alist_extend(p_base, p_assoc[0]);
	x_base_type_alist_extend(p_base, p_assoc[1]);

	p_obj[0] = x_base_type_alist_assoc(p_base, p_args);
	_it_should("return item when found",
		x_firstobj(p_assoc[0]) == x_firstobj(p_obj[0])
	);

	p_obj[1] = x_base_type_alist_assoc(p_base, p_args);
	_it_should("return same item when found",
		p_obj[0] == p_obj[1]
	);


	p_args = x_mkspair(p_base, x_mksatom(p_base, s[1]), p_base);
	p_obj[0] = x_base_type_alist_assoc(p_base, p_args);
	_it_should("return second item when found",
		x_firstobj(p_assoc[1]) == x_firstobj(p_obj[0])
	);

	p_args = x_mkspair(p_base, x_mksatom(p_base, s[2]), p_base);
	p_obj[0] = x_base_type_alist_assoc(p_base, p_args);
	_it_should("return nil when item is not found",
		p_base == p_obj[0]
	);

	x_sys_free(p_base);

	return NULL;
}

static char *test_base_env_alist_extend(void)
{
	x_obj_t *p_base, *p_alist, *p_atoms[3], *p_args;

	p_base = x_mksatom(NULL, 0);

	p_atoms[0] = x_mksatom(p_base, 1);
	p_atoms[1] = x_mksatom(p_base, 2);
	p_atoms[2] = x_mksatom(p_base, 3);


	p_alist = p_base;
	p_args = x_mkspair(NULL, p_atoms[0], p_atoms[1]);
	p_alist = x_base_env_alist_extend(NULL, p_args);
	_it_should("return NULL when base is NULL",
		NULL == p_alist
	);


	p_base = x_mksatom(NULL, NULL);
	p_args = x_mkspair(p_base, p_atoms[0], p_atoms[1]);
	p_alist = x_base_env_alist_extend(p_base, p_args);
	_it_should("return nil when base is not set",
		p_base == p_alist
	);

	x_sys_free(p_base);


	p_base = x_base_make(NULL, NULL);
	p_args = x_mkspair(p_base, p_atoms[0], p_atoms[1]);
	p_alist = x_base_env_alist_extend(p_base, p_args);
	_it_should("extend alist with (#<0x1:0x0> . #<0x1:0x1>)",
		x_obj_type_isspair(p_alist)
		&& x_obj_type_isspair(x_car(p_alist))
		&& x_caar(p_alist) == p_atoms[0]
		&& x_cdar(p_alist) == p_atoms[1]
	);


	p_args = x_mkspair(p_base, p_atoms[1], p_atoms[2]);
	p_alist = x_base_env_alist_extend(p_base, p_args);
	_it_should("extend alist with (#<0x1:0x2> . #<0x1:0x3>)",
		x_obj_type_isspair(p_alist)
		&& x_obj_type_isspair(x_car(p_alist))
		&& x_caar(p_alist) == p_atoms[1]
		&& x_cdar(p_alist) == p_atoms[2]
		&& x_obj_type_isspair(x_cadr(p_alist))
		&& x_caadr(p_alist) == p_atoms[0]
		&& x_cdadr(p_alist) == p_atoms[1]
	);

	x_sys_free(p_base);

	return NULL;
}

static char *test_base_env_alist_assoc(void)
{
	x_obj_t *p_base, *p_obj[2], *p_args, *p_assoc[2];
	x_char_t *s[3] = {
		"item1",
		"item2",
		"item3"
	};

	p_args = x_mkspair(NULL,
		x_mksatom(NULL, s[0]),
		NULL
	);

	p_obj[0] = x_base_env_alist_assoc(NULL, p_args);
	_it_should("return NULL when base is NULL",
		NULL == p_obj[0]
	);

	p_base = x_mksatom(NULL, NULL);
	p_obj[0] = x_base_env_alist_assoc(p_base, p_args);
	_it_should("return nil when base is not set",
		p_base == p_obj[0]
	);

	x_sys_free(p_base);


	p_base = x_base_make(NULL, NULL);

	p_args = x_mkspair(p_base, x_mksatom(p_base, s[0]), p_base);
	p_obj[0] = x_base_env_alist_assoc(p_base, p_args);
	_it_should("return nil when item is not found",
		p_base == p_obj[0]
	);


	p_assoc[0] = x_mkspair(p_base, x_mksatom(p_base, s[0]), p_base);
	p_assoc[1] = x_mkspair(p_base, x_mksatom(p_base, s[1]), p_base);
	x_base_env_alist_extend(p_base, p_assoc[0]);
	x_base_env_alist_extend(p_base, p_assoc[1]);

	p_obj[0] = x_base_env_alist_assoc(p_base, p_args);
	_it_should("return item when found",
		x_firstobj(p_assoc[0]) == x_firstobj(p_obj[0])
	);

	p_obj[1] = x_base_env_alist_assoc(p_base, p_args);
	_it_should("return same item when found",
		p_obj[0] == p_obj[1]
	);


	p_args = x_mkspair(p_base, x_mksatom(p_base, s[1]), p_base);
	p_obj[0] = x_base_env_alist_assoc(p_base, p_args);
	_it_should("return second item when found",
		x_firstobj(p_assoc[1]) == x_firstobj(p_obj[0])
	);

	p_args = x_mkspair(p_base, x_mksatom(p_base, s[2]), p_base);
	p_obj[0] = x_base_env_alist_assoc(p_base, p_args);
	_it_should("return nil when item is not found",
		p_base == p_obj[0]
	);

	x_sys_free(p_base);

	return NULL;
}

static char *test_base_read(void)
{
	x_obj_t *p_args, *p_obj;
	x_char_t *s;


	s = "";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_buffer_length[TEST_HELPER_FILE_STDIN] = 0;
	helper_file_reset();

	p_args = x_mkspair(NULL,
		x_mksatom(NULL, s),
		x_mkspair(NULL, x_mksatom(NULL, 1), NULL)
	);
	p_obj = x_base_read(NULL, p_args);
	_it_should("return the Base", NULL == p_obj);

	x_sys_free(p_obj);


	s = "@";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_buffer_length[TEST_HELPER_FILE_STDIN] = TEST_HELPER_FILE_UNDEFINED;
	helper_file_reset();

	p_args = x_mkspair(NULL,
		x_mksatom(NULL, s),
		x_mkspair(NULL, x_mksatom(NULL, 1), NULL)
	);
	p_obj = x_base_read(NULL, p_args);
	_it_should("return a Character object with the value set",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_issatom(p_obj)
		&& s[0] == x_atomchar(p_obj)
	);

	x_sys_free(p_obj);

	return NULL;
}

static char *test_base_write(void)
{
	x_obj_t *p_args, *p_obj;
	x_char_t s[8];


	helper_file_buffer_ptr[TEST_HELPER_FILE_STDOUT] = s;
	helper_file_buffer_length[TEST_HELPER_FILE_STDOUT] = 1;
	helper_file_reset();

	p_args = x_mkspair(NULL,
		x_mksatom(NULL, "01"),
		x_mkspair(NULL, x_mksatom(NULL, 2), NULL)
	);
	p_obj = x_base_write(NULL, p_args);
	_it_should("return the Base", NULL == p_obj);

	x_sys_free(p_obj);
	x_sys_free(p_obj);


	helper_file_buffer_ptr[TEST_HELPER_FILE_STDOUT] = s;
	helper_file_buffer_length[TEST_HELPER_FILE_STDOUT] = 8;
	helper_file_reset();

	p_args = x_mkspair(NULL,
		x_mksatom(NULL, "@"),
		x_mkspair(NULL, x_mksatom(NULL, 1), NULL)
	);
	p_obj = x_base_write(NULL, p_args);
	_it_should("return a Character object with the value set",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_issatom(p_obj)
		&& s[0] == x_atomstr(p_obj)[0]
	);

	x_sys_free(p_obj);

	return NULL;
}

static char *run_tests() {
	_run_test(test_base_make);
	_run_test(test_base_type_alist_extend);
	_run_test(test_base_type_alist_assoc);
	_run_test(test_base_env_alist_extend);
	_run_test(test_base_env_alist_assoc);
	_run_test(test_base_read);
	_run_test(test_base_write);

	return NULL;
}
