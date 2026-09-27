#ifndef X_EVAL_H
#define X_EVAL_H

/**
 * @file x-eval.h
 * @brief Evaluator object and interface -- x-expr's base object plus the
 *        environment, control-flow, I/O, and metadata fields the evaluator
 *        needs, plus the central x_eval entry point.
 *
 * The base object is a pair tree.  x-expr provides the skeleton (io-group,
 * meta-group, profile, hooks, heap-group); this layer fills the
 * environment/control half it leaves nil and appends a few project fields
 * (booleans, eval-list, token-cache, GC hooks, sigint).
 *
 * Layout (base = x_base(X)):
 *
 *   first: env + ctrl              (x-expr leaves nil; filled here)
 *     env    env (the current environment), env-root
 *     ctrl   save-stack, error-handler, tco-expr, tco-env
 *   rest:  io + meta               (x-expr skeleton)
 *     io     type-alist, line, true, false
 *     meta   profile counters, eval-list, token-cache, sigint
 *            (GC hook + root lists -- mark-hooks, free-hooks,
 *             mark-roots -- now live in x-expr's heap-group;
 *             register via x_heap_{mark,free}_hook_add() and
 *             x_heap_mark_root_add().)
 *
 * Each leaf is a stack cell @c (current . saved); read the current value
 * with @c x_firstobj().  Direct-value exceptions (the slot is the value,
 * no wrapping): save-stack, env, env-root.
 *
 * An ENVIRONMENT is a first-class value: one pair, @c (bindings . parent).
 * The root's parent is nil and its bindings are a tree (x-alist.c's BST,
 * for the size of a loaded library); every other environment's bindings
 * are an alist of @c (name . value) cells and its parent is the
 * environment it was made in.  A procedure call makes a child of the
 * closure's environment; an operative body runs in a child of its static
 * environment and receives the caller's environment as a value; `def`
 * binds in the current environment and `eval` with an environment makes
 * that one current.  The whole of scope is those four sentences, and
 * every save/restore in the evaluator is one pointer, the current
 * environment.  The environment operations are x-env.h; the save and
 * restore are x-tco.h; the top-level bracket is x-toplevel.h.
 *
 * The error handler is itself a pair tree, navigated by x_error_handler_*:
 *   @c (jmp-ptr (saved-env . nil) error-value . line)
 *
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2021 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 */

/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */

#include "x-base.h"

/** The interpreter object: the base object specialized into this project's
 *  execution context.  Serves as the type tag for base/interp objects. */
extern x_satom_t x_eval_obj;

/** Expression flags.
 *  COV -- an expression has been evaluated (coverage tracking).
 *
 *  Three flags lived beside it and are gone with the environment model
 *  they served: SHADOW, a bit on the interned symbol that marked a local
 *  binding; FRAME, a bit on an env spine cell that marked it as part of a
 *  local frame; and FNFRAME, FRAME's refinement for procedure activations.
 *  An environment is an object now, so a frame is a value and needs no
 *  mark.  Flag bits 1, 3 and 4 are free at this layer. */
#define X_OBJ_FLAG_COV		X_OBJ_FLAG_2

/** The TRACE bit -- the first bit above x-expr's, and the one a caller
 *  borrows to mark objects of its own.  (heap tree-mark!), (heap
 *  chain-clear!) and (image write!) take the flag as an argument; every
 *  caller, here and in x-lang, passes this one.  The engine sets it nowhere
 *  itself.  It is named so that nothing of the engine's lands on it: the
 *  eval count below, on this bit, would make every object evaluated an odd
 *  number of times look traced to an image write. */
#define X_OBJ_FLAG_TRACE	0x400

