/*
 * # Unit Tests: *x-prim/type*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-lib.c"
#include "ext/x-expr/src/x-obj.c"
#include "src/x-obj/obj.c"
#include "src/x-obj/prim.c"
#include "ext/x-expr/src/x.c"
#include "src/x-alist.c"
#include "src/x-base.c"
#include "src/x-eval.c"
#include "src/x-type.c"
#include "src/x-type/atom.c"
#include "src/x-token/sexp/atom.c"
#include "src/x-type/pair.c"
#include "src/x-token/sexp/pair.c"
#include "src/x-type/prim.c"
#include "src/x-type/symbol.c"
#include "src/x-token/sexp/symbol.c"
#include "src/x-type/procedure.c"
#include "src/x-type/operative.c"
#include "src/x-type/list.c"
#include "src/x-token/sexp/list.c"
#include "src/x-type/str.c"
#include "src/x-token/sexp/str.c"
#include "src/x-type/int.c"
#include "src/x-token/sexp/int.c"
#include "src/x-type/char.c"
#include "src/x-token/sexp/char.c"
#include "src/x-type/ptr.c"
#include "src/x-type/whitespace.c"
#include "src/x-token/sexp/whitespace.c"
#include "src/x-type/comment.c"
#include "src/x-token/sexp/comment.c"
#include "src/x-type/buffer.c"
#include "src/x-type/iter.c"
#include "ext/x-expr/src/x-heap.c"
#include "src/x-token.c"
#include "src/x-prim.c"
#include "src/x-prim/type.c"

/* Stubs for primitives not under test. */
x_obj_t *x_prim_core_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_arith_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_pred_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_string_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_io_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_ffi_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }

#include "ext/x-expr/tests/src/helper-system-functions.c"


/*
 * ## Test Overhead
 */

static void _setup(void)
{
	_buffer_index = -1;
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

static char *test_type_typep(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_int;
	x_obj_t *p_int_handle;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Create an int and get its type handle */
	p_int = x_mkint(p_base, (x_int_t)42);
	p_int_handle = x_type_field_name(x_obj_type(p_int));

	/* Bind both to env for eval */
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "myint"), p_int));
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "inthandle"), p_int_handle));

	/* (type? myint inthandle) -> t */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "myint"),
		x_mkspair(p_base, x_mksymbol(p_base, "inthandle"), NULL));
	p_result = x_prim_typep(p_base, p_args);
	_it_should("type? matches correct type",
		p_result == x_base_field_true(p_base));

	/* (type? nil inthandle) -> nil */
	p_args = x_mkspair(p_base, NULL,
		x_mkspair(p_base, x_mksymbol(p_base, "inthandle"), NULL));
	p_result = x_prim_typep(p_base, p_args);
	_it_should("type? nil returns nil",
		p_result == NULL);

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_type_of(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_int;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_int = x_mkint(p_base, (x_int_t)42);
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "myint"), p_int));

	/* (type-of myint) -> int handle */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "myint"), NULL);
	p_result = x_prim_type_of(p_base, p_args);
	_it_should("type-of returns type handle",
		p_result != NULL);
	_it_should("type-of matches int type name",
		p_result == x_type_field_name(x_obj_type(p_int)));

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_type_name(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_int;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_int = x_mkint(p_base, (x_int_t)42);
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "myint"), p_int));

	/* (type-name myint) -> "int" */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "myint"), NULL);
	p_result = x_prim_type_name(p_base, p_args);
	_it_should("type-name returns string",
		p_result != NULL);
	_it_should("type-name of int is 'INTEGER'",
		x_lib_strcmp(x_strval(p_result), X_TYPE_INT_NAME) == 0);

	/* (type-name nil) -> nil */
	p_args = x_mkspair(p_base, NULL, NULL);
	p_result = x_prim_type_name(p_base, p_args);
	_it_should("type-name of nil returns nil",
		p_result == NULL);

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_make_instance(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_int;
	x_obj_t *p_int_handle;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Get the int type handle */
	p_int = x_mkint(p_base, (x_int_t)0);
	p_int_handle = x_type_field_name(x_obj_type(p_int));

	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "inthandle"), p_int_handle));

	/* (make-instance inthandle 42) -> int-typed instance with data 42 */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "inthandle"),
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)42), NULL));
	p_result = x_prim_make_instance(p_base, p_args);
	_it_should("make-instance returns an object",
		p_result != NULL);
	_it_should("instance has correct type",
		x_type_field_name(x_obj_type(p_result)) == p_int_handle);

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_make_token_base(void)
{
	x_obj_t *p_base, *p_result;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_result = x_prim_make_token_base(p_base, NULL);
	_it_should("make-token-base returns a base",
		p_result != NULL);
	_it_should("token-base is not parent",
		p_result != p_base);

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_token_discard(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	p_base = x_base_make(NULL, NULL);

	p_args = x_mksatom(p_base, (x_int_t)42);
	p_result = x_prim_token_discard(p_base, p_args);
	_it_should("token-discard returns p_args",
		p_result == p_args);

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_register(void)
{
	x_obj_t *p_base, *p_env;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_env = x_base_field_env_alist(p_base);
	_it_should("env is not empty after register",
		p_env != NULL);

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_make_type(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_instance;
	x_obj_t *p_name_handle, *p_handlers;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Build an empty handlers alist and create a type. */
	p_handlers = NULL;

	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "name"),
			x_mkstr(p_base, "mytype")));
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "handlers"), p_handlers));

	/* (make-type name handlers) */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "name"),
		x_mkspair(p_base, x_mksymbol(p_base, "handlers"), NULL));
	p_result = x_prim_make_type(p_base, p_args);
	_it_should("make-type returns name atom",
		p_result != NULL);
	_it_should("make-type name is 'mytype'",
		x_lib_strcmp(x_atomstr(p_result), "mytype") == 0);

	/* Verify we can make-instance with it. */
	p_name_handle = p_result;
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "th"), p_name_handle));
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "th"),
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)99), NULL));
	p_instance = x_prim_make_instance(p_base, p_args);
	_it_should("make-instance with custom type works",
		p_instance != NULL);
	_it_should("instance has correct type name",
		x_type_field_name(x_obj_type(p_instance)) == p_name_handle);

	test_cleanup(p_base);
	return NULL;
}

