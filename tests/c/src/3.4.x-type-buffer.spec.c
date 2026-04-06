/*
 * # Unit Tests: *x-type/buffer*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

/* We need the GC structures for cleanup. */
#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-lib.c"
#include "ext/x-expr/src/x.c"
#include "ext/x-expr/src/x-obj.c"
#include "src/x-alist.c"
#include "ext/x-expr/src/x-base.c"
#include "src/x-base.c"
#include "ext/x-expr/src/x-heap.c"
#include "src/x-type.c"
#include "src/x-type/prim.c"
#include "src/x-type/char.c"
#include "src/x-token/sexp/char.c"
#include "src/x-type/str.c"
#include "src/x-token/sexp/str.c"
#include "src/x-type/buffer.c"

#define STUB_X_PRIM
#define STUB_X_PROCEDURE
#define STUB_X_OPERATIVE
#define STUB_X_EVAL
#define STUB_X_TOKEN
#define STUB_X_HEAP
#define STUB_X_OBJ_OBJ
#define STUB_X_INT
#define STUB_X_SYMBOL
#define STUB_X_PRIM_REGISTER
#define STUB_X_PRIM_SHADOW
#define STUB_X_PROCEDURE_APPLY
#include "helper-stubs.c"

#include "ext/x-expr/tests/src/test-helper-system.c"

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

#define X_TEST_BUFFER_FLAG		0xff
#define X_TEST_BUFFER_VALUE		(void *)0xa5
#define X_TEST_BUFFER_STR		"ABCDEF"


#define nil			NULL
#define pair(X,Y)	(x_mkspair(p_base, X_OBJ_FLAG_NONE, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, X_OBJ_FLAG_NONE, (X)))

static char *test_obj_type_isbuffer(void)
{
	x_obj_t *p_obj;

	helper_alloc_reset();

	p_obj = x_mkbuffer(NULL, 0);
	_it_should("return true when object is a buffer",
		1 == x_obj_type_isbuffer(NULL, p_obj)
	);
	x_obj_free(NULL, p_obj);

	p_obj = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);
	_it_should("return false when object is not a buffer",
		0 == x_obj_type_isbuffer(NULL, p_obj)
	);
	x_obj_free(NULL, p_obj);

	return NULL;
}

static char *test_bufferval(void)
{
	x_obj_t *p_obj;
	x_char_t *p_buffer, *buffer = X_TEST_BUFFER_VALUE;

	p_obj = x_mkbuffer(NULL, buffer);

	p_buffer = x_bufferval(p_obj);
	_it_should("return the Buffer's value", buffer == p_buffer);

	x_sys_free(p_obj);

	return NULL;
}

static char *test_bufferread(void)
{
	x_obj_t *p_obj;
	x_char_t *p_ptr, *buffer = X_TEST_BUFFER_VALUE;

	p_obj = x_mkbuffer(NULL, buffer);

	p_ptr = x_bufferread(p_obj);
	_it_should("return the Buffer's read pointer", buffer == p_ptr);

	x_sys_free(p_obj);

	return NULL;
}

static char *test_bufferwrite(void)
{
	x_obj_t *p_obj;
	x_char_t *p_ptr, *buffer = X_TEST_BUFFER_VALUE;

	p_obj = x_mkbuffer(NULL, buffer);

	p_ptr = x_bufferwrite(p_obj);
	_it_should("return the Buffer's write pointer", buffer == p_ptr);

	x_sys_free(p_obj);

	return NULL;
}

static char *test_bufferlen(void)
{
	x_obj_t *p_obj;
	x_char_t *buffer = X_TEST_BUFFER_STR;

	p_obj = x_mkbuffer(NULL, buffer);

	_it_should("return the Buffer's length as 0", 0 == x_bufferlen(p_obj));

	x_bufferread(p_obj)++;
	_it_should("return the Buffer's length as 1", 1 == x_bufferlen(p_obj));

	x_sys_free(p_obj);

	return NULL;
}

static char *test_bufferunread(void)
{
	x_obj_t *p_obj;
	x_char_t *buffer = X_TEST_BUFFER_STR;

	p_obj = x_mkbuffer(NULL, buffer);

	_it_should("return the Buffer's length as 0", 0 == x_bufferunread(p_obj));

	x_bufferwrite(p_obj)++;
	_it_should("return the Buffer's length as 1", 1 == x_bufferunread(p_obj));

	x_sys_free(p_obj);

	return NULL;
}

