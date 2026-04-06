/*
 * # Unit Tests: *x-eval*
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
#include "ext/x-expr/src/x.c"
#include "src/x-alist.c"
#include "ext/x-expr/src/x-base.c"
#include "src/x-base.c"
#include "src/x-eval.c"
#include "ext/x-expr/src/x-heap.c"
#include "src/x-type.c"
#include "src/x-type/atom.c"
#include "src/x-token/sexp/atom.c"
#include "src/x-type/pair.c"
#include "src/x-token/sexp/pair.c"
#include "src/x-type/prim.c"

#define STUB_X_PRIM
#define STUB_X_PROCEDURE
#define STUB_X_OPERATIVE
#define STUB_X_HEAP
#define STUB_X_OBJ_OBJ
#define STUB_X_STR
#define STUB_X_PRIM_REGISTER
#define STUB_X_TOKEN
#define STUB_X_PRIM_SHADOW
#define STUB_X_TOKEN_DISPLAY
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
	_buffer_index = -1;
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

x_obj_t *test_type_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_00(p_args);

	x_atomint(p_obj) = ~x_atomint(p_obj);

	return p_obj;
}

x_obj_t *test_type_skip(x_obj_t *p_base, x_obj_t *p_args)
{
	x_0(p_args) = x_10(p_args);

	return p_args;
}

x_satom_t test_type_names[3] = {
		x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = "NONE" }),
		x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = "EVAL" }),
		x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = "SKIP" }),
	},
	test_eval_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_type_eval }),
	test_eval_skip = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = test_type_skip });

static char *test_eval(void)
{
	x_obj_t *p_base, *p_type, *p_args, *p_obj, *p_ret;
	x_int_t i = rand();
	struct x_type_t types[3] = {
		{
			.p_name = test_type_names[0]
		},
		{
			.p_name = test_type_names[1],
			.p_eval = test_eval_prim
		},
		{
			.p_name = test_type_names[2],
			.p_eval = test_eval_skip
		}
	};

	p_args = x_mkpair(NULL, x_mkpair(NULL, NULL, NULL), NULL);
	p_ret = x_eval(NULL, p_args);
	_it_should("evalute nil as nil", x_obj_isnil(NULL, p_ret));

	x_sys_free(x_firstobj(p_args));
	x_sys_free(p_args);


	p_base = x_base_ts_make(NULL, NULL);
	p_obj = x_mksatom(p_base, X_OBJ_FLAG_NONE, i);
	p_args = x_mkpair(p_base, x_mkpair(p_base, p_obj, p_base), p_base);
	p_ret = x_eval(p_base, p_args);
	_it_should("evaluate simple types as themselves",
		x_obj_type_issatom(p_ret)
		&& p_obj == p_ret
	);

	test_cleanup(p_base);


	p_base = x_base_ts_make(NULL, NULL);
	p_type = x_type_struct_make(p_base, types[0]);
	p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE, X_OBJ_LENGTH_ATOM, i);
	p_args = x_mkpair(p_base, x_mkpair(p_base, p_obj, p_base), p_base);
	p_ret = x_eval(p_base, p_args);
	_it_should("evalute complex type's eval function",
		p_type == x_obj_type(p_ret)
		&& p_obj == p_ret
	);

	test_cleanup(p_base);


	p_base = x_base_ts_make(NULL, NULL);
	p_type = x_type_struct_make(p_base, types[1]);
	p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE, X_OBJ_LENGTH_ATOM, i);
	p_args = x_mkpair(p_base, x_mkpair(p_base, p_obj, p_base), p_base);
	p_ret = x_eval(p_base, p_args);
	_it_should("evaluate complex type's eval function",
		p_type == x_obj_type(p_ret)
		&& ~i == x_firstint(p_ret)
	);

	test_cleanup(p_base);


	p_base = x_base_ts_make(NULL, NULL);
	p_type = x_type_struct_make(p_base, types[2]);
	p_obj = x_mksatom(p_base, X_OBJ_FLAG_NONE, i);
	p_args = x_mkpair(p_base,
		x_mkpair(p_base,
			x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE, X_OBJ_LENGTH_ATOM, 0),
			x_mkpair(p_base,
				x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE, X_OBJ_LENGTH_ATOM, 0),
				x_mkpair(p_base, p_obj, p_base))),
		p_base);
	p_ret = x_eval(p_base, p_args);
	_it_should("evaluate complex type's eval function",
		x_obj_type_issatom(p_ret)
		&& p_obj == p_ret
	);

	test_cleanup(p_base);


	p_base = x_base_ts_make(NULL, NULL);
	p_obj = x_mksatom(p_base, X_OBJ_FLAG_NONE, i);
	p_args = x_mkpair(p_base, x_mkpair(p_base, p_obj, p_base), p_base);
	p_ret = x_eval(p_base, p_args);
	_it_should("evaluate unevaluated expressions as themselves",
		x_obj_type_issatom(p_ret)
		&& p_obj == p_ret
	);

	test_cleanup(p_base);

	return NULL;
}

static int tco_eval_calls = 0;
x_obj_t *test_tco_result = NULL;
x_obj_t *test_tco_env_to_set = NULL;

x_obj_t *test_type_tco_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_00(p_args);

	tco_eval_calls++;

	/* Set a self-evaluating satom as the TCO bounce target */
	test_tco_result = x_mksatom(p_base, X_OBJ_FLAG_NONE, 999);
	x_firstobj(x_base_field_tco_expr(p_base)) = test_tco_result;

	return p_obj;
}

