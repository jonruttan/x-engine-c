/** @file callcc.c
 *  @brief First-class continuation primitives (call/cc via stack copying).
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2026 Jon Ruttan
 *  @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
#include "x-prim.h"
#include "x-eval.h"
#include "x-heap.h"
#include <setjmp.h>
#include <string.h>
#include "x-type/list.h"
#include "x-type/prim.h"
#include "x-type/ptr.h"
#include "x-type/procedure.h"
#include "x-type/symbol.h"

/** @name Continuation Data
 *  @{
 *
 *  Stack-copying continuations: capture the C call stack segment from
 *  a known base (set in main) to the current frame.  Invocation restores
 *  the stack and longjmps back to the capture point.
 *
 *  Interpreter state (env, save_stack, error_handler) is saved as a
 *  GC-visible x-lang list in the continuation procedure's closure env.
 */

/** Stack base pointer, set once at startup via x_callcc_init(). */
static void *g_stack_base = NULL;

typedef struct {
	jmp_buf     jmp;
	void       *stack_copy;       /* malloc'd copy of captured stack */
	size_t      stack_size;
	void       *stack_lo;         /* lower address bound of captured stack */
	x_obj_t    *p_result;         /* value passed when continuation invoked */
	x_obj_t    *p_env_alist;      /* interpreter state at invocation time */
	x_obj_t    *p_save_stack;
	x_obj_t    *p_error_handler;
	x_obj_t    *p_local_boundary;
	x_obj_t    *p_global_tree;
	x_obj_t    *p_eval_list_stack;
	x_obj_t    *p_root_chain;     /* GC root-chain head; points into the
	                               * captured segment, valid only after
	                               * the stack bytes are restored */
} x_callcc_cont_t;

/** @} */

/** Establish the stack base address for continuation capture.
 *
 *  Must be called once at interpreter startup, from a frame close to the
 *  bottom of the C call stack (typically @c main).  The address of a local
 *  variable is recorded as the upper bound of all future stack captures.
 *
 *  @note Uses a volatile local to prevent the compiler from eliminating
 *        the anchor variable.
 */
void x_callcc_init(void)
{
	volatile int anchor;

	g_stack_base = (void *)&anchor;
}

/** Store an address strictly below the caller's entire stack frame.
 *
 *  The capture in x_prim_callcc() must include EVERY slot of its own
 *  frame: locals the compiler spills below the address taken (gcc puts
 *  p_base/cont there) would otherwise be restored as garbage after
 *  longjmp -- the crash class that only clang's register allocation
 *  hid.  A callee's local is below the caller's whole frame by
 *  construction, so its address is a safe lower bound.
 *
 *  @note Out-parameter, not a return value: gcc's -Wreturn-local-addr
 *        pass REPLACES a returned local address with NULL at -O; a
 *        store through a pointer (same idiom as x_callcc_init) is only
 *        warned about, never rewritten.
 */
static void x_callcc_sp(void **pp_sp)
{
	volatile char anchor;

	*pp_sp = (void *)&anchor;
}

/** Volatile call path to x_callcc_sp(): inlining would hoist the anchor
 *  back into the caller's frame and re-create the under-capture; a call
 *  through a volatile pointer cannot be inlined or sibling-call folded. */
static void (*volatile x_callcc_sp_fn)(void **) = x_callcc_sp;

/** Restore a captured continuation by replacing the current stack.
 *
 *  Recursively grows the C stack (via a 2 KiB pad per frame) until the
 *  stack pointer is below the lower bound of the captured segment, then
 *  copies the saved bytes back and longjmps to the capture point.
 *
 *  @param cont Continuation whose stack segment and jmp_buf to restore.
 *  @note This function never returns.
 */
static void x_callcc_restore(x_callcc_cont_t *cont);

/** Volatile call path for the recursion: sibling-call optimization would
 *  reuse the frame, so the pad would never advance and the descent would
 *  spin forever.  A call through a volatile pointer cannot be folded. */
static void (*volatile x_callcc_restore_fn)(x_callcc_cont_t *) =
	x_callcc_restore;

static void x_callcc_restore(x_callcc_cont_t *cont)
{
	volatile char pad[2048];

	/* Touch pad to prevent compiler from eliminating it. */
	pad[0] = 0;

	/* Stack grows downward: keep recursing until the WHOLE frame --
	 * not just pad[0]; the compiler may place cont's spill slot and the
	 * return address above the array -- clears the captured segment by
	 * a full pad width.  Otherwise the memcpy below could overwrite
	 * frame slots this call still needs (cont, for the longjmp). */
	if ((char *)&pad[0] > (char *)cont->stack_lo - 2 * (long)sizeof(pad)) {
		x_callcc_restore_fn(cont);
		return; /* never reached */
	}

	/* Stack is deep enough. Restore captured segment and jump back. */
	memcpy(cont->stack_lo, cont->stack_copy, cont->stack_size);
	longjmp(cont->jmp, 1);
}

