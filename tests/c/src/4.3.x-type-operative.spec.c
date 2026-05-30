/*
 * # Unit Tests: *x-type/operative*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "ext/x-expr/tests/src/test-helper-system.c"

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-lib.c"
#include "ext/x-expr/src/x-obj.c"
#include "src/x-obj/obj.c"
#include "src/x-obj/prim.c"
#include "ext/x-expr/src/x.c"
#include "src/x-alist.c"
#include "ext/x-expr/src/x-base.c"
#include "src/x-interp.c"
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

/* Stubs for primitives not under test. */
x_obj_t *x_prim_core_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_arith_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_pred_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_string_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_io_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_type_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_ffi_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_callcc_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_binding_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_closure_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_control_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_quote_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }



/*
 * ## Test Overhead
 */

static void _setup(void)
{
	_buffer_index = -1;
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

static char *test_operative_struct(void)
{
	x_obj_t *p_base, *p_type;

	p_base = x_interp_make(NULL, NULL);
	p_type = x_type_operative_struct(p_base, NULL);

	_it_should("return a type struct",
		p_type != NULL);
	_it_should("have name 'operative'",
		x_type_field_name(p_type) != NULL);
	_it_should("have a make fn",
		x_type_field_make(p_type) != NULL);
	_it_should("have a call fn",
		x_type_field_call(p_type) != NULL);
	_it_should("have a write fn",
		x_type_field_write(p_type) != NULL);

	test_cleanup(p_base);

	return NULL;
}

static char *test_operative_register(void)
{
	x_obj_t *p_base, *p_type1, *p_type2;

	p_base = x_interp_make(NULL, NULL);
	p_type1 = x_type_operative_register(p_base, NULL);

	_it_should("return a type struct",
		p_type1 != NULL);

	/* Second call should return the same cached type */
	p_type2 = x_type_operative_register(p_base, NULL);
	_it_should("return same cached type on second call",
		p_type1 == p_type2);

	test_cleanup(p_base);

	return NULL;
}

static char *test_operative_make(void)
{
	x_obj_t *p_base, *p_op;

	p_base = x_interp_make(NULL, NULL);

	p_op = x_make_operative(p_base, X_OBJ_FLAG_NONE,
		x_mksatom(p_base, X_OBJ_FLAG_NONE, "params"),
		x_mksatom(p_base, X_OBJ_FLAG_NONE, "envparam"),
		x_mksatom(p_base, X_OBJ_FLAG_NONE, "body"),
		x_mksatom(p_base, X_OBJ_FLAG_NONE, "env"));

	_it_should("create an operative object",
		p_op != NULL);
	_it_should("have correct params",
		x_opparams(p_op) != NULL);
	_it_should("have correct envparam",
		x_openvparam(p_op) != NULL);
	_it_should("have correct body",
		x_opbody(p_op) != NULL);

	test_cleanup(p_base);

	return NULL;
}

static char *test_operative_call(void)
{
	x_obj_t *p_base, *p_op;
	x_obj_t *p_params, *p_body, *p_args, *p_result;
	x_obj_t *p_saved_env;

	p_base = x_interp_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Create operative: (op x 99) — variadic param, body is (99).
	 * Ops are lexically scoped: body runs synchronously via x_eval_body
	 * in extend(captured_env, formals); return value is the last form's
	 * value.  After the body the formal frame is shed (op_chain_head
	 * still reachable from env_alist => restore to caller_env). */
	p_params = x_mksymbol(p_base, "x");
	p_body = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 99), NULL);

	p_op = x_make_operative(p_base, X_OBJ_FLAG_NONE,
		p_params, NULL, p_body, x_firstobj(x_interp_field_env_alist(p_base)));

	p_saved_env = x_firstobj(x_interp_field_env_alist(p_base));

	/* Call: (op 42) — args: (op . (42 . nil)) */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_op,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 42), NULL));

	/* Operatives now defer their tail to the trampoline (O(1) C stack);
	 * drive it to completion the way the main eval / apply paths do. */
	p_result = x_eval_tco_trampoline(p_base,
		x_type_operative_call(p_base, p_args));

	_it_should("body's tail value is returned",
		p_result != NULL && x_atomint(p_result) == 99);

	_it_should("env_alist restored to caller (formals shed)",
		x_firstobj(x_interp_field_env_alist(p_base)) == p_saved_env);

	test_cleanup(p_base);

	return NULL;
}

static char *test_operative_call_envparam(void)
{
	x_obj_t *p_base, *p_op;
	x_obj_t *p_envparam, *p_body, *p_args, *p_result;
	x_obj_t *p_caller_env;

	p_base = x_interp_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	p_caller_env = x_firstobj(x_interp_field_env_alist(p_base));

	/* Op with env-param 'e', no params, body is (42).  Lexical scope:
	 * env-param is bound to caller's env during body execution but the
	 * formal frame (which includes the env-param binding) is shed on
	 * unwind since the body doesn't tail-eval away.  Body returns 42. */
	p_envparam = x_mksymbol(p_base, "e");
	p_body = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 42), NULL);

	p_op = x_make_operative(p_base, X_OBJ_FLAG_NONE,
		NULL, p_envparam, p_body, x_firstobj(x_interp_field_env_alist(p_base)));

	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_op, NULL);

	p_result = x_eval_tco_trampoline(p_base,
		x_type_operative_call(p_base, p_args));

	_it_should("body's tail value is returned",
		p_result != NULL && x_atomint(p_result) == 42);

	_it_should("env_alist restored to caller (env-param frame shed)",
		x_firstobj(x_interp_field_env_alist(p_base)) == p_caller_env);

	test_cleanup(p_base);

	return NULL;
}

static char *test_operative_write(void)
{
	x_obj_t *p_base, *p_op, *p_args, *p_ret;
	x_char_t s[64];

	p_base = x_interp_make(NULL, NULL);

	p_op = x_make_operative(p_base, X_OBJ_FLAG_NONE,
		NULL, NULL, NULL, NULL);

	helper_file_buffer_ptr[TEST_HELPER_FILE_STDOUT] = s;
	helper_file_buffer_length[TEST_HELPER_FILE_STDOUT] = 64;
	helper_file_reset();

	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_op, NULL);
	p_ret = x_type_operative_write(p_base, p_args);

	_it_should("return the operative",
		p_ret == p_op);
	_it_should("write output",
		s[0] != '\0');

	test_cleanup(p_base);

	return NULL;
}

static char *run_tests() {
	_run_test(test_operative_struct);
	_run_test(test_operative_register);
	_run_test(test_operative_make);
	_run_test(test_operative_call);
	_run_test(test_operative_call_envparam);
	_run_test(test_operative_write);

	return NULL;
}
