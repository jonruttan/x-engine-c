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
#include "x-interp.h"
#include "x-heap.h"
#include "x-obj.h"
#include "x-type/prim.h"

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
 * error), and iter group.
 *
 * @param p_base  x_obj_t* -- Execution context (for allocation)
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
		nil)))))));

	return p_type;
}

#undef nil
#undef pair
#undef atom

/**
 * Retrieve or create a type struct by name.
 *
 * First checks the type alist cache. On miss, calls the callable in
 * rest of @p p_args to construct the type, then caches it in the
 * type alist for future lookups.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- (type-name . constructor-callable)
 * @return x_obj_t* -- Type struct
 */
x_obj_t *x_type_struct_get(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = NULL;

	if (x_base_isset(p_base)) {
		p_type = x_interp_type_alist_assoc(p_base, p_args);
	}

	/* TODO: GC on exit, with and w/o GC structures. */
	if (x_obj_isnil(p_base, p_type)) {
		p_type = x_callable_call(p_base, x_restobj(p_args));

		if (x_base_isset(p_base)) {
			x_interp_type_alist_extend(p_base, p_type);
		}
	}

	return p_type;
}

/**
 * Dispatch the write hook for a typed object.
 *
 * Looks up the type's write function and applies it to the object.
 * Uses x_callable_apply (not call) to avoid TCO complications.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- (object . ...)
 * @return x_obj_t* -- Write result, or NULL if no write hook
 */
x_obj_t *x_type_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args),
		*p_fn = x_type_field_write(x_obj_type(p_obj));
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_fn }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_obj }, { NULL })
	};

	if ( ! x_obj_isnil(p_base, p_fn)) {
		/* Use prim_apply (no TCO) so all body forms execute
		 * before returning to C. prim_call sets the last form
		 * as a TCO expr that nobody would process here. */
		return x_callable_apply(p_base, (x_obj_t *)args);
	}

	return NULL;
}

/**
 * Dispatch the display hook for a typed object.
 *
 * Looks up the type's display function and applies it. Falls back
 * to x_type_write if no display hook is registered.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- (object . ...)
 * @return x_obj_t* -- Display result
 *
 * @see x_type_write
 */
x_obj_t *x_type_display(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args),
		*p_fn = x_type_field_display(x_obj_type(p_obj));
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_fn }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_obj }, { NULL })
	};

	if ( ! x_obj_isnil(p_base, p_fn)) {
		return x_callable_apply(p_base, (x_obj_t *)args);
	}

	/* Fallback: use write */
	return x_type_write(p_base, p_args);
}

/**
 * Return the type name for an object. x-lang: (type-name obj)
 *
 * For stack atoms/pairs and untyped objects, returns the raw type
 * pointer. For heap-typed objects, extracts the name from the type
 * struct.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- (object)
 * @return x_obj_t* -- Type name object, or NULL
 */
x_obj_t *x_type_prim_type_name(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_name, *p_obj;

	if (x_obj_isnil(p_base, p_args) || x_obj_isnil(p_base, (p_obj = x_firstobj(p_args)))) {
		return NULL;
	}

	if (x_obj_type_issatom(p_obj)
			|| x_obj_type_isspair(p_obj)
			|| x_obj_isnil(p_base, x_obj_type(p_obj))) {
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
 * @param p_base  x_obj_t* -- Execution context
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

	if (x_obj_type_issatom(p_obj) || x_obj_isnil(p_base, x_obj_type(p_obj))) {
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
 * @param p_base  x_obj_t* -- Execution context
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

	if (x_obj_type_issatom(p_obj) || x_obj_isnil(p_base, x_obj_type(p_obj))) {
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
 * For child base objects (type == x_interp_obj), returns the data
 * pointer so the GC traverses the base's pair tree. For custom types,
 * calls the type's mark callback if present. Otherwise falls back to
 * a generic N-slot traversal using the units count.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_obj   x_obj_t* -- Object being marked
 * @param flags   x_obj_flag_t -- GC mark flags
 * @return x_obj_t* -- Data pointer for base objects, or NULL
 */
x_obj_t *x_type_heap_mark(x_obj_t *p_base, x_obj_t *p_obj, x_obj_flag_t flags)
{
	x_obj_t *p_type = x_obj_type(p_obj);

	/* Child base objects (e.g. %sh-base): traverse their pair tree
	 * so type alist entries, env, etc. are not freed by GC. */
	if (p_type == (x_obj_t *)&x_interp_obj) {
		return x_atomobj(p_obj);
	}

	if (p_type != NULL && x_obj_type_isspair(p_type)) {
		x_obj_t *p_mark = x_type_field_mark(p_type);

		if (p_mark != NULL) {
			/* Call type's custom mark callback:
			 * (mark p_obj flags) */
			x_spair_t mark_args[2];

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

		{
			/* Fall back to p_units generic N-slot traversal.
			 * Mark ALL slots via tree_mark (handles non-heap
			 * values safely — they won't be on the heap chain). */
			x_obj_t *p_units = x_type_field_units(p_type);

			if (p_units != NULL) {
				x_int_t n = x_atomint(p_units);
				x_int_t i;

				for (i = 0; i < n; i++) {
					x_heap_tree_mark(p_base,
						x_obj(x_obj_data_i(p_obj, i)),
						flags);
				}
				return NULL;
			}
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
 * @param p_base  x_obj_t* -- Execution context
 * @param p_obj   x_obj_t* -- Object being freed
 */
void x_type_heap_free(x_obj_t *p_base, x_obj_t *p_obj)
{
	x_obj_t *p_type = x_obj_type(p_obj);

	if (p_type != NULL && x_obj_type_isspair(p_type)) {
		x_obj_t *p_free = x_type_field_free(p_type);

		if (p_free != NULL) {
			x_spair_t a[1];

			a[0][X_OBJ_META_TYPE].p = NULL;
			a[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
			x_firstobj((x_obj_t *)a) = p_obj;
			x_restobj((x_obj_t *)a) = NULL;

			x_atomfn(p_free)(p_base, (x_obj_t *)a);
		}
	}
}

