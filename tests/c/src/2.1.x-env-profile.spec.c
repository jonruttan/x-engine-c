/*
 * # Unit Tests: *x-env* under X_PROFILE
 *
 * The profile-env-steps counter.  The rest of the C suite builds without
 * X_PROFILE, so this spec defines it ahead of every source it includes and
 * the counter is compiled in.
 */

#define X_PROFILE

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
#include "ext/x-expr/src/x.c"
#include "src/x-alist.c"
#include "ext/x-expr/src/x-base.c"
#define X_EVAL_OWN
#include "src/x-eval.c"
#include "src/x-env.c"
#include "src/x-tco.c"
#include "src/x-toplevel.c"

#define STUB_X_PRIM
#define STUB_X_LIST
#define STUB_X_PRIM_REGISTER
#define STUB_X_PROCEDURE
#define STUB_X_OPERATIVE
#define STUB_X_HEAP
#define STUB_X_OBJ_OBJ
#define STUB_X_STR
#define STUB_X_TYPE_PRIM
#define STUB_X_SYMBOL_FIND
#include "helper-stubs.c"

x_obj_t *x_type_heap_mark(x_obj_t *p_base, x_obj_t *p_obj, x_obj_flag_t flags) { return NULL; }
void x_type_heap_free(x_obj_t *p_base, x_obj_t *p_obj) {}

/*
 * x-eval and x-toplevel link against the reader, the writer and eval.  No
 * test here reaches them, so each is the least that links.
 */
x_satom_t x_token_eof_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .i = 0 });

x_obj_t *x_token_read(x_obj_t *p_base, x_obj_t *p_args)
{
	return (x_obj_t *)x_token_eof_prim;
}

x_obj_t *x_token_write(x_obj_t *p_base, x_obj_t *p_args) { return NULL; }

x_obj_t *x_eval(x_obj_t *p_base, x_obj_t *p_args) { return NULL; }



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

/*
 * The counter as it stands.
 */
static x_int_t _env_steps(x_obj_t *p_base)
{
	return x_atomint(x_firstobj(x_eval_field_profile_env_steps(p_base)));
}

/* x_env_lookup counts one for each binding it compares in an environment
 * with a parent: a frame, or a module's environment.  The root's lookup is
 * its tree, which profile-bst-hits and profile-bst-misses count, so a
 * lookup that reaches the root adds here only what it compared on the way.
 * As in 2.x-base.spec.c the keys are atoms and the environments children,
 * whose alists compare keys by identity. */
static char *test_env_profile_steps(void)
{
	x_obj_t *p_base, *p_root, *p_child, *p_grand, *p_atoms[3], *p_syms[4];
	x_int_t before;

	p_base = x_eval_make(NULL, NULL);
	p_root = x_eval_field_env_root(p_base);

	_it_should("a fresh base's counter reads zero",
		_env_steps(p_base) == 0);

	p_atoms[0] = x_mksatom(p_base, X_OBJ_FLAG_NONE, 1);
	p_atoms[1] = x_mksatom(p_base, X_OBJ_FLAG_NONE, 2);
	p_atoms[2] = x_mksatom(p_base, X_OBJ_FLAG_NONE, 3);
	p_syms[0] = x_mksatom(p_base, X_OBJ_FLAG_NONE, 10);
	p_syms[1] = x_mksatom(p_base, X_OBJ_FLAG_NONE, 11);
	p_syms[2] = x_mksatom(p_base, X_OBJ_FLAG_NONE, 12);
	p_syms[3] = x_mksatom(p_base, X_OBJ_FLAG_NONE, 13);

	/* The child holds two bindings and the grandchild one. */
	p_child = x_env_make(p_base, p_root);
	x_env_bind(p_base, p_child, p_syms[0], p_atoms[0]);
	x_env_bind(p_base, p_child, p_syms[1], p_atoms[1]);
	p_grand = x_env_make(p_base, p_child);
	x_env_bind(p_base, p_grand, p_syms[2], p_atoms[2]);

	before = _env_steps(p_base);
	x_env_lookup(p_base, p_grand, p_syms[2]);
	_it_should("a hit on an environment's only binding compares one",
		_env_steps(p_base) - before == 1);

	before = _env_steps(p_base);
	x_env_lookup(p_base, p_grand, p_syms[3]);
	_it_should("a miss compares every binding on the way to the root: the grandchild's one and the child's two",
		_env_steps(p_base) - before == 3);

	before = _env_steps(p_base);
	x_env_lookup(p_base, p_root, p_syms[3]);
	_it_should("a lookup that starts at the root compares no alist binding",
		_env_steps(p_base) - before == 0);

	x_sys_free(p_base);
	return NULL;
}

static char *run_tests() {
	_run_test(test_env_profile_steps);

	return NULL;
}
