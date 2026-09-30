#ifndef X_TYPE_VECTOR_H
#define X_TYPE_VECTOR_H

/**
 * @file x-type/vector.h
 * @brief Vector type -- an object whose first data unit holds its length
 *        and whose other data units are its elements.
 *
 * The layout is x-expr's (x-vector.h); the type is this file's. A vector's
 * elements are objects, and the collector follows them: the type declares
 * its units as one leading unit, the length, and a payload counted by it,
 * every unit a reference.
 *
 * A base's slot vector is a vector: the registration gives it the type.
 * Its elements are function pointers, which the collector must not follow,
 * and it does not reach them: the slot vector is the base's first data
 * unit, outside the tree the collector marks from.
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

#include "x-type.h"
#include "x-vector.h"

#define X_TYPE_VECTOR_NAME		"VECTOR"         /**< Canonical type name. */

/**
 * The type's declared unit count: one leading unit, and a payload whose
 * count that unit holds (see x_type_units_count()).
 */
#define X_TYPE_VECTOR_UNITS		(-X_VECTOR_UNITS_LENGTH)

/** @name Predicates
 * @{ */
/** Test whether @p X is a VECTOR object. */
#define x_obj_type_isvector(B,X)	x_obj_is_type((B), (X), X_TYPE_VECTOR_NAME)
/** @} */

/** @name Constructors
 * @{ */
/** Create a vector of the @p N objects that follow, with default flags. */
#define x_mkvector(B, N, ...)	x_make_vector((B), X_OBJ_FLAG_NONE, (N), __VA_ARGS__)
/** @} */

/** Allocate a VECTOR object holding the @p length objects that follow. */
x_obj_t *x_make_vector(x_obj_t *p_base, x_obj_flag_t flags, x_int_t length, ...);

/** Build the VECTOR type struct descriptor. */
x_obj_t *x_type_vector_struct(x_obj_t *p_base, x_obj_t *p_args);
/** Register (or retrieve) the VECTOR type in the type alist. */
x_obj_t *x_type_vector_register(x_obj_t *p_base, x_obj_t *p_args);
/** Type-system length handler for VECTOR objects. */
x_obj_t *x_type_vector_length(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_VECTOR_H */
