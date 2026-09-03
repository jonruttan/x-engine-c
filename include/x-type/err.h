#ifndef X_TYPE_ERR_H
#define X_TYPE_ERR_H

/**
 * @file err.h
 * @brief The raised-error type: a structured error value the language owns.
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
/*
 * # Includes
 */
#include "x-type.h"

#ifndef X_TYPE_ERR_NAME
#define X_TYPE_ERR_NAME		"ERR"	/**< Type-system symbol name */
#endif /* X_TYPE_ERR_NAME */

/*
 * # Macros
 */
/** Test whether object X is a raised error on base B. */
#define x_obj_type_iserr(B,X)	x_obj_is_type((B), (X), X_TYPE_ERR_NAME)

/** An ERR is a PAIR-shaped typed object: (code . obj).
 *
 * The error's CODE: a static-string atom naming what went wrong.
 *
 * The string is the C message literal from the raise site ("Unbound
 * SYMBOL", "type: no + for"), repointed in place on each raise -- the
 * literals have static storage, so no copy and no allocation is needed
 * to make one reachable from x-lang.  The language maps the code to
 * whatever kind vocabulary it likes; the engine does not name kinds. */
#define x_err_code(X)		x_firstobj((X))

/** The error's SUBJECT: what the raise site was complaining about -- the
 * unbound symbol's name, the offending operand -- as a static-string
 * atom, repointed in place like the code.  Empty string when the raise
 * named no subject.
 *
 * A STRING and not the object itself, deliberately.  Raise sites build
 * their subject on the C STACK (x-type/symbol.c:366 fills a local array
 * and passes its address), and the longjmp out of x_eval_error destroys
 * that frame -- so retaining the pointer hands the language a dangling
 * one.  What IS stable is the string it points at: an interned symbol's
 * name outlives any frame.  The engine has always trusted exactly that
 * much (x_eval_error extracts x_atomstr(p_obj) and keeps nothing else),
 * and this slot is that same trust made reachable instead of flattened
 * into a sentence. */
#define x_err_subject(X)	x_restobj((X))

/*
 * # Functions
 */
/** Register or retrieve the ERR type on the base context. */
x_obj_t *x_type_err_register(x_obj_t *p_base, x_obj_t *p_args);

/** Build the ERR type descriptor. */
x_obj_t *x_type_err_struct(x_obj_t *p_base, x_obj_t *p_obj);

#endif /* X_TYPE_ERR_H */
