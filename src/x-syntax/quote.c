/*
 * # Computational Expressions in C
 *
 * ## x-syntax/quote.c -- Syntax - Quotation
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2024 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 *
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
#include "x-prim.h"

/* quote: (quote x) -> x, unevaluated */
static x_obj_t *x_prim_quote(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_form;
	x_args(p_args, 2, NULL, &p_form);

	return p_form;
}

x_obj_t *x_syntax_quote_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_callable_entry_t entries[] = {
		{ "lit", x_prim_quote }
	};

	x_callable_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
