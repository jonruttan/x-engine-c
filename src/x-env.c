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
 * @brief Bindings an environment under a root holds before it keeps a
 *        lookup cache.
 *
 * Below it the walk of the alist is short.  A call frame holds its
 * parameters and the definitions in its body, rarely this many, so a frame
 * seldom keeps one; x-engine-rust gives a frame its shadow map at the same
 * count.
 */
#define X_ENV_CACHE_MIN   16

/**
 * @brief The name of the cell that holds an environment's lookup cache.
 *
 * The reader never makes a symbol spelled this way, so no program binds
 * the name by writing it, and a walk of the alist that takes each cell's
 * name for a symbol finds one here too.
 */
#define X_ENV_CACHE_NAME   "#<cache>"

/**
 * Make an empty environment whose parent is @p p_parent.
 *
 * An environment is one pair, @c (bindings . parent).  A nil parent makes
 * a root, whose bindings are kept as a tree; any other environment keeps
 * an alist, and one under a root that comes to hold many bindings keeps a
 * lookup cache at the head of it (x_env_cache).  x_eval_make builds the
 * base's root this way; procedure and operative calls make children
 * through x_env_extend; `guard` makes one for its error variable.
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
 * The link to an environment's lookup cache, or NULL when it keeps none.
 *
 * An environment under a root that holds X_ENV_CACHE_MIN bindings keeps a
 * lookup cache as the first cell of its alist, and its pair carries
 * X_ENV_FLAG_CACHE:
 *
 *     (#<cache> . (parent #<cache> . tree))
 *
 * The tree holds the cells lookups through the environment have found, by
 * spelling as the root's tree does: the environment's own, and the root's.
 * A cell in it does not go stale: `def` and `set!` update a cell in place,
 * and a `def` that gives the environment a name the cache found in the root
 * puts the new cell in its place (x_env_bind).  The alist after the cache
 * cell still holds every binding, and is what a lookup the cache misses
 * walks.
 *
 * The flag is tested first, on the pair a lookup already holds, so a frame
 * costs nothing here.  The cell is the cache only while its value names the
 * environment's own parent and repeats the cell's own name, so a value a
 * program bound is never searched as a tree, and an alist whose head a
 * program has changed is walked, as any alist is.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_env   x_obj_t* -- An environment with a parent
 * @return x_obj_t* -- The pair @c (name . tree), or NULL
 */
static x_obj_t *x_env_cache(x_obj_t *p_base, x_obj_t *p_env)
{
	x_obj_t *p_cell, *p_value, *p_link;

	if ( ! (x_obj_flags(p_env) & X_ENV_FLAG_CACHE)
		|| x_obj_isnil(p_base, x_env_bindings(p_env))) {
		return NULL;
	}

	p_cell = x_firstobj(x_env_bindings(p_env));
	if (x_obj_isnil(p_base, p_cell) || ! x_obj_type_isspair(p_cell)) {
		return NULL;
	}

	p_value = x_restobj(p_cell);
	if (x_obj_isnil(p_base, p_value)
		|| ! x_obj_type_isspair(p_value)
		|| x_firstobj(p_value) != x_env_parent(p_env)) {
		return NULL;
	}

	p_link = x_restobj(p_value);
	if (x_obj_isnil(p_base, p_link)
		|| ! x_obj_type_isspair(p_link)
		|| x_firstobj(p_link) != x_firstobj(p_cell)) {
		return NULL;
	}

	return p_link;
}

/**
 * Give an environment under a root its lookup cache: an empty tree in a
 * cell at the head of its alist, and X_ENV_FLAG_CACHE on its pair.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_env   x_obj_t* -- The environment, whose parent is a root
 */
static void x_env_cache_make(x_obj_t *p_base, x_obj_t *p_env)
{
	x_obj_t *p_name = x_mksymbol(p_base, X_ENV_CACHE_NAME);

	x_env_bindings(p_env) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkspair(p_base, X_OBJ_FLAG_NONE, p_name,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_env_parent(p_env),
				x_mkspair(p_base, X_OBJ_FLAG_NONE, p_name, NULL))),
		x_env_bindings(p_env));
	x_obj_flags(p_env) |= X_ENV_FLAG_CACHE;
}

/**
 * Whether a lookup cache can hold a cell named @p p_sym.
 *
 * The tree steers by spelling, so it holds only a symbol: a name of the
 * cache's own name's type.
 *
 * @param p_link  x_obj_t* -- The cache's @c (name . tree) link
 * @param p_sym   x_obj_t* -- The name
 * @return int -- Nonzero when @p p_sym is a symbol
 */
static int x_env_cache_holds(x_obj_t *p_link, x_obj_t *p_sym)
{
	return x_obj_type(x_firstobj(p_link)) == x_obj_type(p_sym);
}

/**
 * Put @p p_entry in a lookup cache, in the place of the cell it holds for
 * the same name, if it holds one.
 *
 * @param p_base   x_obj_t* -- Base (execution context)
 * @param p_link   x_obj_t* -- The cache's @c (name . tree) link
 * @param p_entry  x_obj_t* -- A @c (name . value) cell
 */
