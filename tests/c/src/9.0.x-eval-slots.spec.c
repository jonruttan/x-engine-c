/*
 * # Unit Tests: *x-eval-slots*
 */

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
#include "src/x-vector.c"
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
#include "src/x-prim/type.c"
#include "src/x-type/vector.c"
#include "src/x-prim/base.c"
#include "src/x-prim/buffer.c"
#include "src/x-prim/iter.c"

/* Stubs for primitives not under test. */
x_obj_t *x_prim_core_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_arith_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_pred_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_string_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_io_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_heap_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_image_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_ffi_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_callcc_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_binding_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_closure_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_control_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_quote_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }

/* Wrap direct prim call with NULL self (self-passing convention) */
#define PCALL(fn, base, args) \
	fn((base), x_mkspair((base), X_OBJ_FLAG_NONE, NULL, (args)))
#define PCALL0(fn, base) \
	fn((base), x_mkspair((base), X_OBJ_FLAG_NONE, NULL, NULL))



/*
 * ## Test Overhead
 */

static void _setup(void)
{
	_buffer_index = -1;
	/* Use system allocator: unified callable layout creates more objects
	 * than the 1024-slot guaranteed pool can handle when registering
	 * prims on multiple bases. */
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
	x_obj_t *p_gc = p_base, *p_tmp;

	while (p_gc) {
		p_tmp = x_obj_heap(p_gc);
		x_sys_free(p_gc);
		p_gc = p_tmp;
	}
}


/*
 * ## Test Helpers
 */

static x_obj_t *test_make_base(void)
{
	x_obj_t *p_base = x_eval_make(NULL, NULL);

	x_prim_register(p_base, NULL);

	return p_base;
}

/* Call slot I of base B with argument vector A. */
#define SLOT(B,I,A)		x_eval_slot((B), (I))((B), (A))

static int _replaced_called;
static x_obj_t *_replaced_args;
static x_satom_t _replaced_answer = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = 99 });

static x_obj_t *_replaced_fn(x_obj_t *p_base, x_obj_t *p_args)
{
	_replaced_called++;
	_replaced_args = p_args;

	return (x_obj_t *)_replaced_answer;
}

static x_obj_t *_prim_fn(x_obj_t *p_base, x_obj_t *p_args)
{
	return (x_obj_t *)_replaced_answer;
}


/*
 * ## Test Runners
 */

static char *test_slots_positions(void)
{
	_it_should("number the slots from 0",
		0 == X_SLOT_EVAL
		&& 1 == X_SLOT_EVAL_LIST
		&& 2 == X_SLOT_EVAL_BODY
		&& 3 == X_SLOT_EVAL_BODY_TCO
		&& 4 == X_SLOT_EVAL_TCO_TRAMPOLINE
		&& 5 == X_SLOT_EVAL_OP_BODY
		&& 6 == X_SLOT_CALLABLE_CALL
		&& 7 == X_SLOT_CALLABLE_APPLY
		&& 8 == X_SLOT_OBJ_PRIM_CALL
		&& 9 == X_SLOT_ENV_LOOKUP
		&& 10 == X_SLOT_ENV_BIND
		&& 11 == X_SLOT_ENV_EXTEND
		&& 12 == X_SLOT_ALIST_BST_LOOKUP
		&& 13 == X_SLOT_TOKEN_READ
		&& 14 == X_SLOT_TOKEN_ANALYSE
		&& 15 == X_SLOT_TOKEN_DELIMIT
		&& 16 == X_SLOT_LEN
	);

	return NULL;
}

