/*
 * # Unit Tests: *x-cli*
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"
#include "x-type/buffer.h"

#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "ext/x-expr/tests/src/test-helper-system.c"

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-lib.c"
#include "ext/x-expr/src/x-obj.c"
#include "ext/x-expr/src/x-base.c"
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
#include "src/x-prim/core.c"
#include "src/x-syntax/binding.c"
#include "src/x-syntax/closure.c"
#include "src/x-syntax/control.c"
#include "src/x-syntax/quote.c"
#include "src/x-prim/arith.c"
#include "src/x-prim/pred.c"
#include "src/x-prim/string.c"
#define x_prim_atomic x_prim_atomic_io
#include "src/x-prim/io.c"
#include "src/x-prim/type.c"
#include "src/x-prim/ffi.c"
x_obj_t *x_prim_callcc_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
#include "src/x-exp/quote.c"

x_obj_t *x_prim_syscall(x_obj_t *p_base, x_obj_t *p_args) { return NULL; }
x_obj_t *x_prim_include(x_obj_t *p_base, x_obj_t *p_args) { return NULL; }
#include "src/x-cli.c"



/*
 * ## Test Overhead
 */

static void _setup(void)
{
	_buffer_index = -1;
	helper_set_alloc(MEM_SYSTEM);
	helper_sys_funcs.exit = mock_exit;
	helper_sys_funcs.malloc = helper_malloc;
	helper_sys_funcs.free = helper_free;
}

static void _teardown(void)
{
}

void test_cleanup(x_obj_t *p_base)
{
	x_obj_t *p_gc = p_base, *p_tmp, *p_alloc;
	size_t extra = (p_base != NULL
		&& !x_obj_isnil(p_base, x_obj_type(p_base))
		&& x_base_isset(p_base))
		? (size_t)x_atomint(x_firstobj(x_base_field_obj_meta_extra(p_base))) : 0;

	while (p_gc) {
		p_tmp = x_obj_heap(p_gc);
		p_alloc = (x_obj_flags(p_gc) & X_OBJ_FLAG_META) ? p_gc - extra : p_gc;
		x_sys_free(p_alloc);
		p_gc = p_tmp;
	}
}


/*
 * ## Test Runners
 */

static char *test_cli_init(void)
{
	x_obj_t *p_base;
	x_char_t buffer[256];

	p_base = init(NULL, buffer);
	_it_should("init returns non-NULL base",
		p_base != NULL);
	_it_should("init sets buffer on base",
		x_firstobj(x_base_field_buffer(p_base)) != NULL);
	_it_should("init registers primitives (eval is bound)",
		!x_obj_isnil(p_base, x_firstobj(x_base_field_env_alist(p_base))));

	test_cleanup(p_base);
	return NULL;
}

static char *run_tests() {
	_run_test(test_cli_init);

	return NULL;
}