/** The EVAL COUNT -- under X_PROFILE, how many times evaluation has reached
 *  an object.  It counts exactly where COV marks: every expression x_eval
 *  evaluates, a trampolined tail included, and every body cell x_eval_body
 *  and x_eval_body_tco step onto.  So a procedure's first body cell counts
 *  its calls, and the counts through its body are the evaluation it did
 *  itself.
 *
 *  It lives in the FLAGS WORD, above x-expr's bits and the TRACE bit, so it
 *  costs no allocation and needs no initializing: x_obj_alloc writes the
 *  whole word, and every object is born at zero.  Read it with
 *  x_obj_evals().  Reflective x-lang code reads the same bits through
 *  %obj-evals-shift and %obj-evals-bits in tools/contract/obj-layout.x.
 *
 *  20 BITS, AND IT SATURATES.  x-expr's sweep clears the mark with
 *  `flags &= ~mark`, and the mark is an x_obj_flag_t, an unsigned int, so
 *  on a 64-bit host the complement zero-extends and every collection clears
 *  bits 32 and up of every object it keeps.  Bits 11 to 30 survive that, and
 *  they keep the word non-negative where bit 31 is a 32-bit host's sign.  A
 *  count therefore stops at X_OBJ_EVALS_MAX (1048575) instead of wrapping: a
 *  node reading the maximum was reached at least that often. */
#define X_OBJ_EVALS_SHIFT	11
#define X_OBJ_EVALS_BITS	20
#define X_OBJ_EVALS_ONE		((x_int_t)1 << X_OBJ_EVALS_SHIFT)
#define X_OBJ_EVALS_MAX		(((x_int_t)1 << X_OBJ_EVALS_BITS) - 1)
#define x_obj_evals(X)		((x_obj_flags(X) >> X_OBJ_EVALS_SHIFT) & X_OBJ_EVALS_MAX)

/* TRACE sits above x-expr's highest flag, the count above TRACE, and the
 * count ends below bit 31.  An x-expr bump that grows the flags, or an edit
 * that moves or widens either, fails to compile here instead of sharing
 * bits without a word. */
typedef char x_assert_trace_above_flags[
	X_OBJ_FLAG_MASK < X_OBJ_FLAG_TRACE ? 1 : -1];
typedef char x_assert_evals_above_trace[
	(x_int_t)X_OBJ_FLAG_TRACE < X_OBJ_EVALS_ONE ? 1 : -1];
typedef char x_assert_evals_below_sign[
	X_OBJ_EVALS_SHIFT + X_OBJ_EVALS_BITS <= 31 ? 1 : -1];

/**
 * @defgroup error_handler Error Handler Macros
 * @brief Navigate the error handler pair tree
 *        @c (jmp-ptr (saved-env . previous) error-value . line).
 *        A guard's handler carries the handler it displaced in the
 *        previous slot, so every installed handler stays reachable from
 *        the error_handler slot while the innermost is; a base-eval
 *        handler leaves it nil and is consed onto the target's stack.
 * @{
 */
#define x_error_handler_jmp(H)				x_ptrval(x_firstobj(H))
#define x_error_handler_saved_env(H)		x_001(H)
#define x_error_handler_error(H)			x_011(H)
#define x_error_handler_line(H)				x_111(H)
/** @} */

/**
 * @defgroup base_field Base Field Accessor Macros
 * @brief Navigate the base object's pair tree.
 *
 * @c x_eval_field_* macros return a field's stack cell @c (current .
 * saved); read the current value with @c x_firstobj().  The @c x_eval_env,
 * @c x_eval_ctrl, @c x_eval_io_state, and @c x_eval_state macros are
 * the group anchors the fields hang off.  @c x_base, @c io, @c meta, @c
 * hooks, and @c heap come from x-base.h (x-expr).
 * @{
 */

#include "x-eval-layout.h"	/* generated: x_eval_env/ctrl/io_state/state anchors + x_eval_field_* */