/* Two-bounce eval: first call bounces with nil tco_env,
 * second call bounces with non-nil tco_env (the "later iteration"
 * path in x_eval lines 67-74), third call resolves. */
x_obj_t *test_type_tco_eval_two_bounce(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_00(p_args);

	tco_eval_calls++;

	if (tco_eval_calls == 1) {
		/* First bounce: leave tco_env nil */
		x_firstobj(x_base_field_tco_expr(p_base)) = p_obj;
		return p_obj;
	}

	if (tco_eval_calls == 2) {
		/* Second bounce: set tco_env for env restore.
		 * tco_env holds compound ((env . boundary) . bst). */
		if (test_tco_env_to_set != NULL) {
			x_firstobj(x_base_field_tco_env(p_base)) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
				x_mkspair(p_base, X_OBJ_FLAG_NONE, test_tco_env_to_set, NULL),
				NULL);
		}
		x_firstobj(x_base_field_tco_expr(p_base)) = p_obj;
		return p_obj;
	}

	/* Third call: resolve — don't set tco_expr */
	test_tco_result = p_obj;
	return p_obj;
}

static char *test_eval_tco(void)
{
	x_obj_t *p_base, *p_type, *p_obj, *p_args, *p_ret;
	x_int_t tco_count;
	struct x_type_t tco_type;
	x_satom_t tco_type_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .s = "TCO" });
	x_satom_t tco_eval_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .fn = test_type_tco_eval });

	/* TCO bounce: eval fn sets tco_expr, x_eval loops back */
	p_base = x_base_ts_make(NULL, NULL);

	x_lib_memset(&tco_type, 0, sizeof(tco_type));
	tco_type.p_name = tco_type_name;
	tco_type.p_eval = tco_eval_prim;

	p_type = x_type_struct_make(p_base, tco_type);
	p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE,
		X_OBJ_LENGTH_ATOM, 42);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj, NULL), NULL);

	tco_eval_calls = 0;
	p_ret = x_eval(p_base, p_args);
	_it_should("bounce via TCO and return bounced result",
		p_ret == test_tco_result);
	_it_should("call eval fn once (bounce resolves to self-eval)",
		1 == tco_eval_calls);

	tco_count = x_atomint(x_base_field_profile_tco(p_base));
	_it_should("increment TCO profile counter",
		tco_count == 1);

	test_cleanup(p_base);

	/* TCO env restore: tco_env set by eval fn */
	p_base = x_base_ts_make(NULL, NULL);
	p_type = x_type_struct_make(p_base, tco_type);
	p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE,
		X_OBJ_LENGTH_ATOM, 42);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj, NULL), NULL);

	/* Set a tco_env so that after bounce, env is restored */
	{
		x_obj_t *p_saved_env = x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mkspair(p_base, X_OBJ_FLAG_NONE,
				x_mksatom(p_base, X_OBJ_FLAG_NONE, "key"),
				x_mksatom(p_base, X_OBJ_FLAG_NONE, "val")),
			NULL);

		x_firstobj(x_base_field_tco_env(p_base)) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, p_saved_env, NULL),
			NULL);

		/* Modify env to something else */
		x_firstobj(x_base_field_env_alist(p_base)) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mkspair(p_base, X_OBJ_FLAG_NONE,
				x_mksatom(p_base, X_OBJ_FLAG_NONE, "other"),
				x_mksatom(p_base, X_OBJ_FLAG_NONE, "env")),
			NULL);

		tco_eval_calls = 0;
		p_ret = x_eval(p_base, p_args);

		/* After TCO bounce resolves, env should NOT be restored
		 * because tco_env was set before the trampoline started
		 * (it gets picked up as p_tco_env_save on first iteration) */
		_it_should("TCO with tco_env bounces correctly",
			p_ret == test_tco_result);
	}

	test_cleanup(p_base);

	return NULL;
}

