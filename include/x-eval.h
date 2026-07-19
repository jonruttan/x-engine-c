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
 *     env    env-alist, env-local-boundary, env-global-tree, shadow-list
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
 * no wrapping): save-stack, env-local-boundary, env-global-tree,
 * shadow-list.
 *
 * The error handler is itself a pair tree, navigated by x_error_handler_*:
 *   @c (jmp-ptr (saved-env . saved-boundary) error-value . line)
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

/** Symbol/expression/env flags.
 *  SHADOW -- retired (GH #47): the bit lived on the INTERNED symbol, so one
 *            activation's local binding blinded every other lexical chain's
 *            BST lookup for that name.  The define remains only for the
 *            dormant clear machinery; nothing sets it anymore.
 *  COV    -- an expression has been evaluated (coverage tracking).
 *  FRAME  -- an env-alist SPINE cons belonging to a local frame (params,
 *            let/closure defs, op formals and env-param, guard binds).
 *            Symbol lookup walks the frame region of the chain BEFORE the
 *            global BST, so enclosing-frame locals shadow globals with the
 *            correct lexical semantics; unmarked spine cells are the global
 *            region, answered by the BST.  See x_type_symbol_eval. */
/** FNFRAME -- set IN ADDITION to FRAME on spine cells of PROCEDURE (fn)
 *  activations.  Operative frames are transparent to top-levelness by
 *  design (the REPL op, def-class's tail-eval'd defs), so a def with an
 *  empty save-stack is top-level UNLESS the env head carries FNFRAME --
 *  the save-stack alone lies at TCO tails, where a fn-body def used to
 *  classify top-level and leak into the BST (GH #47). */
#define X_OBJ_FLAG_SHADOW	X_OBJ_FLAG_1
#define X_OBJ_FLAG_COV		X_OBJ_FLAG_2
#define X_OBJ_FLAG_FRAME	X_OBJ_FLAG_3
#define X_OBJ_FLAG_FNFRAME	X_OBJ_FLAG_4

/**
 * @defgroup error_handler Error Handler Macros
 * @brief Navigate the error handler pair tree
 *        @c (jmp-ptr (saved-env . saved-boundary) error-value . line).
 * @{
 */
#define x_error_handler_jmp(H)				x_ptrval(x_firstobj(H))
#define x_error_handler_saved_env(H)		x_001(H)
#define x_error_handler_saved_boundary(H)	x_101(H)
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

/** Capacity of the error-message scratch buffer; its atom lives at
 *  x_eval_field_error_str (a static in x_eval_make, reached via the base). */
#define X_ERROR_BUF_SIZE	256

/** @} */ /* end base_field */

/** Build the interpreter object (x-expr base object extended). */
x_obj_t *x_eval_make(x_obj_t *p_base, x_obj_t *p_args);

/** Extend the type alist with a new type entry. */
x_obj_t *x_eval_type_alist_extend(x_obj_t *p_base, x_obj_t *p_args);

/** Look up a type in the base type alist. */
x_obj_t *x_eval_type_alist_assoc(x_obj_t *p_base, x_obj_t *p_args);

/** Push a file descriptor onto the file-input stack. */
x_obj_t *x_eval_filein_push(x_obj_t *p_base, x_int_t fd);

/** Pop the top file descriptor from the file-input stack. */
x_obj_t *x_eval_filein_pop(x_obj_t *p_base);

/** Push a buffer onto the input buffer stack. */
x_obj_t *x_eval_buffer_push(x_obj_t *p_base, x_obj_t *p_buffer);

/** Pop the top buffer from the input buffer stack. */
x_obj_t *x_eval_buffer_pop(x_obj_t *p_base);

/** Extend the environment alist with new bindings. */
x_obj_t *x_eval_env_alist_extend(x_obj_t *p_base, x_obj_t *p_args);

/** Load and evaluate a source file. */
x_obj_t *x_eval_load(x_obj_t *p_base, x_obj_t *p_args);

/** Write a string to the output. */
x_obj_t *x_eval_write_str(x_obj_t *p_base, x_obj_t *p_args);

/** Signal an error with the given message and irritant object. */
void x_eval_error(x_obj_t *p_base, x_char_t *message, x_obj_t *p_obj);

/** @name Argument Access Macros
 *  @{ */
#define x_eval_arg_exp(X)		x_0((X)) /**< Extract the expression from eval args. */
/** @} */

/** Evaluate an expression in the current environment (TCO trampoline). */
x_obj_t *x_eval(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_EVAL_H */
