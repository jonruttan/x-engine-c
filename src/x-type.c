/** @file x-type.c
 *  @brief Type struct construction, dispatch, and GC hooks for the type system.
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
/*
 * # Includes
 */
#include "x-type.h"
#include "x-eval.h"
#include "x-heap.h"
#include "x-obj.h"
#include "x-type/prim.h"
#include "x-type/symbol.h"
#include "x-type/int.h"

#define nil			NULL
#define pair(X,Y)	(x_mkspair(p_base, X_OBJ_FLAG_NONE, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, X_OBJ_FLAG_NONE, (X)))

/**
 * Construct a type struct from a type descriptor.
 *
 * Builds the canonical nested-pair structure that represents a type
 * in the type alist: name-stack, data-stack, heap group (mark, make,
 * free, clone, units, length), proc group (call, eval), cvt group
 * (from, to), IO group (analyse, delimit, read, write, display,
 * error), iter group, and ops group.
 *
 * @param p_base  x_obj_t* -- Base (execution context) (for allocation)
 * @param type    struct x_type_t -- Type descriptor with all hook pointers
 * @return x_obj_t* -- Newly allocated type struct (pair tree)
 */
x_obj_t *x_type_struct_make(x_obj_t *p_base, struct x_type_t type)
{
	x_obj_t *p_type =
		/* name-stack */
		pair(pair(type.p_name, nil),
		/* data-stack */
		pair(pair(type.p_data, nil),
		/* Heap: '(mark-stack make-stack free-stack clone-stack units-stack length-stack) */
		pair(pair(pair(type.p_mark, nil),
			pair(pair(type.p_make, nil),
			pair(pair(type.p_free, nil),
			pair(pair(type.p_clone, nil),
			pair(pair(type.p_units, nil),
			pair(pair(type.p_length, nil),
			nil)))))),
		/* Proc: '(call-stack eval-stack) */
		pair(pair(pair(type.p_call, nil),
			pair(pair(type.p_eval, nil),
			nil)),
		/* Cvt: '(from-stack to-stack) */
		pair(pair(pair(type.p_from, nil),
			pair(pair(type.p_to, nil),
			nil)),
		/* IO: '(analyse-stack delimit-stack read-stack write-stack display-stack) */
		pair(pair(pair(type.p_analyse, nil),
			pair(pair(type.p_delimit, nil),
			pair(pair(type.p_read, nil),
			pair(pair(type.p_write, nil),
			pair(pair(type.p_display, nil),
			nil))))),
		/* Iter: '(iter-stack) */
		pair(pair(pair(type.p_iter, nil),
			nil),
		/* Ops: '(ops-stack) */
		pair(pair(pair(type.p_ops, nil),
			nil),
		nil))))))));

	return p_type;
}

#undef nil
#undef pair
#undef atom

/* Look up p_sym in a ((op-sym . handler) ...) alist by interned pointer;
 * NULL when absent.  Prepend-registration means the newest handler wins. */
static x_obj_t *x_type_ops_lookup(x_obj_t *p_base, x_obj_t *p_ops, x_obj_t *p_sym)
{
	x_obj_t *p_cur;

	for (p_cur = p_ops; ! x_obj_isnil(p_base, p_cur); p_cur = x_restobj(p_cur)) {
		if (x_firstobj(x_firstobj(p_cur)) == p_sym)
			return x_restobj(x_firstobj(p_cur));
	}
	return NULL;
}

/* 1 if p_type's from-conversion alist declares a conversion from the type
 * named p_name (entries are (type-handle . handler); the handle IS the
 * type's name atom, so the walk compares by pointer). */
static int x_type_from_has(x_obj_t *p_base, x_obj_t *p_type, x_obj_t *p_name)
{
	x_obj_t *p_cur;

	for (p_cur = x_type_field_from(p_type); ! x_obj_isnil(p_base, p_cur);
			p_cur = x_restobj(p_cur)) {
		if (x_firstobj(x_firstobj(p_cur)) == p_name)
			return 1;
	}
	return 0;
}

