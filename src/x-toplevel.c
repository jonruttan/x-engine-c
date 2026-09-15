/** @file x-toplevel.c
 *  @brief The top-level bracket around a file's forms and around eval!.
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
#include "x-toplevel.h"
#include "x-eval.h"
#include "x-heap.h"

/**
 * Enter the top-level bracket: what a form evaluated at top level sees,
 * whatever environment is current when it is asked for.
 *
 * A top-level form's `def`s must bind in the root, and the closures it
 * makes must capture the root -- not the environment of whatever was being
 * evaluated when the form was asked for.  Two doors ask: x_eval_load, for
 * every form of a file (`include` runs under whatever called it, and its
 * x-level wrapper is a closure), and eval!, for the one form the REPL loop
 * reads (lib/he.x reaches `(repl)` through `(unless %batch? (do (%banner)
 * (repl)))`, so every form typed at the prompt sits under those frames).
 * The bracket is two moves: the save-stack is hidden (nil), so a form sees
 * an empty stack exactly as at the true top level and each x_eval balances
 * its own pushes; and the root is made current.
 *
 * The displaced state is HEAP: the caller's environment and its save-stack,
 * and for the length of the bracket nothing on the base tree reaches them.
 * A C local is not a root (x-heap.h), so a form that collects (a library
 * collecting between definitions is ordinary) would sweep them, and the
 * caller would walk freed memory on its next lookup.  So both are parked
 * on the root chain in the caller's own struct, one registered node
 * holding the two pointers; the pop is in x_toplevel_leave.  The error
 * path needs nothing more: a guard restores the root chain and the
 * save-stack from its own snapshot, so a longjmp out of a bracketed form
 * drops the node with the C frame that owns it.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_t     x_toplevel_t* -- the caller's bracket state, filled here
 * @see x_toplevel_leave, x_eval_load, x_prim_eval_immediate
 */
void x_toplevel_enter(x_obj_t *p_base, x_toplevel_t *p_t)
{
	int i;
	x_obj_t **pp_root = x_heap_root_slot(p_base);

	p_t->p_saved_stack = x_eval_field_save_stack(p_base);
	x_eval_field_save_stack(p_base) = NULL;

	p_t->p_saved_env = x_eval_field_env(p_base);
	x_eval_field_env(p_base) = x_eval_field_env_root(p_base);

	/* Pair-typed, as the root chain requires: the mark walk descends only
	 * spair pairs.  Built at run time in the caller's struct -- every unit
	 * zeroed, then the type and flags words -- where the static form would
	 * have used the x_obj_set initializer. */
	for (i = 0; i < (int)(X_OBJ_META_LEN + X_OBJ_UNITS_PAIR); i++) {
		p_t->parked[i].i = 0;
	}
	x_obj_type(p_t->parked) = (x_obj_t *)x_type_pair_obj;
	x_obj_flags(p_t->parked) = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)p_t->parked) = p_t->p_saved_env;
	x_restobj((x_obj_t *)p_t->parked) = p_t->p_saved_stack;
	x_heap_root_push(pp_root, p_t->parked);
}

/**
 * Leave the top-level bracket: unroot the parked state and put it back.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_t     x_toplevel_t* -- the state x_toplevel_enter filled
 * @see x_toplevel_enter
 */
void x_toplevel_leave(x_obj_t *p_base, x_toplevel_t *p_t)
{
	x_obj_t **pp_root = x_heap_root_slot(p_base);

	x_heap_root_pop(pp_root);

	x_eval_field_save_stack(p_base) = p_t->p_saved_stack;
	x_eval_field_env(p_base) = p_t->p_saved_env;
}


