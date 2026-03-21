/*
 * # Computational Expressions in C
 *
 * ## x-prim/callcc.c -- Implementation - Primitives - First-class Continuations
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2024 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 *
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
/*
 * # Includes
 */
#include "x-prim.h"
#include "x-base.h"
#include "x-eval.h"
#include <setjmp.h>
#include <string.h>
#include "x-type/list.h"
#include "x-type/prim.h"
#include "x-type/ptr.h"
#include "x-type/procedure.h"
#include "x-type/symbol.h"

/*
 * # Continuation Data
 *
 * Stack-copying continuations: capture the C call stack segment from
 * a known base (set in main) to the current frame. Invocation restores
 * the stack and longjmps back to the capture point.
 *
 * Interpreter state (env, save_stack, error_handler) is saved as a
 * GC-visible x-lang list in the continuation procedure's closure env.
 */

/* Stack base pointer, set once at startup via x_callcc_init. */
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
} x_callcc_cont_t;

/*
 * # Stack Base
 */
void x_callcc_init(void)
{
	volatile int anchor;

	g_stack_base = (void *)&anchor;
}

/*
 * # Stack Restore
 *
 * Recursively grow the stack past the captured segment, then
 * memcpy the saved stack back and longjmp to the capture point.
 */
static void x_callcc_restore(x_callcc_cont_t *cont)
{
	volatile char pad[2048];

	/* Touch pad to prevent compiler from eliminating it. */
	pad[0] = 0;

	/* Stack grows downward: keep recursing until we're past
	 * the lower bound of the captured segment. */
	if ((char *)&pad[0] > (char *)cont->stack_lo) {
		x_callcc_restore(cont);
		return; /* never reached */
	}

	/* Stack is deep enough. Restore captured segment and jump back. */
	memcpy(cont->stack_lo, cont->stack_copy, cont->stack_size);
	longjmp(cont->jmp, 1);
}

/*
 * # Primitives
 */

/* %cc-invoke: (%cc-invoke ptr state args-list) -> restore continuation
 * args-list is from the variadic k: () for 0 args, (val) for 1 arg. */
static x_obj_t *x_prim_cc_invoke(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_ptr, *p_state, *p_args_list;
	p_args = x_restobj(p_args);
	p_ptr = x_prim_eval_arg(p_base, x_firstobj(p_args));
	p_state = x_prim_eval_arg(p_base,
		x_firstobj(x_restobj(p_args)));
	p_args_list = x_prim_eval_arg(p_base,
		x_firstobj(x_restobj(x_restobj(p_args))));
	x_obj_t *p_val = x_obj_isnil(p_base, p_args_list)
		? NULL : x_firstobj(p_args_list);
	x_callcc_cont_t *cont = (x_callcc_cont_t *)x_ptrval(p_ptr);

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

	/* Grow stack and restore. Does not return. */
	x_callcc_restore(cont);

	return NULL; /* unreachable */
}

/* call/cc: (call/cc proc) -> capture continuation, call (proc k) */
static x_obj_t *x_prim_callcc(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_proc, *p_ptr, *p_state, *p_k, *p_result;
	x_obj_t *p_val_sym, *p_ptr_sym, *p_state_sym, *p_invoke_sym;
	x_obj_t *p_params, *p_body, *p_env;
	x_callcc_cont_t *cont;
	char *stack_lo;
	size_t stack_size, total;
	p_args = x_restobj(p_args);

	/* Evaluate the procedure argument. */
	p_proc = x_prim_eval_arg(p_base, x_firstobj(p_args));

	/* Approximate current stack pointer. */
	stack_lo = (char *)&p_proc;
	stack_size = (size_t)((char *)g_stack_base - stack_lo);

	/* Allocate continuation struct + stack copy in one block. */
	total = sizeof(x_callcc_cont_t) + stack_size;
	cont = (x_callcc_cont_t *)x_sys_malloc(total);
	cont->stack_copy = (char *)cont + sizeof(x_callcc_cont_t);
	cont->p_result = NULL;

	if (setjmp(cont->jmp) != 0) {
		/* Continuation was invoked. Restore interpreter state and
		 * return the value passed to the continuation. */
		x_base_field_env_alist(p_base) = cont->p_env_alist;
		x_base_field_env_local_boundary(p_base) = cont->p_local_boundary;
		x_base_field_env_global_tree(p_base) = cont->p_global_tree;
		x_base_field_save_stack(p_base) = cont->p_save_stack;
		x_base_field_error_handler(p_base) = cont->p_error_handler;
		x_base_field_eval_list_stack(p_base) = cont->p_eval_list_stack;
		x_base_field_tco_expr(p_base) = NULL;
		x_base_field_tco_env(p_base) = NULL;

		return cont->p_result;
	}

	/* Capture the stack segment. */
	memcpy(cont->stack_copy, stack_lo, stack_size);
	cont->stack_size = stack_size;
	cont->stack_lo = stack_lo;

	/* Build GC-visible interpreter state list:
	 * (env-alist save-stack error-handler local-boundary global-tree) */
	p_state = x_mklist(p_base,
		x_base_field_env_alist(p_base),
		x_mklist(p_base,
			x_base_field_save_stack(p_base),
			x_mklist(p_base,
				x_base_field_error_handler(p_base),
				x_mklist(p_base,
					x_base_field_env_local_boundary(p_base),
					x_mklist(p_base,
						x_base_field_env_global_tree(p_base),
						x_mklist(p_base,
							x_base_field_eval_list_stack(p_base),
							NULL))))));

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
	p_env = x_mkspair(p_base,
		x_mkspair(p_base, p_ptr_sym, p_ptr),
		x_mkspair(p_base,
			x_mkspair(p_base, p_state_sym, p_state),
			x_base_field_env_alist(p_base)));

	/* Create k as a procedure (fn). */
	p_k = x_mkproc(p_base, p_params, p_body, p_env,
		x_base_field_env_global_tree(p_base));

	/* Call proc(k) using type_prim_apply: (proc k) */
	{
		x_spair_t call_args[2] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE,
				{ p_proc }, { (x_obj_t *)(call_args + 1) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE,
				{ p_k }, { NULL })
		};

		p_result = x_type_prim_apply(p_base, (x_obj_t *)call_args);
	}

	return p_result;
}

/*
 * # Registration
 */
x_obj_t *x_prim_callcc_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_prim_entry_t entries[] = {
		{ "%cc-invoke", x_prim_cc_invoke },
		{ "call/cc", x_prim_callcc }
	};

	x_prim_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