/** Invoke a captured continuation, restoring its stack and interpreter state.
 *  x-lang: (%cc-invoke ptr state args-list)
 *
 *  This is the internal entry point called by the closure @c k that
 *  @c call/cc hands to its procedure.  @p args-list is the variadic
 *  argument collector: @c () for zero arguments, @c (val) for one.
 *
 *  @param p_base Interpreter base context.
 *  @param p_args Unevaluated argument list: (%cc-invoke ptr state args-list).
 *  @return Does not return; transfers control via longjmp.
 *  @see x_prim_callcc, x_callcc_restore
 */
static x_obj_t *x_prim_cc_invoke(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_ptr, *p_state, *p_args_list, *p_passed, *p_val;
	x_callcc_cont_t *cont;
	x_eargs(p_base, p_args, 4, NULL, &p_ptr, &p_state, &p_args_list);
	/* %cc-args is bare-variadic, so it binds the full self-passed
	 * list (k val ...): the continuation's value sits after the self
	 * slot.  (k) with no value passes nil through. */
	p_passed = x_obj_isnil(p_base, p_args_list)
		? NULL : x_restobj(p_args_list);
	p_val = x_obj_isnil(p_base, p_passed)
		? NULL : x_firstobj(p_passed);
	cont = (x_callcc_cont_t *)x_ptrval(p_ptr);

	/* Store the return value and interpreter state in cont.
	 * State is extracted from the GC-visible list:
	 * (env-alist save-stack error-handler) */
	cont->p_result = p_val;
	cont->p_env_alist = x_firstobj(p_state);
	cont->p_save_stack = x_firstobj(x_restobj(p_state));
	cont->p_error_handler = x_firstobj(x_restobj(x_restobj(p_state)));
	cont->p_local_boundary = x_firstobj(x_restobj(x_restobj(x_restobj(p_state))));
	cont->p_global_tree = x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(p_state)))));
	cont->p_eval_list_stack = x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(p_state))))));
	cont->p_root_chain = (x_obj_t *)x_ptrval(
		x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(p_state))))))));

	/* Grow stack and restore. Does not return. */
	x_callcc_restore(cont);

	return NULL; /* unreachable */
}

/** Capture the current continuation and pass it to a procedure.
 *  x-lang: (call/cc proc)
 *
 *  Allocates a continuation struct with a copy of the C stack segment
 *  from the current frame up to @c g_stack_base, saves interpreter state
 *  in a GC-visible list, and constructs a variadic closure @c k that
 *  invokes @c %cc-invoke when called.  Then applies @p proc to @c k.
 *
 *  When @c k is later invoked, the saved stack is restored via
 *  x_callcc_restore() and control returns here through longjmp, yielding
 *  the value passed to @c k.
 *
 *  @param p_base Interpreter base context.
 *  @param p_args Unevaluated argument list: (call/cc proc).
 *  @return The result of @p proc, or the value passed to the continuation.
 *  @see x_prim_cc_invoke, x_callcc_restore
 */
