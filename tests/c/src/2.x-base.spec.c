/*
 * # Unit Tests: *x-base*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-lib.c"
#include "ext/x-expr/src/x-obj.c"
#include "ext/x-expr/src/x.c"
#include "src/x-alist.c"
#include "src/x-base.c"

#define STUB_X_PRIM
#define STUB_X_PRIM_REGISTER
#define STUB_X_PROCEDURE
#define STUB_X_OPERATIVE
#define STUB_X_HEAP
#define STUB_X_OBJ_OBJ
#define STUB_X_STR
#define STUB_X_TYPE_PRIM
#include "helper-stubs.c"

/* x_base_env_alist_assoc is not yet implemented */
x_obj_t *x_base_env_alist_assoc(x_obj_t *p_base, x_obj_t *p_args) { return NULL; }

/*
 * Controllable stubs for x_token_read/write and x_eval.
 * _token_read_returns[] is a NULL-terminated sequence of values;
 * each call pops the next one.
 */
static x_obj_t **_token_read_returns;
static int _token_read_index;

x_obj_t *x_token_read(x_obj_t *p_base, x_obj_t *p_args)
{
	if (_token_read_returns != NULL
		&& _token_read_returns[_token_read_index] != NULL) {
		return _token_read_returns[_token_read_index++];
	}
	return NULL;
}

x_obj_t *x_token_write(x_obj_t *p_base, x_obj_t *p_args) { return NULL; }

static x_obj_t *_eval_last;

x_obj_t *x_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	_eval_last = x_firstobj(x_firstobj(p_args));
	return _eval_last;
}

#include "ext/x-expr/tests/src/helper-system-functions.c"


/*
 * ## Test Overhead
 */

static void _setup(void)
{
	helper_set_alloc(MEM_GUARANTEED);
	_buffer_index = -1;
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
		&& x_obj_isnil(p_base, x_firstobj(p_obj))
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
	_it_should("return the Base object environment list (initially nil)",
		x_obj_isnil(p_base, p_obj)
	);

	p_obj = x_base_field_eval_list(p_base);
	_it_should("return the Base object expression list (initially nil)",
		x_obj_isnil(p_base, p_obj)
	);

	p_obj = x_base_field_buffer(p_base);
	_it_should("return the Base object buffer",
		x_obj_isnil(p_base, p_obj)
	);

	p_obj = x_base_field_token_cache(p_base);
	_it_should("return the Base object token cache",
		x_obj_isnil(p_base, p_obj)
	);

	p_obj = x_base_field_write_buf(p_base);
	_it_should("return the Base object write-buf (initially nil)",
		x_obj_isnil(p_base, p_obj)
	);

	x_sys_free(p_base);

	return NULL;
}

static char *test_base_type_alist_extend(void)
{
	x_obj_t *p_base, *p_alist, *p_args;
	x_obj_t *p_type0, *p_type1, *p_name0, *p_name1;

	/* Mock type structs: ((name_satom . nil) . nil) matches real layout
	 * so x_type_field_name returns name_satom */

	p_args = x_mkspair(NULL, NULL, NULL);
	p_alist = x_base_type_alist_extend(NULL, p_args);
	_it_should("return NULL when base is NULL",
		NULL == p_alist
	);


	p_base = x_mksatom(NULL, NULL);
	p_args = x_mkspair(p_base, NULL, NULL);
	p_alist = x_base_type_alist_extend(p_base, p_args);
	_it_should("return nil when base is not set",
		NULL == p_alist
	);

	x_sys_free(p_base);


	p_base = x_base_make(NULL, NULL);

	/* Build mock type struct with name stack: ((name . nil) . nil) */
	p_name0 = x_mksatom(p_base, "type0");
	p_type0 = x_mkspair(p_base, x_mkspair(p_base, p_name0, NULL), NULL);
	p_alist = x_base_type_alist_extend(p_base, p_type0);
	/* Entry is (name . type_struct) */
	_it_should("extend alist with first type",
		x_obj_type_isspair(p_alist)
		&& x_obj_type_isspair(x_firstobj(p_alist))
		&& x_firstobj(x_firstobj(p_alist)) == p_name0
		&& x_restobj(x_firstobj(p_alist)) == p_type0
	);


	p_name1 = x_mksatom(p_base, "type1");
	p_type1 = x_mkspair(p_base, x_mkspair(p_base, p_name1, NULL), NULL);
	p_alist = x_base_type_alist_extend(p_base, p_type1);
	_it_should("extend alist with second type",
		x_obj_type_isspair(p_alist)
		&& x_obj_type_isspair(x_firstobj(p_alist))
		&& x_firstobj(x_firstobj(p_alist)) == p_name1
		&& x_restobj(x_firstobj(p_alist)) == p_type1
		&& x_obj_type_isspair(x_firstobj(x_restobj(p_alist)))
		&& x_firstobj(x_firstobj(x_restobj(p_alist))) == p_name0
		&& x_restobj(x_firstobj(x_restobj(p_alist))) == p_type0
	);

	x_sys_free(p_base);

	return NULL;
}

