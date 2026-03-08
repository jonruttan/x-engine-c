/*
 * # Unit Tests: *x-eval*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-lib.c"
#include "src/x-obj.c"
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

x_obj_t *x_token_read(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_token_write(x_obj_t *p_base, x_obj_t *p_args) { return p_args; }

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

void test_cleanup(x_obj_t *p_base)
{
	x_obj_t *p_gc = p_base, *p_tmp;

	while (p_gc) {
		p_tmp = x_obj_gc(p_gc);
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


	p_base = x_mksatom(NULL, NULL);
	p_obj = x_mksatom(p_base, i);
	p_args = x_mkpair(p_base, x_mkpair(p_base, p_obj, p_base), p_base);
	p_ret = x_eval(p_base, p_args);
	_it_should("evaluate simple types as themselves",
		x_obj_type_issatom(p_ret)
		&& p_obj == p_ret
	);

	test_cleanup(p_base);


	p_base = x_mksatom(NULL, NULL);
	p_type = x_type_struct_make(p_base, types[0]);
	p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE, X_OBJ_LENGTH_ATOM, i);
	p_args = x_mkpair(p_base, x_mkpair(p_base, p_obj, p_base), p_base);
	p_ret = x_eval(p_base, p_args);
	_it_should("evalute complex type's eval function",
		p_type == x_obj_type(p_ret)
		&& p_obj == p_ret
	);

	test_cleanup(p_base);


	p_base = x_mksatom(NULL, NULL);
	p_type = x_type_struct_make(p_base, types[1]);
	p_obj = x_obj_make(p_base, p_type, X_OBJ_FLAG_NONE, X_OBJ_LENGTH_ATOM, i);
	p_args = x_mkpair(p_base, x_mkpair(p_base, p_obj, p_base), p_base);
	p_ret = x_eval(p_base, p_args);
	_it_should("evaluate complex type's eval function",
		p_type == x_obj_type(p_ret)
		&& ~i == x_firstint(p_ret)
	);

	test_cleanup(p_base);


	p_base = x_mksatom(NULL, NULL);
	p_type = x_type_struct_make(p_base, types[2]);
	p_obj = x_mksatom(p_base, i);
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


	p_base = x_mksatom(NULL, NULL);
	p_obj = x_mksatom(p_base, i);
	p_args = x_mkpair(p_base, x_mkpair(p_base, p_obj, p_base), p_base);
	p_ret = x_eval(p_base, p_args);
	_it_should("evaluate unevaluated expressions as themselves",
		x_obj_type_issatom(p_ret)
		&& p_obj == p_ret
	);

	test_cleanup(p_base);

	return NULL;
}

static char *run_tests() {
	_run_test(test_eval);

	return NULL;
}
