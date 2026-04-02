/*
 * # Unit Tests: *x-prim body-eval helpers*
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
 * ## x_eval_body
 */
static char *test_body_eval(void)
{
	x_obj_t *p_base, *p_body, *p_result;

	/* nil body returns NULL */
	p_base = x_base_make(NULL, NULL);
	p_result = x_eval_body(p_base, NULL);
	_it_should("return NULL for nil body", p_result == NULL);
	test_cleanup(p_base);

	/* single self-evaluating form returns it */
	p_base = x_base_make(NULL, NULL);
	p_body = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 42), NULL);
	p_result = x_eval_body(p_base, p_body);
	_it_should("return single form's value",
		p_result != NULL && x_atomint(p_result) == 42);
	test_cleanup(p_base);

	/* multi-form body returns last result */
	p_base = x_base_make(NULL, NULL);
	p_body = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 10),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 20),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 30), NULL)));
	p_result = x_eval_body(p_base, p_body);
	_it_should("return last form's value in multi-form body",
		p_result != NULL && x_atomint(p_result) == 30);
	test_cleanup(p_base);

	return NULL;
}

/*
 * ## x_eval_body_tco
 */
static char *test_body_eval_tco(void)
{
	x_obj_t *p_base, *p_body, *p_saved_env, *p_result;

	/* nil body pops save-stack, restores env, returns NULL.
	 * Save-stack entries are ((env . boundary) . (bst . flag1)). */
	p_base = x_base_make(NULL, NULL);
	p_saved_env = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 99), NULL);
	x_base_field_save_stack(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, p_saved_env, NULL),
			x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, NULL)),
		x_base_field_save_stack(p_base));
	x_base_field_env_alist(p_base) = NULL;
	p_result = x_eval_body_tco(p_base, NULL);
	_it_should("restore env for nil body",
		x_base_field_env_alist(p_base) == p_saved_env);
	_it_should("return NULL for nil body (tco)", p_result == NULL);
	test_cleanup(p_base);

	/* single form sets tco_expr and tco_env, pops save-stack */
	p_base = x_base_make(NULL, NULL);
	p_saved_env = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 88), NULL);
	x_base_field_save_stack(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, p_saved_env, NULL),
			x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, NULL)),
		x_base_field_save_stack(p_base));
	p_body = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 42), NULL);
	x_base_field_tco_env(p_base) = NULL;
	p_result = x_eval_body_tco(p_base, p_body);
	_it_should("set tco_expr for single form",
		x_base_field_tco_expr(p_base) != NULL
		&& x_atomint(x_base_field_tco_expr(p_base)) == 42);
	_it_should("set tco_env compound with saved_env",
		x_firstobj(x_firstobj(x_base_field_tco_env(p_base))) == p_saved_env);
	_it_should("return NULL when setting tco_expr", p_result == NULL);
	x_base_field_tco_expr(p_base) = NULL;
	x_base_field_tco_env(p_base) = NULL;
	test_cleanup(p_base);

	/* nil last form: pops save-stack, restores env, no TCO */
	p_base = x_base_make(NULL, NULL);
	p_saved_env = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 77), NULL);
	x_base_field_save_stack(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, p_saved_env, NULL),
			x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, NULL)),
		x_base_field_save_stack(p_base));
	p_body = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, NULL);
	p_result = x_eval_body_tco(p_base, p_body);
	_it_should("restore env for nil last form",
		x_base_field_env_alist(p_base) == p_saved_env);
	_it_should("return NULL for nil last form", p_result == NULL);
	_it_should("not set tco_expr for nil last form",
		x_obj_isnil(p_base, x_base_field_tco_expr(p_base)));
	test_cleanup(p_base);

	/* multi-form body: evals all but last, sets tco_expr for last */
	p_base = x_base_make(NULL, NULL);
	p_saved_env = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 44), NULL);
	x_base_field_save_stack(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, p_saved_env, NULL),
			x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, NULL)),
		x_base_field_save_stack(p_base));
	p_body = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 10),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 20),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 30), NULL)));
	x_base_field_tco_env(p_base) = NULL;
	p_result = x_eval_body_tco(p_base, p_body);
	_it_should("set tco_expr to last form in multi-form body (tco)",
		x_base_field_tco_expr(p_base) != NULL
		&& x_atomint(x_base_field_tco_expr(p_base)) == 30);
	_it_should("return NULL for multi-form body (tco)",
		p_result == NULL);
	x_base_field_tco_expr(p_base) = NULL;
	x_base_field_tco_env(p_base) = NULL;
	test_cleanup(p_base);

	/* tco_env idempotent: doesn't overwrite if already set */
	p_base = x_base_make(NULL, NULL);
	{
		x_obj_t *p_existing_tco_env = x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mksatom(p_base, X_OBJ_FLAG_NONE, 66), NULL);
		p_saved_env = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 55), NULL);
		x_base_field_save_stack(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mkspair(p_base, X_OBJ_FLAG_NONE,
				x_mkspair(p_base, X_OBJ_FLAG_NONE, p_saved_env, NULL),
				x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, NULL)),
			x_base_field_save_stack(p_base));
		p_body = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 42), NULL);
		x_base_field_tco_env(p_base) = p_existing_tco_env;
		p_result = x_eval_body_tco(p_base, p_body);
		_it_should("not overwrite existing tco_env",
			x_base_field_tco_env(p_base) == p_existing_tco_env);
		x_base_field_tco_expr(p_base) = NULL;
		x_base_field_tco_env(p_base) = NULL;
	}
	test_cleanup(p_base);

	return NULL;
}

