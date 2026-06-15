#ifndef X_TYPE_OPERATIVE_H
#define X_TYPE_OPERATIVE_H

/**
 * @file x-type/operative.h
 * @brief Operative (fexpr / dynamic-scope combiner) type for x-lang.
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2026 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 *
 * @details
 * An operative is a two-unit heap object with callable layout:
 * @code
 *   slot 0              slot 1 (state_list)
 *   +-----------------+ +------------------------------------------+
 *   | fn_ptr          | | (params . (envparam . (body . env)))     |
 *   | (x_type_        | |                                          |
 *   |  operative_call)| |  params ---- formal parameter tree       |
 *   +-----------------+ |  envparam -- symbol bound to caller env  |
 *                       |  body ------ list of body expressions    |
 *                       |  env ------- captured lexical env alist  |
 *                       +------------------------------------------+
 * @endcode
 *
 * @note Unlike procedures, the operative state list has NO BST field.
 *       Operatives receive unevaluated arguments and the caller's
 *       dynamic environment, so they do not need a captured BST snapshot.
 *
 * @note Slot 0 holds a raw C function pointer, NOT a heap object.
 *       The GC mark callback (x_type_operative_mark) must skip slot 0
 *       and only traverse slot 1 (the state list).
 *
 * @see x_type_operative_mark in operative.c
 * @see x_type_operative_call for the TCO call path
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
#include "x-type.h"
#include "x-type/prim.h"

#define X_TYPE_OPERATIVE_NAME		"OPERATIVE"   /**< Type name string. */
#define X_TYPE_OPERATIVE_WRITE_STR	"#<op>"       /**< Display representation. */
#define X_TYPE_OPERATIVE_WRITE_LEN	5             /**< Length of display string. */

/** @name Type predicates */
/** @{ */
#define x_obj_type_isoperative(B,X)	x_obj_is_type((B), (X), X_TYPE_OPERATIVE_NAME) /**< Test if object is an operative. */
/** @} */

/** @name State accessors
 *  Operative state list: (params . (envparam . (body . env))).
 *  Stored in x_callable_state (slot 1) of [fn-ptr][state] layout.
 *  GC traverses via the p_units=2 fallback in x_type_heap_mark.
 */
/** @{ */
#define x_opstate(X)				x_callable_state((X))                              /**< Full state list. */
#define x_opparams(X)				x_firstobj(x_opstate((X)))                          /**< Parameter tree. */
#define x_openvparam(X)				x_firstobj(x_restobj(x_opstate((X))))               /**< Environment parameter name. */
#define x_opbody(X)					x_firstobj(x_restobj(x_restobj(x_opstate((X)))))    /**< Body expression list. */
#define x_openv(X)					x_restobj(x_restobj(x_restobj(x_opstate((X)))))     /**< Captured environment. */
/** @} */

/** @name Convenience constructors */
/** @{ */
#define x_mkop(B,P,EP,BD,E)		x_make_operative((B), X_OBJ_FLAG_NONE, (P), (EP), (BD), (E)) /**< Make operative with default flags. */
/** @} */

/** @name Static primitive atoms for the type struct. */
/** @{ */
extern x_satom_t x_type_operative_name,
	x_type_operative_make_prim,
	x_type_operative_call_prim,
	x_type_operative_write_prim,
	x_type_operative_struct_prim;
/** @} */

/** Allocate a new operative object on the heap. */
x_obj_t *x_make_operative(x_obj_t *p_base, x_obj_flag_t flags,
	x_obj_t *p_params, x_obj_t *p_envparam, x_obj_t *p_body, x_obj_t *p_env);

/** Register (or retrieve) the OPERATIVE type struct on p_base. */
x_obj_t *x_type_operative_register(x_obj_t *p_base, x_obj_t *p_args);
/** Build the OPERATIVE type struct descriptor. */
x_obj_t *x_type_operative_struct(x_obj_t *p_base, x_obj_t *p_args);
/** Type-dispatch make callback for OPERATIVE. */
x_obj_t *x_type_operative_make(x_obj_t *p_base, x_obj_t *p_args);
/** Type-dispatch call callback -- evaluate an operative application. */
x_obj_t *x_type_operative_call(x_obj_t *p_base, x_obj_t *p_args);
/** Type-dispatch write callback -- print "#<op>". */
x_obj_t *x_type_operative_write(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_OPERATIVE_H */