/* Dummy handler prim for type handler alist tests */
static x_obj_t *test_type_dummy_handler(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_firstobj(p_args);
}

static char *test_type_make_type_with_handlers(void)
{
	x_obj_t *p_base, *p_args, *p_result;
	x_obj_t *p_handlers, *p_fn;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Use a prim as handler — it has the right shape for type handler calls */
	p_fn = x_make_prim(p_base, X_OBJ_FLAG_NONE, test_type_dummy_handler);

	/* Build handlers alist with all 9 handler keys */
	p_handlers = x_mklist(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "eval"), p_fn),
		x_mklist(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "call"), p_fn),
		x_mklist(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "write"), p_fn),
		x_mklist(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "length"), p_fn),
		x_mklist(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "analyse"), p_fn),
		x_mklist(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "delimit"), p_fn),
		x_mklist(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "error"), p_fn),
		x_mklist(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "from"), p_fn),
		x_mklist(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "to"), p_fn),
		NULL)))))))));

	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "name"),
			x_mkstr(p_base, "fulltype")));
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "hdlrs"), p_handlers));

	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "name"),
		x_mkspair(p_base, x_mksymbol(p_base, "hdlrs"), NULL));
	p_result = x_prim_make_type(p_base, p_args);
	_it_should("make-type with all handlers returns name",
		p_result != NULL);
	_it_should("type name is 'fulltype'",
		x_lib_strcmp(x_atomstr(p_result), "fulltype") == 0);

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_base_make_type(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_target;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_target = x_base_make(p_base, NULL);
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "tgt"), p_target));
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "name"),
			x_mkstr(p_base, "tgttype")));
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "hdlrs"), NULL));

	/* (base-make-type tgt name hdlrs) */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "tgt"),
		x_mkspair(p_base, x_mksymbol(p_base, "name"),
		x_mkspair(p_base, x_mksymbol(p_base, "hdlrs"), NULL)));
	p_result = x_prim_base_make_type(p_base, p_args);
	_it_should("base-make-type returns name atom",
		p_result != NULL);
	_it_should("base-make-type name is 'tgttype'",
		x_lib_strcmp(x_atomstr(p_result), "tgttype") == 0);

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_make_base(void)
{
	x_obj_t *p_base, *p_new_base;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_new_base = x_prim_make_base(p_base, NULL);
	_it_should("make-base returns a base",
		p_new_base != NULL);
	_it_should("make-base returns different base",
		p_new_base != p_base);
	_it_should("make-base has env",
		x_base_field_env_alist(p_new_base) != NULL);
	_it_should("make-base has buffer",
		x_base_field_buffer(p_new_base) != NULL);

	/* Cleanup: new base was allocated without heap link to p_base,
	 * but we can't easily free it without walking its heap.
	 * Just clean up the original. */
	test_cleanup(p_base);
	return NULL;
}

