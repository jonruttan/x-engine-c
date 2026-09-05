/*
 * # Unit Tests: *x-eval* -- loading a file while a caller is mid-call
 *
 * x_eval_load displaces the includer's state for the duration of a load: the
 * save-stack is hidden so a loaded file's top-level defs bind globally, and
 * the includer's lexical frames are stripped off the env head so a closure
 * the file defines does not capture them.  Both are right.  The question
 * this file asks is where the displaced state WAITS -- because the collector
 * is precise, marks from the base and the root chain, and never scans the C
 * stack.  A loaded file that collects (a library that calls (heap-collect)
 * between definitions is ordinary) must find the includer's frames and
 * restore compound still reachable, or the includer resumes into freed
 * memory.
 *
 * The assertion is chain MEMBERSHIP, not a read of the cell: the sweep
 * unlinks what it frees, so "still on the allocation chain" is the
 * collector's own answer to "did this survive", and it is the same answer on
 * every allocator.  Reading the cell instead would pass on macOS, whose
 * allocator leaves a freed cell's payload intact for a while, and crash on
 * glibc, which threads a free list through it at once -- which is how this
 * bug reached x86-64 Linux CI only.
 */

#define TEST_RUNNER_OVERHEAD
#include "test-runner.h"
#include "x-type/buffer.h"

#ifndef X_GC
#define X_GC
#endif /* X_GC */

#include "ext/x-expr/tests/src/test-helper-system.c"

#include "ext/x-expr/src/x-sys.c"
#include "ext/x-expr/src/x-stdlib.c"
#include "ext/x-expr/src/x-lib.c"
#include "ext/x-expr/src/x-obj.c"
#include "ext/x-expr/src/x-base.c"
#include "src/x-obj/obj.c"
#include "src/x-obj/prim.c"
#include "ext/x-expr/src/x.c"
#include "src/x-alist.c"
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
#include "src/x-prim/heap.c"
#include "src/x-prim/image.c"
#include "src/x-prim/type.c"
#include "src/x-prim/base.c"
#include "src/x-prim/buffer.c"
#include "src/x-prim/iter.c"
#include "src/x-prim/ffi.c"
x_obj_t *x_prim_callcc_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }

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
 * Is @p p_obj still on the allocation chain?
 *
 * The sweep unlinks every object it frees, so membership is the collector's
 * own record of what survived -- and the walk never touches @p p_obj itself,
 * so asking about a swept cell is not a read of freed memory.
 */
static int _on_heap_chain(x_obj_t *p_base, x_obj_t *p_obj)
{
	x_obj_t *p_node;

	for (p_node = x_obj_heap(p_base); p_node != NULL; p_node = x_obj_heap(p_node)) {
		if (p_node == p_obj) {
			return 1;
		}
	}

	return 0;
}

/*
 * Feed @p src as the file being loaded and run x_eval_load over it.
 */
static x_obj_t *_load(x_obj_t *p_base, x_char_t *src, size_t len)
{
	x_obj_t *p_result;

	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = src;
	helper_file_buffer_remaining[TEST_HELPER_FILE_STDIN] = len;
	helper_file_reset();

	p_result = x_eval_load(p_base, p_base);

	helper_file_buffer_remaining[TEST_HELPER_FILE_STDIN] = TEST_HELPER_FILE_UNDEFINED;

	return p_result;
}


/*
 * ## Test Runners
 */

/*
 * The includer is a procedure mid-call: its formal is a FRAME cell at the
 * env head and its restore compound is the save-stack top -- exactly what
 * x_type_procedure_call leaves behind while a body runs.  The file it loads
 * does one thing, collect, so the only question is reachability.
 */
