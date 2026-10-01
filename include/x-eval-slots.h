#ifndef X_EVAL_SLOTS_H
#define X_EVAL_SLOTS_H

/**
 * @file x-eval-slots.h
 * @brief The base's slot vector -- the engine's routines, each at a fixed
 *        position, called through the position.
 *
 * @details
 * A base holds a @e slot @e vector: a vector (see @ref x-vector.h) whose
 * elements are function pointers, one per slot. The engine reaches a
 * routine by its position in the vector, so a routine can be replaced
 * while the engine runs by storing another function pointer in its slot.
 *
 * A slot holds a function pointer and nothing more. Every routine in a
 * slot has the engine's one signature, #x_fn_t:
 *
 * @code
 *   x_obj_t *fn(x_obj_t *p_base, x_obj_t *p_args);
 * @endcode
 *
 * and @p p_args is the routine's @e argument @e run: a run of datum words,
 * one per argument, in the order the slot's row in the layout contract
 * gives them. A word holds what the argument is: an object pointer, an
 * integer, a string. The run has no header and no length; the contract
 * says how many words there are and what each holds. The caller writes
 * the words, in stack storage, and the routine reads them back:
 *
 * @code
 *   x_obj_t args[2] = { { .p = p_env }, { .p = p_sym } };
 *
 *   p = x_eval_call(p_base, X_SLOT_ENV_LOOKUP, args);
 *   ...
 *   p_env = x_obj(p_args[0]);
 *   p_sym = x_obj(p_args[1]);
 * @endcode
 *
 * A call through a slot allocates nothing. The elements of a heap vector
 * are a run of words too, so a caller that holds its arguments in a
 * vector passes the address of its first element.
 *
 * The slot vector is the base object's second data unit. x-expr makes a
 * base of one unit holding the tree; x_eval_make() makes the engine's base
 * of two, the tree first, where x-expr reads it, and the vector second,
 * so a call locates the vector in one load from the base. Each base has a
 * vector of its own, filled from the engine's table when the base is made,
 * and a slot replaced in one base is replaced there alone.
 * The positions are part of the layout contract: they are listed in
 * tools/contract/base-slots.x, which tools/check/base-slots.sh diffs
 * against this header, and a language that replaces a routine reads its
 * position from there. Each slot's comment gives its arguments and the
 * type of the word each travels in: an object, an integer or a string.
 * The collector is the engine's: the mark and sweep phases and the
 * allocation of an object are routines in the vector, written over
 * x-expr's chain, traversal, sweep and hooks, which the engine calls by
 * name and which are never in a slot.
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
#include "x-vector.h"

/**
 * @name Slot Positions
 * @{
 */

/**
 * The positions in the slot vector.
 */
enum x_eval_slot_enum
{
	/** x_eval(). Arguments: (expression). Types: (object). */
	X_SLOT_EVAL = 0,

	/** x_eval_list(). Arguments: (args). Types: (object). */
	X_SLOT_EVAL_LIST,

	/** x_eval_body(). Arguments: (body). Types: (object). */
	X_SLOT_EVAL_BODY,

	/** x_eval_body_tco(). Arguments: (body). Types: (object). */
	X_SLOT_EVAL_BODY_TCO,

	/** x_eval_tco_trampoline(). Arguments: (result). Types: (object). */
	X_SLOT_EVAL_TCO_TRAMPOLINE,

	/** x_eval_op_body(). Arguments: (body, caller). Types: (object, object). */
	X_SLOT_EVAL_OP_BODY,

	/** x_callable_call(). Arguments: (args). Types: (object). */
	X_SLOT_CALLABLE_CALL,

	/** x_callable_apply(). Arguments: (args). Types: (object). */
	X_SLOT_CALLABLE_APPLY,

	/** x_obj_prim_call(). Arguments: (args). Types: (object). */
	X_SLOT_OBJ_PRIM_CALL,

