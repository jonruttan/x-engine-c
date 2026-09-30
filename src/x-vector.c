/**
 * @file x-vector.c
 * @brief The vector layout's constructor and static lengths, and the slot
 *        vector a base holds (see x-vector.h, x-eval-slots.h).
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

#include <stdarg.h>

#include "x-vector.h"
#include "x-eval-slots.h"

/* The static length atoms of x-vector.h, one for each of the lengths 0 to
 * X_VECTOR_LENGTH_STATIC_LEN - 1. */
x_satom_t x_vector_length_objs[X_VECTOR_LENGTH_STATIC_LEN] = {
	x_obj_set(NULL, X_OBJ_FLAG_NONE, {.i = 0}),
	x_obj_set(NULL, X_OBJ_FLAG_NONE, {.i = 1}),
	x_obj_set(NULL, X_OBJ_FLAG_NONE, {.i = 2}),
	x_obj_set(NULL, X_OBJ_FLAG_NONE, {.i = 3}),
	x_obj_set(NULL, X_OBJ_FLAG_NONE, {.i = 4}),
	x_obj_set(NULL, X_OBJ_FLAG_NONE, {.i = 5}),
	x_obj_set(NULL, X_OBJ_FLAG_NONE, {.i = 6}),
	x_obj_set(NULL, X_OBJ_FLAG_NONE, {.i = 7}),
	x_obj_set(NULL, X_OBJ_FLAG_NONE, {.i = 8})
};

/**
 * Allocate a vector and initialize its elements.
 *
 * A vector of @p length elements has its length in its first data unit
 * and its elements after it (see x-vector.h). The length is one of the
 * static length atoms when there is one for it, and an atom of its own
 * otherwise.
 *
 * @param p_base Base (allocation context).
 * @param p_type The type object to assign, or NULL.
 * @param flags  Initial object flags.
 * @param length The number of elements.
 * @param ...    The elements, @p length of them, each an `x_obj_t *`.
 * @return The new vector, or NULL on allocation failure.
 */
x_obj_t *x_vector_make(x_obj_t *p_base, x_obj_t *p_type, x_obj_flag_t flags,
	x_int_t length, ...)
{
	x_obj_t *p_obj;
	x_int_t i;
	va_list ap;

	p_obj = x_obj_alloc(p_base, p_type, flags, (size_t)x_vector_units(length));

	if (p_obj == NULL) {
		return NULL;
	}

	/* Every unit holds an object or nil before the length is allocated. */
	x_vectorlengthobj(p_obj) = NULL;

	va_start(ap, length);

	for (i = 0; i < length; i++) {
		x_vectorobj(p_obj, i) = va_arg(ap, x_obj_t *);
	}

	va_end(ap);

	x_vectorlengthobj(p_obj) = length < X_VECTOR_LENGTH_STATIC_LEN
		? x_vector_length_obj(length)
		: x_mksatom(p_base, X_OBJ_FLAG_NONE, length);

	return p_obj;
}

/**
 * Make a slot vector of @p length slots, every one empty.
 *
 * The slot vector is a vector: its first data unit holds its length, and
 * its slots follow. It has no type, and must not be given one: the vector
 * sits in the base's tree, where the collector's mark reaches it, and an
 * object with no type is marked and not descended -- its elements are
 * function pointers, not objects. It is allocated with X_OBJ_FLAG_SHARED,
 * so a sweep keeps it for as long as the base lives, and its length is a
 * static atom or a SHARED one for the same reason.
 *
 * @param p_base Base the vector belongs to (allocation context).
 * @param length The number of slots.
 * @return The slot vector, or NULL when it could not be allocated.
 */
x_obj_t *x_slots_make(x_obj_t *p_base, x_int_t length)
{
	x_obj_t *p_slots;
	x_int_t i;

	p_slots = x_obj_alloc(p_base, NULL, X_OBJ_FLAG_SHARED,
		(size_t)x_vector_units(length));

	if (p_slots == NULL) {
		return NULL;
	}

	x_vectorlengthobj(p_slots) = length < X_VECTOR_LENGTH_STATIC_LEN
		? x_vector_length_obj(length)
		: x_mksatom(p_base, X_OBJ_FLAG_SHARED, length);

	for (i = 0; i < length; i++) {
		x_slot(p_slots, i) = NULL;
	}

	return p_slots;
}