/*
 * Generic-operator dispatch: the ops half of the type system's hot path.
 *
 * EVERY value's type tag is its type pair tree -- ints included
 * (x_type_int_make stamps the int tree) -- so "is it typed" is not the
 * dispatch test; CARRYING A HANDLER is.  If either operand's type has a
 * handler registered for the op in its ops alist, call it as (handler a b);
 * the handler receives the raw operands and owns any coercion.  This
 * replaces the load-order-dependent set! wrapper chain: types REGISTER ops
 * (type-push-op), nothing wraps ambient names.
 *
 * When BOTH sides carry a handler for the op:
 *   - same type        -> that type's handler (no promotion question);
 *   - different types  -> the declared from-relation decides: the side whose
 *     type registers a conversion FROM the other side's type absorbs it (e.g.
 *     complex declares from float/rational/int, so complex wins over float).
 *     The winning handler owns the coercion.  No invented ordering: the rule
 *     reads the relation the cvt from-alists already declare.  Neither side
 *     declaring the other falls through.
 *
 * Ops-less types (int, str, ...) have a nil ops alist, so int/int
 * arithmetic falls through to the callers' pure-C path after a few pointer
 * reads -- no interning, no allocation.  When a handler does fire, the call
 * frame is stack-built (zero heap allocation) and the op symbol is interned
 * so the alist walk compares by pointer.
 */
int x_type_op_try(x_obj_t *p_base, x_char_t *op, x_obj_t *p_a, x_obj_t *p_b,
	x_obj_t **pp_result)
{
	x_obj_t *p_ta, *p_tb, *p_ops_a, *p_ops_b, *p_sym, *p_ha, *p_hb, *p_handler;
	x_spair_t call[3] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL })
	};

	/* Resolve each side's ops alist (guard: a static atom's tag is not a
	 * pair tree, and its fields must not be navigated). */
	p_ta = p_a == NULL ? NULL : x_obj_type(p_a);
	p_tb = p_b == NULL ? NULL : x_obj_type(p_b);
	p_ops_a = (p_ta != NULL && x_obj_type_isspair(p_ta))
		? x_type_field_ops(p_ta) : NULL;
	p_ops_b = (p_tb != NULL && x_obj_type_isspair(p_tb))
		? x_type_field_ops(p_tb) : NULL;
	if (x_obj_isnil(p_base, p_ops_a) && x_obj_isnil(p_base, p_ops_b))
		return 0;

	p_sym = x_mksymbol(p_base, op);
	p_ha = x_obj_isnil(p_base, p_ops_a)
		? NULL : x_type_ops_lookup(p_base, p_ops_a, p_sym);
	p_hb = x_obj_isnil(p_base, p_ops_b)
		? NULL : x_type_ops_lookup(p_base, p_ops_b, p_sym);

	if (p_ha == NULL && p_hb == NULL)
		return 0;
	if (p_hb == NULL) {
		p_handler = p_ha;
	} else if (p_ha == NULL) {
		p_handler = p_hb;
	} else if (p_ta == p_tb) {
		/* Same type on both sides: no promotion question. */
		p_handler = p_ha;
	} else if (x_type_from_has(p_base, p_ta, x_type_field_name(p_tb))) {
		p_handler = p_ha;             /* a's type absorbs b's */
	} else if (x_type_from_has(p_base, p_tb, x_type_field_name(p_ta))) {
		p_handler = p_hb;             /* b's type absorbs a's */
	} else {
		return 0;                     /* unrelated types: not ours to decide */
	}

	/* (handler a b) on a stack-built frame -- zero heap allocation. */
	x_firstobj((x_obj_t *)call) = p_handler;
	x_restobj((x_obj_t *)call) = (x_obj_t *)(call + 1);
	x_firstobj((x_obj_t *)(call + 1)) = p_a;
	x_restobj((x_obj_t *)(call + 1)) = (x_obj_t *)(call + 2);
	x_firstobj((x_obj_t *)(call + 2)) = p_b;
	x_restobj((x_obj_t *)(call + 2)) = NULL;

	*pp_result = x_callable_call(p_base, (x_obj_t *)call);
	return 1;
}

