/*
 * # Unit Tests: *the image cells and the per-type save and load handlers*
 *
 * docs/state-image-format.md (x-lang) 4.3: a type saves and loads its own
 * payload.  Every handler writes [n][kind word]*n into the buffer it is
 * handed, with evaluated arguments (obj buf).
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

/* We need the GC structures for cleanup. */
#ifndef X_GC
#define X_GC
#endif /* X_GC */
#include "ext/x-expr/tests/src/test-helper-system.c"

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-stdlib.c"
#include "ext/x-expr/src/x-lib.c"
#include "ext/x-expr/src/x.c"
#include "ext/x-expr/src/x-obj.c"
#include "src/x-alist.c"
#include "ext/x-expr/src/x-base.c"
#define STUB_X_EVAL
#include "src/x-eval.c"
#include "ext/x-expr/src/x-heap.c"
#include "src/x-type.c"
#include "src/x-type/prim.c"
#include "src/x-type/char.c"
#include "src/x-type/err.c"
#include "src/x-token/sexp/char.c"
#include "src/x-type/buffer.c"
#include "src/x-type/int.c"
#include "src/x-token/sexp/int.c"
#include "src/x-type/str.c"
#include "src/x-token/sexp/str.c"
#include "src/x-type/ptr.c"

#define STUB_X_PRIM
#define STUB_X_PROCEDURE
#define STUB_X_OPERATIVE
#define STUB_X_EVAL
#define STUB_X_TOKEN
#define STUB_X_HEAP
#define STUB_X_OBJ_OBJ
#define STUB_X_SYMBOL
#define STUB_X_PRIM_REGISTER
#define STUB_X_PRIM_SHADOW
#define STUB_X_PROCEDURE_APPLY
#include "helper-stubs.c"


/*
 * ## Test Overhead
 */

static void _setup(void)
{
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

/* The evaluated argument list a save handler receives: (obj buf). */
static x_obj_t *save_args(x_obj_t *p_base, x_obj_t *p_obj, x_int_t *buf)
{
	return x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mkptr(p_base, buf), NULL));
}

x_obj_t *test_prim_fn(x_obj_t *p_base, x_obj_t *p_args)
{
	return p_args;
}


/*
 * ## Test Runners
 */

