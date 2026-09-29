#ifndef X_EVAL_SLOTS_H
#define X_EVAL_SLOTS_H

/**
 * @file x-eval-slots.h
 * @brief The engine's slots -- the evaluator's routines in the base's
 *        function vector, each at a fixed position.
 *
 * x-expr owns the first positions of the slot vector (see x-slots.h); the
 * engine's follow them. Every slot function has the one signature,
 * #x_fn_t, and takes its arguments as an argument vector: the arguments
 * of the routine it serves, in the routine's own order, after the base.
 *
 * The positions are part of the layout contract. They are listed in
 * tools/contract/base-slots.x, which tools/check/base-slots.sh diffs
 * against the two headers, and which x-lang reads a position from.
 *
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2026 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */

#include "x-base.h"

/**
 * @name Slot Positions
 * @{
 */

/**
 * The positions the engine owns in the slot vector.
 */
enum x_eval_slot_enum
{
	/** x_eval(). Arguments: (args). */
	X_SLOT_EVAL = X_SLOT_EXPR_LEN,

	/** x_eval_arg(). Arguments: (arg). */
	X_SLOT_EVAL_ARG,

	/** x_eval_list(). Arguments: (args). */
	X_SLOT_EVAL_LIST,

	/** x_eval_body(). Arguments: (body). */
	X_SLOT_EVAL_BODY,

	/** x_eval_body_tco(). Arguments: (body). */
	X_SLOT_EVAL_BODY_TCO,

	/** x_eval_tco_trampoline(). Arguments: (result). */
	X_SLOT_EVAL_TCO_TRAMPOLINE,

	/** x_eval_op_body(). Arguments: (body, caller). */
	X_SLOT_EVAL_OP_BODY,

	/** x_callable_call(). Arguments: (args). */
	X_SLOT_CALLABLE_CALL,

	/** x_callable_apply(). Arguments: (args). */
	X_SLOT_CALLABLE_APPLY,

	/** x_obj_prim_call(). Arguments: (args). */
	X_SLOT_OBJ_PRIM_CALL,

	/** x_env_lookup(). Arguments: (env, symbol). */
	X_SLOT_ENV_LOOKUP,

	/** x_env_bind(). Arguments: (env, symbol, value). */
	X_SLOT_ENV_BIND,

	/** x_env_extend(). Arguments: (parent, params, values). */
	X_SLOT_ENV_EXTEND,

	/** x_alist_bst_lookup(). Arguments: (tree, symbol). */
	X_SLOT_ALIST_BST_LOOKUP,

	/** x_token_read(). Arguments: (args). */
	X_SLOT_TOKEN_READ,

	/** x_token_analyse(). Arguments: (args, pointer to the label). */
	X_SLOT_TOKEN_ANALYSE,

	/** x_token_delimit(). Arguments: (args). */
	X_SLOT_TOKEN_DELIMIT,

	/** The length of the engine's slot vector. */
	X_SLOT_LEN
};

/** @} */

/**
 * @name Slot Functions
 * @{
 */

/** Fill the slots of @p p_base with the engine's and x-expr's routines. */
x_obj_t *x_eval_slots_install(x_obj_t *p_base);

/** Slot function for x_eval(). */
x_obj_t *x_slot_eval(x_obj_t *p_base, x_obj_t *p_args);

/** Slot function for x_eval_arg(). */
x_obj_t *x_slot_eval_arg(x_obj_t *p_base, x_obj_t *p_args);

/** Slot function for x_eval_list(). */
x_obj_t *x_slot_eval_list(x_obj_t *p_base, x_obj_t *p_args);

/** Slot function for x_eval_body(). */
x_obj_t *x_slot_eval_body(x_obj_t *p_base, x_obj_t *p_args);

/** Slot function for x_eval_body_tco(). */
x_obj_t *x_slot_eval_body_tco(x_obj_t *p_base, x_obj_t *p_args);

/** Slot function for x_eval_tco_trampoline(). */
x_obj_t *x_slot_eval_tco_trampoline(x_obj_t *p_base, x_obj_t *p_args);

/** Slot function for x_eval_op_body(). */
x_obj_t *x_slot_eval_op_body(x_obj_t *p_base, x_obj_t *p_args);

/** Slot function for x_callable_call(). */
x_obj_t *x_slot_callable_call(x_obj_t *p_base, x_obj_t *p_args);

/** Slot function for x_callable_apply(). */
x_obj_t *x_slot_callable_apply(x_obj_t *p_base, x_obj_t *p_args);

/** Slot function for x_obj_prim_call(). */
x_obj_t *x_slot_obj_prim_call(x_obj_t *p_base, x_obj_t *p_args);

/** Slot function for x_env_bind(). */
x_obj_t *x_slot_env_bind(x_obj_t *p_base, x_obj_t *p_args);

/** Slot function for x_env_extend(). */
x_obj_t *x_slot_env_extend(x_obj_t *p_base, x_obj_t *p_args);

/** Slot function for x_token_read(). */
x_obj_t *x_slot_token_read(x_obj_t *p_base, x_obj_t *p_args);

/** Slot function for x_token_analyse(). */
x_obj_t *x_slot_token_analyse(x_obj_t *p_base, x_obj_t *p_args);

/** Slot function for x_token_delimit(). */
x_obj_t *x_slot_token_delimit(x_obj_t *p_base, x_obj_t *p_args);

/** @} */

#endif /* X_EVAL_SLOTS_H */