	/** x_env_lookup(). Arguments: (env, symbol). Types: (object, object). */
	X_SLOT_ENV_LOOKUP,

	/** x_env_bind(). Arguments: (env, symbol, value).
	 *  Types: (object, object, object). */
	X_SLOT_ENV_BIND,

	/** x_env_extend(). Arguments: (parent, params, values).
	 *  Types: (object, object, object). */
	X_SLOT_ENV_EXTEND,

	/** x_alist_bst_lookup(). Arguments: (tree, symbol). Types: (object, object). */
	X_SLOT_ALIST_BST_LOOKUP,

	/** x_token_read(). Arguments: (args). Types: (object). */
	X_SLOT_TOKEN_READ,

	/** x_token_analyse(). Arguments: (args, label). Types: (object, integer).
	 *  The routine stores the label it declares in the second word. */
	X_SLOT_TOKEN_ANALYSE,

	/** x_token_delimit(). Arguments: (args). Types: (object). */
	X_SLOT_TOKEN_DELIMIT,

	/** x_heap_mark_phase(), the collector's mark phase. Arguments: ().
	 *  Types: (). */
	X_SLOT_HEAP_MARK,

	/** x_heap_sweep_phase(), the collector's sweep phase. Arguments: ().
	 *  Types: (). */
	X_SLOT_HEAP_SWEEP,

	/** x_eval_alloc(), allocate an object. Arguments: (type, flags, units).
	 *  Types: (object, integer, integer). */
	X_SLOT_OBJ_ALLOC,

	/** The length of the slot vector. */
	X_SLOT_LEN
};

/** @} */

/**
 * @name Slot Access
 * @{
 */

/**
 * The function pointer in slot @p I of slot vector @p V (an lvalue).
 */
#define x_slot(V,I)					x_vectorfn((V), (I))

/**
 * An argument run written as an expression: the datum initializers of its
 * words, in order. It lasts as long as the block the expression is in.
 *
 * @code
 *   x_eval_call(p_base, X_SLOT_EVAL, x_argrun({ .p = p_expr }));
 * @endcode
 */
#define x_argrun(...)				((x_obj_t[]){ __VA_ARGS__ })

/** The slot vector of base @p B (an lvalue): its second data unit. */
#define x_eval_slots(B)				x_restobj((B))

/** The function pointer in slot @p I of base @p B (an lvalue). */
#define x_eval_slot(B,I)			x_slot(x_eval_slots((B)), (I))

/**
 * Test whether base @p B has a slot vector: one load. The base must have
 * two units: an object standing as an allocation context is a pair with
 * nothing in it, never an atom, since the second unit is read.
 */
#define x_eval_slots_isset(B) \
	((B) != NULL && x_eval_slots((B)) != NULL)

/**
 * Call the function in slot @p I of base @p B with argument run @p A. The
 * base must have a slot vector and the slot must hold a function: one
 * load for the vector, one for the slot, and the call.
 */
#define x_eval_call(B,I,A)			(x_eval_slot((B), (I))((B), (A)))

/**
 * Call the function in slot @p I of base @p B with argument run @p A, or
 * the routine @p FN when there is no base or the base has no slot vector.
 * For the engine's own calls, which may run before a base exists, or on a
 * base x-expr made; a base x_eval_make() made always has its slots.
 */
#define x_eval_call_or(B,I,FN,A) \
	((x_eval_slots_isset((B)) ? x_eval_slot((B), (I)) : (FN))((B), (A)))

/**
 * Test whether slot @p I of base @p B holds a function: the base has a
 * slot vector and the slot is not empty.
 */
#define x_eval_slot_isset(B,I) \
	(x_eval_slots_isset((B)) && x_eval_slot((B), (I)) != NULL)

/** Make a slot vector of @p length slots, every one empty. */
x_obj_t *x_slots_make(x_obj_t *p_base, x_int_t length);

/** @} */

#endif /* X_EVAL_SLOTS_H */