/** Capacity of a base's READER buffer -- the one the tokenizer reads through,
 *  hung off x_base_field_buffer.
 *
 *  ONE NUMBER FOR ALL OF THEM.  x_base_field_buffer is a STACK: the base is
 *  constructed with one of these, and a reader may push another (a file read
 *  pushes one, and tokenizing a string pushes one sized exactly to the string).
 *  The constructed buffer is the bottom of that stack, so it is the default
 *  rather than the usual target -- which is how it came to be 256 bytes in
 *  x_prim_make_base while the CLI gave the same field 65536, a 256x difference
 *  between two sites filling one slot, with only the larger one named.
 *
 *  Named for what it buffers rather than for who allocates it: the earlier name
 *  lived in x-cli.c, so nothing outside the CLI could reach it even to agree
 *  with it.
 *
 *  NOTE the buffer object records cursors (x_bufferval / x_bufferread /
 *  x_bufferwrite) but NOT capacity, so this bound is not available to the code
 *  that fills a buffer.  One constant is the floor of fixing that, not the fix.
 *
 *  It used to have a sibling, X_ERROR_BUF_SIZE, sized for the same reason and
 *  kept separate so a change to one could not silently move the other.  That
 *  one is gone: a raise no longer formats a message into a scratch buffer at
 *  all.  It stores the message literal and the subject as two pointers in a
 *  typed ERR (x-type/err.c), which also retires the hazard that constant's
 *  comment was mostly about -- the symbol was appended LAST, so an
 *  over-long message silently dropped the one detail worth reading, the name
 *  that was unbound.  Nothing is concatenated now, so nothing can be
 *  truncated. */
#define X_READ_BUF_SIZE	65536

/** @} */ /* end base_field */

/** Build the interpreter object (x-expr base object extended). */
x_obj_t *x_eval_make(x_obj_t *p_base, x_obj_t *p_args);

/** Extend the type alist with a new type entry. */
x_obj_t *x_eval_type_alist_extend(x_obj_t *p_base, x_obj_t *p_args);

/** Look up a type in the base type alist. */
x_obj_t *x_eval_type_alist_assoc(x_obj_t *p_base, x_obj_t *p_args);

/** Push a buffer onto the input buffer stack. */
x_obj_t *x_eval_buffer_push(x_obj_t *p_base, x_obj_t *p_buffer);

x_obj_t *x_eval_load(x_obj_t *p_base, x_obj_t *p_args);

/** Signal an error with the given message and irritant object. */
void x_eval_error(x_obj_t *p_base, x_char_t *message, x_obj_t *p_obj);

/** @name Argument Access Macros
 *  @{ */
#define x_eval_arg_exp(X)		x_0((X)) /**< Extract the expression from eval args. */
/** @} */

/** Evaluate an expression in the current environment (TCO trampoline). */
x_obj_t *x_eval(x_obj_t *p_base, x_obj_t *p_args);

/** Evaluate a single argument expression. */
x_obj_t *x_eval_arg(x_obj_t *p_base, x_obj_t *p_arg);

/** @name Evaluation Entry Points
 * @{ */

/** Evaluate an argument list, returning a list of results. */
x_obj_t *x_eval_list(x_obj_t *p_base, x_obj_t *p_args);

/** Raise unless a spine position is a cell first/rest may navigate (#69, #487). */
void x_eval_spine_guard(x_obj_t *p_base, x_obj_t *p_obj);

/** Read the argument at a peeked spine position, guarding a dotted tail (#487). */
x_obj_t *x_eval_spine_first(x_obj_t *p_base, x_obj_t *p_pos);

/** Evaluate a body (sequence of expressions), returning the last result. */
x_obj_t *x_eval_body(x_obj_t *p_base, x_obj_t *p_body);

/** Evaluate a body with TCO, setting up a trampoline for the tail call. */
x_obj_t *x_eval_body_tco(x_obj_t *p_base, x_obj_t *p_body);

/** Execute the TCO trampoline loop until a non-TCO result is produced. */
x_obj_t *x_eval_tco_trampoline(x_obj_t *p_base, x_obj_t *p_result);

/** Defer an operative body's tail to the outer trampoline: evaluate the
 *  non-tail forms, then set tco_expr to the tail and tco_env to the
 *  caller's environment, which the trampoline restores after the tail. */
x_obj_t *x_eval_op_body(x_obj_t *p_base, x_obj_t *p_body, x_obj_t *p_caller);

/** @} */

#endif /* X_EVAL_H */