static char *test_bufferlastchar(void)
{
	x_obj_t *p_obj;
	x_char_t *buffer = X_TEST_BUFFER_STR;

	p_obj = x_mkbuffer(NULL, buffer);

	x_bufferread(p_obj)++;
	_it_should("return the Buffer's last character",
		X_TEST_BUFFER_STR[0] == x_bufferlastchar(p_obj)
	);

	x_sys_free(p_obj);

	return NULL;
}

static char *test_mkbuffer(void)
{
	x_obj_t *p_base, *p_obj;

	p_obj = x_mkbuffer(NULL, X_TEST_BUFFER_VALUE);
	_it_should("make a Buffer object and set its values",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_isbuffer(NULL, p_obj)
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj)
		&& X_TEST_BUFFER_VALUE == x_bufferval(p_obj)
		&& X_TEST_BUFFER_VALUE == x_bufferread(p_obj)
		&& X_TEST_BUFFER_VALUE == x_bufferwrite(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);

	p_obj = x_mkbuffer(p_base, X_TEST_BUFFER_VALUE);
	_it_should("make a Buffer object, attach it to the Base object, and set its values",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_isbuffer(p_base, p_obj)
		&& X_OBJ_FLAG_NONE == x_obj_flags(p_obj)
		&& p_obj == x_obj_heap(p_base)
		&& X_TEST_BUFFER_VALUE == x_bufferval(p_obj)
		&& X_TEST_BUFFER_VALUE == x_bufferread(p_obj)
		&& X_TEST_BUFFER_VALUE == x_bufferwrite(p_obj)
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_mkfbuffer(void)
{
	x_obj_t *p_base, *p_obj;
	x_obj_flag_t flags = rand();

	p_obj = x_mkfbuffer(NULL, flags, X_TEST_BUFFER_VALUE);
	_it_should("make a Buffer object and set its values",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_isbuffer(NULL, p_obj)
		&& flags == (x_obj_flag_t)x_obj_flags(p_obj)
		&& X_TEST_BUFFER_VALUE == x_bufferval(p_obj)
		&& X_TEST_BUFFER_VALUE == x_bufferread(p_obj)
		&& X_TEST_BUFFER_VALUE == x_bufferwrite(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);

	p_obj = x_mkfbuffer(p_base, flags, X_TEST_BUFFER_VALUE);
	_it_should("make a Buffer object, attach it to the Base object, and set its values",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_isbuffer(p_base, p_obj)
		&& flags == (x_obj_flag_t)x_obj_flags(p_obj)
		&& p_obj == x_obj_heap(p_base)
		&& X_TEST_BUFFER_VALUE == x_bufferval(p_obj)
		&& X_TEST_BUFFER_VALUE == x_bufferread(p_obj)
		&& X_TEST_BUFFER_VALUE == x_bufferwrite(p_obj)
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_mkbufferown(void)
{
	x_obj_t *p_base, *p_obj;

	p_obj = x_mkbufferown(NULL, X_TEST_BUFFER_VALUE);
	_it_should("make an owned Buffer object and set its values",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_isbuffer(NULL, p_obj)
		&& X_OBJ_FLAG_OWN == x_obj_flags(p_obj)
		&& X_TEST_BUFFER_VALUE == x_bufferval(p_obj)
		&& X_TEST_BUFFER_VALUE == x_bufferread(p_obj)
		&& X_TEST_BUFFER_VALUE == x_bufferwrite(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);

	p_obj = x_mkbufferown(p_base, X_TEST_BUFFER_VALUE);
	_it_should("make a Buffer object, , attach it to the Base object, and set its values",
		! x_obj_isnil(p_base, p_obj)
		&& x_obj_type_isbuffer(p_base, p_obj)
		&& X_OBJ_FLAG_OWN == x_obj_flags(p_obj)
		&& p_obj == x_obj_heap(p_base)
		&& X_TEST_BUFFER_VALUE == x_bufferval(p_obj)
		&& X_TEST_BUFFER_VALUE == x_bufferread(p_obj)
		&& X_TEST_BUFFER_VALUE == x_bufferwrite(p_obj)
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_mkfbufferown(void)
{
	x_obj_t *p_base, *p_obj;
	x_obj_flag_t flags = rand();

	p_obj = x_mkfbufferown(NULL, flags, X_TEST_BUFFER_VALUE);
	_it_should("make an owned Buffer object",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_isbuffer(NULL, p_obj)
		&& (x_obj_flag_t)(X_OBJ_FLAG_OWN | flags) == (x_obj_flag_t)x_obj_flags(p_obj)
		&& X_TEST_BUFFER_VALUE == x_bufferval(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);

	p_obj = x_mkfbufferown(p_base, flags, X_TEST_BUFFER_VALUE);
	_it_should("make a Buffer object",
		! x_obj_isnil(p_base, p_obj)
		&& p_obj == x_obj_heap(p_base)
		&& x_obj_type_isbuffer(p_base, p_obj)
		&& (x_obj_flag_t)(X_OBJ_FLAG_OWN | flags) == (x_obj_flag_t)x_obj_flags(p_obj)
		&& X_TEST_BUFFER_VALUE == x_bufferval(p_obj)
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_make_buffer(void)
{
	x_obj_t *p_base, *p_obj;

	p_obj = x_make_buffer(NULL, X_TEST_BUFFER_FLAG, X_TEST_BUFFER_VALUE);
	_it_should("make a Buffer object with flags",
		! x_obj_isnil(NULL, p_obj)
		&& x_obj_type_isbuffer(NULL, p_obj)
		&& X_TEST_BUFFER_FLAG == x_obj_flags(p_obj)
		&& X_TEST_BUFFER_VALUE == x_bufferval(p_obj)
	);

	x_sys_free(p_obj);


	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);

	p_obj = x_make_buffer(p_base, X_TEST_BUFFER_FLAG, X_TEST_BUFFER_VALUE);
	_it_should("make a Buffer object with flags",
		! x_obj_isnil(p_base, p_obj)
		&& p_obj == x_obj_heap(p_base)
		&& x_obj_type_isbuffer(p_base, p_obj)
		&& X_TEST_BUFFER_FLAG == x_obj_flags(p_obj)
		&& X_TEST_BUFFER_VALUE == x_bufferval(p_obj)
	);

	x_sys_free(p_obj);
	x_sys_free(p_base);

	return NULL;
}

static char *test_type_buffer_struct(void)
{
	x_obj_t *p_base, *p_type;

	helper_alloc_reset();

	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, 0);
	p_type = x_type_buffer_struct(p_base, p_base);
	_it_should("return Buffer Type list",
		! x_obj_isnil(p_base, p_type)
		&& 0 == strcmp(X_TYPE_BUFFER_NAME, x_atomstr(x_type_field_name(p_type)))
	);

	_it_should("contain the Name object",
		x_type_buffer_name == x_type_field_name(p_type)
	);

	_it_should("set the Data object to nil",
		NULL == x_type_field_data(p_type)
	);

	_it_should("set the Make primitive",
		x_type_buffer_make == x_primval(x_type_field_make(p_type))
	);

	_it_should("not set the Free primitive",
		NULL == x_type_field_free(p_type)
	);

	_it_should("not set the Clone primitive",
		NULL == x_type_field_clone(p_type)
	);

	_it_should("not set the Units primitive",
		NULL == x_type_field_units(p_type)
	);

	_it_should("not set the Length primitive",
		NULL == x_type_field_length(p_type)
	);

	_it_should("not set the Call primitive",
		NULL == x_type_field_call(p_type)
	);

	_it_should("not set the Eval primitive",
		NULL == x_type_field_eval(p_type)
	);

	_it_should("not set the From alist",
		NULL == x_type_field_from(p_type)
	);

	_it_should("not set the To alist",
		NULL == x_type_field_to(p_type)
	);

	_it_should("not set the Analyse primitive",
		NULL == x_type_field_analyse(p_type)
	);

	_it_should("not set the Delimit primitive",
		NULL == x_type_field_delimit(p_type)
	);

	_it_should("set the Write primitive",
		x_type_buffer_write_prim == x_type_field_write(p_type)
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_type_buffer_register(void)
{
	x_obj_t *p_base, *p_type;

	p_base = x_base_ts_make(NULL, NULL);

	p_type = x_type_buffer_register(p_base, p_base);
	_it_should("return the Buffer type object",
		0 == x_lib_strcmp(X_TYPE_BUFFER_NAME, x_strval(x_type_field_name(p_type)))
	);
	_it_should("add the Buffer type to the Type alist",
		p_type == x_restobj(x_firstobj(x_base_field_type_alist(p_base)))
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_base_alist_assoc(void)
{
	x_obj_t *p_base, *p_type;

	helper_alloc_reset();

	p_base = x_base_ts_make(NULL, NULL);
	x_base_type_alist_extend(p_base, x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkspair(p_base, X_OBJ_FLAG_NONE, x_type_buffer_name, NULL), atom(1)));

	p_type = x_base_type_alist_assoc(p_base, x_mkspair(p_base, X_OBJ_FLAG_NONE, x_type_buffer_name, NULL));
	_it_should("find the type in the Type alist and return its properties",
		x_type_buffer_name == x_type_field_name(p_type)
	);

	return NULL;
}

static char *test_type_buffer_make(void)
{
	x_obj_t *p_base, *p_args, *p_buffer, *p_obj[2];
	x_char_t *value = X_TEST_BUFFER_VALUE;

	helper_alloc_reset();

	/* NULL p_base object */
	p_buffer = x_mksatom(NULL, X_OBJ_FLAG_NONE, value);
	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_buffer, NULL);
	p_obj[0] = x_type_buffer_make(NULL, p_args);
	_it_should("make a Buffer object",
		! x_obj_isnil(NULL, p_obj[0])
		&& x_obj_type_isbuffer(NULL, p_obj[0])
		&& value == x_bufferval(p_obj[0])
	);

	p_obj[1] = x_type_buffer_make(NULL, p_args);
	_it_should("make a second Buffer object",
		! x_obj_isnil(NULL, p_obj[1])
		&& x_obj_type_isbuffer(NULL, p_obj[1])
		&& value == x_bufferval(p_obj[1])
	);

	_it_should("have returned a different Type object for both objects",
		x_obj_type(p_obj[0]) != x_obj_type(p_obj[1])
	);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(p_args);
	x_sys_free(p_buffer);


	helper_alloc_reset();

	/* Empty p_base object */
	p_base = x_mksatom(NULL, X_OBJ_FLAG_NONE, NULL);
	p_buffer = x_mksatom(p_base, X_OBJ_FLAG_NONE, value);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_buffer, NULL);

	p_obj[0] = x_type_buffer_make(p_base, p_args);
	_it_should("make a Buffer object",
		! x_obj_isnil(NULL, p_obj[0])
		&& x_obj_type_isbuffer(p_base, p_obj[0])
		&& value == x_bufferval(p_obj[0])
	);

	p_obj[1] = x_type_buffer_make(p_base, p_args);
	_it_should("make a second buffer object",
		! x_obj_isnil(p_base, p_obj[1])
		&& x_obj_type_isbuffer(p_base, p_obj[1])
		&& value == x_bufferval(p_obj[1])
	);

	_it_should("have returned a different Type object for both objects",
		x_obj_type(p_obj[0]) != x_obj_type(p_obj[1])
	);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(p_args);
	x_sys_free(p_buffer);
	x_sys_free(p_base);


	helper_alloc_reset();

	/* With p_base object */
	p_base = x_base_ts_make(NULL, NULL);
	p_buffer = x_mksatom(p_base, X_OBJ_FLAG_NONE, value);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_buffer, NULL);

	p_obj[0] = x_type_buffer_make(p_base, p_args);
	_it_should("make a Buffer object with a base object",
		! x_obj_isnil(p_base, p_obj[0])
		&& x_obj_type_isbuffer(p_base, p_obj[0])
	);

	p_obj[1] = x_type_buffer_make(p_base, p_args);
	_it_should("make a second Buffer object a base object",
		! x_obj_isnil(p_base, p_obj[1])
		&& x_obj_type_isbuffer(p_base, p_obj[1])
	);
	_it_should("have returned the same type object for both objects",
		x_obj_type(p_obj[0]) == x_obj_type(p_obj[1])
	);

	x_sys_free(p_obj[1]);
	x_sys_free(p_obj[0]);
	x_sys_free(p_args);
	x_sys_free(p_buffer);
	x_sys_free(p_base);


	return NULL;
}

static char *test_type_buffer_reset(void)
{
	x_obj_t *p_obj, *p_ret, *p_args;

	p_obj = x_mkbuffer(NULL, X_TEST_BUFFER_VALUE);
	x_bufferread(p_obj) += 1;
	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_obj, NULL);
	p_ret = x_type_buffer_reset(NULL, p_args);

	_it_should("reset the Buffer object",
		p_obj == p_ret
		&& x_bufferval(p_obj) == x_bufferread(p_obj)
	);

	x_sys_free(p_args);
	x_sys_free(p_obj);

	return NULL;
}

static char *test_type_buffer_retain(void)
{
	x_obj_t *p_obj, *p_ret, *p_args;
	x_char_t s[sizeof(X_TEST_BUFFER_STR)] = X_TEST_BUFFER_STR;

	p_obj = x_mkbuffer(NULL, s);
	x_bufferread(p_obj) += 3;
	x_bufferwrite(p_obj) += 6;
	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_obj, NULL);
	p_ret = x_type_buffer_retain(NULL, p_args);

	_it_should("reset the Buffer object and retain the unread portion",
		p_obj == p_ret
		&& x_bufferread(p_obj) == x_bufferval(p_obj)
		&& x_bufferwrite(p_obj) == x_bufferval(p_obj) + 3
		&& 0 == strncmp(&X_TEST_BUFFER_STR[3], x_bufferval(p_obj), 3)
	);

	x_sys_free(p_args);
	x_sys_free(p_obj);

	return NULL;
}

static char *test_type_buffer_append(void)
{
	x_obj_t *p_obj, *p_ret, *p_char, *p_args;
	x_char_t buffer[2];

	p_obj = x_mkbuffer(NULL, buffer);
	p_char = x_mksatom(NULL, X_OBJ_FLAG_NONE, X_TEST_BUFFER_STR[0]);
	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_obj, x_mkspair(NULL, X_OBJ_FLAG_NONE, p_char, NULL));
	p_ret = x_type_buffer_append(NULL, p_args);

	_it_should("append a Character to the Buffer object",
		p_obj == p_ret
		&& x_bufferval(p_obj) + 1 == x_bufferwrite(p_obj)
		&& X_TEST_BUFFER_STR[0] == x_bufferval(p_obj)[0]
	);

	x_atomchar(p_char) = X_TEST_BUFFER_STR[1];
	p_ret = x_type_buffer_append(NULL, p_args);

	_it_should("append a second Character to the Buffer object",
		p_obj == p_ret
		&& x_bufferval(p_obj) + 2 == x_bufferwrite(p_obj)
		&& X_TEST_BUFFER_STR[1] == x_bufferval(p_obj)[1]
	);

	x_sys_free(p_char);
	x_sys_free(x_restobj(p_args));
	x_sys_free(p_args);
	x_sys_free(p_obj);

	return NULL;
}

#define X_TEST_BUFFER_STR_EMPTY "@@"

static char *test_type_buffer_read(void)
{
	/* TODO: check for file errors */
	x_obj_t *p_args, *p_buffer, *p_ret;
	x_char_t *s, buffer[2] = X_TEST_BUFFER_STR_EMPTY;

	s = X_TEST_BUFFER_STR;
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_buffer = x_mkbuffer(NULL, buffer);
	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_buffer, NULL);
	p_ret = x_type_buffer_read(NULL, p_args);
	_it_should("append read Character to the Buffer object",
		p_buffer == p_ret
		&& X_TEST_BUFFER_STR[0] == buffer[0]
		&& X_TEST_BUFFER_STR[0] == x_bufferread(p_buffer)[-1]
	);

	/* Shift read pointer back one char, don't read anything. */
	x_bufferread(p_buffer) -= 1;
	p_ret = x_type_buffer_read(NULL, p_args);
	_it_should("not append a second read Character to the Buffer object",
		p_buffer == p_ret
		&& X_TEST_BUFFER_STR_EMPTY[1] == buffer[1]
	);

	/* Advance the read pointer, should read a new character. */
	p_ret = x_type_buffer_read(NULL, p_args);
	_it_should("append a second read Character to the Buffer object",
		p_buffer == p_ret
		&& X_TEST_BUFFER_STR[1] == buffer[1]
	);

	x_sys_free(p_args);
	x_sys_free(p_args);

	return NULL;
}

static char *test_type_buffer_read_text(void)
{
	x_obj_t *p_args, *p_buffer, *p_ret;
	x_char_t *s, buffer[2];

	s = X_TEST_BUFFER_STR;
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_buffer = x_mkbuffer(NULL, buffer);
	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_buffer, NULL);
	p_ret = x_type_buffer_read(NULL, p_args);
	_it_should("append read Character to the Buffer object",
		p_buffer == p_ret
		&& X_TEST_BUFFER_STR[0] == buffer[0]
	);

	s = "";
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;
	helper_file_reset();

	p_ret = x_type_buffer_read_text(NULL, p_args);
	_it_should("return the Base", NULL == p_ret);

	x_sys_free(p_args);
	x_sys_free(p_args);

	return NULL;
}

static char *test_type_buffer_mark(void)
{
	x_obj_t *p_base, *p_buffer, *p_args, *p_ret;
	x_char_t buffer[8];

	helper_alloc_reset();

	p_base = x_base_ts_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_buffer,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 0), NULL));

	p_ret = x_type_buffer_mark(p_base, p_args);
	_it_should("return NULL after marking", NULL == p_ret);

	return NULL;
}

static char *test_type_buffer_write(void)
{
	x_obj_t *p_base, *p_buffer, *p_args, *p_ret;
	char buf[64];
	x_char_t buffer[8];

	helper_alloc_reset();
	memset(buf, 0, sizeof(buf));

	p_base = x_base_ts_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, buffer);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_buffer, NULL);

	helper_file_buffer_ptr[STDOUT_FILENO] = buf;
	file_buffer_ptr[STDOUT_FILENO][TEST_HELPER_FILE_WRITE] = buf;

	p_ret = x_type_buffer_write(p_base, p_args);

	file_buffer_ptr[STDOUT_FILENO][TEST_HELPER_FILE_WRITE] = NULL;
	helper_file_buffer_ptr[STDOUT_FILENO] = NULL;

	_it_should("return the buffer object", p_buffer == p_ret);
	_it_should("write the buffer representation",
		0 == x_lib_strncmp(buf, X_TYPE_BUFFER_WRITE_STR,
			X_TYPE_BUFFER_WRITE_LEN));

	return NULL;
}