/**
 * Retrieve or create a type struct by name.
 *
 * First checks the type alist cache. On miss, calls the callable in
 * rest of @p p_args to construct the type, then caches it in the
 * type alist for future lookups.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- (type-name . constructor-callable)
 * @return x_obj_t* -- Type struct
 */
x_obj_t *x_type_struct_get(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = NULL;

	if (x_base_isset(p_base)) {
		p_type = x_eval_type_alist_assoc(p_base, p_args);
	}

	/* TODO: GC on exit, with and w/o GC structures. */
	if (x_obj_isnil(p_base, p_type)) {
		p_type = x_callable_call(p_base, x_restobj(p_args));

		if (x_base_isset(p_base)) {
			x_eval_type_alist_extend(p_base, p_type);
		}
	}

	return p_type;
}

/**
 * Return the type name for an object. x-lang: (type-name obj)
 *
 * For stack atoms/pairs and untyped objects, returns the raw type
 * pointer. For heap-typed objects, extracts the name from the type
 * struct.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- (object)
 * @return x_obj_t* -- Type name object, or NULL
 */
x_obj_t *x_type_prim_type_name(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_name, *p_obj;

	if (x_obj_isnil(p_base, p_args) || x_obj_isnil(p_base, (p_obj = x_firstobj(p_args)))) {
		return NULL;
	}

	/* The raw-pointer branch also covers non-pair-tree type tags (the
	 * base sentinel x_eval_obj, a static atom): navigating their fields
	 * reads past the tag string. Same rule as x_type_op_try. */
	if (x_obj_type_issatom(p_obj)
			|| x_obj_type_isspair(p_obj)
			|| x_obj_isnil(p_base, x_obj_type(p_obj))
			|| ! x_obj_type_isspair(x_obj_type(p_obj))) {
		return x_obj_type(p_obj);
	}

	p_name = x_type_field_name(x_obj_type(p_obj));

	if (x_obj_isnil(p_base, p_name)) {
		return NULL;
	}

	return p_name;
}

/**
 * Return the unit count for an object. x-lang: (units obj)
 *
 * Dispatches to pair or atom unit primitives for built-in types.
 * For custom types, calls the type's units hook function.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- (object)
 * @return x_obj_t* -- Integer unit count, or NULL
 */
x_obj_t *x_type_prim_units(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_units, *p_obj;

	if (x_obj_isnil(p_base, p_args) || x_obj_isnil(p_base, (p_obj = x_firstobj(p_args)))) {
		return NULL;
	}

	if (x_obj_type_isspair(p_obj)) {
		return x_pair_prim_units(p_base, p_args);
	}

	/* Non-pair-tree type tags (base sentinel) fall back to atom units:
	 * their fields must not be navigated (see x_type_op_try). */
	if (x_obj_type_issatom(p_obj) || x_obj_isnil(p_base, x_obj_type(p_obj))
			|| ! x_obj_type_isspair(x_obj_type(p_obj))) {
		return x_atom_prim_units(p_base, p_args);
	}

	p_units = x_type_field_units(x_obj_type(p_obj));

	if (x_obj_isnil(p_base, p_units) || x_obj_isnil(p_base, x_atomobj(p_units))) {
		return NULL;
	}

	return (*x_atomfn(p_units))(p_base, p_args);
}

/**
 * Return the length for an object. x-lang: (length obj)
 *
 * Dispatches to pair or atom length primitives for built-in types.
 * For custom types, calls the type's length hook function.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- (object)
 * @return x_obj_t* -- Integer length, or NULL
 */
x_obj_t *x_type_prim_length(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_length, *p_obj;

	if (x_obj_isnil(p_base, p_args) || x_obj_isnil(p_base, (p_obj = x_firstobj(p_args)))) {
		return NULL;
	}

	if (x_obj_type_isspair(p_obj)) {
		return x_pair_prim_length(p_base, p_args);
	}

	/* Non-pair-tree type tags (base sentinel) fall back to atom length:
	 * their fields must not be navigated (see x_type_op_try). */
	if (x_obj_type_issatom(p_obj) || x_obj_isnil(p_base, x_obj_type(p_obj))
			|| ! x_obj_type_isspair(x_obj_type(p_obj))) {
		return x_atom_prim_length(p_base, p_args);
	}

	p_length = x_type_field_length(x_obj_type(p_obj));

	if (x_obj_isnil(p_base, p_length) || x_obj_isnil(p_base, x_atomobj(p_length))) {
		return NULL;
	}

	return (*x_atomfn(p_length))(p_base, p_args);
}