/*
 * ## x_eval_body_tco_simple
 */
static char *test_body_eval_tco_simple(void)
{
	x_obj_t *p_base, *p_body, *p_result;

	/* nil body returns NULL */
	p_base = x_base_make(NULL, NULL);
	p_result = x_eval_body_tco_simple(p_base, NULL);
	_it_should("return NULL for nil body (simple)", p_result == NULL);
	test_cleanup(p_base);

	/* single form sets tco_expr, returns NULL */
	p_base = x_base_make(NULL, NULL);
	p_body = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 42), NULL);
	x_base_field_tco_env(p_base) = NULL;
	p_result = x_eval_body_tco_simple(p_base, p_body);
	_it_should("set tco_expr for single form (simple)",
		x_base_field_tco_expr(p_base) != NULL
		&& x_atomint(x_base_field_tco_expr(p_base)) == 42);
	_it_should("return NULL when setting tco_expr (simple)",
		p_result == NULL);
	_it_should("not set tco_env (simple)",
		x_obj_isnil(p_base, x_base_field_tco_env(p_base)));
	x_base_field_tco_expr(p_base) = NULL;
	test_cleanup(p_base);

	/* multi-form: evals all but last, sets tco_expr for last */
	p_base = x_base_make(NULL, NULL);
	p_body = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 10),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 20), NULL));
	x_base_field_tco_env(p_base) = NULL;
	p_result = x_eval_body_tco_simple(p_base, p_body);
	_it_should("set tco_expr to last form in multi-form (simple)",
		x_base_field_tco_expr(p_base) != NULL
		&& x_atomint(x_base_field_tco_expr(p_base)) == 20);
	_it_should("return NULL for multi-form (simple)",
		p_result == NULL);
	x_base_field_tco_expr(p_base) = NULL;
	test_cleanup(p_base);

	return NULL;
}

/*
 * ## x_eval_tco_trampoline
 */
static char *test_tco_trampoline(void)
{
	x_obj_t *p_base, *p_result, *p_initial;

	/* no tco_expr: returns p_result unchanged */
	p_base = x_base_make(NULL, NULL);
	p_initial = x_mksatom(p_base, X_OBJ_FLAG_NONE, 42);
	x_base_field_tco_expr(p_base) = NULL;
	p_result = x_eval_tco_trampoline(p_base, p_initial);
	_it_should("return p_result when no tco_expr",
		p_result == p_initial);
	test_cleanup(p_base);

	/* single tco_expr: evaluates it (self-eval atom) */
	p_base = x_base_make(NULL, NULL);
	x_base_field_tco_expr(p_base) = x_mksatom(p_base, X_OBJ_FLAG_NONE, 99);
	x_base_field_tco_env(p_base) = NULL;
	p_result = x_eval_tco_trampoline(p_base, NULL);
	_it_should("evaluate single tco_expr",
		p_result != NULL && x_atomint(p_result) == 99);
	test_cleanup(p_base);

	/* tco_env restore: tco_env holds compound ((env . boundary) . (bst . flag1)) */
	p_base = x_base_make(NULL, NULL);
	{
		x_obj_t *p_env = x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mksatom(p_base, X_OBJ_FLAG_NONE, 77), NULL);
		x_obj_t *p_tco_env = x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, p_env, NULL),
			x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, NULL));
		x_base_field_tco_expr(p_base) = x_mksatom(p_base, X_OBJ_FLAG_NONE, 55);
		x_base_field_tco_env(p_base) = p_tco_env;
		x_base_field_env_alist(p_base) = NULL;
		p_result = x_eval_tco_trampoline(p_base, NULL);
		_it_should("restore env from tco_env",
			x_base_field_env_alist(p_base) == p_env);
	}
	test_cleanup(p_base);

	return NULL;
}

