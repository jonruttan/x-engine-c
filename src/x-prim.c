/** @file x-prim.c
 *  @brief Primitive evaluation helpers, environment extension, and registration.
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
/*
 * # Includes
 */
#include "x-prim.h"
#include "x-syntax.h"
#include "x-alist.h"
#include "x-eval.h"
#include "x-env.h"
#include "x-type.h"
#include "x-type/list.h"
#include "x-type/prim.h"
#include "x-type/symbol.h"

/**
 * Bind a C primitive function in the base's root environment.
 *
 * Creates a symbol, wraps the C function as a prim object, and binds the
 * pair in the root, where every environment in the base reaches it.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param name    x_char_t* -- Symbol name to bind
 * @param fn      x_fn_t -- C function pointer
 *
 * @details **Symbol interning.**  x_make_symbol interns the name so
 *          that all references to the same name share a single symbol
 *          object.  This enables O(1) pointer-identity comparison in
 *          the environment lookups.
 *
 * @note Called during interpreter bootstrap (x_prim_register) and
 *       by FFI registration.  All bindings created here are permanent
 *       globals that survive scope changes.
 *
 * @see x_value_bind          -- lower-level helper that binds any value
 * @see x_callable_bind_table -- batch registration of multiple primitives
 * @see x_env_bind            -- the binder, which `def` shares
 */
void x_callable_bind(x_obj_t *p_base, x_char_t *name, x_fn_t fn)
{
	x_obj_t *p_sym = x_make_symbol(p_base, X_OBJ_FLAG_NONE, name),
		*p_prim = x_mkprim(p_base, fn);

	x_env_bind(p_base, x_eval_field_env_root(p_base), p_sym, p_prim);
}

/**
 * Bind a named symbol to an arbitrary value in the base's root environment.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param name    x_char_t* -- Symbol name to bind
 * @param p_val   x_obj_t* -- Value to bind
 *
 * @see x_callable_bind -- convenience wrapper for binding C primitives
 */
void x_value_bind(x_obj_t *p_base, x_char_t *name, x_obj_t *p_val)
{
	x_obj_t *p_sym;
	x_obj_t **p_cell = x_heap_root_slot(p_base);
	x_spair_t root = x_obj_set((x_obj_t *)x_type_pair_obj, X_OBJ_FLAG_NONE,
		{ NULL }, { NULL });

	/* Root p_val while x_make_symbol allocates.  Allocation itself cannot
	 * collect (the trigger is explicit-only), but the registered cell keeps
	 * this frame correct should that policy ever change. */
	x_firstobj((x_obj_t *)root) = p_val;
	x_heap_root_push(p_cell, root);
	p_sym = x_make_symbol(p_base, X_OBJ_FLAG_NONE, name);
	x_heap_root_pop(p_cell);

	x_env_bind(p_base, x_eval_field_env_root(p_base), p_sym, p_val);
}

/**
 * Bind an array of C primitive entries into the global environment.
 *
 * Iterates @p table and calls x_callable_bind for each entry.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param table   const x_callable_entry_t* -- Array of (name, fn) entries
 * @param count   int -- Number of entries
 *
 * @see x_callable_bind
 */
void x_callable_bind_table(x_obj_t *p_base, const x_callable_entry_t *table, int count)
{
	int i;

	for (i = 0; i < count; i++) {
		x_callable_bind(p_base, table[i].name, table[i].fn);
	}
}

/*
 * # Primitives catalog
 *
 * Stored in the base's prims slot as an alist-of-alists
 * ((type . ((method . #<prim>) ...)) ...) keyed by type/section namespace,
 * with bare method names.  Built at registration time alongside the env
 * bindings; it becomes the source the (lean) env is populated from once
 * de-registration lands, and later migrates onto the type objects as static
 * methods.  Namespace/method names are interned, so lookups compare by pointer.
 */

/** The catalog value (car of the prims cell); nil before any registration. */
x_obj_t *x_prims(x_obj_t *p_base)
{
	return x_firstobj(x_eval_field_prims(p_base));
}

/* The (ns . methods) domain pair for p_ns, or NULL.  ns is interned. */
static x_obj_t *x_prims_domain_pair(x_obj_t *p_base, x_obj_t *p_ns)
{
	x_obj_t *p_cur = x_prims(p_base);

	while ( ! x_obj_isnil(p_base, p_cur)) {
		if (x_firstobj(x_firstobj(p_cur)) == p_ns)
			return x_firstobj(p_cur);
		p_cur = x_restobj(p_cur);
	}
	return NULL;
}