/**
 * GC mark hook for typed heap objects.
 *
 * For child base objects (type == x_eval_obj), returns the data
 * pointer so the GC traverses the base's pair tree. For custom types,
 * calls the type's mark callback if present. Otherwise falls back to
 * a generic N-slot traversal using the units count.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_obj   x_obj_t* -- Object being marked
 * @param flags   x_obj_flag_t -- GC mark flags
 * @return x_obj_t* -- Data pointer for base objects, or NULL
 */
x_obj_t *x_type_heap_mark(x_obj_t *p_base, x_obj_t *p_obj, x_obj_flag_t flags)
{
	x_obj_t *p_type = x_obj_type(p_obj);
	x_obj_t *p_mark;
	x_obj_t *p_units;
	x_spair_t mark_args[2];
	x_int_t n;
	x_int_t i;

	/* Child base objects (e.g. %sh-base): traverse their pair tree
	 * so type alist entries, env, etc. are not freed by GC. */
	if (p_type == (x_obj_t *)&x_eval_obj) {
		return x_atomobj(p_obj);
	}

	if (p_type != NULL && x_obj_type_isspair(p_type)) {
		p_mark = x_type_field_mark(p_type);

		if (p_mark != NULL) {
			/* Call type's custom mark callback:
			 * (mark p_obj flags) */
			mark_args[0][X_OBJ_META_TYPE].p = NULL;
			mark_args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
			x_firstobj((x_obj_t *)mark_args) = p_obj;
			x_restobj((x_obj_t *)mark_args)
				= (x_obj_t *)(mark_args + 1);

			mark_args[1][X_OBJ_META_TYPE].p = NULL;
			mark_args[1][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
			x_firstint((x_obj_t *)(mark_args + 1)) = flags;
			x_restobj((x_obj_t *)(mark_args + 1)) = NULL;

			x_atomfn(p_mark)(p_base, (x_obj_t *)mark_args);
			return NULL;
		}

		/* Fall back to p_units generic N-slot traversal.
		 * Mark ALL slots via tree_mark (handles non-heap
		 * values safely — they won't be on the heap chain).
		 *
		 * A NEGATIVE units count is the dynamic-size sentinel:
		 * slot 0 holds an INT atom with the payload count (the
		 * vector convention -- (Vector make n) allocates n+1
		 * slots with slot 0 = n).  Without this, per-instance
		 * sized objects had no units to walk and their payloads
		 * were never marked: a Dict held across a REPL turn
		 * (the REPL collects each turn) lost its bucket alists
		 * and the next access segfaulted. */
		p_units = x_type_field_units(p_type);

		if (p_units != NULL) {
			n = x_atomint(p_units);

			if (n < 0) {
				n = x_atomint(x_obj(x_obj_data_i(p_obj, 0)))
					+ 1;
			}

			for (i = 0; i < n; i++) {
				x_heap_tree_mark(p_base,
					x_obj(x_obj_data_i(p_obj, i)),
					flags);
			}
			return NULL;
		}
	}

	return NULL;
}

/**
 * GC free hook for typed heap objects.
 *
 * If the object's type has a free callback, invokes it to release
 * type-specific resources before the heap cell is reclaimed.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_obj   x_obj_t* -- Object being freed
 */
void x_type_heap_free(x_obj_t *p_base, x_obj_t *p_obj)
{
	x_obj_t *p_type = x_obj_type(p_obj);
	x_obj_t *p_free;
	x_spair_t a[1];

	if (p_type != NULL && x_obj_type_isspair(p_type)) {
		p_free = x_type_field_free(p_type);

		if (p_free != NULL) {
			a[0][X_OBJ_META_TYPE].p = NULL;
			a[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
			x_firstobj((x_obj_t *)a) = p_obj;
			x_restobj((x_obj_t *)a) = NULL;

			x_atomfn(p_free)(p_base, (x_obj_t *)a);
		}
	}
}

