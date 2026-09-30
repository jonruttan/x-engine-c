#ifndef X_VECTOR_H
#define X_VECTOR_H

/**
 * @file x-vector.h
 * @brief The vector layout -- an object whose first data unit holds its
 *        length and whose other data units are its elements.
 *
 * @details
 * A vector of N elements has N + 1 data units. Unit 0 holds the @e length:
 * an atom whose datum is N. Units 1 to N hold the elements, element I in
 * unit I + 1.
 *
 * @code
 *   vector of 3
 *   +---------+
 *   | header  |
 *   +---------+ <- data start
 *   | length  | ----> atom: 3
 *   | elem 0  |
 *   | elem 1  |
 *   | elem 2  |
 *   +---------+
 * @endcode
 *
 * This file supplies the layout and nothing more: the accessors below, the
 * static length atoms, and a constructor with no type. The VECTOR type an
 * object of this layout carries is x-type/vector's, and so is what its
 * elements are: a base's slot vector holds function pointers (see
 * @ref x-eval-slots.h). The elements are a run of words, which is what a
 * routine in a slot takes as its arguments.
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

#include "x-obj.h"

/**
 * @name Vector Layout
 * @{
 */

/** Data units consumed by a vector's length. */
#define X_VECTOR_UNITS_LENGTH		1

/** Number of data units in a vector of @p N elements. */
#define x_vector_units(N)			(X_VECTOR_UNITS_LENGTH + (N))

/** @} */

/**
 * @name Static Lengths
 * @{
 */

/**
 * The number of static length atoms: lengths 0 to
 * #X_VECTOR_LENGTH_STATIC_LEN - 1 have one.
 */
#define X_VECTOR_LENGTH_STATIC_LEN	9

/** Static atoms holding the lengths 0 to #X_VECTOR_LENGTH_STATIC_LEN - 1. */
extern x_satom_t x_vector_length_objs[X_VECTOR_LENGTH_STATIC_LEN];

/** The static atom holding the length @p N. */
#define x_vector_length_obj(N)		((x_obj_t *)x_vector_length_objs[(N)])

/** @} */

/**
 * @name Vector Accessors
 * @{
 */

/** The length atom of vector @p V (an lvalue). */
#define x_vectorlengthobj(V)		x_obj(x_obj_data_i((V), 0))

/** The length of vector @p V. */
#define x_vectorlength(V)			x_atomint(x_vectorlengthobj((V)))

/** Element @p I of vector @p V, as a raw datum (an lvalue). */
#define x_vector_i(V,I)				x_obj_data_i((V), X_VECTOR_UNITS_LENGTH + (I))

/** Element @p I of vector @p V, as an object (an lvalue). */
#define x_vectorobj(V,I)			x_obj(x_vector_i((V), (I)))

/** Element @p I of vector @p V, as a function pointer (an lvalue). */
#define x_vectorfn(V,I)				x_fn(x_vector_i((V), (I)))

/** @} */

/**
 * @name Vector Constructors
 * @{
 */

/**
 * Heap-allocate a vector with no type holding the @p N objects that
 * follow. The `s` is the one of x_mksatom() and x_mkspair(): the built-in
 * layout, with no type of the engine's.
 */
#define x_mksvector(B,N,...) \
	x_vector_make((B), NULL, X_OBJ_FLAG_NONE, (N), __VA_ARGS__)

/** Allocate a vector of type @p p_type holding the @p length objects that follow. */
x_obj_t *x_vector_make(x_obj_t *p_base, x_obj_t *p_type, x_obj_flag_t flags,
	x_int_t length, ...);

/** @} */

#endif /* X_VECTOR_H */