static void x_env_cache_put(x_obj_t *p_base, x_obj_t *p_link,
	x_obj_t *p_entry)
{
	x_restobj(p_link) = x_alist_bst_insert(p_base, x_restobj(p_link),
		p_entry, X_OBJ_FLAG_NONE);
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
 * An environment that keeps a lookup cache (x_env_cache), a module's, is
 * asked its cache first, and a cell found past it, in its alist or in the
 * root, goes into the cache, so the next lookup of the name through the
 * environment is one search of a tree of the names used there: a lookup
 * from inside a module costs what one from the root does, whatever the
 * module's size.
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
 * the identity lookup at the root missed, and what it finds is never put
 * in a cache.
 *
 * Under X_PROFILE, each binding compared in an environment's alist counts
 * one in the base's profile-env-steps cell.  The root's tree and a cache
 * count their own lookups, in profile-bst-hits and profile-bst-misses, so
 * the walk to the root is what this one cell adds.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_env   x_obj_t* -- The environment to start from
 * @param p_sym   x_obj_t* -- The symbol
 * @return x_obj_t* -- The @c (name . value) cell, or NULL when unbound
 */
x_obj_t *x_env_lookup(x_obj_t *p_base, x_obj_t *p_env, x_obj_t *p_sym)
{
	x_obj_t *p_cell, *p_entry, *p_own, *p_link;
	x_obj_t *p_missed = NULL;

	for (; ! x_obj_isnil(p_base, p_env); p_env = x_env_parent(p_env)) {
		if (x_env_isroot(p_base, p_env)) {
			p_entry = x_alist_bst_lookup(p_base,
				x_env_bindings(p_env), p_sym);
			if ( ! x_obj_isnil(p_base, p_entry)) {
				if (p_missed != NULL) {
					x_env_cache_put(p_base, p_missed, p_entry);
				}

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

		p_cell = x_env_bindings(p_env);
		p_link = x_env_cache(p_base, p_env);
		if (p_link != NULL && x_env_cache_holds(p_link, p_sym)) {
			p_entry = x_alist_bst_lookup(p_base, x_restobj(p_link), p_sym);
			if ( ! x_obj_isnil(p_base, p_entry)) {
				return p_entry;
			}

			/* A miss walks the alist past the cache cell.  The cache's
			 * environment's parent is a root, so a miss there too is
			 * looked up next in the root, which puts what it finds here. */
			p_cell = x_restobj(p_cell);
			p_missed = p_link;
		} else {
			p_link = NULL;
		}

		for (; ! x_obj_isnil(p_base, p_cell); p_cell = x_restobj(p_cell)) {
#ifdef X_PROFILE
			if (x_base_isset(p_base))
				x_atomint(x_firstobj(x_eval_field_profile_env_steps(p_base)))++;
#endif
			if (x_firstobj(x_firstobj(p_cell)) == p_sym) {
				if (p_link != NULL) {
					x_env_cache_put(p_base, p_link, x_firstobj(p_cell));
				}

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
 * An environment under a root whose alist this brings to X_ENV_CACHE_MIN
 * bindings is given a lookup cache (x_env_cache_make).  In one that keeps
 * a cache, a new binding goes just after the cache cell, which stays at the
 * head, and into the cache only in the place of a cell the cache found for
 * the name in the root: that cell is the root's, and is never written from
 * here.
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
	x_obj_t *p_cell, *p_pair, *p_link;
	int n_cells = 0;

	if (x_env_isroot(p_base, p_env)) {
		p_cell = x_alist_bst_lookup(p_base, x_env_bindings(p_env), p_sym);
		if ( ! x_obj_isnil(p_base, p_cell)) {
			x_restobj(p_cell) = p_val;
			return p_val;
		}

		p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_sym, p_val);
		x_env_bindings(p_env) = x_alist_bst_insert(p_base,
			x_env_bindings(p_env), p_pair, X_OBJ_FLAG_SHARED);

		return p_val;
	}

	p_link = x_env_cache(p_base, p_env);
	p_cell = x_env_bindings(p_env);
	if (p_link != NULL) {
		p_cell = x_restobj(p_cell);
	}

	for (; ! x_obj_isnil(p_base, p_cell); p_cell = x_restobj(p_cell)) {
		if (x_firstobj(x_firstobj(p_cell)) == p_sym) {
			x_restobj(x_firstobj(p_cell)) = p_val;
			return p_val;
		}

		n_cells++;
	}

	p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_sym, p_val);

	if (p_link != NULL) {
		x_restobj(x_env_bindings(p_env)) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
			p_pair, x_restobj(x_env_bindings(p_env)));

		if (x_env_cache_holds(p_link, p_sym)
			&& ! x_obj_isnil(p_base,
				x_alist_bst_lookup(p_base, x_restobj(p_link), p_sym))) {
			x_env_cache_put(p_base, p_link, p_pair);
		}

		return p_val;
	}

	x_env_bindings(p_env) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		p_pair, x_env_bindings(p_env));

	if (n_cells + 1 >= X_ENV_CACHE_MIN
		&& x_env_isroot(p_base, x_env_parent(p_env))) {
		x_env_cache_make(p_base, p_env);
	}

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
