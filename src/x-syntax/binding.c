/** @file binding.c
 *  @brief Syntax - Binding Forms (def, set!)
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2026 Jon Ruttan
 *  @license MIT No Attribution (MIT-0)
 */

/*     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
#include "x-prim.h"
#include "x-alist.h"
#include "x-eval.h"
#include "x-env.h"
#include "x-type/symbol.h"

/**
 * Define form. x-lang: (def name value)
 *
 * Binds name to the evaluated value in the CURRENT environment (fexpr --
 * name is not evaluated, value is explicitly evaluated).  That is the
 * whole rule: at top level the current environment is the root, in a
 * procedure body it is the call's own environment, in an operative body
 * the operative's, and under `(eval form e)` it is `e`.  A name the
 * environment already binds is rebound in place; a parent's binding of
 * the same name is left alone and shadowed.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated argument list; expects (caller name value).
 * @return The evaluated value.
 *
 * @details **Eval-before-bind.**  The value expression is evaluated
 *          BEFORE the name is bound, so the value expression cannot
 *          reference the binding being created.  A recursive definition
 *          still works, because a closure body resolves names at call
 *          time and the binding is in the environment by then.
 *
 * @see x_prim_set   -- mutation form (does not create bindings)
 * @see x_env_bind   -- the binder this is a form over
 */
static x_obj_t *x_prim_define(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_name, *p_val;

	x_args(p_base, p_args, 2, NULL, &p_name);
	p_val = x_eval_arg(p_base,
		x_eval_spine_first(p_base, x_11(p_args)));

	return x_env_bind(p_base, x_eval_field_env(p_base), p_name, p_val);
}

/**
 * Mutation form. x-lang: (set! name value)
 *
 * Mutates the existing binding of name to the evaluated value (fexpr --
 * name is not evaluated, value is explicitly evaluated).  The binding is
 * the one symbol evaluation would find: the current environment's, or the
 * nearest ancestor's.  Signals an error if the symbol is unbound.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated argument list; expects (caller name value).
 * @return The evaluated value.
 * @note Raises "Unbound symbol" if name has no existing binding.
 * @see x_prim_define
 */
static x_obj_t *x_prim_set(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_name, *p_val, *p_entry;
	/* Error-path name wrapper; filled only when the lookup misses. */
	x_satom_t sym_name;

	x_args(p_base, p_args, 2, NULL, &p_name);
	p_val = x_eval_arg(p_base,
		x_eval_spine_first(p_base, x_11(p_args)));

	p_entry = x_env_lookup(p_base, x_eval_field_env(p_base), p_name);
	if (p_entry != NULL) {
		x_restobj(p_entry) = p_val;
		return p_val;
	}

	sym_name[X_OBJ_META_TYPE].p = (x_obj_t *)x_type_atom_obj;
	sym_name[X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_atomstr((x_obj_t *)sym_name) = x_symbolval(p_name);
	x_obj_error(p_base, "Unbound "X_TYPE_SYMBOL_NAME,
		(x_obj_t *)&sym_name);

	return NULL;
}

/**
 * Register binding syntax primitives.
 *
 * Binds: def, set!.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unused.
 * @return p_base.
 */
x_obj_t *x_syntax_binding_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_callable_entry_t entries[] = {
		{ "def", x_prim_define },
		{ "set!", x_prim_set }
	};

	x_callable_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
