/** @file x-eval-slots.c
 *  @brief The engine's slots -- the slot functions for the evaluator's
 *         routines, and the table that fills a base's slot vector.
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
#include "x-eval-slots.h"
#include "x-eval.h"
#include "x-env.h"
#include "x-alist.h"
#include "x-heap.h"
#include "x-token.h"
#include "x-obj/prim.h"
#include "x-type/prim.h"

/*
 * # Objects and the Heap
 */

/**
 * Slot function for x_obj_alloc().
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Argument vector: (type, flags, units)
 * @return x_obj_t* -- What x_obj_alloc() returns
 */
x_obj_t *x_slot_obj_alloc(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_obj_alloc(p_base,
		x_vectorobj(p_args, 0),
		(x_obj_flag_t)x_atomint(x_vectorobj(p_args, 1)),
		(size_t)x_atomint(x_vectorobj(p_args, 2)));
}

/**
 * Slot function for x_obj_free().
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Argument vector: (object)
 * @return x_obj_t* -- NULL
 */
x_obj_t *x_slot_obj_free(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_free(p_base, x_vectorobj(p_args, 0));

	return NULL;
}

/**
 * Slot function for x_heap_tree_mark().
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Argument vector: (object, flags)
 * @return x_obj_t* -- What x_heap_tree_mark() returns
 */
x_obj_t *x_slot_heap_tree_mark(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_heap_tree_mark(p_base,
		x_vectorobj(p_args, 0),
		(x_obj_flag_t)x_atomint(x_vectorobj(p_args, 1)));
}

/**
 * Slot function for x_heap_sweep().
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Argument vector: (object, flags)
 * @return x_obj_t* -- What x_heap_sweep() returns
 */
x_obj_t *x_slot_heap_sweep(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_heap_sweep(p_base,
		x_vectorobj(p_args, 0),
		(x_obj_flag_t)x_atomint(x_vectorobj(p_args, 1)));
}

/**
 * Slot function for x_heap_root_chain_mark().
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Argument vector: (flags)
 * @return x_obj_t* -- NULL
 */
x_obj_t *x_slot_heap_root_chain_mark(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_heap_root_chain_mark(p_base,
		(x_obj_flag_t)x_atomint(x_vectorobj(p_args, 0)));
}

/*
 * # The Evaluator
 */

/**
 * Slot function for x_eval().
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Argument vector: (args)
 * @return x_obj_t* -- What x_eval() returns
 */
x_obj_t *x_slot_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_eval(p_base, x_vectorobj(p_args, 0));
}

/**
 * Slot function for x_eval_arg().
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Argument vector: (arg)
 * @return x_obj_t* -- What x_eval_arg() returns
 */
x_obj_t *x_slot_eval_arg(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_eval_arg(p_base, x_vectorobj(p_args, 0));
}

/**
 * Slot function for x_eval_list().
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Argument vector: (args)
 * @return x_obj_t* -- What x_eval_list() returns
 */
x_obj_t *x_slot_eval_list(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_eval_list(p_base, x_vectorobj(p_args, 0));
}

/**
 * Slot function for x_eval_body().
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Argument vector: (body)
 * @return x_obj_t* -- What x_eval_body() returns
 */
x_obj_t *x_slot_eval_body(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_eval_body(p_base, x_vectorobj(p_args, 0));
}

/**
 * Slot function for x_eval_body_tco().
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Argument vector: (body)
 * @return x_obj_t* -- What x_eval_body_tco() returns
 */
x_obj_t *x_slot_eval_body_tco(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_eval_body_tco(p_base, x_vectorobj(p_args, 0));
}

/**
 * Slot function for x_eval_tco_trampoline().
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Argument vector: (result)
 * @return x_obj_t* -- What x_eval_tco_trampoline() returns
 */
x_obj_t *x_slot_eval_tco_trampoline(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_eval_tco_trampoline(p_base, x_vectorobj(p_args, 0));
}

/**
 * Slot function for x_eval_op_body().
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Argument vector: (body, caller)
 * @return x_obj_t* -- What x_eval_op_body() returns
 */
x_obj_t *x_slot_eval_op_body(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_eval_op_body(p_base,
		x_vectorobj(p_args, 0),
		x_vectorobj(p_args, 1));
}

/*
 * # Calling
 */