static char *test_type_buffer_read_readonly(void)
{
	x_obj_t *p_args, *p_buffer, *p_ret;
	x_char_t buffer[2] = "";

	helper_alloc_reset();

	p_buffer = x_mkfbuffer(NULL, X_OBJ_FLAG_RO, buffer);
	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE, p_buffer, NULL);

	p_ret = x_type_buffer_read(NULL, p_args);
	_it_should("return NULL for readonly buffer with no data",
		NULL == p_ret);

	x_sys_free(p_args);
	x_sys_free(p_buffer);

	return NULL;
}

static char *run_tests() {
	_run_test(test_obj_type_isbuffer);
	_run_test(test_bufferval);
	_run_test(test_bufferread);
	_run_test(test_bufferwrite);
	_run_test(test_bufferlen);
	_run_test(test_bufferunread);
	_run_test(test_bufferlastchar);
	_run_test(test_mkbuffer);
	_run_test(test_mkfbuffer);
	_run_test(test_mkbufferown);
	_run_test(test_mkfbufferown);
	_run_test(test_make_buffer);
	_run_test(test_type_buffer_struct);
	_run_test(test_type_buffer_register);
	_run_test(test_base_alist_assoc);
	_run_test(test_type_buffer_make);
	_run_test(test_type_buffer_reset);
	_run_test(test_type_buffer_retain);
	_run_test(test_type_buffer_append);
	_run_test(test_type_buffer_read);
	_run_test(test_type_buffer_read_text);
	_run_test(test_type_buffer_mark);
	_run_test(test_type_buffer_write);
	_run_test(test_type_buffer_read_readonly);

	return NULL;
}
