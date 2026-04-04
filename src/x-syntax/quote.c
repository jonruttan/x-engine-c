/** @file quote.c
 *  @brief Syntax - Quotation (lit)
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2024 Jon Ruttan
 *  @license MIT No Attribution (MIT-0)
 */

/*     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
#include "x-prim.h"

/**
 * Quote form. x-lang: (lit expr)
 *
 * Returns the argument unevaluated (fexpr -- args are not evaluated).
 *
 * @param p_base  Execution context.
 * @param p_args  Unevaluated argument list; expects (caller expr).
 * @return The unevaluated expression.
 */
static x_obj_t *x_prim_quote(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_form;
	x_args(p_args, 2, NULL, &p_form);

	return p_form;
}

/**
 * Register quotation syntax primitives.
 *
 * Binds: lit.
 *
 * @param p_base  Execution context.
 * @param p_args  Unused.
 * @return p_base.
 */
x_obj_t *x_syntax_quote_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_callable_entry_t entries[] = {
		{ "lit", x_prim_quote }
	};

	x_callable_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
