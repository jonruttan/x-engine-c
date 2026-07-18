#ifndef X_TYPE_ITER_H
#define X_TYPE_ITER_H

/**
 * @file x-type/iter.h
 * @brief Iterator type for x-lang (lazy traversal of sequences).
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

#ifndef X_TYPE_ITER_NAME
#define X_TYPE_ITER_NAME			"ITER"      /**< Type name string (overridable). */
#endif /* X_TYPE_ITER_NAME */


/** @name Type predicates */
/** @{ */
#define x_obj_type_isiter(B,X)		x_obj_is_type((B), (X), X_TYPE_ITER_NAME) /**< Test if object is an iterator. */
/** @} */

/** @name Field accessors
 *  Iterator layout: (step-fn . current-value) -- a boxed GENERATOR.
 *  Steps are PURE (they never touch the box); x_type_iter_next owns the
 *  write-back.  Step ABIs: a SATOM step is a raw C fn over a caller-owned
 *  state cell (zero-alloc); any other callable is functional,
 *  (step state) -> (value . next-state) | nil -- the Gen/Seq contract.
 */
/** @{ */
#define x_iterprim(X)				x_firstobj((X))                /**< Step function (callable). */
#define x_iterval(X)				x_restobj((X))                 /**< Current value (nil when exhausted). */
#define x_iterempty(B,X)			x_obj_isnil((B), x_iterval(X)) /**< True when iterator is exhausted. */
/** @} */

/** @name Convenience constructors */
/** @{ */
#define x_mkiter(B, FN, L)			x_make_iter((B), X_OBJ_FLAG_NONE, (FN), (L)) /**< Make iterator with default flags. */
#define x_mkfiter(B, F, FN, L)		x_make_iter((B), (F), (FN), (L))              /**< Make iterator with explicit flags. */
/** @} */

/** Allocate a new iterator object on the heap. */
x_obj_t *x_make_iter(x_obj_t *p_base, x_obj_flag_t flags, void *p1, void *p2);
/** Register (or retrieve) the ITER type struct on p_base. */
x_obj_t *x_type_iter_register(x_obj_t *p_base, x_obj_t *p_args);
/** Build the ITER type struct descriptor. */
x_obj_t *x_type_iter_struct(x_obj_t *p_base, x_obj_t *p_args);
/** Type-dispatch make callback for ITER. */
x_obj_t *x_type_iter_make(x_obj_t *p_base, x_obj_t *p_args);
/** Test whether an iterator is exhausted. */
x_obj_t *x_type_iter_isempty(x_obj_t *p_base, x_obj_t *p_args);
/** Advance an iterator by one step, returning the current element. */
x_obj_t *x_type_iter_next(x_obj_t *p_base, x_obj_t *p_args);
/** Step an iterator functionally: (value . next-iterator) pair, or NULL. */
x_obj_t *x_type_iter_step(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_ITER_H */