/* Splice one (method . value) entry into p_ns's catalog domain, creating the
 * domain on first use.  The shared filing core of C-side registration
 * (x_prims_add) and x-lang-side registration (prim-reg!).  p_ns and p_entry
 * must already be pinned by the caller -- the conses here can trigger GC.
 *
 * Conses are built with x_mklist (the list-pair type), NOT x_mkspair: the
 * catalog must be an ordinary iterable x-lang list so (prims) supports pair?,
 * map, fold, etc. -- the x-lang catalog->methods mapping walks it.  Structural
 * x_mkspair pairs are not pair? and segfault the iterator protocol. */
static void x_prims_file(x_obj_t *p_base, x_obj_t *p_ns, x_obj_t *p_entry)
{
	x_obj_t *p_dom, *p_methods, *p_newdom;
	x_obj_t **p_cell = x_heap_root_slot(p_base);
	x_spair_t root = x_obj_set((x_obj_t *)x_type_pair_obj, X_OBJ_FLAG_NONE,
		{ NULL }, { NULL });

	p_dom = x_prims_domain_pair(p_base, p_ns);

	if (p_dom != NULL) {
		/* Existing namespace: prepend the entry to its method alist (a
		 * re-registration shadows the older entry on lookup). */
		x_restobj(p_dom) = x_mklist(p_base,
			p_entry, x_restobj(p_dom));
	} else {
		/* New namespace: prepend (ns . (entry)) to the catalog, rooting the
		 * partial spine across each subsequent cons. */
		p_methods = x_mklist(p_base, p_entry, NULL);
		x_firstobj((x_obj_t *)root) = p_methods;
		x_heap_root_push(p_cell, root);
		p_newdom = x_mklist(p_base, p_ns, p_methods);
		x_restobj((x_obj_t *)root) = p_newdom;
		x_firstobj(x_eval_field_prims(p_base)) = x_mklist(p_base,
			p_newdom, x_prims(p_base));
		x_heap_root_pop(p_cell);
	}
}

/* File one (ns/method -> fn) entry into the catalog.  The freshly interned
 * symbols, prim, and conses are pinned on the eval-list root across the
 * allocations that follow, since they are not yet reachable from the base and
 * -O2 stack scanning is unreliable (see the gc-rooting note). */
static void x_prims_add(x_obj_t *p_base, x_char_t *ns, x_char_t *method, x_fn_t fn)
{
	x_obj_t *p_ns, *p_entry;
	x_obj_t **p_cell = x_heap_root_slot(p_base);
	x_spair_t root = x_obj_set((x_obj_t *)x_type_pair_obj, X_OBJ_FLAG_NONE,
		{ NULL }, { NULL });

	p_ns = x_make_symbol(p_base, X_OBJ_FLAG_NONE, ns);
	x_firstobj((x_obj_t *)root) = p_ns;
	x_heap_root_push(p_cell, root);

	/* (method . #<prim>) */
	p_entry = x_mklist(p_base,
		x_make_symbol(p_base, X_OBJ_FLAG_NONE, method),
		x_mkprim(p_base, fn));
	x_restobj((x_obj_t *)root) = p_entry;

	x_prims_file(p_base, p_ns, p_entry);

	x_heap_root_pop(p_cell);
}

/* Keep-list names: the approved permanent global vocabulary (the operators,
 * the identity predicates, call/cc -- the R5 keep-list).  These bind bare
 * even when their catalog namespace is de-registered: eq?/same? file under
 * ns `obj`, the operators under ns `int`, call/cc under ns `ctrl`, and those
 * namespaces' OTHER names retire while these stay. */
static int x_prims_name_kept(const x_char_t *name)
{
	static const char *const list[] = {
		"+", "-", "*", "/", "%", "~", "&", "|", "^", "<<", ">>",
		"=", "<", "eq?", "same?", "call/cc", NULL
		/* (write/display left this list when the printers homed to x:
		 * boot/printer.x binds the bare verbs itself.) */
	};
	int i;

	if (name == NULL)
		return 0;
	for (i = 0; list[i] != NULL; i++)
		if (x_lib_strcmp(name, list[i]) == 0)
			return 1;
	return 0;
}

