/** @file x-env.c
 *  @brief Environments -- make, look up, bind, and the child a call makes.
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2026 Jon Ruttan
 *  @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
#include "x-env.h"
#include "x-alist.h"
#include "x-type/list.h"
#include "x-type/symbol.h"

/**
 * Make an empty environment whose parent is @p p_parent.
 *
 * An environment is one pair, @c (bindings . parent).  A nil parent makes
 * a root, whose bindings are kept as a tree; any other environment keeps
 * an alist.  x_eval_make builds the base's root this way; procedure and
 * operative calls make children through x_env_extend; `guard` makes one
 * for its error variable.
 *
 * @param p_base    x_obj_t* -- Base (execution context)
 * @param p_parent  x_obj_t* -- The enclosing environment, or nil for a root
 * @return x_obj_t* -- The new, empty environment
 */
x_obj_t *x_env_make(x_obj_t *p_base, x_obj_t *p_parent)
{
	return x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, p_parent);
}

/**
 * The base's own symbol spelled like @p p_sym, or NULL.
 *
 * Symbols intern per base, so a symbol read or made in another base is a
 * different object from this base's symbol of the same spelling.  The
 * intern table answers which object this base uses for the spelling.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_sym   x_obj_t* -- A symbol, this base's or another's
 * @return x_obj_t* -- This base's symbol for the spelling, or NULL when
 *                     it has none
 */
static x_obj_t *x_env_own_symbol(x_obj_t *p_base, x_obj_t *p_sym)
{
	/* x_type_symbol_find reads the name through the first argument's
	 * string slot, which a symbol has. */
	x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_sym }, { NULL });
	x_obj_t *p_node = x_type_symbol_find(p_base, (x_obj_t *)args);

	return x_obj_isnil(p_base, p_node) ? NULL : x_firstobj(p_node);
}

/**
 * The cell binding @p p_sym in @p p_env or an ancestor.
 *
 * Walks from @p p_env to the root: an environment with a parent searches
 * its alist of @c (name . value) cells by symbol identity; the root
 * searches its tree, also by identity.  The first hit wins, so a child's
 * binding shadows a parent's.  This is the whole of symbol lookup --
 * x_type_symbol_eval and `set!` call nothing else.
 *
 * Symbols intern per base, and a name is found by identity, not by
 * spelling: a base's own symbol finds only what was bound under it, so a
 * name the host bound into a child under the host's symbol is not found
 * by the child's symbol of the same spelling.  A FOREIGN symbol -- one
 * interned in another base, as every symbol of a form the host read and
 * evaluates in a child is -- has no identity here, so it stands for this
 * base's own symbol of its spelling, and that is what is looked up.  That
 * is what lets `(base eval B (lit (+ 2 3)))` hand a child the host's `+`
 * and reach the child's binding of its own `+`.  The retry runs only when
 * the identity lookup at the root missed.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_env   x_obj_t* -- The environment to start from
 * @param p_sym   x_obj_t* -- The symbol
 * @return x_obj_t* -- The @c (name . value) cell, or NULL when unbound
 */
x_obj_t *x_env_lookup(x_obj_t *p_base, x_obj_t *p_env, x_obj_t *p_sym)
{
	x_obj_t *p_cell, *p_entry, *p_own;

	for (; ! x_obj_isnil(p_base, p_env); p_env = x_env_parent(p_env)) {
		if (x_env_isroot(p_base, p_env)) {
			p_entry = x_alist_bst_lookup(p_base,
				x_env_bindings(p_env), p_sym);
			if ( ! x_obj_isnil(p_base, p_entry)) {
				return p_entry;
			}

			p_own = x_env_own_symbol(p_base, p_sym);
			if (p_own != NULL && p_own != p_sym) {
				p_entry = x_alist_bst_lookup(p_base,
					x_env_bindings(p_env), p_own);
				if ( ! x_obj_isnil(p_base, p_entry)) {
					return p_entry;
				}
			}
			continue;
		}

		for (p_cell = x_env_bindings(p_env);
			! x_obj_isnil(p_base, p_cell);
			p_cell = x_restobj(p_cell)) {
			if (x_firstobj(x_firstobj(p_cell)) == p_sym) {
				return x_firstobj(p_cell);
			}
		}
	}

	return NULL;
}

/**
 * Bind @p p_sym to @p p_val in @p p_env itself.
 *
 * A binding the environment already holds is updated in place; otherwise
 * one is added -- to the root's tree, or in front of another
 * environment's alist.  A parent's binding of the same name is never
 * touched: it is shadowed, which is what a definition in a child means.
 * `def` is this on the current environment; the C binding doors and
 * `base bind` are this on a root.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_env   x_obj_t* -- The environment to bind in
 * @param p_sym   x_obj_t* -- The symbol
 * @param p_val   x_obj_t* -- The value
 * @return x_obj_t* -- @p p_val
 *
 * @note The tree insert mutates in place (x_alist_bst_insert), so every
 *       closure whose chain reaches this root sees the new binding at its
 *       next lookup -- a top-level definition made after a closure was
 *       created is visible to it, as it must be.
 */