/**
 * Slot function for x_callable_call().
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Argument vector: (args)
 * @return x_obj_t* -- What x_callable_call() returns
 */
x_obj_t *x_slot_callable_call(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_callable_call(p_base, x_vectorobj(p_args, 0));
}

/**
 * Slot function for x_callable_apply().
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Argument vector: (args)
 * @return x_obj_t* -- What x_callable_apply() returns
 */
x_obj_t *x_slot_callable_apply(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_callable_apply(p_base, x_vectorobj(p_args, 0));
}

/**
 * Slot function for x_obj_prim_call().
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Argument vector: (args)
 * @return x_obj_t* -- What x_obj_prim_call() returns
 */
x_obj_t *x_slot_obj_prim_call(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_obj_prim_call(p_base, x_vectorobj(p_args, 0));
}

/*
 * # Environments
 */

/*
 * # The Reader
 */

/**
 * Slot function for x_token_read().
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Argument vector: (args)
 * @return x_obj_t* -- What x_token_read() returns
 */
x_obj_t *x_slot_token_read(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_token_read(p_base, x_vectorobj(p_args, 0));
}

/**
 * Slot function for x_token_analyse().
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Argument vector: (args, pointer to the label)
 * @return x_obj_t* -- What x_token_analyse() returns
 */
x_obj_t *x_slot_token_analyse(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_token_analyse(p_base,
		x_vectorobj(p_args, 0),
		(x_int_t *)x_atomptr(x_vectorobj(p_args, 1)));
}

/**
 * Slot function for x_token_delimit().
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Argument vector: (args)
 * @return x_obj_t* -- What x_token_delimit() returns
 */
x_obj_t *x_slot_token_delimit(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_token_delimit(p_base, x_vectorobj(p_args, 0));
}

/*
 * # The Table
 */

/**
 * The routines a base's slots start with, by position.
 *
 * The six hooks are not here: x_eval_make() hands them to x_base_make(),
 * since a base needs them from the moment it is made.
 */
static const x_fn_t x_eval_slots[X_SLOT_LEN] = {
	[X_SLOT_OBJ_ALLOC] = x_slot_obj_alloc,
	[X_SLOT_OBJ_FREE] = x_slot_obj_free,
	[X_SLOT_HEAP_TREE_MARK] = x_slot_heap_tree_mark,
	[X_SLOT_HEAP_SWEEP] = x_slot_heap_sweep,
	[X_SLOT_HEAP_ROOT_CHAIN_MARK] = x_slot_heap_root_chain_mark,

	[X_SLOT_EVAL] = x_slot_eval,
	[X_SLOT_EVAL_ARG] = x_slot_eval_arg,
	[X_SLOT_EVAL_LIST] = x_slot_eval_list,
	[X_SLOT_EVAL_BODY] = x_slot_eval_body,
	[X_SLOT_EVAL_BODY_TCO] = x_slot_eval_body_tco,
	[X_SLOT_EVAL_TCO_TRAMPOLINE] = x_slot_eval_tco_trampoline,
	[X_SLOT_EVAL_OP_BODY] = x_slot_eval_op_body,

	[X_SLOT_CALLABLE_CALL] = x_slot_callable_call,
	[X_SLOT_CALLABLE_APPLY] = x_slot_callable_apply,
	[X_SLOT_OBJ_PRIM_CALL] = x_slot_obj_prim_call,


	[X_SLOT_TOKEN_READ] = x_slot_token_read,
	[X_SLOT_TOKEN_ANALYSE] = x_slot_token_analyse,
	[X_SLOT_TOKEN_DELIMIT] = x_slot_token_delimit
};

/**
 * Fill the slots of a base with the engine's and x-expr's routines.
 *
 * A slot the table leaves empty is left as the base has it, so the hooks
 * x_eval_make() set stay in place.
 *
 * @param p_base  x_obj_t* -- The base whose slots to fill
 * @return x_obj_t* -- @p p_base
 */
x_obj_t *x_eval_slots_install(x_obj_t *p_base)
{
	x_int_t i;

	for (i = 0; i < X_SLOT_LEN; i++) {
		if (x_eval_slots[i] != NULL) {
			x_base_slot(p_base, i) = x_eval_slots[i];
		}
	}

	return p_base;
}