/* Namespaces whose bare env names have been de-registered: their prims live
 * only in the catalog and the object-system classes, not as transitional bare
 * names.  Migrating call sites to the class access (e.g. (clock) -> (Sys clock))
 * lets a namespace move here; the list shrinks the env one namespace at a time,
 * and the whole de-registration check is removed once it is empty. */
static int x_prims_ns_deregistered(const x_char_t *ns)
{
	static const char *const list[] = {
		"sys", "iter", "base", "heap", "str", "obj", "buf", "tok", "type",
		"ptr", "ffi", "io", "char", "int", "mem", NULL
	};
	int i;

	if (ns == NULL)
		return 0;
	for (i = 0; list[i] != NULL; i++)
		if (x_lib_strcmp(ns, list[i]) == 0)
			return 1;
	return 0;
}

/**
 * Bind a table into the env and file its cataloged entries.
 *
 * Each entry is bound into the env by @c name (transitional, so existing code
 * keeps working) and, when it carries a namespace, filed into the catalog
 * under @c (ns . ((method . prim) ...)).  De-registration removes the env
 * binding here, leaving the catalog as the single source.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param table   const x_prim_entry_t* -- Array of entries
 * @param count   int -- Number of entries
 */
void x_prims_bind_table(x_obj_t *p_base, const x_prim_entry_t *table, int count)
{
	int i;

	for (i = 0; i < count; i++) {
		/* Bind the transitional bare name unless the namespace has been
		 * de-registered -- then the catalog/classes are the only source.
		 * Keep-list names (the approved global vocabulary) always bind. */
		if ( ! x_prims_ns_deregistered(table[i].ns)
				|| x_prims_name_kept(table[i].name))
			x_callable_bind(p_base, table[i].name, table[i].fn);
		if (table[i].ns != NULL)
			x_prims_add(p_base, table[i].ns, table[i].method, table[i].fn);
	}
}

/* The x-lang-facing catalog protocol (prims / prim-domain / prim-ref /
 * prim-reg! / use) used to live here as C prims; it is pure x-lang now
 * (boot/registry.x reads, boot/reflect.x writes), walking the same prims
 * cell via the committed path contract tools/contract/base-paths.x. */

/**
 * Register all built-in primitives into the environment.
 *
 * Binds the #t and #f boolean singletons, caches them in the base
 * object, then delegates to each primitive module's register function
 * (core, quote, binding, closure, control, arith, pred, string, io,
 * type, ffi, callcc).
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Unused
 * @return x_obj_t* -- p_base
 */
x_obj_t *x_prim_register(x_obj_t *p_base, x_obj_t *p_args)
{
	/* Bind #t and #f as boolean singletons, cache in base. */
	x_value_bind(p_base, x_atomstr(x_true_obj), (x_obj_t *)&x_true_obj);
	x_firstobj(x_eval_field_true(p_base)) = (x_obj_t *)&x_true_obj;

	x_value_bind(p_base, x_atomstr(x_false_obj), (x_obj_t *)&x_false_obj);
	x_firstobj(x_eval_field_false(p_base)) = (x_obj_t *)&x_false_obj;

	/* The catalog access protocol (prims / prim-domain / prim-ref /
	 * prim-reg!) is pure x-lang: boot/registry.x walks the prims cell via
	 * tools/contract/base-paths.x before any other X code loads, and boot/reflect.x
	 * supplies the mutating half.  C only FILES the catalog, right here. */
	x_prim_core_register(p_base, p_args);
	x_syntax_quote_register(p_base, p_args);
	x_syntax_binding_register(p_base, p_args);
	x_syntax_closure_register(p_base, p_args);
	x_syntax_control_register(p_base, p_args);
	x_prim_arith_register(p_base, p_args);
	x_prim_pred_register(p_base, p_args);
	x_prim_string_register(p_base, p_args);
	x_prim_io_register(p_base, p_args);
	x_prim_heap_register(p_base, p_args);
	x_prim_image_register(p_base, p_args);
	x_prim_type_register(p_base, p_args);
	x_prim_base_register(p_base, p_args);
	x_prim_buffer_register(p_base, p_args);
	x_prim_iter_register(p_base, p_args);
	x_prim_ffi_register(p_base, p_args);
	x_prim_callcc_register(p_base, p_args);
#ifdef X_SIGNAL
	x_prim_signal_register(p_base, p_args);
#endif

	return p_base;
}
