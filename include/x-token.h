#ifndef X_TOKEN_H
#define X_TOKEN_H

/*
 * # Computational Expressions in C
 *
 * ## x-token.h -- Header - Token
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
#include "x-obj.h"

/*
 * # Defines
 */
#define x_token_read_arg_prim(X)		x_0((X))
#define x_token_read_arg_buffer(X)		x_0((X))
#define x_token_read_arg_score(X)		x_01((X))
#define x_token_read_arg_char(X)		x_0(x_11((X)))
#define x_token_read_arg_self(X)		x_0(x_1(x_11((X))))

/* State objects: prim-like but with configurable next-state slots.
 * Slot 0: C function pointer (same offset as x_primval)
 * Slot 1: next-on-done (returned instead of scoring)
 * Slot 2: next-on-loop (returned instead of self)
 * FLAG_3 marks objects with extra state slots.
 */
#define X_OBJ_FLAG_STATE		X_OBJ_FLAG_3
#define x_state_has_slots(S)	(x_obj_flags(S) & X_OBJ_FLAG_STATE)
#define x_state_slot(S, N)		(x_state_has_slots(S) ? (&x_firstobj(S))[(N)] : NULL)
#define X_STATE_DONE 1
#define X_STATE_LOOP 2

/*
 * # Data Structures
 */
x_obj_t *x_token_delimit(x_obj_t *p_base, x_obj_t *p_obj);
x_obj_t *x_token_analyse(x_obj_t *p_base, x_obj_t *p_obj);
x_obj_t *x_token_read(x_obj_t *p_base, x_obj_t *p_obj);
x_obj_t *x_token_write(x_obj_t *p_base, x_obj_t *p_obj);
x_obj_t *x_token_display(x_obj_t *p_base, x_obj_t *p_obj);

#endif /* X_TOKEN_H */