x_obj_t *x_env_bind(x_obj_t *p_base, x_obj_t *p_env,
	x_obj_t *p_sym, x_obj_t *p_val)
{
	x_obj_t *p_cell, *p_pair;

	if (x_env_isroot(p_base, p_env)) {
		p_cell = x_alist_bst_lookup(p_base, x_env_bindings(p_env), p_sym);
		if ( ! x_obj_isnil(p_base, p_cell)) {
			x_restobj(p_cell) = p_val;
			return p_val;
		}

		p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_sym, p_val);
		x_env_bindings(p_env) = x_alist_bst_insert(p_base,
			x_env_bindings(p_env), p_pair);

		return p_val;
	}

	for (p_cell = x_env_bindings(p_env);
		! x_obj_isnil(p_base, p_cell);
		p_cell = x_restobj(p_cell)) {
		if (x_firstobj(x_firstobj(p_cell)) == p_sym) {
			x_restobj(x_firstobj(p_cell)) = p_val;
			return p_val;
		}
	}

	p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_sym, p_val);
	x_env_bindings(p_env) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		p_pair, x_env_bindings(p_env));

	return p_val;
}

/* The child a call makes is the evaluator's business: unit tests that
 * exercise only the base layer omit it by defining STUB_X_EVAL or
 * X_EVAL_OWN before #including this file, as they omit x-eval.c's engine. */
#if !defined(STUB_X_EVAL) && !defined(X_EVAL_OWN)
/**
 * Make a child environment with parameters bound to values.
 *
 * The environment a procedure body or an operative body runs in: a fresh
 * @c (bindings . parent) pair whose parent is @p p_parent, the closure's
 * or the operative's static environment, and whose bindings are the
 * parameters.  Handles three cases: (1) variadic -- a bare symbol binds to
 * the entire remaining value list, (2) base -- no more params, (3) one
 * parameter to one value, then the rest.
 *
 * @param p_base   x_obj_t* -- Base (execution context)
 * @param p_parent x_obj_t* -- The environment the new one is a child of
 * @param p_params x_obj_t* -- Parameter list (or single symbol for variadic)
 * @param p_vals   x_obj_t* -- Value list
 * @return x_obj_t* -- The new environment
 *
 * @details **The parent is never modified.**  The bindings are new cells
 *          in the new environment; @p p_parent is only pointed at.  Fewer
 *          values than parameters binds the remainder to nil, symmetric
 *          with surplus values, which are ignored once the parameters run
 *          out.
 *
 * @note The variadic case (bare symbol for p_params) binds the ENTIRE
 *       remaining value list, not just one value.  This implements
 *       rest-parameter semantics: @c (fn (a . rest) ...).
 *
 * @see x_env_bind        -- `def`, the same binder one name at a time
 * @see x_eval_body_tco   -- saves/restores env around a body
 */
x_obj_t *x_env_extend(x_obj_t *p_base, x_obj_t *p_parent,
	x_obj_t *p_params, x_obj_t *p_vals)
{
	x_obj_t *p_env = x_env_make(p_base, p_parent);
	x_obj_t *p_pair;
	x_obj_t *p_val;
	x_obj_t **pp_spine;

	while ( ! x_obj_isnil(p_base, p_params)) {
		/* Variadic: single symbol binds to entire remaining arg list. */
		if (x_obj_type_issymbol(p_base, p_params)) {
			/* Callers self-pass via transient stack pairs (NULL type
			 * slot) at the head of p_vals -- x_type_procedure_call's sp,
			 * x_callable_apply sites' stack-built arg lists.  A bare-
			 * variadic binding captures the spine itself, and the binding
			 * outlives those frames (TCO defers the body to the
			 * trampoline; apply-path closures can escape with the env),
			 * so materialize every leading stack pair on the heap.  Heap
			 * spines carry x_type_pair_obj and pass through untouched. */
			for (pp_spine = &p_vals;
				*pp_spine != NULL && x_obj_type(*pp_spine) == NULL;
				pp_spine = &x_restobj(*pp_spine)) {
				*pp_spine = x_mklist(p_base,
					x_firstobj(*pp_spine), x_restobj(*pp_spine));
			}

			p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_params, p_vals);
			x_env_bindings(p_env) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
				p_pair, x_env_bindings(p_env));

			return p_env;
		}

		/* One parameter to one value; a missing value is nil. */
		p_val = x_obj_isnil(p_base, p_vals) ? NULL : x_firstobj(p_vals);
		p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_firstobj(p_params), p_val);
		x_env_bindings(p_env) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
			p_pair, x_env_bindings(p_env));

		p_params = x_restobj(p_params);
		p_vals = x_obj_isnil(p_base, p_vals) ? NULL : x_restobj(p_vals);
	}

	return p_env;
}

#endif /* !STUB_X_EVAL && !X_EVAL_OWN -- evaluator engine */
