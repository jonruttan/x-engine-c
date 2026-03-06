#ifndef X_LISP_H
#define X_LISP_H

/*
 * # Computational Expressions in C
 *
 * ## x-obj.h -- Header - Objects
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
#include "x-lib.h"

/* TODO: Deprecate as requirement. */

#define x_cons(B,X,Y)		x_mkspair((B), (X), (Y))

#define x_car(X)			x_firstobj(X)
#define x_cdr(X)			x_restobj(X)
#define x_caar(X)			x_car(x_car(X))
#define x_cadr(X)			x_car(x_cdr(X))
#define x_cdar(X)			x_cdr(x_car(X))
#define x_cddr(X)			x_cdr(x_cdr(X))
#define x_caaar(X)			x_car(x_caar(X))
#define x_caadr(X)			x_car(x_cadr(X))
#define x_cadar(X)			x_car(x_cdar(X))
#define x_caddr(X)			x_car(x_cddr(X))
#define x_cdaar(X)			x_cdr(x_caar(X))
#define x_cdadr(X)			x_cdr(x_cadr(X))
#define x_cddar(X)			x_cdr(x_cdar(X))
#define x_cdddr(X)			x_cdr(x_cddr(X))
#define x_caaaar(X)			x_car(x_caaar(X))
#define x_caaadr(X)			x_car(x_caadr(X))
#define x_caadar(X)			x_car(x_cadar(X))
#define x_caaddr(X)			x_car(x_caddr(X))
#define x_cadaar(X)			x_car(x_cdaar(X))
#define x_cadadr(X)			x_car(x_cdadr(X))
#define x_caddar(X)			x_car(x_cddar(X))
#define x_cadddr(X)			x_car(x_cdddr(X))
#define x_cdaaar(X)			x_cdr(x_caaar(X))
#define x_cdaadr(X)			x_cdr(x_caadr(X))
#define x_cdadar(X)			x_cdr(x_cadar(X))
#define x_cdaddr(X)			x_cdr(x_caddr(X))
#define x_cddaar(X)			x_cdr(x_cdaar(X))
#define x_cddadr(X)			x_cdr(x_cdadr(X))
#define x_cdddar(X)			x_cdr(x_cddar(X))
#define x_cddddr(X)			x_cdr(x_cdddr(X))

#define x_setcar(X,Y)		(x_car((X)) = (Y))
#define x_setcdr(X,Y)		(x_cdr((X)) = (Y))

#endif /* X_LISP_H */