static char *test_slots_make(void)
{
	x_obj_t *p_base;
	x_int_t i, set;

	p_base = x_eval_make(NULL, NULL);

	_it_should("hold the slot vector in the base's slots field",
		x_eval_slots_isset(p_base)
		&& x_eval_slots(p_base) == x_eval_field_slots(p_base)
	);

	_it_should("make a base with the routines set",
		x_eval == x_eval_slot(p_base, X_SLOT_EVAL)
		&& x_env_lookup == x_eval_slot(p_base, X_SLOT_ENV_LOOKUP)
		&& x_alist_bst_lookup == x_eval_slot(p_base, X_SLOT_ALIST_BST_LOOKUP)
		&& x_callable_call == x_eval_slot(p_base, X_SLOT_CALLABLE_CALL)
		&& x_token_read == x_eval_slot(p_base, X_SLOT_TOKEN_READ)
	);

	for (set = 0, i = 0; i < X_SLOT_LEN; i++) {
		if (x_eval_slot_isset(p_base, i)) {
			set++;
		}
	}

	_it_should("make the slot vector a vector of the engine's length",
		X_SLOT_LEN == x_vectorlength(x_eval_slots(p_base))
	);

	_it_should("fill every slot when the base is made",
		X_SLOT_LEN == set
	);

	_it_should("keep x-expr's hooks where x-expr looks for them",
		x_type_prim_type_name
			== x_atomfn(x_firstobj(x_base_field_hook_type_name(p_base)))
		&& x_type_prim_units
			== x_atomfn(x_firstobj(x_base_field_hook_units(p_base)))
		&& x_type_prim_length
			== x_atomfn(x_firstobj(x_base_field_hook_length(p_base)))
	);

	test_cleanup(p_base);
	return NULL;
}

static char *test_slots_eval(void)
{
	x_obj_t *p_base, *p_int, *p_list, *p_ret, *p_direct;
	x_obj_t args[2] = { { .p = NULL }, { .p = NULL } };

	p_base = test_make_base();
	p_int = x_mkint(p_base, (x_int_t)42);

	args[0].p = p_int;
	_it_should("evaluate through the eval slot",
		p_int == SLOT(p_base, X_SLOT_EVAL, args)
	);

	p_list = x_mklist(p_base, p_int, x_mklist(p_base, p_int, NULL));
	args[0].p = p_list;
	p_ret = SLOT(p_base, X_SLOT_EVAL_LIST, args);
	_it_should("evaluate a list through the eval-list slot",
		p_ret != NULL
		&& p_int == x_firstobj(p_ret)
		&& p_int == x_firstobj(x_restobj(p_ret))
		&& NULL == x_restobj(x_restobj(p_ret))
	);

	args[0].p = p_list;
	_it_should("evaluate a body through the eval-body slot",
		p_int == SLOT(p_base, X_SLOT_EVAL_BODY, args)
	);

	/* The tail of a body is deferred, and the trampoline delivers it.  The
	 * body routine is called as a procedure call calls it: with the
	 * caller's environment saved. */
	x_tco_env_save(p_base);
	p_direct = x_eval_tco_trampoline(p_base, x_argrun({ .p = x_eval_body_tco(p_base, x_argrun({ .p = p_list })) }));
	x_tco_env_save(p_base);
	args[0].p = p_list;
	p_ret = SLOT(p_base, X_SLOT_EVAL_BODY_TCO, args);
	args[0].p = p_ret;
	p_ret = SLOT(p_base, X_SLOT_EVAL_TCO_TRAMPOLINE, args);
	_it_should("evaluate a body's tail through the body-tco and trampoline slots",
		p_int == p_ret && p_direct == p_ret
	);

	/* An operative's body leaves its tail and the caller's environment
	 * for the trampoline. */
	args[0].p = p_list;
	args[1].p = x_eval_field_env(p_base);
	p_ret = SLOT(p_base, X_SLOT_EVAL_OP_BODY, args);
	_it_should("leave an operative body's tail for the trampoline through the op-body slot",
		NULL == p_ret
		&& p_int == x_firstobj(x_eval_field_tco_expr(p_base))
		&& x_eval_field_env(p_base) == x_firstobj(x_eval_field_tco_env(p_base))
	);
	x_firstobj(x_eval_field_tco_expr(p_base)) = NULL;
	x_firstobj(x_eval_field_tco_env(p_base)) = NULL;

	test_cleanup(p_base);
	return NULL;
}