static char *test_type_struct_image_cells(void)
{
	x_obj_t *p_base, *p_type;
	struct x_type_t type = {
		.p_save = (x_obj_t *)x_type_int_save_prim,
		.p_name = x_type_int_name
	};

	p_base = x_eval_make(NULL, NULL);
	p_type = x_type_struct_make(p_base, type);

	_it_should("hold the save handler the struct was made with",
		x_type_field_save(p_type) == (x_obj_t *)x_type_int_save_prim
	);
	_it_should("hold nil for a load handler that was not given",
		x_type_field_load(p_type) == NULL
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_type_str_save(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t buf[8];

	p_base = x_eval_make(NULL, NULL);
	p_obj = x_mkstr(p_base, (x_char_t *)"abc");

	x_type_str_save(p_base, save_args(p_base, p_obj, buf));
	_it_should("save a string as one bytes unit holding its text",
		buf[0] == 1
		&& buf[1] == X_TYPE_UNIT_BYTES
		&& buf[2] == (x_int_t)x_strval(p_obj)
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_type_int_save(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t buf[8];

	p_base = x_eval_make(NULL, NULL);
	p_obj = x_mkint(p_base, 42);

	x_type_int_save(p_base, save_args(p_base, p_obj, buf));
	_it_should("save an integer as one machine word",
		buf[0] == 1
		&& buf[1] == X_TYPE_UNIT_WORD
		&& buf[2] == 42
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_type_char_save(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t buf[8];

	p_base = x_eval_make(NULL, NULL);
	p_obj = x_mkchar(p_base, 'a');

	x_type_char_save(p_base, save_args(p_base, p_obj, buf));
	_it_should("save a character as one machine word",
		buf[0] == 1
		&& buf[1] == X_TYPE_UNIT_WORD
		&& buf[2] == (x_int_t)'a'
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_type_prim_save(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t buf[8];

	p_base = x_eval_make(NULL, NULL);
	p_obj = x_mkprim(p_base, test_prim_fn);

	x_type_prim_save(p_base, save_args(p_base, p_obj, buf));
	_it_should("save a primitive as two foreign units, the function first",
		buf[0] == 2
		&& buf[1] == X_TYPE_UNIT_FOREIGN
		&& buf[2] == (x_int_t)test_prim_fn
		&& buf[3] == X_TYPE_UNIT_FOREIGN
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_type_ptr_save(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t buf[8];

	p_base = x_eval_make(NULL, NULL);
	p_obj = x_mkptr(p_base, (void *)0x1234);

	x_type_ptr_save(p_base, save_args(p_base, p_obj, buf));
	_it_should("save a pointer as one foreign unit",
		buf[0] == 1
		&& buf[1] == X_TYPE_UNIT_FOREIGN
		&& buf[2] == 0x1234
	);

	test_cleanup(p_base);

	return NULL;
}

static char *test_type_buffer_save_load(void)
{
	x_obj_t *p_base, *p_buffer, *p_inner, *p_outer;
	x_char_t *copy;
	x_int_t outer[8], inner[8];

	p_base = x_eval_make(NULL, NULL);
	p_buffer = x_mkbuffer(p_base, (x_char_t *)"hello");
	p_inner = x_restobj(p_buffer);
	x_bufferread(p_buffer) = x_bufferval(p_buffer) + 2;
	x_bufferwrite(p_buffer) = x_bufferval(p_buffer) + 5;

	_it_should("mark the inner and not the outer with the inner flag",
		(x_obj_flags(p_inner) & X_TYPE_BUFFER_FLAG_INNER) != 0
		&& (x_obj_flags(p_buffer) & X_TYPE_BUFFER_FLAG_INNER) == 0
	);

	x_type_buffer_save(p_base, save_args(p_base, p_buffer, outer));
	_it_should("save the outer as its bytes, its inner and the consumed count",
		outer[0] == 3
		&& outer[1] == X_TYPE_UNIT_BYTES
		&& outer[2] == (x_int_t)x_bufferval(p_buffer)
		&& outer[3] == X_TYPE_UNIT_REF
		&& outer[4] == (x_int_t)p_inner
		&& outer[5] == X_TYPE_UNIT_WORD
		&& outer[6] == 2
	);

	x_type_buffer_save(p_base, save_args(p_base, p_inner, inner));
	_it_should("save the inner as two words, the unread count last",
		inner[0] == 2
		&& inner[1] == X_TYPE_UNIT_WORD
		&& inner[3] == X_TYPE_UNIT_WORD
		&& inner[4] == 3
	);

	/* What the loader hands the load: an outer of three units over a fresh
	 * copy of the bytes, an inner holding the saved words. */
	copy = x_lib_strndup((x_char_t *)"hello", 5);
	p_outer = x_obj_alloc(p_base, x_obj_type(p_buffer), X_OBJ_FLAG_NONE, 3);
	x_firststr(p_outer) = copy;
	x_restobj(p_outer) = p_inner;
	x_obj_data_i(p_outer, 2).i = outer[6];
	x_firstint(p_inner) = inner[2];
	x_restint(p_inner) = inner[4];

	x_type_buffer_load(p_base, x_mkspair(p_base, X_OBJ_FLAG_NONE, p_outer, NULL));
	_it_should("put the read and write pointers back into the new bytes, consumed prefix kept",
		x_bufferread(p_outer) == copy + 2
		&& x_bufferwrite(p_outer) == copy + 5
	);

	x_sys_free(copy);
	test_cleanup(p_base);

	return NULL;
}

static char *run_tests() {
	_run_test(test_type_struct_image_cells);
	_run_test(test_type_str_save);
	_run_test(test_type_int_save);
	_run_test(test_type_char_save);
	_run_test(test_type_prim_save);
	_run_test(test_type_ptr_save);
	_run_test(test_type_buffer_save_load);

	return NULL;
}

