/** @file x-type/symbol.c
 *  @brief Interned symbol type -- BST index, make, find, and 3-step eval.
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2021 Jon Ruttan
 *  @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */

#include "x-type/symbol.h"
#include "x-type/str.h"
#include "x-alist.h"
#include "x-eval.h"
#include "x-eval.h"
#include "x-token/sexp/symbol.h"

x_satom_t x_type_symbol_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_SYMBOL_NAME }),
	x_type_symbol_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_symbol_make }),
	x_type_symbol_eval_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_symbol_eval }),
	x_type_symbol_struct_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_symbol_struct });

/**
 * Allocate (or retrieve interned) SYMBOL for string @p s.
 *
 * Packs the string and flags into stack-allocated args and delegates
 * to x_type_symbol_make(), which handles interning.
 *
 * @param p_base  Base (execution context).
 * @param flags   Object flags (e.g. @c X_OBJ_FLAG_OWN).
 * @param s       Null-terminated symbol name.
 * @return Interned SYMBOL object.
 */
x_obj_t *x_make_symbol(x_obj_t *p_base, x_obj_flag_t flags, x_char_t *s)
{
	x_satom_t o_symbol = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .s = s }),
		o_flags = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = flags });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_symbol }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_flags }, { NULL })
	};

	return x_type_symbol_make(p_base, (x_obj_t *)args);
}

/**
 * Build the SYMBOL type struct descriptor.
 *
 * Populates name, make, eval, analyse, read, write, and display hooks.
 *
 * @param p_base  Base (execution context).
 * @param p_obj   Unused.
 * @return Type struct pair-tree for SYMBOL.
 */
x_obj_t *x_type_symbol_struct(x_obj_t *p_base, x_obj_t *p_obj)
{
	struct x_type_t type = {
		.p_name = x_type_symbol_name,
		.p_make = x_type_symbol_make_prim,
		.p_eval = x_type_symbol_eval_prim,
		.p_analyse = x_sexp_symbol_analyse_prim,
		.p_read = x_sexp_symbol_read_prim
	};

	return x_type_struct_make(p_base, type);
}

/**
 * @name Symbol Intern BST
 *
 * Index for O(log n) symbol lookup by name.
 * Node layout: @c (symbol . (left . right)), keyed by x_symbolval.
 * Stored in @c x_restobj(x_symbol_data(p_type)).
 * @{
 */
#define x_symbol_bst(T) x_restobj(x_symbol_data((T)))

/**
 * Search the BST for a node whose symbol matches @p name.
 *
 * @param p_base  Base (execution context).
 * @param p_tree  BST root node (or NULL).
 * @param name    Symbol name to find.
 * @return BST node containing the symbol, or NULL if not found.
 */
static x_obj_t *sym_bst_lookup(x_obj_t *p_base, x_obj_t *p_tree,
	x_char_t *name)
{
	x_obj_t *p_sym, *p_children;
	int cmp;

	while ( ! x_obj_isnil(p_base, p_tree)) {
		p_sym = x_firstobj(p_tree);
		cmp = x_lib_strcmp(name, x_symbolval(p_sym));
		if (cmp == 0) {
			return p_tree;
		}
		p_children = x_restobj(p_tree);
		p_tree = (cmp < 0)
			? x_firstobj(p_children)
			: x_restobj(p_children);
	}

	return NULL;
}

/**
 * Insert a symbol into the BST, returning the (possibly unchanged) root.
 *
 * If the symbol already exists, the tree is returned unmodified.
 *
 * @param p_base  Base (execution context) (for allocation).
 * @param p_tree  Current BST root (or NULL for empty tree).
 * @param p_sym   Symbol object to insert.
 * @return BST root after insertion.
 */
static x_obj_t *sym_bst_insert(x_obj_t *p_base, x_obj_t *p_tree,
	x_obj_t *p_sym)
{
	x_obj_t *p_walk, *p_children;
	int cmp;

	if (x_obj_isnil(p_base, p_tree)) {
		return x_mkspair(p_base, X_OBJ_FLAG_NONE, p_sym,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, NULL));
	}

	p_walk = p_tree;
	for (;;) {
		cmp = x_lib_strcmp(x_symbolval(p_sym),
			x_symbolval(x_firstobj(p_walk)));
		if (cmp == 0) {
			return p_tree;
		}
		p_children = x_restobj(p_walk);
		if (cmp < 0) {
			if (x_obj_isnil(p_base, x_firstobj(p_children))) {
				x_firstobj(p_children) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
					p_sym, x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, NULL));
				return p_tree;
			}
			p_walk = x_firstobj(p_children);
		} else {
			if (x_obj_isnil(p_base, x_restobj(p_children))) {
				x_restobj(p_children) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
					p_sym, x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, NULL));
				return p_tree;
			}
			p_walk = x_restobj(p_children);
		}
	}
}