static char *test_slots_call(void)
{
	x_obj_t *p_base, *p_prim, *p_call;
	x_obj_t args[1] = { { .p = NULL } };

	p_base = test_make_base();
	p_prim = x_make_prim(p_base, X_OBJ_FLAG_NONE, _prim_fn);
	p_call = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_prim, NULL);
	args[0].p = p_call;

	_it_should("call through the callable-call slot",
		(x_obj_t *)_replaced_answer == SLOT(p_base, X_SLOT_CALLABLE_CALL, args)
		&& x_callable_call(p_base, x_argrun({ .p = p_call })) == SLOT(p_base, X_SLOT_CALLABLE_CALL, args)
	);

	_it_should("apply through the callable-apply slot",
		(x_obj_t *)_replaced_answer == SLOT(p_base, X_SLOT_CALLABLE_APPLY, args)
		&& x_callable_apply(p_base, x_argrun({ .p = p_call })) == SLOT(p_base, X_SLOT_CALLABLE_APPLY, args)
	);

	_it_should("call an object's type through the obj-prim-call slot",
		x_obj_prim_call(p_base, x_argrun({ .p = p_call })) == SLOT(p_base, X_SLOT_OBJ_PRIM_CALL, args)
	);

	test_cleanup(p_base);
	return NULL;
}

static char *test_slots_env(void)
{
	x_obj_t *p_base, *p_env, *p_child, *p_sym, *p_param, *p_val, *p_cell;
	x_obj_t args[3] = { { .p = NULL }, { .p = NULL }, { .p = NULL } };

	p_base = test_make_base();
	p_env = x_eval_field_env(p_base);
	p_sym = x_mksymbol(p_base, "slot-spec-name");
	p_param = x_mksymbol(p_base, "slot-spec-param");
	p_val = x_mkint(p_base, (x_int_t)7);

	args[0].p = p_env;
	args[1].p = p_sym;
	_it_should("find nothing through the env-lookup slot before the name is bound",
		NULL == SLOT(p_base, X_SLOT_ENV_LOOKUP, args)
	);

	args[2].p = p_val;
	_it_should("bind through the env-bind slot",
		p_val == SLOT(p_base, X_SLOT_ENV_BIND, args)
	);

	p_cell = SLOT(p_base, X_SLOT_ENV_LOOKUP, args);
	_it_should("find the binding through the env-lookup slot",
		p_cell != NULL
		&& p_val == x_restobj(p_cell)
		&& p_cell == x_env_lookup(p_base, x_argrun({ .p = p_env }, { .p = p_sym }))
	);

	args[0].p = x_env_bindings(p_env);
	_it_should("find the binding in the root's tree through the bst-lookup slot",
		p_cell == SLOT(p_base, X_SLOT_ALIST_BST_LOOKUP, args)
	);

	args[0].p = p_env;
	args[1].p = x_mklist(p_base, p_param, NULL);
	args[2].p = x_mklist(p_base, p_val, NULL);
	p_child = SLOT(p_base, X_SLOT_ENV_EXTEND, args);
	p_cell = x_env_lookup(p_base, x_argrun({ .p = p_child }, { .p = p_param }));
	_it_should("make a child environment through the env-extend slot",
		p_child != NULL
		&& p_env == x_env_parent(p_child)
		&& p_cell != NULL
		&& p_val == x_restobj(p_cell)
	);

	test_cleanup(p_base);
	return NULL;
}



/* A buffer holding one character of input, read and not yet consumed. */
static x_obj_t *test_token_buffer(x_obj_t *p_base, char *input)
{
	x_obj_t *p_buffer;

	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = input;
	helper_file_buffer_remaining[TEST_HELPER_FILE_STDIN] = x_lib_strlen(input);
	helper_file_reset();
	p_buffer = x_mkbufferown(p_base, (x_char_t *)x_sys_malloc(X_READ_BUF_SIZE));
	x_type_buffer_read(p_base, x_mkspair(p_base, X_OBJ_FLAG_NONE, p_buffer, NULL));

	return p_buffer;
}