static char *test_eval_nil_base(void)
{
	x_obj_t *p_args, *p_ret;

	/* nil expression returns NULL */
	p_args = x_mkspair(NULL, X_OBJ_FLAG_NONE, x_mkspair(NULL, X_OBJ_FLAG_NONE, NULL, NULL), NULL);
	p_ret = x_eval(NULL, p_args);
	_it_should("return NULL for nil expression",
		p_ret == NULL);

	return NULL;
}

static char *test_eval_tco_env_restore(void)
{
	x_obj_t *p_base, *p_type, *p_obj, *p_args, *p_ret;
	x_obj_t *p_restore_env;
	struct x_type_t tco_type;
	x_satom_t tco_type_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .s = "TCO2" });
	x_satom_t tco_eval_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .fn = test_type_tco_eval_two_bounce });

	p_base = x_base_ts_make(NULL, NULL);

	x_lib_memset(&tco_type, 0, sizeof(tco_type));
	tco_type.p_name = tco_type_name;
	tco_type.p_eval = tco_eval_prim;

	p_type = x_type_struct_make(p_base, tco_type);
	p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE,
		X_OBJ_LENGTH_ATOM, 42);
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj, NULL), NULL);

	/* Set up env to restore: initial tco_env nil, second bounce sets it */
	p_restore_env = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mksatom(p_base, X_OBJ_FLAG_NONE, "restored"),
			x_mksatom(p_base, X_OBJ_FLAG_NONE, "env")),
		NULL);

	test_tco_env_to_set = p_restore_env;
	tco_eval_calls = 0;
	p_ret = x_eval(p_base, p_args);

	_it_should("TCO two-bounce returns correct result",
		p_ret == test_tco_result);
	_it_should("called eval fn three times",
		3 == tco_eval_calls);
	_it_should("restore env from later tco_env",
		x_firstobj(x_base_field_env_alist(p_base)) == p_restore_env);

	test_tco_env_to_set = NULL;
	test_cleanup(p_base);

	return NULL;
}

static char *run_tests() {
	_run_test(test_eval);
	_run_test(test_eval_tco);
	_run_test(test_eval_nil_base);
	_run_test(test_eval_tco_env_restore);

	return NULL;
}