/*
 * ## x_eval_arg
 */
static char *test_eval_arg(void)
{
	x_obj_t *p_base, *p_result;

	/* self-evaluating atom passes through eval */
	p_base = x_base_make(NULL, NULL);
	p_result = x_eval_arg(p_base, x_mksatom(p_base, X_OBJ_FLAG_NONE, 42));
	_it_should("eval_arg returns self-evaluating atom",
		p_result != NULL && x_atomint(p_result) == 42);
	test_cleanup(p_base);

	/* nil arg returns NULL */
	p_base = x_base_make(NULL, NULL);
	p_result = x_eval_arg(p_base, NULL);
	_it_should("eval_arg returns NULL for nil", p_result == NULL);
	test_cleanup(p_base);

	return NULL;
}

/*
 * ## x_eval_list
 */
static char *test_evlis(void)
{
	x_obj_t *p_base, *p_args, *p_result;

	/* nil list returns NULL */
	p_base = x_base_make(NULL, NULL);
	p_result = x_eval_list(p_base, NULL);
	_it_should("evlis returns NULL for nil", p_result == NULL);
	test_cleanup(p_base);

	/* single-element list */
	p_base = x_base_make(NULL, NULL);
	p_args = x_mklist(p_base, x_mksatom(p_base, X_OBJ_FLAG_NONE, 7), NULL);
	p_result = x_eval_list(p_base, p_args);
	_it_should("evlis single element",
		p_result != NULL && x_atomint(x_firstobj(p_result)) == 7);
	_it_should("evlis single element rest is nil",
		x_obj_isnil(p_base, x_restobj(p_result)));
	test_cleanup(p_base);

	/* multi-element list */
	p_base = x_base_make(NULL, NULL);
	p_args = x_mklist(p_base, x_mksatom(p_base, X_OBJ_FLAG_NONE, 1),
		x_mklist(p_base, x_mksatom(p_base, X_OBJ_FLAG_NONE, 2),
		x_mklist(p_base, x_mksatom(p_base, X_OBJ_FLAG_NONE, 3), NULL)));
	p_result = x_eval_list(p_base, p_args);
	_it_should("evlis multi first", x_atomint(x_firstobj(p_result)) == 1);
	_it_should("evlis multi second",
		x_atomint(x_firstobj(x_restobj(p_result))) == 2);
	_it_should("evlis multi third",
		x_atomint(x_firstobj(x_restobj(x_restobj(p_result)))) == 3);
	test_cleanup(p_base);

	return NULL;
}

/*
 * ## x_env_extend
 */
static char *test_multiple_extend(void)
{
	x_obj_t *p_base, *p_env, *p_params, *p_vals, *p_result;

	/* nil params returns env unchanged */
	p_base = x_base_make(NULL, NULL);
	p_env = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 99), NULL);
	p_result = x_env_extend(p_base, p_env, NULL, NULL);
	_it_should("nil params returns env", p_result == p_env);
	test_cleanup(p_base);

	/* single param binding (using pair-type param list) */
	p_base = x_base_make(NULL, NULL);
	p_env = NULL;
	p_params = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 1), NULL);
	p_vals = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 10), NULL);
	p_result = x_env_extend(p_base, p_env, p_params, p_vals);
	_it_should("single binding: key is 1",
		x_atomint(x_firstobj(x_firstobj(p_result))) == 1);
	_it_should("single binding: val is 10",
		x_atomint(x_restobj(x_firstobj(p_result))) == 10);
	test_cleanup(p_base);

	/* multiple param bindings */
	p_base = x_base_make(NULL, NULL);
	p_env = NULL;
	p_params = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 1),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 2), NULL));
	p_vals = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 10),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 20), NULL));
	p_result = x_env_extend(p_base, p_env, p_params, p_vals);
	_it_should("multi binding: first entry key is 2",
		x_atomint(x_firstobj(x_firstobj(p_result))) == 2);
	_it_should("multi binding: first entry val is 20",
		x_atomint(x_restobj(x_firstobj(p_result))) == 20);
	_it_should("multi binding: second entry key is 1",
		x_atomint(x_firstobj(x_firstobj(x_restobj(p_result)))) == 1);
	test_cleanup(p_base);

	/* variadic: symbol as params binds to entire arg list */
	p_base = x_base_make(NULL, NULL);
	p_env = NULL;
	p_params = x_mksymbol(p_base, (x_char_t *)"rest");
	p_vals = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 1),
		x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 2), NULL));
	p_result = x_env_extend(p_base, p_env, p_params, p_vals);
	_it_should("variadic: val is entire list",
		x_restobj(x_firstobj(p_result)) == p_vals);
	test_cleanup(p_base);

	return NULL;
}