static x_obj_t *x_prim_callcc(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_proc, *p_ptr, *p_state, *p_k, *p_result;
	x_obj_t *p_val_sym, *p_ptr_sym, *p_state_sym, *p_invoke_sym;
	x_obj_t *p_params, *p_body, *p_env;
	x_callcc_cont_t *cont;
	char *stack_lo;
	void *sp_lo;
	size_t stack_size, total;
	x_spair_t call_args[2];

	/* Evaluate the procedure argument. */
	x_eargs(p_base, p_args, 2, NULL, &p_proc);

	/* Lower capture bound: from a callee frame, so it sits below every
	 * slot of THIS frame (see x_callcc_sp) -- &local would miss the
	 * slots the compiler placed under it. */
	x_callcc_sp_fn(&sp_lo);
	stack_lo = (char *)sp_lo;
	stack_size = (size_t)((char *)g_stack_base - stack_lo);

	/* Allocate continuation struct + stack copy in one block. */
	total = sizeof(x_callcc_cont_t) + stack_size;
	cont = (x_callcc_cont_t *)x_sys_malloc(total);
	cont->stack_copy = (char *)cont + sizeof(x_callcc_cont_t);
	cont->p_result = NULL;

	if (setjmp(cont->jmp) != 0) {
		/* Continuation was invoked. Restore interpreter state and
		 * return the value passed to the continuation. */
		x_firstobj(x_eval_field_env_alist(p_base)) = cont->p_env_alist;
		x_eval_field_env_local_boundary(p_base) = cont->p_local_boundary;
		x_eval_field_env_global_tree(p_base) = cont->p_global_tree;
		x_eval_field_save_stack(p_base) = cont->p_save_stack;
		x_firstobj(x_eval_field_error_handler(p_base)) = cont->p_error_handler;
		x_firstobj(x_eval_field_eval_list(p_base)) = cont->p_eval_list_stack;
		/* The chain's nodes live in the restored stack bytes, so the
		 * head is valid again exactly here -- after the memcpy in
		 * x_callcc_restore, never while the continuation is dormant. */
		x_heap_root_chain(p_base) = cont->p_root_chain;
		x_firstobj(x_eval_field_tco_expr(p_base)) = NULL;
		x_firstobj(x_eval_field_tco_env(p_base)) = NULL;

		return cont->p_result;
	}

	/* Capture the stack segment. */
	memcpy(cont->stack_copy, stack_lo, stack_size);
	cont->stack_size = stack_size;
	cont->stack_lo = stack_lo;

	/* Build GC-visible interpreter state list:
	 * (env-alist save-stack error-handler local-boundary global-tree
	 *  eval-list root-chain).  The root-chain head is wrapped as an
	 * opaque ptr atom: its nodes are stack memory, dead while the
	 * continuation is dormant, so tree-marking this state list must
	 * not traverse them. */
	p_state = x_mklist(p_base,
		x_firstobj(x_eval_field_env_alist(p_base)),
		x_mklist(p_base,
			x_eval_field_save_stack(p_base),
			x_mklist(p_base,
				x_firstobj(x_eval_field_error_handler(p_base)),
				x_mklist(p_base,
					x_eval_field_env_local_boundary(p_base),
					x_mklist(p_base,
						x_eval_field_env_global_tree(p_base),
						x_mklist(p_base,
							x_firstobj(x_eval_field_eval_list(p_base)),
							x_mklist(p_base,
								x_mkptr(p_base,
									(void *)x_heap_root_chain(p_base)),
								NULL)))))));

	/* Wrap continuation struct as POINTER with OWN flag.
	 * GC will free the struct (and embedded stack copy). */
	p_ptr = x_mkptrown(p_base, cont);

	/* Build k = (fn %cc-args (%cc-invoke %cc-ptr %cc-state %cc-args))
	 * Variadic: %cc-args collects all args as a list.
	 * %cc-ptr and %cc-state captured in k's closure env. */
	p_val_sym = x_mksymbol(p_base, (x_char_t *)"%cc-args");
	p_ptr_sym = x_mksymbol(p_base, (x_char_t *)"%cc-ptr");
	p_state_sym = x_mksymbol(p_base, (x_char_t *)"%cc-state");
	p_invoke_sym = x_mksymbol(p_base, (x_char_t *)"%cc-invoke");

	/* params: %cc-args (variadic — single symbol, not a list) */
	p_params = p_val_sym;

	/* body: ((%cc-invoke %cc-ptr %cc-state %cc-args)) */
	p_body = x_mklist(p_base,
		x_mklist(p_base, p_invoke_sym,
			x_mklist(p_base, p_ptr_sym,
				x_mklist(p_base, p_state_sym,
					x_mklist(p_base, p_val_sym, NULL)))),
		NULL);

	/* env: extend current env with %cc-ptr and %cc-state */
	p_env = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_ptr_sym, p_ptr),
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, p_state_sym, p_state),
			x_firstobj(x_eval_field_env_alist(p_base))));

	/* Create k as a procedure (fn). */
	p_k = x_mkproc(p_base, p_params, p_body, p_env,
		x_eval_field_env_global_tree(p_base));

	/* Call proc(k) using type_prim_apply: (proc k) */
	call_args[0][X_OBJ_META_TYPE].p = NULL;
	call_args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)call_args) = p_proc;
	x_restobj((x_obj_t *)call_args) = (x_obj_t *)(call_args + 1);
	call_args[1][X_OBJ_META_TYPE].p = NULL;
	call_args[1][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)(call_args + 1)) = p_k;
	x_restobj((x_obj_t *)(call_args + 1)) = NULL;

	p_result = x_callable_apply(p_base, (x_obj_t *)call_args);

	return p_result;
}

/** Register continuation primitives.
 *
 *  Binds: @c %cc-invoke, @c call/cc.
 *
 *  @param p_base Interpreter base context.
 *  @param p_args Unused.
 *  @return @p p_base.
 */
x_obj_t *x_prim_callcc_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_prim_entry_t entries[] = {
		{ "%cc-invoke", x_prim_cc_invoke, NULL,   NULL      },
		{ "call/cc",    x_prim_callcc,    "ctrl", "call/cc" }
	};

	x_prims_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