static char *test_load_keeps_the_includer_across_a_collect(void)
{
	x_obj_t *p_base, *p_sym, *p_frame, *p_binding, *p_stack, *p_compound;
	x_char_t buffer[256];
	static x_char_t src[] = "(heap-collect)\n";

	p_base = init(NULL, buffer);
	/* The heap namespace binds no bare names (its prims are reached through
	 * the catalog, which is x-lang), so name the collector for the source. */
	x_callable_bind(p_base, (x_char_t *)"heap-collect", x_prim_heap_collect);

	p_sym = x_make_symbol(p_base, X_OBJ_FLAG_NONE, (x_char_t *)"includer-local");
	p_frame = x_env_extend(p_base,
		x_firstobj(x_eval_field_env_alist(p_base)),
		x_mklist(p_base, p_sym, NULL),
		x_mklist(p_base, x_mkint(p_base, 7), NULL));
	x_firstobj(x_eval_field_env_alist(p_base)) = p_frame;
	p_binding = x_firstobj(p_frame);

	x_tco_compound_save(p_base);
	p_stack = x_eval_field_save_stack(p_base);
	p_compound = x_firstobj(p_stack);

	_it_should("the includer's formal is a FRAME cell at the env head",
		(x_obj_flags(p_frame) & X_OBJ_FLAG_FRAME) != 0
		&& x_firstobj(p_binding) == p_sym);

	_load(p_base, src, sizeof(src) - 1);

	_it_should("the env head is the includer's frame again",
		x_firstobj(x_eval_field_env_alist(p_base)) == p_frame);
	_it_should("the save-stack is the includer's again",
		x_eval_field_save_stack(p_base) == p_stack);

	_it_should("the includer's frame cell survived the collect",
		_on_heap_chain(p_base, p_frame));
	_it_should("the includer's binding survived the collect",
		_on_heap_chain(p_base, p_binding));
	_it_should("the includer's save-stack cell survived the collect",
		_on_heap_chain(p_base, p_stack));
	_it_should("the includer's restore compound survived the collect",
		_on_heap_chain(p_base, p_compound));

	/* Only read what the chain says is alive. */
	if (_on_heap_chain(p_base, p_frame) && _on_heap_chain(p_base, p_binding)) {
		_it_should("the frame still binds the formal to its value",
			x_firstobj(p_binding) == p_sym
			&& x_intval(x_restobj(p_binding)) == 7);
	}

	test_cleanup(p_base);
	return NULL;
}

/*
 * The other half of the contract, unchanged: with the includer's state
 * parked, a loaded file's top-level def still lands in the GLOBAL scope,
 * not in the includer's frame -- the reason the state is displaced at all.
 */
static char *test_load_still_binds_top_level_defs_globally(void)
{
	x_obj_t *p_base, *p_sym, *p_frame, *p_entry;
	x_char_t buffer[256];
	static x_char_t src[] = "(def loaded-global 42)\n";

	p_base = init(NULL, buffer);

	p_sym = x_make_symbol(p_base, X_OBJ_FLAG_NONE, (x_char_t *)"includer-local");
	p_frame = x_env_extend(p_base,
		x_firstobj(x_eval_field_env_alist(p_base)),
		x_mklist(p_base, p_sym, NULL),
		x_mklist(p_base, x_mkint(p_base, 7), NULL));
	x_firstobj(x_eval_field_env_alist(p_base)) = p_frame;
	x_tco_compound_save(p_base);

	_load(p_base, src, sizeof(src) - 1);

	_it_should("the includer's frame is still the env head",
		x_firstobj(x_eval_field_env_alist(p_base)) == p_frame);
	_it_should("the loaded def did not extend the includer's frame",
		x_firstobj(x_firstobj(p_frame)) == p_sym);

	p_entry = x_alist_bst_lookup(p_base,
		x_eval_field_env_global_tree(p_base),
		x_make_symbol(p_base, X_OBJ_FLAG_NONE, (x_char_t *)"loaded-global"));
	_it_should("the loaded def is in the global tree",
		!x_obj_isnil(p_base, p_entry)
		&& x_intval(x_restobj(p_entry)) == 42);

	test_cleanup(p_base);
	return NULL;
}

static char *run_tests() {
	_run_test(test_load_keeps_the_includer_across_a_collect);
	_run_test(test_load_still_binds_top_level_defs_globally);

	return NULL;
}
