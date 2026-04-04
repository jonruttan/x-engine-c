#ifndef X_TYPE_PROCEDURE_H
#define X_TYPE_PROCEDURE_H

/**
 * @file x-type/procedure.h
 * @brief Procedure (applicative closure) type for x-lang.
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2024 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 *
 * @details
 * A procedure is a two-unit heap object with callable layout:
 * @code
 *   slot 0              slot 1 (state_list)
 *   +-----------------+ +------------------------------------------+
 *   | fn_ptr          | | (params . (body . (env . bst)))          |
 *   | (x_type_        | |                                          |
 *   |  procedure_call)| |  params ---- formal parameter tree       |
 *   +-----------------+ |  body ------ list of body expressions    |
 *                       |  env ------- captured lexical env alist  |
 *                       |  bst ------- captured global BST root    |
 *                       +------------------------------------------+
 * @endcode
 *
 * @note Slot 0 holds a raw C function pointer, NOT a heap object.
 *       The GC mark callback (x_type_procedure_mark) must skip slot 0
 *       and only traverse slot 1 (the state list).  Marking slot 0
 *       as a heap pointer would corrupt the GC free-list.
 *
 * When X_OBJ_FLAG_WRAP is set, the procedure is a wrapped applicative:
 * @c env holds the underlying combiner instead of a closure environment,
 * and @c call dispatches to that combiner after evaluating args.
 *
 * @see x_type_procedure_mark in procedure.c
 * @see x_type_procedure_call for the TCO call path
 * @see x_type_procedure_apply for the non-TCO apply path
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
#include "x-type.h"
#include "x-type/prim.h"

#define X_TYPE_PROCEDURE_NAME		"PROCEDURE"     /**< Type name string. */
#define X_TYPE_PROCEDURE_WRITE_STR	"#<fn>"         /**< Display representation. */
#define X_TYPE_PROCEDURE_WRITE_LEN	5               /**< Length of display string. */

/** @name Type predicates */
/** @{ */
#define x_obj_type_isprocedure(B,X)	x_obj_is_type((B), (X), X_TYPE_PROCEDURE_NAME) /**< Test if object is a procedure. */
/** @} */

/** @name State accessors
 *  Procedure state list: (params . (body . (env . bst))).
 *  Stored in x_callable_state (slot 1) of [fn-ptr][state] layout.
 *  GC traverses via the p_units=2 fallback in x_type_heap_mark.
 */
/** @{ */
#define x_procstate(X)				x_callable_state((X))                              /**< Full state list. */
#define x_procparams(X)				x_firstobj(x_procstate((X)))                        /**< Parameter tree. */
#define x_procbody(X)				x_firstobj(x_restobj(x_procstate((X))))             /**< Body expression list. */
#define x_procenv(X)				x_firstobj(x_restobj(x_restobj(x_procstate((X))))) /**< Captured environment. */
#define x_procbst(X)				x_restobj(x_restobj(x_restobj(x_procstate((X)))))  /**< Captured global BST. */
/** @} */

#define X_OBJ_FLAG_WRAP				X_OBJ_FLAG_1 /**< Flag marking a wrapped applicative combiner. */

/** @name Convenience constructors */
/** @{ */
#define x_mkproc(B,P,BD,E,T)		x_make_procedure((B), X_OBJ_FLAG_NONE, (P), (BD), (E), (T)) /**< Make unwrapped procedure. */
#define x_mkfproc(B,F,P,BD,E,T)	x_make_procedure((B), (F), (P), (BD), (E), (T))             /**< Make procedure with flags. */
#define x_mkwrap(B,C)				x_make_procedure((B), X_OBJ_FLAG_WRAP, NULL, NULL, (C), NULL) /**< Wrap a combiner as applicative. */
/** @} */

/** @name Static primitive atoms for the type struct. */
/** @{ */
extern x_satom_t x_type_procedure_name,
	x_type_procedure_make_prim,
	x_type_procedure_call_prim,
	x_type_procedure_write_prim,
	x_type_procedure_struct_prim;
/** @} */

/** Allocate a new procedure object on the heap. */
x_obj_t *x_make_procedure(x_obj_t *p_base, x_obj_flag_t flags,
	x_obj_t *p_params, x_obj_t *p_body, x_obj_t *p_env, x_obj_t *p_bst);

/** Register (or retrieve) the PROCEDURE type struct on p_base. */
x_obj_t *x_type_procedure_register(x_obj_t *p_base, x_obj_t *p_args);
/** Build the PROCEDURE type struct descriptor. */
x_obj_t *x_type_procedure_struct(x_obj_t *p_base, x_obj_t *p_args);
/** Type-dispatch make callback for PROCEDURE. */
x_obj_t *x_type_procedure_make(x_obj_t *p_base, x_obj_t *p_args);
/** Type-dispatch call callback -- evaluate a procedure application. */
x_obj_t *x_type_procedure_call(x_obj_t *p_base, x_obj_t *p_args);
/** Non-TCO apply path for (apply f args). */
x_obj_t *x_type_procedure_apply(x_obj_t *p_base, x_obj_t *p_args);
/** Type-dispatch write callback -- print "#<fn>". */
x_obj_t *x_type_procedure_write(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_PROCEDURE_H */