/*
 * ## x_callable_bind
 */
static char *test_bind(void)
{
	x_obj_t *p_base, *p_env;

	/* bind adds symbol-prim pair to env */
	p_base = x_base_make(NULL, NULL);
	x_callable_bind(p_base, (x_char_t *)"test-fn", x_eval_body);
	p_env = x_base_field_env_alist(p_base);
	_it_should("bind extends env", ! x_obj_isnil(p_base, p_env));
	_it_should("bind: key is symbol",
		x_obj_type_issymbol(p_base, x_firstobj(x_firstobj(p_env))));
	_it_should("bind: val is prim",
		x_obj_type_isprim(p_base, x_restobj(x_firstobj(p_env))));
	test_cleanup(p_base);

	return NULL;
}

/*
 * ## x_prim_register
 */
static char *test_register(void)
{
	x_obj_t *p_base, *p_result;

	/* register returns p_base and sets #t/#f */
	p_base = x_base_make(NULL, NULL);
	p_result = x_prim_register(p_base, NULL);
	_it_should("register returns p_base", p_result == p_base);
	_it_should("register sets #t",
		! x_obj_isnil(p_base, x_base_field_true(p_base)));
	_it_should("register sets #f",
		! x_obj_isnil(p_base, x_base_field_false(p_base)));
	test_cleanup(p_base);

	return NULL;
}

/*
 * ## x_obj_prim_call — nil call field path
 */
static char *test_prim_call_nil_call(void)
{
	x_obj_t *p_base, *p_type, *p_obj, *p_args, *p_ret;
	struct x_type_t type_desc;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Type with nil call field */
	x_lib_memset(&type_desc, 0, sizeof(type_desc));
	type_desc.p_name = x_mksatom(p_base, X_OBJ_FLAG_NONE, "NOCALL");
	type_desc.p_units = (x_obj_t *)&x_type_units_pair_obj;
	p_type = x_type_struct_make(p_base, type_desc);

	p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE,
		X_OBJ_LENGTH_PAIR, NULL, NULL);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj, NULL);
	p_ret = x_obj_prim_call(p_base, p_args);
	_it_should("prim_call returns NULL for nil call field",
		p_ret == NULL);

	test_cleanup(p_base);

	return NULL;
}

/*
 * ## x_obj_prim_call — procedure call path
 */
static char *test_prim_call_procedure(void)
{
	x_obj_t *p_base, *p_proc, *p_type, *p_obj, *p_args, *p_ret;
	struct x_type_t type_desc;
	x_obj_t *p_body;

	p_base = x_base_make(NULL, NULL);
	x_prim_register(p_base, NULL);

	/* Create a procedure: (fn () 42) — no params, body returns 42 */
	p_body = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_mksatom(p_base, X_OBJ_FLAG_NONE, 42), NULL);
	p_proc = x_make_procedure(p_base, 0, NULL, p_body,
		x_base_field_env_alist(p_base),
		x_base_field_env_global_tree(p_base));

	/* Create a custom type whose call field is the procedure */
	x_lib_memset(&type_desc, 0, sizeof(type_desc));
	type_desc.p_name = x_mksatom(p_base, X_OBJ_FLAG_NONE, "CALLABLE");
	type_desc.p_units = (x_obj_t *)&x_type_units_pair_obj;
	type_desc.p_call = p_proc;
	p_type = x_type_struct_make(p_base, type_desc);

	/* Create an instance of the callable type */
	p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE,
		X_OBJ_LENGTH_PAIR, NULL, NULL);

	/* Call via x_obj_prim_call — exercises procedure path */
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj, NULL);
	x_base_field_tco_expr(p_base) = NULL;
	p_ret = x_obj_prim_call(p_base, p_args);

	/* procedure_call sets tco_expr, returns NULL */
	_it_should("prim_call procedure path sets tco_expr",
		x_base_field_tco_expr(p_base) != NULL
		&& x_atomint(x_base_field_tco_expr(p_base)) == 42);
	_it_should("prim_call procedure path returns NULL",
		p_ret == NULL);

	x_base_field_tco_expr(p_base) = NULL;
	test_cleanup(p_base);

	return NULL;
}

static char *run_tests() {
	_run_test(test_body_eval);
	_run_test(test_body_eval_tco);
	_run_test(test_body_eval_tco_simple);
	_run_test(test_tco_trampoline);
	_run_test(test_eval_arg);
	_run_test(test_evlis);
	_run_test(test_multiple_extend);
	_run_test(test_bind);
	_run_test(test_register);
	_run_test(test_prim_call_nil_call);
	_run_test(test_prim_call_procedure);

	return NULL;
}
