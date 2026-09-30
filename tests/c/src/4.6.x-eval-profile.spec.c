/*
 * # Unit Tests: *x-eval* under X_PROFILE -- the eval count
 *
 * Under X_PROFILE the evaluator counts, in each object's flags word, how
 * many times evaluation reached it (x-eval.h, X_OBJ_EVALS_*).  The rest of
 * the C suite builds without X_PROFILE or X_COV, so this spec defines both
 * ahead of every source it includes -- the pair x-bin-profile is built
 * with -- and checks the count, and that the two agree.
 */

#define X_PROFILE
#define X_COV

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"

#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "ext/x-expr/tests/src/test-helper-system.c"

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-stdlib.c"
#include "ext/x-expr/src/x-lib.c"
#include "ext/x-expr/src/x-obj.c"
#include "src/x-obj/obj.c"
#include "src/x-obj/prim.c"
#include "ext/x-expr/src/x.c"
#include "src/x-alist.c"
#include "ext/x-expr/src/x-base.c"
#include "src/x-eval.c"
#include "src/x-env.c"
#include "src/x-tco.c"
#include "src/x-toplevel.c"
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
#include "src/x-type/err.c"
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
x_obj_t *x_prim_heap_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_image_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_base_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_buffer_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_iter_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
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

/* The flags word with the count's bits taken out. */
#define _flags_but_evals(X) \
	(x_obj_flags(X) & ~(X_OBJ_EVALS_MAX << X_OBJ_EVALS_SHIFT))

/* A three-form body of self-evaluating atoms, its forms in p_forms. */
static x_obj_t *_body3(x_obj_t *p_base, x_obj_t **p_forms)
{
	p_forms[0] = x_mksatom(p_base, X_OBJ_FLAG_NONE, 10);
	p_forms[1] = x_mksatom(p_base, X_OBJ_FLAG_NONE, 20);
	p_forms[2] = x_mksatom(p_base, X_OBJ_FLAG_NONE, 30);

	return x_mkspair(p_base, X_OBJ_FLAG_NONE, p_forms[0],
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_forms[1],
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_forms[2], NULL)));
}

/*
 * ## x_eval
 */
static char *test_eval_counts(void)
{
	x_obj_t *p_base, *p_obj;
	x_int_t flags;

	p_base = x_eval_make(NULL, NULL);
	p_obj = x_mksatom(p_base, X_OBJ_FLAG_RO | X_OBJ_FLAG_3, 42);
	flags = x_obj_flags(p_obj);

	_it_should("a fresh object's count reads zero",
		x_obj_evals(p_obj) == 0);

	x_eval(p_base, x_argrun({ .p = p_obj }));
	x_eval(p_base, x_argrun({ .p = p_obj }));
	x_eval(p_base, x_argrun({ .p = p_obj }));
	_it_should("each evaluation of an object counts one",
		x_obj_evals(p_obj) == 3);
	_it_should("the count leaves every flag bit alone but COV",
		_flags_but_evals(p_obj) == (flags | X_OBJ_FLAG_COV));
	_it_should("an odd count leaves TRACE clear: nothing looks traced",
		(x_obj_flags(p_obj) & X_OBJ_FLAG_TRACE) == 0);

	x_eval(p_base, x_argrun({ .p = NULL }));
	_it_should("a nil expression has nothing to count and is still nil",
		x_eval(p_base, x_argrun({ .p = NULL })) == NULL);

	test_cleanup(p_base);

	return NULL;
}

/*
 * ## x_eval_body
 */
static char *test_body_counts(void)
{
	x_obj_t *p_base, *p_body, *p_forms[3];

	p_base = x_eval_make(NULL, NULL);
	p_body = _body3(p_base, p_forms);

	x_eval_body(p_base, x_argrun({ .p = p_body }));
	x_eval_body(p_base, x_argrun({ .p = p_body }));
	_it_should("x_eval_body counts each body cell once a walk",
		x_obj_evals(p_body) == 2
		&& x_obj_evals(x_restobj(p_body)) == 2
		&& x_obj_evals(x_restobj(x_restobj(p_body))) == 2);
	_it_should("and each form it evaluates",
		x_obj_evals(p_forms[0]) == 2
		&& x_obj_evals(p_forms[1]) == 2
		&& x_obj_evals(p_forms[2]) == 2);

	test_cleanup(p_base);

	return NULL;
}

/*
 * ## A procedure's calls
 *
 * The call path defers the body's tail to the trampoline (x_eval_body_tco);
 * the apply path evaluates the whole body in place (x_eval_body).  Either
 * way the first body cell is stepped onto once a call.
 */
static char *test_procedure_counts(void)
{
	x_obj_t *p_base, *p_body, *p_proc, *p_args, *p_forms[3];
	int i;

	p_base = x_eval_make(NULL, NULL);
	p_body = _body3(p_base, p_forms);
	p_proc = x_make_procedure(p_base, 0, NULL, p_body,
		x_eval_field_env(p_base));
	p_args = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_proc, NULL);

	for (i = 0; i < 4; i++) {
		x_type_procedure_call(p_base, p_args);
		x_eval_tco_trampoline(p_base, x_argrun({ .p = NULL }));
	}
	_it_should("a called procedure's first body cell counts its calls",
		x_obj_evals(p_body) == 4);
	_it_should("the tail it deferred counts when the trampoline evaluates it",
		x_obj_evals(p_forms[2]) == 4);
	_it_should("and so does every form ahead of the tail",
		x_obj_evals(p_forms[0]) == 4 && x_obj_evals(p_forms[1]) == 4);

	for (i = 0; i < 3; i++) {
		x_type_procedure_apply(p_base, p_args);
	}
	_it_should("an applied procedure's calls count on the same cell",
		x_obj_evals(p_body) == 7 && x_obj_evals(p_forms[2]) == 7);

	test_cleanup(p_base);

	return NULL;
}

