#ifndef X_EVAL_SLOTS_H
#define X_EVAL_SLOTS_H

/**
 * @file x-eval-slots.h
 * @brief The engine's slots -- the evaluator's routines in the base's
 *        function vector, each at a fixed position.
 *
 * x-expr owns the first positions of the slot vector (see x-slots.h); the
 * engine's follow them. A slot holds the routine itself: every routine
 * here has the one signature, #x_fn_t, and takes its arguments as an
 * argument run, a run of datum words. x_eval_make() fills the slots when
 * it makes a base.
 *
 * The positions are part of the layout contract. They are listed in
 * tools/contract/base-slots.x, which tools/check/base-slots.sh diffs
 * against the two headers, and which x-lang reads a position from. Each
 * slot's comment gives its arguments and the kind of word each travels
 * in: an object, an integer or a string.
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
	/** x_eval(). Arguments: (expression). Kinds: (object). */
	X_SLOT_EVAL = X_SLOT_EXPR_LEN,

	/** x_eval_list(). Arguments: (args). Kinds: (object). */
	X_SLOT_EVAL_LIST,

	/** x_eval_body(). Arguments: (body). Kinds: (object). */
	X_SLOT_EVAL_BODY,

	/** x_eval_body_tco(). Arguments: (body). Kinds: (object). */
	X_SLOT_EVAL_BODY_TCO,

	/** x_eval_tco_trampoline(). Arguments: (result). Kinds: (object). */
	X_SLOT_EVAL_TCO_TRAMPOLINE,

	/** x_eval_op_body(). Arguments: (body, caller). Kinds: (object, object). */
	X_SLOT_EVAL_OP_BODY,

	/** x_callable_call(). Arguments: (args). Kinds: (object). */
	X_SLOT_CALLABLE_CALL,

	/** x_callable_apply(). Arguments: (args). Kinds: (object). */
	X_SLOT_CALLABLE_APPLY,

	/** x_obj_prim_call(). Arguments: (args). Kinds: (object). */
	X_SLOT_OBJ_PRIM_CALL,

	/** x_env_lookup(). Arguments: (env, symbol). Kinds: (object, object). */
	X_SLOT_ENV_LOOKUP,

	/** x_env_bind(). Arguments: (env, symbol, value).
	 *  Kinds: (object, object, object). */
	X_SLOT_ENV_BIND,

	/** x_env_extend(). Arguments: (parent, params, values).
	 *  Kinds: (object, object, object). */
	X_SLOT_ENV_EXTEND,

	/** x_alist_bst_lookup(). Arguments: (tree, symbol). Kinds: (object, object). */
	X_SLOT_ALIST_BST_LOOKUP,

	/** x_token_read(). Arguments: (args). Kinds: (object). */
	X_SLOT_TOKEN_READ,

	/** x_token_analyse(). Arguments: (args, label). Kinds: (object, integer).
	 *  The routine stores the label it declares in the second word. */
	X_SLOT_TOKEN_ANALYSE,

	/** x_token_delimit(). Arguments: (args). Kinds: (object). */
	X_SLOT_TOKEN_DELIMIT,

	/** The length of the engine's slot vector. */
	X_SLOT_LEN
};

/** @} */

#endif /* X_EVAL_SLOTS_H */
