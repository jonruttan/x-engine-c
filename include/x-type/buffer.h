#ifndef X_TYPE_BUFFER_H
#define X_TYPE_BUFFER_H

/*
 * # Computational Expressions in C
 *
 * ## x-type/buffer.h -- Header - Type - Buffer
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2021 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 *
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
/*
 * # Includes
 */
#include "x-type.h"

#ifndef X_TYPE_BUFFER_NAME
#define X_TYPE_BUFFER_NAME		"BUFFER"
#endif /* X_TYPE_BUFFER_NAME */

#define X_TYPE_BUFFER_WRITE_STR		"#<buffer>"
#define X_TYPE_BUFFER_WRITE_LEN		9

/*
 * # Macros
 */
#define x_obj_type_isbuffer(B,X)	x_obj_is_type((B), (X), X_TYPE_BUFFER_NAME)

#define x_bufferval(X)				x_firststr(X)
#define x_bufferread(X)				x_firststr(x_restobj(X))
#define x_bufferwrite(X)			x_reststr(x_restobj(X))
#define x_bufferlen(X)				(x_bufferread(X) - x_bufferval(X))
#define x_bufferunread(X)			(x_bufferwrite(X) - x_bufferread(X))
#define x_bufferlastchar(X)			(x_bufferread(X)[-1])

#define x_mkbuffer(B, P)			x_make_buffer((B), X_OBJ_FLAG_NONE, (P))
#define x_mkfbuffer(B, F, P)		x_make_buffer((B), (F), (P))
#define x_mkbufferown(B, P)			x_make_buffer((B), X_OBJ_FLAG_OWN, (P))
#define x_mkfbufferown(B, F, P)		x_make_buffer((B), X_OBJ_FLAG_OWN | (F), (P))

/*
 * # Data Structures
 */
x_obj_t *x_make_buffer(x_obj_t *p_base, x_obj_flag_t flags, void *p);

x_obj_t *x_type_buffer_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_buffer_struct(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_buffer_make(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_buffer_reset(x_obj_t *p_base, x_obj_t *p_obj);
x_obj_t *x_type_buffer_retain(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_buffer_append(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_buffer_read(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_buffer_read_text(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_type_buffer_write(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_BUFFER_H */