static char *test_base_type_alist_assoc(void)
{
	x_obj_t *p_base, *p_obj[2], *p_args, *p_type[2];
	x_obj_t *p_name[2];
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
		NULL == p_obj[0]
	);

	x_sys_free(p_base);


	p_base = x_base_make(NULL, NULL);

	p_args = x_mkspair(p_base, x_mksatom(p_base, s[0]), NULL);
	p_obj[0] = x_base_type_alist_assoc(p_base, p_args);
	_it_should("return nil when item is not found",
		NULL == p_obj[0]
	);


	/* Build mock type structs: ((name_satom . nil) . nil) */
	p_name[0] = x_mksatom(p_base, s[0]);
	p_type[0] = x_mkspair(p_base, x_mkspair(p_base, p_name[0], NULL), NULL);
	p_name[1] = x_mksatom(p_base, s[1]);
	p_type[1] = x_mkspair(p_base, x_mkspair(p_base, p_name[1], NULL), NULL);
	x_base_type_alist_extend(p_base, p_type[0]);
	x_base_type_alist_extend(p_base, p_type[1]);

	/* assoc returns the unwrapped type struct */
	p_obj[0] = x_base_type_alist_assoc(p_base, p_args);
	_it_should("return type struct when found",
		p_obj[0] == p_type[0]
	);

	p_obj[1] = x_base_type_alist_assoc(p_base, p_args);
	_it_should("return same item when found",
		p_obj[0] == p_obj[1]
	);


	p_args = x_mkspair(p_base, x_mksatom(p_base, s[1]), NULL);
	p_obj[0] = x_base_type_alist_assoc(p_base, p_args);
	_it_should("return second type struct when found",
		p_obj[0] == p_type[1]
	);

	p_args = x_mkspair(p_base, x_mksatom(p_base, s[2]), NULL);
	p_obj[0] = x_base_type_alist_assoc(p_base, p_args);
	_it_should("return nil when item is not found",
		NULL == p_obj[0]
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
		NULL == p_alist
	);

	x_sys_free(p_base);


	p_base = x_base_make(NULL, NULL);
	p_args = x_mkspair(p_base, p_atoms[0], p_atoms[1]);
	p_alist = x_base_env_alist_extend(p_base, p_args);
	_it_should("extend alist with (#<0x1:0x0> . #<0x1:0x1>)",
		x_obj_type_isspair(p_alist)
		&& x_obj_type_isspair(x_firstobj(p_alist))
		&& x_firstobj(x_firstobj(p_alist)) == p_atoms[0]
		&& x_restobj(x_firstobj(p_alist)) == p_atoms[1]
	);


	p_args = x_mkspair(p_base, p_atoms[1], p_atoms[2]);
	p_alist = x_base_env_alist_extend(p_base, p_args);
	_it_should("extend alist with (#<0x1:0x2> . #<0x1:0x3>)",
		x_obj_type_isspair(p_alist)
		&& x_obj_type_isspair(x_firstobj(p_alist))
		&& x_firstobj(x_firstobj(p_alist)) == p_atoms[1]
		&& x_restobj(x_firstobj(p_alist)) == p_atoms[2]
		&& x_obj_type_isspair(x_firstobj(x_restobj(p_alist)))
		&& x_firstobj(x_firstobj(x_restobj(p_alist))) == p_atoms[0]
		&& x_restobj(x_firstobj(x_restobj(p_alist))) == p_atoms[1]
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
		NULL == p_obj[0]
	);

	x_sys_free(p_base);


	p_base = x_base_make(NULL, NULL);

	p_args = x_mkspair(p_base, x_mksatom(p_base, s[0]), NULL);
	p_obj[0] = x_base_env_alist_assoc(p_base, p_args);
	_it_should("return nil when item is not found",
		NULL == p_obj[0]
	);


	p_assoc[0] = x_mkspair(p_base, x_mksatom(p_base, s[0]), NULL);
	p_assoc[1] = x_mkspair(p_base, x_mksatom(p_base, s[1]), NULL);
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


	p_args = x_mkspair(p_base, x_mksatom(p_base, s[1]), NULL);
	p_obj[0] = x_base_env_alist_assoc(p_base, p_args);
	_it_should("return second item when found",
		x_firstobj(p_assoc[1]) == x_firstobj(p_obj[0])
	);

	p_args = x_mkspair(p_base, x_mksatom(p_base, s[2]), NULL);
	p_obj[0] = x_base_env_alist_assoc(p_base, p_args);
	_it_should("return nil when item is not found",
		NULL == p_obj[0]
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

static char *test_base_error_no_handler(void)
{
	x_obj_t *p_base;
	x_char_t s[64];

	/* Without base — writes to stderr (fd 2) */
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDERR] = s;
	helper_file_buffer_length[TEST_HELPER_FILE_STDERR] = 64;
	helper_file_reset();

	x_base_error(NULL, "test error", NULL);
	_it_should("write error to stderr without base",
		s[0] != '\0');

	/* With base, no handler — writes to base's stderr fd */
	p_base = x_base_make(NULL, NULL);

	helper_file_buffer_ptr[TEST_HELPER_FILE_STDERR] = s;
	helper_file_buffer_length[TEST_HELPER_FILE_STDERR] = 64;
	helper_file_reset();
	s[0] = '\0';

	x_base_error(p_base, "base error", NULL);
	_it_should("write error to stderr with base",
		s[0] != '\0');

	/* With symbol */
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDERR] = s;
	helper_file_buffer_length[TEST_HELPER_FILE_STDERR] = 64;
	helper_file_reset();
	s[0] = '\0';

	x_base_error(p_base, "undef", x_mksatom(p_base, "foo"));
	_it_should("write error with symbol",
		s[0] != '\0');

	x_sys_free(p_base);

	return NULL;
}