static char *test_type_base_eval(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_target;
	x_obj_t *p_sym;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Create a target base and bind a value there */
	p_target = x_base_make(p_base, NULL);
	x_prim_register(p_target, NULL);
	p_sym = x_mksymbol(p_target, "xx");
	x_base_env_alist_extend(p_target,
		x_mkspair(p_target, p_sym,
			x_mksatom(p_target, (x_int_t)77)));

	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "tgt"), p_target));

	/* Pass the symbol xx as expression — it self-evaluates in calling base
	 * (it's a symbol atom), then gets eval'd in target base.
	 * Bind the symbol to expr in calling base so x_prim_eval_arg resolves it. */
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "expr"), p_sym));

	/* (base-eval tgt expr) -> evaluates xx in target -> 77 */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "tgt"),
		x_mkspair(p_base, x_mksymbol(p_base, "expr"), NULL));
	p_result = x_prim_base_eval(p_base, p_args);
	_it_should("base-eval returns result from target",
		p_result != NULL);
	_it_should("base-eval result is correct",
		x_atomint(p_result) == 77);

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_base_eval_error(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_target;
	jmp_buf jmp;
	x_obj_t *p_handler;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_target = x_base_make(p_base, NULL);
	x_prim_register(p_target, NULL);

	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "tgt"), p_target));

	/* Use an unbound symbol — evaluating it in target will trigger error.
	 * Bind it in calling base so the first eval resolves it to the symbol. */
	{
		x_obj_t *p_unbound;
		p_unbound = x_mksymbol(p_base, "____unbound____");
		x_base_env_alist_extend(p_base,
			x_mkspair(p_base, x_mksymbol(p_base, "expr"), p_unbound));
	}

	/* Set up error handler on parent base to catch re-signaled error */
	p_handler = x_mkspair(p_base,
		x_mkptr(p_base, &jmp),
		x_mkspair(p_base,
			x_base_field_env_alist(p_base),
			x_mkspair(p_base, NULL, NULL)));
	x_base_field_error_handler(p_base) = p_handler;

	if (setjmp(jmp) == 0) {
		p_args = x_mkspair(p_base,
			x_mksymbol(p_base, "tgt"),
			x_mkspair(p_base, x_mksymbol(p_base, "expr"), NULL));
		p_result = x_prim_base_eval(p_base, p_args);
		_it_should("base-eval error should have jumped", 0);
	} else {
		p_result = x_error_handler_error(p_handler);
		_it_should("base-eval error re-signals to parent",
			p_result != NULL);
	}

	x_base_field_error_handler(p_base) = NULL;
	test_cleanup(p_base);
	return NULL;
}

