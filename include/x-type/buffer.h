#ifndef X_TYPE_BUFFER_H
#define X_TYPE_BUFFER_H

/**
 * @file x-type/buffer.h
 * @brief Byte buffer type with read/write cursors for tokenizer input.
 *
 * A BUFFER wraps a character array with separate read and write pointers,
 * supporting incremental consumption from stdin or a pre-filled string.
 * The read cursor advances as characters are consumed; the write cursor
 * advances as new input arrives.  Read-only buffers (X_OBJ_FLAG_RO) do
 * not extend from stdin when exhausted.
 *
 * Buffer layout: @c (base-ptr . (read-ptr . write-ptr)).
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

#ifndef X_TYPE_BUFFER_NAME
#define X_TYPE_BUFFER_NAME		"BUFFER"     /**< Canonical type name (overridable). */
#endif /* X_TYPE_BUFFER_NAME */


/** @name Predicates
 * @{ */
/** Test whether @p X is a BUFFER object. */
#define x_obj_type_isbuffer(B,X)	x_obj_is_type((B), (X), X_TYPE_BUFFER_NAME)
/** @} */

/** @name Accessors
 * @{ */
/** Base pointer -- start of the underlying character array. */
#define x_bufferval(X)				x_firststr(X)
/** Read cursor -- next character to be consumed. */
#define x_bufferread(X)				x_firststr(x_restobj(X))
/** Write cursor -- next position for incoming data. */
#define x_bufferwrite(X)			x_reststr(x_restobj(X))
/** Number of characters already consumed (read - base). */
#define x_bufferlen(X)				(x_bufferread(X) - x_bufferval(X))
/** Number of characters available to read (write - read). */
#define x_bufferunread(X)			(x_bufferwrite(X) - x_bufferread(X))
/** The last character consumed (one behind the read cursor). */
#define x_bufferlastchar(X)			(x_bufferread(X)[-1])
/** True when the read cursor has reached the write cursor. */
#define x_buffereof(X)				(x_bufferread(X) >= x_bufferwrite(X))
/** @} */

/** @name Constructors
 * @{ */
/** Create a read/write buffer with default flags. */
#define x_mkbuffer(B, P)			x_make_buffer((B), X_OBJ_FLAG_NONE, (P))
/** Create a read-only buffer. */
#define x_mkbufferro(B, P)			x_make_buffer((B), X_OBJ_FLAG_RO, (P))
/** Create a buffer with explicit flags. */
#define x_mkfbuffer(B, F, P)		x_make_buffer((B), (F), (P))
/** Create an owning buffer (frees underlying array on GC). */
#define x_mkbufferown(B, P)			x_make_buffer((B), X_OBJ_FLAG_OWN, (P))
/** Create an owning buffer with extra flags. */
#define x_mkfbufferown(B, F, P)		x_make_buffer((B), X_OBJ_FLAG_OWN | (F), (P))
/** @} */

/** Allocate a BUFFER wrapping the character array at @p p. */
x_obj_t *x_make_buffer(x_obj_t *p_base, x_obj_flag_t flags, void *p);

/** Register (or retrieve) the BUFFER type in the type alist. */
x_obj_t *x_type_buffer_register(x_obj_t *p_base, x_obj_t *p_args);
/** Build the BUFFER type struct descriptor. */
x_obj_t *x_type_buffer_struct(x_obj_t *p_base, x_obj_t *p_args);
/** GC mark handler -- flags inner bookkeeping object without traversing raw pointers. */
x_obj_t *x_type_buffer_mark(x_obj_t *p_base, x_obj_t *p_args);
/** Type-system make handler for BUFFER objects. */
x_obj_t *x_type_buffer_make(x_obj_t *p_base, x_obj_t *p_args);
/** Reset both cursors to the base pointer. */
x_obj_t *x_type_buffer_reset(x_obj_t *p_base, x_obj_t *p_obj);
/** Compact unread data to the front of the array. */
x_obj_t *x_type_buffer_retain(x_obj_t *p_base, x_obj_t *p_args);
/** Append a single character at the write cursor. */
x_obj_t *x_type_buffer_append(x_obj_t *p_base, x_obj_t *p_args);
/** Read one character, extending from stdin if needed. */
x_obj_t *x_type_buffer_read(x_obj_t *p_base, x_obj_t *p_args);
/** Read one text character, treating NUL as EOF. */
x_obj_t *x_type_buffer_read_text(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_BUFFER_H */
