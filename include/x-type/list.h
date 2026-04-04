#ifndef X_TYPE_LIST_H
#define X_TYPE_LIST_H

/**
 * @file x-type/list.h
 * @brief List (pair-chain) type -- construction, indexing, eval, and iteration.
 *
 * A LIST is a pair-length object forming a linked chain of (first . rest)
 * cells.  Lists are the primary composite structure: they serve as both
 * data containers and as s-expression forms that are evaluated by
 * resolving the first element as an operator and dispatching through
 * the callable protocol.
 *
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2024 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 */

/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */

#include "x-type.h"

#ifndef X_TYPE_LIST_NAME
#define X_TYPE_LIST_NAME		"LIST"  /**< Canonical type name (overridable). */
#endif /* X_TYPE_LIST_NAME */

/** @name Eval Helpers
 * @{ */
/** Extract the expression from an eval argument frame. */
#define x_eval_arg_exp(X)			x_0((X))
/** @} */

/** @name Predicates
 * @{ */
/** Test whether @p X is a LIST object. */
#define x_obj_type_islist(B,X)		x_obj_is_type((B), (X), X_TYPE_LIST_NAME)
/** @} */

/** @name Constructors
 * @{ */
/** Create a list pair with default flags. */
#define x_mklist(B,P1,P2)			x_make_list((B), X_OBJ_FLAG_NONE, (P1), (P2))
/** Create a list pair with explicit flags. */
#define x_mkflist(B,F,P1,P2)		x_make_list((B), (F), (P1), (P2))
/** @} */

/** Allocate a LIST pair with first = @p p1, rest = @p p2. */
x_obj_t *x_make_list(x_obj_t *p_base, x_obj_flag_t flags, void *p1, void *p2);
/** Register (or retrieve) the LIST type in the type alist. */
x_obj_t *x_type_list_register(x_obj_t *p_base, x_obj_t *p_args);
/** Build the LIST type struct descriptor. */
x_obj_t *x_type_list_struct(x_obj_t *p_base, x_obj_t *p_args);
/** Type-system make handler for LIST objects. */
x_obj_t *x_type_list_make(x_obj_t *p_base, x_obj_t *p_args);
/** Compute the length of a list by walking to its end. */
x_obj_t *x_type_list_length(x_obj_t *p_base, x_obj_t *p_args);
/** Call handler -- index or slice a list. */
x_obj_t *x_type_list_call(x_obj_t *p_base, x_obj_t *p_args);
/** Eval handler -- resolve operator and dispatch through callable. */
x_obj_t *x_type_list_eval(x_obj_t *p_base, x_obj_t *p_args);
/** Iterator step -- advance a list iterator by one element. */
x_obj_t *x_type_list_iter(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_LIST_H */