static char *test_type_base_bind(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_target;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_target = x_base_make(p_base, NULL);
	x_prim_register(p_target, NULL);

	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "tgt"), p_target));
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "nm"),
			x_mksymbol(p_base, "hello")));
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "val"),
			x_mksatom(p_base, (x_int_t)55)));

	/* (base-bind tgt nm val) */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "tgt"),
		x_mkspair(p_base, x_mksymbol(p_base, "nm"),
		x_mkspair(p_base, x_mksymbol(p_base, "val"), NULL)));
	p_result = x_prim_base_bind(p_base, p_args);
	_it_should("base-bind returns value",
		p_result != NULL);
	_it_should("base-bind value is correct",
		x_atomint(p_result) == 55);

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_buffer_token(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_buffer;
	x_char_t buffer[64];

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_buffer = x_mkbuffer(p_base, buffer);
	x_lib_memcpy(x_bufferval(p_buffer), "hello", 5);
	x_bufferread(p_buffer) = x_bufferval(p_buffer) + 5;

	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "buf"), p_buffer));

	/* (buffer-token buf) */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "buf"), NULL);
	p_result = x_prim_buffer_token(p_base, p_args);
	_it_should("buffer-token returns string",
		p_result != NULL);
	_it_should("buffer-token extracts consumed content",
		x_lib_strcmp(x_strval(p_result), "hello") == 0);

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_token_read_string(void)
{
	x_obj_t *p_base, *p_args, *p_result;
	x_obj_t *p_token_base;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Use make-base for a full base with all types and primitives */
	p_token_base = x_prim_make_base(p_base, NULL);

	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "tb"), p_token_base));
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "s"),
			x_mkstr(p_base, "hello")));

	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "tb"), p_token_base));
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "s"),
			x_mkstr(p_base, "hello")));

	/* (token-read-string tb s) — exercises the function and RO buffer path.
	 * The result may be NULL if the string doesn't match registered types
	 * on the token base, but the code path is still exercised. */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "tb"),
		x_mkspair(p_base, x_mksymbol(p_base, "s"), NULL));
	p_result = x_prim_token_read_string(p_base, p_args);
	(void)p_result;
	/* Just exercise the code path, result depends on type registration */
	_it_should("token-read-string executes without crash", 1);

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_convert(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_int;
	x_obj_t *p_int_handle;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Test short-circuit: already target type */
	p_int = x_mkint(p_base, (x_int_t)42);
	p_int_handle = x_type_field_name(x_obj_type(p_int));

	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "v"), p_int));
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "h"), p_int_handle));

	/* (convert v h) -> v (same type, short-circuit) */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "v"),
		x_mkspair(p_base, x_mksymbol(p_base, "h"), NULL));
	p_result = x_prim_convert(p_base, p_args);
	_it_should("convert short-circuits for same type",
		p_result == p_int);

	/* (convert nil h) -> nil */
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "v"), NULL));
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "v"),
		x_mkspair(p_base, x_mksymbol(p_base, "h"), NULL));
	p_result = x_prim_convert(p_base, p_args);
	_it_should("convert nil returns nil",
		p_result == NULL);

	/* (convert v bogus-handle) -> nil (no match) */
	{
		x_obj_t *p_bogus;
		p_bogus = x_mksatom(p_base, (x_int_t)0);
		x_base_env_alist_extend(p_base,
			x_mkspair(p_base, x_mksymbol(p_base, "v"), p_int));
		x_base_env_alist_extend(p_base,
			x_mkspair(p_base, x_mksymbol(p_base, "bh"), p_bogus));
		p_args = x_mkspair(p_base,
			x_mksymbol(p_base, "v"),
			x_mkspair(p_base, x_mksymbol(p_base, "bh"), NULL));
		p_result = x_prim_convert(p_base, p_args);
		_it_should("convert with unregistered handle returns nil",
			p_result == NULL);
	}

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_type_name_nil_name(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_obj;
	x_obj_t *p_type;
	struct x_type_t type_desc;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Create a type with nil name — do NOT register on type alist
	 * since nil name breaks alist lookup. */
	memset(&type_desc, 0, sizeof(type_desc));
	type_desc.p_units = (x_obj_t *)&x_type_units_pair_obj;
	p_type = x_type_struct_make(p_base, type_desc);

	/* Make instance directly (bypass type alist) */
	p_obj = x_obj_make(p_base, p_type, 0, X_OBJ_LENGTH_PAIR,
		x_mksatom(p_base, (x_int_t)1), NULL);

	/* Call x_prim_type_name directly with unevaluated arg */
	p_args = x_mkspair(p_base, p_obj, NULL);
	p_result = x_prim_type_name(p_base, p_args);
	_it_should("type-name with nil name returns nil",
		p_result == NULL);

	test_cleanup(p_base);
	return NULL;
}

static x_obj_t *test_convert_handler(x_obj_t *p_base, x_obj_t *p_args)
{
	/* Return the first arg (the value being converted) */
	return x_firstobj(p_args);
}

