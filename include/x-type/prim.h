#ifndef X_TYPE_PRIM_H
#define X_TYPE_PRIM_H

/**
 * @file x-type/prim.h
 * @brief Primitive (C function) type and unified callable dispatch.
 *
 * A PRIMITIVE wraps a C function pointer (@c x_fn_t) as a first-class
 * object.  The callable_call / callable_apply functions provide unified
 * dispatch for all callable types (primitives, closures, operatives,
 * and type-internal handlers).
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

#include "x-type.h"

#define X_TYPE_PRIM_NAME		"PRIMITIVE"  /**< Canonical type name. */

/** @name Predicates
 * @{ */
/** Test whether @p X is a PRIMITIVE object. */
#define x_obj_type_isprim(B,X)	x_obj_is_type((B), (X), X_TYPE_PRIM_NAME)
/** @} */

/** @name Accessors
 * @{ */
/** Extract the @c x_fn_t function pointer from a callable. */
#define x_primval(X)			x_firstfn((X))
/** Access the state slot (second object) of a callable. */
#define x_callable_state(X)		x_secondobj((X))
/** @} */

/** @name Constructors
 * @{ */
/** Create a PRIMITIVE with default flags. */
#define x_mkprim(B,FN)			x_make_prim((B), X_OBJ_FLAG_NONE, (FN))
/** Create a PRIMITIVE with explicit flags. */
#define x_mkfprim(B,F,FN)		x_make_prim((B), (F), (FN))
/** @} */

/** @name Static Atoms
 * @{ */
extern x_satom_t x_type_prim_name,          /**< Static atom: type name. */
	x_type_prim_make_prim,                   /**< Static atom: make handler. */
	x_callable_call_prim,                    /**< Static atom: call handler. */
	x_type_prim_struct_prim;                 /**< Static atom: struct constructor. */
/** @} */

/** Allocate a PRIMITIVE object wrapping @p fn. */
x_obj_t *x_make_prim(x_obj_t *p_base, x_obj_flag_t flags, x_fn_t fn);

/** Register (or retrieve) the PRIMITIVE type in the type alist. */
x_obj_t *x_type_prim_register(x_obj_t *p_base, x_obj_t *p_args);
/** Build the PRIMITIVE type struct descriptor. */
x_obj_t *x_type_prim_struct(x_obj_t *p_base, x_obj_t *p_args);
/** Type-system make handler for PRIMITIVE objects. */
x_obj_t *x_type_prim_make(x_obj_t *p_base, x_obj_t *p_args);
/** Unified call dispatch for all callable types. */
x_obj_t *x_callable_call(x_obj_t *p_base, x_obj_t *p_args);
/** Unified apply dispatch with TCO trampoline support. */
x_obj_t *x_callable_apply(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_PRIM_H */