/*
 * ## The count's bounds
 */
static char *test_count_saturates(void)
{
	x_obj_t *p_base, *p_obj;

	p_base = x_eval_make(NULL, NULL);
	p_obj = x_mksatom(p_base, X_OBJ_FLAG_NONE, 7);
	x_obj_flags(p_obj) |= (X_OBJ_EVALS_MAX - 1) << X_OBJ_EVALS_SHIFT;

	x_eval(p_base, x_argrun({ .p = p_obj }));
	_it_should("a count one short of the maximum reaches it",
		x_obj_evals(p_obj) == X_OBJ_EVALS_MAX);

	x_eval(p_base, x_argrun({ .p = p_obj }));
	_it_should("and stops there instead of wrapping",
		x_obj_evals(p_obj) == X_OBJ_EVALS_MAX);
	_it_should("nothing spills past the count's top bit",
		(x_obj_flags(p_obj) >> (X_OBJ_EVALS_SHIFT + X_OBJ_EVALS_BITS)) == 0);

	test_cleanup(p_base);

	return NULL;
}

/* The reason the count stops at bit 30: x-expr's sweep clears a mark with an
 * x_obj_flag_t complement, which zero-extends into a 64-bit word and would
 * clear every bit from 32 up.  An object outside any base's chain has a nil
 * heap link, so a sweep from it visits it alone. */
static char *test_count_survives_sweep(void)
{
	x_obj_t *p_base, *p_obj;

	p_base = x_eval_make(NULL, NULL);
	p_obj = x_mksatom(NULL, X_OBJ_FLAG_NONE, 7);
	x_obj_flags(p_obj) |= (X_OBJ_EVALS_MAX << X_OBJ_EVALS_SHIFT)
		| X_OBJ_FLAG_MARK;

	x_heap_sweep(p_base, x_argrun({ .p = p_obj }, { .i = X_OBJ_FLAG_MARK }));
	_it_should("a sweep that keeps an object clears its mark",
		(x_obj_flags(p_obj) & X_OBJ_FLAG_MARK) == 0);
	_it_should("and leaves its count whole",
		x_obj_evals(p_obj) == X_OBJ_EVALS_MAX);

	x_sys_free(p_obj);
	test_cleanup(p_base);

	return NULL;
}

/* TRACE is the bit a caller borrows to mark objects of its own -- an image
 * write, most often -- and a traced object is evaluated like any other. */
static char *test_count_keeps_trace(void)
{
	x_obj_t *p_base, *p_obj;

	p_base = x_eval_make(NULL, NULL);
	p_obj = x_mksatom(p_base, X_OBJ_FLAG_NONE, 7);
	x_heap_tree_mark(p_base, x_argrun({ .p = p_obj }, { .i = X_OBJ_FLAG_TRACE }));

	x_eval(p_base, x_argrun({ .p = p_obj }));
	x_eval(p_base, x_argrun({ .p = p_obj }));
	_it_should("a traced object counts as any other",
		x_obj_evals(p_obj) == 2);
	_it_should("and stays traced",
		(x_obj_flags(p_obj) & X_OBJ_FLAG_TRACE) != 0);

	x_heap_chain_clear(p_obj, X_OBJ_FLAG_TRACE);
	_it_should("clearing the trace leaves the count",
		(x_obj_flags(p_obj) & X_OBJ_FLAG_TRACE) == 0
		&& x_obj_evals(p_obj) == 2);

	test_cleanup(p_base);

	return NULL;
}

/*
 * ## COV and the count agree
 */
static char *test_cov_agrees(void)
{
	x_obj_t *p_base, *p_reached, *p_not;

	p_base = x_eval_make(NULL, NULL);
	p_reached = x_mksatom(p_base, X_OBJ_FLAG_NONE, 1);
	p_not = x_mksatom(p_base, X_OBJ_FLAG_NONE, 2);

	x_eval(p_base, x_argrun({ .p = p_reached }));
	_it_should("an object evaluation reached carries COV and a count",
		(x_obj_flags(p_reached) & X_OBJ_FLAG_COV)
		&& x_obj_evals(p_reached) == 1);
	_it_should("one it never reached carries neither",
		(x_obj_flags(p_not) & X_OBJ_FLAG_COV) == 0
		&& x_obj_evals(p_not) == 0);

	test_cleanup(p_base);

	return NULL;
}

static char *run_tests() {
	_run_test(test_eval_counts);
	_run_test(test_body_counts);
	_run_test(test_procedure_counts);
	_run_test(test_count_saturates);
	_run_test(test_count_survives_sweep);
	_run_test(test_count_keeps_trace);
	_run_test(test_cov_agrees);

	return NULL;
}