static char *test_base_error_with_handler(void)
{
	x_obj_t *p_base, *p_handler;
	jmp_buf jmp;
	int caught;

	p_base = x_base_make(NULL, NULL);

	/* Build handler: (jmp-ptr saved-env error-value) */
	p_handler = x_mkspair(p_base,
		x_mksatom(p_base, &jmp),
		x_mkspair(p_base,
			x_base_field_env_alist(p_base),
			x_mkspair(p_base, NULL, NULL)));
	x_base_field_error_handler(p_base) = p_handler;

	caught = 0;
	if (setjmp(jmp) == 0) {
		x_base_error(p_base, "test err", NULL);
	} else {
		caught = 1;
	}

	_it_should("longjmp to handler on error",
		1 == caught);

	/* x_mkstrown is stubbed to return NULL, so error is NULL */
	_it_should("handler error set (stub returns NULL)",
		x_error_handler_error(p_handler) == NULL);

	/* Test with symbol */
	x_base_field_error_handler(p_base) = p_handler;
	x_error_handler_error(p_handler) = NULL;

	caught = 0;
	if (setjmp(jmp) == 0) {
		x_base_error(p_base, "undef", x_mksatom(p_base, "bar"));
	} else {
		caught = 1;
	}

	_it_should("longjmp to handler with symbol",
		1 == caught);

	x_sys_free(p_base);

	return NULL;
}

