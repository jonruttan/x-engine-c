/*
 * # Computational Expressions in C
 *
 * ## x-prim/gc.c -- Implementation - Primitives - Garbage Collection
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
/*
 * # Includes
 */
#include "x-prim.h"
#include "x-base.h"
#include "x-heap.h"
#include "x-type.h"
#include "x-type/int.h"
#include "x-obj/prim.h"

/* heap-sweep: (heap-sweep) -> sweep unmarked objects from heap */
static x_obj_t *x_prim_heap_sweep(x_obj_t *p_base, x_obj_t *p_args)
{
#ifdef X_PROFILE
	if (x_base_isset(p_base))
		x_atomint(x_base_field_profile_gc_runs(p_base))++;
#endif

	/* Call free hooks before sweep */
	if (x_base_isset(p_base)) {
		x_obj_t *p_hooks = x_base_field_heap_free_hooks(p_base);
		x_spair_t hook_args[1];

		hook_args[0][X_OBJ_META_TYPE].p = NULL;
		hook_args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;

		while ( ! x_obj_isnil(p_base, p_hooks)) {
			x_firstobj((x_obj_t *)hook_args) = x_firstobj(p_hooks);
			x_restobj((x_obj_t *)hook_args) = NULL;
			x_obj_prim_call(p_base, (x_obj_t *)hook_args);
			p_hooks = x_restobj(p_hooks);
		}
	}

	x_heap_sweep(p_base, x_obj_heap(p_base), X_OBJ_FLAG_HEAP,
		x_type_heap_free);

	return NULL;
}

/* heap-count: (heap-count) -> count objects on heap */
static x_obj_t *x_prim_heap_count(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p = x_obj_heap(p_base);
	long count = 0;

	while (p) {
		count++;
		p = x_obj_heap(p);
	}

	return x_mkint(p_base, count);
}

/* heap-mark: (heap-mark) -> mark reachable objects on heap */
static x_obj_t *x_prim_heap_mark(x_obj_t *p_base, x_obj_t *p_args)
{
	/* Normal mark: trace from base data tree */
	x_heap_mark(p_base, x_atomobj(p_base), X_OBJ_FLAG_HEAP,
		x_type_heap_mark);

	/* Conservative stack scan: mark objects referenced from C stack */
	x_heap_mark_stack(p_base, X_OBJ_FLAG_HEAP, x_type_heap_mark);

	/* Mark all registered GC roots */
	if (x_base_isset(p_base)) {
		x_obj_t *p_roots = x_base_field_heap_mark_roots(p_base);

		while ( ! x_obj_isnil(p_base, p_roots)) {
			x_heap_mark(p_base, x_firstobj(p_roots),
				X_OBJ_FLAG_HEAP, x_type_heap_mark);
			p_roots = x_restobj(p_roots);
		}
	}

	/* Call mark hooks (each is a callable) */
	if (x_base_isset(p_base)) {
		x_obj_t *p_hooks = x_base_field_heap_mark_hooks(p_base);
		x_spair_t hook_args[1];

		hook_args[0][X_OBJ_META_TYPE].p = NULL;
		hook_args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;

		while ( ! x_obj_isnil(p_base, p_hooks)) {
			x_firstobj((x_obj_t *)hook_args) = x_firstobj(p_hooks);
			x_restobj((x_obj_t *)hook_args) = NULL;
			x_obj_prim_call(p_base, (x_obj_t *)hook_args);
			p_hooks = x_restobj(p_hooks);
		}
	}

	return NULL;
}

/* gc-pin!: (gc-pin! obj) -> recursively mark object and all reachable
 * objects as SYSTEM (immune to GC sweep). Uses the same traversal
 * pattern as x_heap_mark. */
static x_obj_t *x_prim_system_mark(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_interp_eval_arg(p_base, x_firstobj(p_args));

	/* Reuse the mark traversal with SYSTEM flag */
	x_heap_mark(p_base, p_obj, X_OBJ_FLAG_SYSTEM, x_type_heap_mark);

	return p_obj;
}

x_obj_t *x_prim_gc_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_prim_entry_t entries[] = {
		{ "heap-mark", x_prim_heap_mark },
		{ "heap-sweep", x_prim_heap_sweep },
		{ "heap-count", x_prim_heap_count },
		{ "gc-pin!", x_prim_system_mark }
	};

	x_prim_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