static char *test_slots_token(void)
{
	x_obj_t *p_base, *p_buffer, *p_read, *p_direct;
	x_char_t *buffer;
	x_obj_t args[2] = { { .p = NULL }, { .p = NULL } };
	x_spair_t delimit_args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { (x_obj_t *)(delimit_args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL }),
	};

	p_base = test_make_base();

	/* Empty input: every routine has one answer for it, and the slot
	 * function gives the routine's. */
	helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = "";
	helper_file_buffer_remaining[TEST_HELPER_FILE_STDIN] = 0;
	helper_file_reset();

	buffer = (x_char_t *)x_sys_malloc(X_READ_BUF_SIZE);
	p_buffer = x_mkbufferown(p_base, buffer);
	p_read = x_mkpair(p_base, p_buffer, p_base);

	args[0].p = p_read;
	_it_should("read the end of input through the token-read slot",
		(x_obj_t *)x_token_eof_prim == SLOT(p_base, X_SLOT_TOKEN_READ, args)
		&& x_token_read(p_base, x_argrun({ .p = p_read })) == SLOT(p_base, X_SLOT_TOKEN_READ, args)
	);

	/* The analysis stores the label it declares in the run's second word. */
	args[1].i = 7;
	_it_should("analyse the end of input through the token-analyse slot",
		NULL == SLOT(p_base, X_SLOT_TOKEN_ANALYSE, args)
		&& 0 == args[1].i
	);

	/* The end of input stays the answer of a base that has read it, until
	 * its input is given back. */
	x_atomint(x_firstobj(x_base_field_filein(p_base))) = STDIN_FILENO;

	/* A delimiter moves its buffer's read position back, so each call is
	 * given a buffer of its own holding the same character. */
	args[0].p = (x_obj_t *)delimit_args;

	p_buffer = test_token_buffer(p_base, ")");
	x_firstobj((x_obj_t *)delimit_args) = p_buffer;
	p_direct = x_token_delimit(p_base, x_argrun({ .p = (x_obj_t *)delimit_args }));
	_it_should("find a delimiter by the routine",
		p_buffer == p_direct
	);

	p_buffer = test_token_buffer(p_base, ")");
	x_firstobj((x_obj_t *)delimit_args) = p_buffer;
	_it_should("find a delimiter through the token-delimit slot",
		p_buffer == SLOT(p_base, X_SLOT_TOKEN_DELIMIT, args)
	);

	p_buffer = test_token_buffer(p_base, "A");
	x_firstobj((x_obj_t *)delimit_args) = p_buffer;
	p_direct = x_token_delimit(p_base, x_argrun({ .p = (x_obj_t *)delimit_args }));
	p_buffer = test_token_buffer(p_base, "A");
	x_firstobj((x_obj_t *)delimit_args) = p_buffer;
	_it_should("answer for a letter through the token-delimit slot as the routine does",
		(NULL == p_direct)
			== (NULL == SLOT(p_base, X_SLOT_TOKEN_DELIMIT, args))
	);

	test_cleanup(p_base);
	return NULL;
}