/** @} */

/**
 * Register (or retrieve) the SYMBOL type and initialize its data field.
 *
 * On first call, allocates the intern list and BST root in the type's
 * data slot.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unused.
 * @return The registered SYMBOL type object.
 */
x_obj_t *x_type_symbol_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_symbol_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_symbol_struct_prim }, { NULL })
	};
	x_obj_t *p_type = x_type_struct_get(p_base, (x_obj_t *)args);

	if (x_obj_isnil(p_base, x_symbol_data(p_type))) {
		x_symbol_data(p_type) = x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, NULL);
	}

	return p_type;
}

/**
 * Type-system make handler -- intern or allocate a new SYMBOL.
 *
 * If a symbol with the same name already exists, returns the existing
 * interned object.  Otherwise allocates a new one, appends it to the
 * intern list, and inserts it into the BST.  Ownership of the source
 * string is transferred when the source has @c X_OBJ_FLAG_OWN set.
 *
 * @details
 * Interning sequence:
 * 1. Calls x_type_symbol_find which does a BST lookup (O(log n)) on the
 *    type's intern BST.  If the symbol already exists, the existing
 *    heap-allocated symbol object is returned immediately -- no allocation.
 * 2. On miss, allocates a new symbol atom via x_obj_make, prepends it to
 *    the linear intern list (x_symbol_data_list), and inserts it into the
 *    intern BST (x_symbol_bst) for future lookups.
 * 3. String ownership: if the source atom has X_OBJ_FLAG_OWN, that flag
 *    is transferred to the new symbol and cleared on the source, ensuring
 *    exactly one owner for the malloc'd string.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Argument list: (name-atom [flags-atom]).
 * @return Interned SYMBOL object.
 *
 * @note TODO: allow the symbol list to be optionally supplied by argument.
 * @see x_type_symbol_find for the BST lookup path
 * @see sym_bst_insert for the intern BST insertion (mutating, not path-copying)
 */
x_obj_t *x_type_symbol_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = x_type_symbol_register(p_base, p_base),
		*p_symbol = x_0(p_args), *p_obj = x_type_symbol_find(p_base, p_args);
	x_obj_flag_t flags = x_obj_isnil(p_base, x_1(p_args))
		? 0 : x_firstint(x_01(p_args));

	if ( ! x_obj_isnil(p_base, p_obj)) {
		return x_firstobj(p_obj);
	}

	/* Transfer string ownership from source to new symbol. */
	if (x_obj_flags(p_symbol) & X_OBJ_FLAG_OWN) {
		flags |= X_OBJ_FLAG_OWN;
		x_obj_flags(p_symbol) &= ~X_OBJ_FLAG_OWN;
	}

	p_obj = x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_ATOM,
		x_symbolval(p_symbol));
	x_symbol_data_list(p_type) = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj,
		x_symbol_data_list(p_type));
	x_symbol_bst(p_type) = sym_bst_insert(p_base,
		x_symbol_bst(p_type), p_obj);

	return p_obj;
}

/**
 * Look up an interned symbol by name using the BST index.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Argument list whose first element wraps the name string.
 * @return BST node containing the symbol, or NULL if not interned.
 */
x_obj_t *x_type_symbol_find(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = x_type_symbol_register(p_base, p_base),
		*p_result;
	x_char_t *name = x_firststr(x_firstobj(p_args));

#ifdef X_PROFILE
	if (x_base_isset(p_base))
		x_atomint(x_firstobj(x_eval_field_profile_sym_find_calls(p_base)))++;
#endif

	/* BST lookup: O(log n) */
	p_result = sym_bst_lookup(p_base, x_symbol_bst(p_type), name);
	if ( ! x_obj_isnil(p_base, p_result)) {
		return p_result;
	}

	return NULL;
}