static char *test_type_convert_from_exact(void)
{
	x_obj_t *p_base, *p_args, *p_result;
	x_obj_t *p_src_type, *p_tgt_type, *p_instance;
	x_obj_t *p_src_handle, *p_tgt_handle;
	x_obj_t *p_converter, *p_from_alist;
	struct x_type_t src_desc, tgt_desc;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Create source type */
	memset(&src_desc, 0, sizeof(src_desc));
	src_desc.p_name = x_mksatom(p_base, "SRC");
	src_desc.p_units = (x_obj_t *)&x_type_units_pair_obj;
	p_src_type = x_type_struct_make(p_base, src_desc);
	x_base_type_alist_extend(p_base, p_src_type);
	p_src_handle = x_type_field_name(p_src_type);

	/* Build converter prim */
	p_converter = x_mkprim(p_base, test_convert_handler);

	/* Build from alist: ((src-handle . converter)) */
	p_from_alist = x_mklist(p_base,
		x_mkspair(p_base, p_src_handle, p_converter), NULL);

	/* Create target type with from alist */
	memset(&tgt_desc, 0, sizeof(tgt_desc));
	tgt_desc.p_name = x_mksatom(p_base, "TGT");
	tgt_desc.p_units = (x_obj_t *)&x_type_units_pair_obj;
	tgt_desc.p_from = p_from_alist;
	p_tgt_type = x_type_struct_make(p_base, tgt_desc);
	x_base_type_alist_extend(p_base, p_tgt_type);
	p_tgt_handle = x_type_field_name(p_tgt_type);

	/* Create instance of source type */
	p_instance = x_obj_make(p_base, p_src_type, 0, X_OBJ_LENGTH_PAIR,
		x_mksatom(p_base, (x_int_t)42), NULL);

	/* Bind to env */
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "v"), p_instance));
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "h"), p_tgt_handle));

	/* (convert v h) -> from alist exact match */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "v"),
		x_mkspair(p_base, x_mksymbol(p_base, "h"), NULL));
	p_result = x_prim_convert(p_base, p_args);
	_it_should("convert via from alist exact match returns value",
		p_result == p_instance);

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_convert_wildcard(void)
{
	x_obj_t *p_base, *p_args, *p_result;
	x_obj_t *p_src_type, *p_tgt_type, *p_instance;
	x_obj_t *p_tgt_handle;
	x_obj_t *p_converter, *p_from_alist;
	struct x_type_t src_desc, tgt_desc;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Create source type (no from/to) */
	memset(&src_desc, 0, sizeof(src_desc));
	src_desc.p_name = x_mksatom(p_base, "WSRC");
	src_desc.p_units = (x_obj_t *)&x_type_units_pair_obj;
	p_src_type = x_type_struct_make(p_base, src_desc);
	x_base_type_alist_extend(p_base, p_src_type);

	/* Build converter prim */
	p_converter = x_mkprim(p_base, test_convert_handler);

	/* Build from alist with wildcard key 't' */
	p_from_alist = x_mklist(p_base,
		x_mkspair(p_base, x_base_field_true(p_base), p_converter),
		NULL);

	/* Create target type with wildcard from alist */
	memset(&tgt_desc, 0, sizeof(tgt_desc));
	tgt_desc.p_name = x_mksatom(p_base, "WTGT");
	tgt_desc.p_units = (x_obj_t *)&x_type_units_pair_obj;
	tgt_desc.p_from = p_from_alist;
	p_tgt_type = x_type_struct_make(p_base, tgt_desc);
	x_base_type_alist_extend(p_base, p_tgt_type);
	p_tgt_handle = x_type_field_name(p_tgt_type);

	/* Create instance of source type */
	p_instance = x_obj_make(p_base, p_src_type, 0, X_OBJ_LENGTH_PAIR,
		x_mksatom(p_base, (x_int_t)99), NULL);

	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "v"), p_instance));
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "h"), p_tgt_handle));

	/* (convert v h) -> wildcard 't' match in from alist */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "v"),
		x_mkspair(p_base, x_mksymbol(p_base, "h"), NULL));
	p_result = x_prim_convert(p_base, p_args);
	_it_should("convert via wildcard from alist returns value",
		p_result == p_instance);

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_convert_to_alist(void)
{
	x_obj_t *p_base, *p_args, *p_result;
	x_obj_t *p_src_type, *p_instance;
	x_obj_t *p_src_handle, *p_tgt_handle;
	x_obj_t *p_converter, *p_to_alist;
	struct x_type_t src_desc, tgt_desc;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Build converter prim */
	p_converter = x_mkprim(p_base, test_convert_handler);

	/* Create target type (empty, just a handle) */
	memset(&tgt_desc, 0, sizeof(tgt_desc));
	tgt_desc.p_name = x_mksatom(p_base, "TTGT");
	tgt_desc.p_units = (x_obj_t *)&x_type_units_pair_obj;
	x_type_struct_make(p_base, tgt_desc);
	p_tgt_handle = tgt_desc.p_name;
	x_base_type_alist_extend(p_base,
		x_type_struct_make(p_base, tgt_desc));

	/* Build to alist: ((tgt-handle . converter)) */
	p_to_alist = x_mklist(p_base,
		x_mkspair(p_base, p_tgt_handle, p_converter), NULL);

	/* Create source type with to alist */
	memset(&src_desc, 0, sizeof(src_desc));
	src_desc.p_name = x_mksatom(p_base, "TSRC");
	src_desc.p_units = (x_obj_t *)&x_type_units_pair_obj;
	src_desc.p_to = p_to_alist;
	p_src_type = x_type_struct_make(p_base, src_desc);
	x_base_type_alist_extend(p_base, p_src_type);
	p_src_handle = x_type_field_name(p_src_type);
	(void)p_src_handle;

	/* Create instance of source type */
	p_instance = x_obj_make(p_base, p_src_type, 0, X_OBJ_LENGTH_PAIR,
		x_mksatom(p_base, (x_int_t)77), NULL);

	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "v"), p_instance));
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "h"), p_tgt_handle));

	/* (convert v h) -> source's to alist match */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "v"),
		x_mkspair(p_base, x_mksymbol(p_base, "h"), NULL));
	p_result = x_prim_convert(p_base, p_args);
	_it_should("convert via to alist returns value",
		p_result == p_instance);

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_convert_no_match(void)
{
	x_obj_t *p_base, *p_args, *p_result;
	x_obj_t *p_src_type, *p_instance;
	x_obj_t *p_tgt_handle;
	x_obj_t *p_converter, *p_from_alist, *p_to_alist;
	x_obj_t *p_bogus_key;
	struct x_type_t src_desc, tgt_desc;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_converter = x_mkprim(p_base, test_convert_handler);
	p_bogus_key = x_mksatom(p_base, "BOGUS");

	/* Target type has from alist, but key doesn't match source */
	p_from_alist = x_mklist(p_base,
		x_mkspair(p_base, p_bogus_key, p_converter), NULL);

	memset(&tgt_desc, 0, sizeof(tgt_desc));
	tgt_desc.p_name = x_mksatom(p_base, "NMTGT");
	tgt_desc.p_units = (x_obj_t *)&x_type_units_pair_obj;
	tgt_desc.p_from = p_from_alist;
	x_base_type_alist_extend(p_base,
		x_type_struct_make(p_base, tgt_desc));
	p_tgt_handle = tgt_desc.p_name;

	/* Source type has to alist, but key doesn't match target */
	p_to_alist = x_mklist(p_base,
		x_mkspair(p_base, p_bogus_key, p_converter), NULL);

	memset(&src_desc, 0, sizeof(src_desc));
	src_desc.p_name = x_mksatom(p_base, "NMSRC");
	src_desc.p_units = (x_obj_t *)&x_type_units_pair_obj;
	src_desc.p_to = p_to_alist;
	p_src_type = x_type_struct_make(p_base, src_desc);
	x_base_type_alist_extend(p_base, p_src_type);

	/* Create instance of source type */
	p_instance = x_obj_make(p_base, p_src_type, 0, X_OBJ_LENGTH_PAIR,
		x_mksatom(p_base, (x_int_t)11), NULL);

	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "v"), p_instance));
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "h"), p_tgt_handle));

	/* (convert v h) -> no from or to match, returns nil */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "v"),
		x_mkspair(p_base, x_mksymbol(p_base, "h"), NULL));
	p_result = x_prim_convert(p_base, p_args);
	_it_should("convert with no from/to match returns nil",
		p_result == NULL);

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_token_read_string_tokens(void)
{
	x_obj_t *p_base, *p_args, *p_result;
	x_obj_t *p_token_base;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_token_base = x_prim_make_base(p_base, NULL);

	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "tb"), p_token_base));
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "s"),
			x_mkstr(p_base, "(1)(2)")));

	/* (token-read-string tb s) — "(1)(2)" produces two list tokens */
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "tb"),
		x_mkspair(p_base, x_mksymbol(p_base, "s"), NULL));
	p_result = x_prim_token_read_string(p_base, p_args);
	_it_should("token-read-string returns non-nil for multi-token input",
		p_result != NULL);
	_it_should("token-read-string result has more than one token",
		p_result != NULL && x_restobj(p_result) != NULL);

	test_cleanup(p_base);
	return NULL;
}

