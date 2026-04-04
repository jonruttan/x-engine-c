#ifndef X_TYPE_PTR_H
#define X_TYPE_PTR_H

/**
 * @file x-type/ptr.h
 * @brief Opaque pointer type -- wraps a raw @c void* as a managed object.
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

#define X_TYPE_PTR_NAME		"POINTER"        /**< Canonical type name. */
#define X_TYPE_PTR_WRITE_STR	"#<ptr>"     /**< External representation. */
#define X_TYPE_PTR_WRITE_LEN	6            /**< Length of write string. */

/** @name Predicates
 * @{ */
/** Test whether @p X is a POINTER object. */
#define x_obj_type_isptr(B,X)	x_obj_is_type((B), (X), X_TYPE_PTR_NAME)
/** @} */

/** @name Accessors
 * @{ */
/** Extract the raw @c void* from a pointer object. */
#define x_ptrval(X)				x_firstptr((X))
/** @} */

/** @name Constructors
 * @{ */
/** Create a pointer object with default flags. */
#define x_mkptr(B, P)			x_make_ptr((B), X_OBJ_FLAG_NONE, (P))
/** Create a pointer object with explicit flags. */
#define x_mkfptr(B, F, P)		x_make_ptr((B), (F), (P))
/** Create an owning pointer object. */
#define x_mkptrown(B, P)		x_make_ptr((B), X_OBJ_FLAG_OWN, (P))
/** Create an owning pointer object with extra flags. */
#define x_mkfptrown(B, F, P)	x_make_ptr((B), X_OBJ_FLAG_OWN | (F), (P))
/** @} */

/** Allocate a POINTER object wrapping @p p. */
x_obj_t *x_make_ptr(x_obj_t *p_base, x_obj_flag_t flags, void *p);

/** Build the POINTER type struct descriptor. */
x_obj_t *x_type_ptr_struct(x_obj_t *p_base, x_obj_t *p_args);
/** Register (or retrieve) the POINTER type in the type alist. */
x_obj_t *x_type_ptr_register(x_obj_t *p_base, x_obj_t *p_args);
/** Type-system make handler for POINTER objects. */
x_obj_t *x_type_ptr_make(x_obj_t *p_base, x_obj_t *p_args);
/** Type-system write handler -- outputs @c #\<ptr\>. */
x_obj_t *x_type_ptr_write(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_PTR_H */