static char *test_slots_replace(void)
{
	x_obj_t *p_base, *p_int;
	x_fn_t was;
	x_obj_t args[1] = { { .p = NULL } };

	p_base = test_make_base();
	p_int = x_mkint(p_base, (x_int_t)42);
	args[0].p = p_int;

	was = x_eval_slot(p_base, X_SLOT_EVAL);
	x_eval_slot(p_base, X_SLOT_EVAL) = _replaced_fn;
	_replaced_called = 0;
	_replaced_args = NULL;

	_it_should("call the function a slot was given",
		(x_obj_t *)_replaced_answer == SLOT(p_base, X_SLOT_EVAL, args)
		&& 1 == _replaced_called
		&& (x_obj_t *)args == _replaced_args
	);

	_it_should("leave the other slots as they were",
		x_eval_list == x_eval_slot(p_base, X_SLOT_EVAL_LIST)
		&& x_eval_body == x_eval_slot(p_base, X_SLOT_EVAL_BODY)
	);

	x_eval_slot(p_base, X_SLOT_EVAL) = was;

	_it_should("call the engine's routine again once the slot is put back",
		p_int == SLOT(p_base, X_SLOT_EVAL, args)
		&& 1 == _replaced_called
	);

	test_cleanup(p_base);
	return NULL;
}

static char *test_slots_vector_type(void)
{
	x_obj_t *p_base, *p_type, *p_a, *p_b, *p_vector;

	p_base = test_make_base();
	p_type = x_type_vector_register(p_base, p_base);

	_it_should("leave the base's slot vector untyped, so the mark does not follow its elements",
		p_type != NULL
		&& NULL == x_obj_type(x_eval_slots(p_base))
	);

	p_a = x_mkint(p_base, (x_int_t)1);
	p_b = x_mkint(p_base, (x_int_t)2);
	p_vector = x_mkvector(p_base, 2, p_a, p_b);
	_it_should("make a vector of the objects it is given",
		x_obj_type_isvector(p_base, p_vector)
		&& 2 == x_vectorlength(p_vector)
		&& p_a == x_vectorobj(p_vector, 0)
		&& p_b == x_vectorobj(p_vector, 1)
	);

	_it_should("count a vector's units as its length and its elements",
		x_vector_units(2) == x_obj_units(p_base, p_vector)
	);

	/* The collector follows a vector's elements and its length. */
	x_heap_tree_mark(p_base, p_vector, X_OBJ_FLAG_MARK);
	_it_should("mark a vector's elements",
		X_OBJ_FLAG_MARK == (x_obj_flags(p_a) & X_OBJ_FLAG_MARK)
		&& X_OBJ_FLAG_MARK == (x_obj_flags(p_b) & X_OBJ_FLAG_MARK)
		&& X_OBJ_FLAG_MARK
			== (x_obj_flags(x_vectorlengthobj(p_vector)) & X_OBJ_FLAG_MARK)
	);

	test_cleanup(p_base);
	return NULL;
}

static char *test_slots_child_base(void)
{
	x_obj_t *p_base, *p_child;
	x_int_t i, set;

	p_base = test_make_base();
	p_child = PCALL0(x_prim_make_base, p_base);

	_it_should("give a child base a slot vector of its own",
		p_child != NULL
		&& x_eval_slots(p_child) != NULL
		&& x_eval_slots(p_child) != x_eval_slots(p_base)
	);

	for (set = 0, i = 0; i < X_SLOT_LEN; i++) {
		if (x_eval_slot_isset(p_child, i)
				&& x_eval_slot(p_child, i) == x_eval_slot(p_base, i)) {
			set++;
		}
	}

	_it_should("fill a child base's slots with the engine's routines",
		X_SLOT_LEN == set
	);

	x_eval_slot(p_child, X_SLOT_EVAL) = _replaced_fn;

	_it_should("leave the parent's slot as it was when a child's is replaced",
		x_eval == x_eval_slot(p_base, X_SLOT_EVAL)
	);

	test_cleanup(p_child);
	test_cleanup(p_base);
	return NULL;
}

static char *run_tests() {
	_run_test(test_slots_positions);
	_run_test(test_slots_make);
	_run_test(test_slots_eval);
	_run_test(test_slots_call);
	_run_test(test_slots_env);
	_run_test(test_slots_token);
	_run_test(test_slots_replace);
	_run_test(test_slots_vector_type);
	_run_test(test_slots_child_base);

	return NULL;
}