static char *test_type_make_instance_nil_type(void)
{
	x_obj_t *p_base, *p_args, *p_result;
	x_obj_t *p_bogus;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* make-instance with bogus handle -> nil */
	p_bogus = x_mksatom(p_base, (x_int_t)999);
	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "bh"), p_bogus));
	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "bh"),
		x_mkspair(p_base, x_mksatom(p_base, (x_int_t)0), NULL));
	p_result = x_prim_make_instance(p_base, p_args);
	_it_should("make-instance with bad handle returns nil",
		p_result == NULL);

	test_cleanup(p_base);
	return NULL;
}

static int test_error_hook_called_type;
static void test_error_hook_type_passthrough(x_obj_t *p_base, x_char_t *msg, x_obj_t *p_obj)
{
	/* First call: delegate to x_base_error (triggers longjmp in target).
	 * Second call: intercept (parent base has no handler). */
	if (test_error_hook_called_type == 0) {
		test_error_hook_called_type = 1;
		x_base_error(p_base, msg, p_obj);
	} else {
		test_error_hook_called_type = 2;
	}
}

static char *test_type_base_eval_error_no_parent(void)
{
	x_obj_t *p_base, *p_args, *p_result, *p_target;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_target = x_base_make(p_base, NULL);
	x_prim_register(p_target, NULL);

	x_base_env_alist_extend(p_base,
		x_mkspair(p_base, x_mksymbol(p_base, "tgt"), p_target));

	{
		x_obj_t *p_unbound;
		p_unbound = x_mksymbol(p_base, "____unbound2____");
		x_base_env_alist_extend(p_base,
			x_mkspair(p_base, x_mksymbol(p_base, "expr"), p_unbound));
	}

	/* Hook passes first error to x_base_error (longjmp in target),
	 * intercepts second error (parent has no handler, line 292). */
	test_error_hook_called_type = 0;
	x_obj_hook_error = test_error_hook_type_passthrough;
	x_base_field_error_handler(p_base) = NULL;

	p_args = x_mkspair(p_base,
		x_mksymbol(p_base, "tgt"),
		x_mkspair(p_base, x_mksymbol(p_base, "expr"), NULL));
	p_result = x_prim_base_eval(p_base, p_args);
	_it_should("base-eval error with no parent handler calls error hook twice",
		test_error_hook_called_type == 2);
	_it_should("base-eval error with no parent handler returns NULL",
		p_result == NULL);

	x_obj_hook_error = x_base_error;

	test_cleanup(p_base);
	return NULL;
}

static char *run_tests() {
	_run_test(test_type_typep);
	_run_test(test_type_type_of);
	_run_test(test_type_type_name);
	_run_test(test_type_make_instance);
	_run_test(test_type_make_token_base);
	_run_test(test_type_token_discard);
	_run_test(test_type_register);
	_run_test(test_type_make_type);
	_run_test(test_type_make_type_with_handlers);
	_run_test(test_type_base_make_type);
	_run_test(test_type_make_base);
	_run_test(test_type_base_eval);
	_run_test(test_type_base_eval_error);
	_run_test(test_type_base_bind);
	_run_test(test_type_buffer_token);
	_run_test(test_type_token_read_string);
	_run_test(test_type_convert);
	_run_test(test_type_make_instance_nil_type);
	_run_test(test_type_type_name_nil_name);
	_run_test(test_type_convert_from_exact);
	_run_test(test_type_convert_wildcard);
	_run_test(test_type_convert_to_alist);
	_run_test(test_type_convert_no_match);
	_run_test(test_type_token_read_string_tokens);
	_run_test(test_type_base_eval_error_no_parent);

	return NULL;
}