/**
 * Type-system eval handler -- 3-step environment lookup for symbols.
 *
 * 1. Walk the leading FRAME-marked run of the alist (the lexical frame
 *    region: every enclosing frame's locals, innermost first).
 * 2. BST lookup for globals (O(log n)).
 * 3. Continue the alist walk through the remaining chain (rare fallback).
 *
 * Raises an "Unbound SYMBOL" error if the symbol is not found.
 *
 * @details
 * The 3-step design optimizes for the common case where most lookups
 * hit either a local binding (step 1) or a global (step 2):
 *
 * @code
 *   env alist:  [local0]F -> [local1]F -> [captured]F -> [globals...]
 *                  ^                                        ^
 *                  |                                        |
 *              Step 1: walk the F(RAME) run             Step 3: walk from here
 *              (frame depth = lexical nesting, small)   (rare: base-bind cells)
 *
 *   BST:           [m]
 *                 /   \
 *              [d]     [s]         Step 2: O(log n) global lookup
 *             / \     / \
 *           ...  ... ...  ...
 * @endcode
 *
 * **Step 1** walks the leading run of X_OBJ_FLAG_FRAME-marked spine
 * cells: bindings from @c fn params, @c let, closure-scope @c def, op
 * formals and env-params -- for the CURRENT frame and every enclosing
 * frame on the chain.  Locals therefore always win over globals with
 * correct lexical semantics.  (GH #47: the old walk stopped at the
 * current frame's boundary, so an enclosing-frame capture -- e.g. the
 * dispatch ops' env-param @c e referenced from a nested arg-eval
 * closure -- fell through to a same-named global via step 2, and the
 * compensating SHADOW bit on the interned symbol made one activation's
 * shadowing blind every OTHER chain's global lookup.)
 *
 * **Step 2** performs a BST lookup on the global tree for O(log n)
 * access to top-level definitions.
 *
 * **Step 3** continues the linear walk through the rest of the chain:
 * bindings on the chain that are neither frame-marked nor BST-indexed
 * (base-bind cells; frame cells below a global cell consed onto a frame
 * head by a tail-position top-level def).
 *
 * @param p_base  Base (execution context).
 * @param p_args  Eval argument frame containing the symbol expression.
 * @return The bound value, or NULL on error.
 *
 * @see x_alist_bst_lookup for the BST search used in step 2
 * @see X_OBJ_FLAG_FRAME for the frame marking set by x_env_extend
 */
x_obj_t *x_type_symbol_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_sym_obj = x_firstobj(x_eval_arg_exp(p_args));
	x_obj_t *p_alist, *p_entry;
	/* Error-path name wrapper; filled only when the lookup misses. */
	x_satom_t sym_name;

	if ( ! x_base_isset(p_base)) {
		return NULL;
	}

	p_alist = x_firstobj(x_eval_field_env_alist(p_base));

	/* Step 1: walk the lexical frame region -- the leading run of
	 * FRAME-marked spine cells (params, let/closure defs, op formals and
	 * env-params).  This covers ENCLOSING frames too, so a captured local
	 * always wins over a same-named global; the old head-to-boundary walk
	 * saw only the current frame and let the BST hijack enclosing-frame
	 * captures (GH #47: a top-level (def e ...) poisoned every op's
	 * (eval expr e), and the compensating SHADOW bit on the interned
	 * symbol blinded OTHER chains' globals -- "Unbound SYMBOL new"). */
	while ( ! x_obj_isnil(p_base, p_alist)
		&& (x_obj_flags(p_alist) & X_OBJ_FLAG_FRAME)) {
		if (x_firstobj(x_firstobj(p_alist)) == p_sym_obj) {
			return x_restobj(x_firstobj(p_alist));
		}
		p_alist = x_restobj(p_alist);
	}

	/* Step 2: BST lookup (globals; O(log n)) */
	p_entry = x_alist_bst_lookup(p_base,
		x_eval_field_env_global_tree(p_base), p_sym_obj);
	if ( ! x_obj_isnil(p_base, p_entry)) {
		return x_restobj(p_entry);
	}

	/* Step 3: continue the walk through the remaining chain (bindings
	 * outside both the frame region and the BST -- base-bind cells, or
	 * frame cells sitting below a global cell a tail-position top-level
	 * def consed onto a frame head) */
	while ( ! x_obj_isnil(p_base, p_alist)) {
		if (x_firstobj(x_firstobj(p_alist)) == p_sym_obj) {
			return x_restobj(x_firstobj(p_alist));
		}
		p_alist = x_restobj(p_alist);
	}

	/* Not found: error */
	sym_name[X_OBJ_META_TYPE].p = (x_obj_t *)x_type_atom_obj;
	sym_name[X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_atomstr((x_obj_t *)sym_name) = x_symbolval(p_sym_obj);
	x_obj_error(p_base, "Unbound "X_TYPE_SYMBOL_NAME,
		(x_obj_t *)&sym_name);

	return NULL;
}