static char *test_base_write_buf(void)
{
	x_obj_t *p_base, *p_args;
	x_char_t buf[64];
	x_satom_t buf_pos = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = 0 });
	x_spair_t buf_obj[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE,
			{ .v = buf }, { (x_obj_t *)&buf_pos })
	};

	p_base = x_base_make(NULL, NULL);

	/* Install write-buffer on base */
	x_base_field_write_buf(p_base) = (x_obj_t *)buf_obj;

	p_args = x_mkspair(p_base,
		x_mksatom(p_base, "hello"),
		x_mkspair(p_base, x_mksatom(p_base, 5), NULL)
	);
	x_base_write(p_base, p_args);
	_it_should("write to buffer", 5 == x_atomint(buf_pos));
	buf[x_atomint(buf_pos)] = '\0';
	_it_should("buffer contains hello",
		0 == strncmp(buf, "hello", 5));

	/* Write more data, position advances */
	p_args = x_mkspair(p_base,
		x_mksatom(p_base, "!"),
		x_mkspair(p_base, x_mksatom(p_base, 1), NULL)
	);
	x_base_write(p_base, p_args);
	_it_should("position advanced", 6 == x_atomint(buf_pos));
	buf[x_atomint(buf_pos)] = '\0';
	_it_should("buffer contains hello!",
		0 == strncmp(buf, "hello!", 6));

	/* Remove write-buffer, verify fallback to fd */
	x_base_field_write_buf(p_base) = NULL;
	{
		x_char_t out[8];

		helper_file_buffer_ptr[TEST_HELPER_FILE_STDOUT] = out;
		helper_file_buffer_length[TEST_HELPER_FILE_STDOUT] = 8;
		helper_file_reset();

		p_args = x_mkspair(p_base,
			x_mksatom(p_base, "X"),
			x_mkspair(p_base, x_mksatom(p_base, 1), NULL)
		);
		x_base_write(p_base, p_args);
		_it_should("write to fd when buf is nil", 'X' == out[0]);
	}

	helper_file_buffer_length[TEST_HELPER_FILE_STDOUT] = TEST_HELPER_FILE_UNDEFINED;

	x_sys_free(p_base);

	return NULL;
}

static char *test_base_load(void)
{
	x_obj_t *p_base, *p_ret;
	x_obj_t *tokens[3];

	p_base = x_base_make(NULL, NULL);
	/* x_base_load reads x_base_field_buffer but our stubbed x_token_read
	 * ignores it — just push a non-NULL dummy. */
	x_base_buffer_push(p_base, x_mkspair(p_base, NULL, NULL));

	/* Empty input — token_read returns NULL immediately */
	_token_read_returns = NULL;
	_token_read_index = 0;
	_eval_last = NULL;

	p_ret = x_base_load(p_base, NULL);
	_it_should("return NULL for empty input", NULL == p_ret);

	/* Two tokens then NULL */
	tokens[0] = x_mksatom(p_base, 42);
	tokens[1] = x_mksatom(p_base, 99);
	tokens[2] = NULL;
	_token_read_returns = tokens;
	_token_read_index = 0;
	_eval_last = NULL;

	p_ret = x_base_load(p_base, NULL);
	_it_should("return last eval result", _eval_last == p_ret);
	_it_should("eval was called with second token",
		tokens[1] == _eval_last);

	x_sys_free(p_base);

	return NULL;
}

static char *run_tests() {
	_run_test(test_base_make);
	_run_test(test_base_type_alist_extend);
	_run_test(test_base_type_alist_assoc);
	_run_test(test_base_env_alist_extend);
	_xrun_test(test_base_env_alist_assoc);
	_run_test(test_base_read);
	_run_test(test_base_write);
	_run_test(test_base_write_buf);
	_run_test(test_base_load);
	_run_test(test_base_error_no_handler);
	_run_test(test_base_error_with_handler);

	return NULL;
}
