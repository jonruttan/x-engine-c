/**
 * @file x-type/iter.c
 * @brief Iterator type implementation (lazy traversal of sequences).
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2024 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
#include "x-type/iter.h"
#include "x-obj.h"
#include "x-eval.h"
#include "x-type/prim.h"

x_satom_t x_type_iter_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_ITER_NAME }),
	x_type_iter_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_iter_make }),
	x_type_iter_struct_prim = x_obj_set(x_type_pair_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_iter_struct });

/**
 * Allocate a new iterator object on the heap.
 *
 * The iterator stores a step function (p1) and a current value (p2)
 * in the standard pair layout.
 *
 * @param p_base  x_obj_t*    -- Base (execution context)
 * @param flags   x_obj_flag_t -- Object flags
 * @param p1      void*        -- Step function (callable)
 * @param p2      void*        -- Initial value (nil when exhausted)
 * @return Heap-allocated iterator object
 */
x_obj_t *x_make_iter(x_obj_t *p_base, x_obj_flag_t flags, void *p1, void *p2)
{
	x_satom_t o_flags = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = flags });
	x_spair_t o_iter = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .v = p1 }, { .v = p2 }),
		args[2] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_iter }, { (x_obj_t *)(args + 1) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_flags }, { NULL })
		};

	return x_type_iter_make(p_base, (x_obj_t *)args);
}

/**
 * Build the ITER type struct descriptor.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Unused
 * @return Type struct pair list
 */
x_obj_t *x_type_iter_struct(x_obj_t *p_base, x_obj_t *p_args)
{
	struct x_type_t type = {
		.p_name = x_type_iter_name,
		.p_make = x_type_iter_make_prim,
		/* Two payload slots (step-fn . value), a pair layout. Without
		 * units the GC mark hook never traced them: an iterator held
		 * across a collect lost its step function or state and the
		 * next step segfaulted -- the same untraced-payload class as
		 * the vector-payload fix (a Gen driving a C iterator kept one
		 * alive across a REPL turn). */
		.p_units = (x_obj_t *)&x_type_units_pair_obj
	};

	return x_type_struct_make(p_base, type);
}

/**
 * Register (or retrieve) the ITER type struct on p_base.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Unused
 * @return The registered type struct object
 */
x_obj_t *x_type_iter_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_iter_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_iter_struct_prim }, { NULL })
	};

	return x_type_struct_get(p_base, (x_obj_t *)args);
}

/**
 * Type-dispatch make callback: construct an iterator from x-lang args.
 *
 * Expects args: ((step-fn . value) [flags]).
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Construction arguments
 * @return New iterator object
 */
x_obj_t *x_type_iter_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = x_type_iter_register(p_base, p_base);
	x_obj_flag_t flags = x_obj_isnil(p_base, x_1(p_args))
		? 0 : x_firstint(x_01(p_args));

	return x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_PAIR, x_00(p_args), x_10(p_args));
}

/**
 * Test whether an iterator is exhausted.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- (iterator)
 * @return p_base (truthy) if empty, p_args (non-nil/falsy) if not
 */
x_obj_t *x_type_iter_isempty(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_iterempty(p_base, x_firstobj(p_args)) ? p_base : p_args;
}

/**
 * Advance an iterator by one step.
 *
 * The iterator is a boxed GENERATOR: (step . state).  Steps are pure --
 * they never mutate the box; this driver owns the single write-back.
 * Two step ABIs, discriminated exactly as x_callable_call does:
 *
 *  - SATOM step (raw C fn): pure cell ABI.  Called with a caller-owned
 *    stack cell (state . nil); the step returns the value and writes the
 *    successor state into the cell's first slot.  Zero heap allocation,
 *    so the tokenizer's stack iterators stay GC-silent.
 *
 *  - Any other callable (x-lang fn, heap prim): functional ABI.  Called
 *    as (step state) via the apply path (state is data, never re-evaluated)
 *    and must return (value . next-state), or nil when exhausted.  This is
 *    the Gen / Seq step contract.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- (iterator)
 * @return The yielded element, or NULL when exhausted
 */
x_obj_t *x_type_iter_next(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_iter = x_firstobj(p_args), *p_obj;
	x_spair_t cell[2];

	cell[0][X_OBJ_META_TYPE].p = NULL;
	cell[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;

	if (x_obj_type_issatom(x_iterprim(p_iter))) {
		x_firstobj((x_obj_t *)cell) = x_iterval(p_iter);
		x_restobj((x_obj_t *)cell) = NULL;
		p_obj = (*x_primval(x_iterprim(p_iter)))(p_base, (x_obj_t *)cell);
		x_iterval(p_iter) = x_firstobj((x_obj_t *)cell);
		return p_obj;
	}

	x_firstobj((x_obj_t *)cell) = x_iterprim(p_iter);
	x_restobj((x_obj_t *)cell) = (x_obj_t *)(cell + 1);
	cell[1][X_OBJ_META_TYPE].p = NULL;
	cell[1][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)(cell + 1)) = x_iterval(p_iter);
	x_restobj((x_obj_t *)(cell + 1)) = NULL;

	p_obj = x_callable_apply(p_base, (x_obj_t *)cell);
	if (x_obj_isnil(p_base, p_obj)) {
		x_iterval(p_iter) = NULL;
		return NULL;
	}
	x_iterval(p_iter) = x_restobj(p_obj);
	return x_firstobj(p_obj);
}

/**
 * Step an iterator FUNCTIONALLY -- no mutation, generator view.
 *
 * The persistent complement of x_type_iter_next: yields
 * (value . next-iterator) as a fresh pair (the Seq step shape), leaving
 * the given iterator untouched, or nil when it is exhausted.  This is
 * the X-boundary door that lets Gen pipelines run on C steps; the two
 * allocations happen here, where allocation is legal.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- (iterator)
 * @return (value . next-iterator) pair, or NULL when exhausted
 */
x_obj_t *x_type_iter_step(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_iter = x_firstobj(p_args), *p_obj, *p_next;
	x_spair_t cell[2];

	if (x_iterempty(p_base, p_iter)) {
		return NULL;
	}

	cell[0][X_OBJ_META_TYPE].p = NULL;
	cell[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;

	if (x_obj_type_issatom(x_iterprim(p_iter))) {
		x_firstobj((x_obj_t *)cell) = x_iterval(p_iter);
		x_restobj((x_obj_t *)cell) = NULL;
		p_obj = (*x_primval(x_iterprim(p_iter)))(p_base, (x_obj_t *)cell);
		p_next = x_firstobj((x_obj_t *)cell);
	} else {
		x_firstobj((x_obj_t *)cell) = x_iterprim(p_iter);
		x_restobj((x_obj_t *)cell) = (x_obj_t *)(cell + 1);
		cell[1][X_OBJ_META_TYPE].p = NULL;
		cell[1][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
		x_firstobj((x_obj_t *)(cell + 1)) = x_iterval(p_iter);
		x_restobj((x_obj_t *)(cell + 1)) = NULL;
		p_obj = x_callable_apply(p_base, (x_obj_t *)cell);
		if (x_obj_isnil(p_base, p_obj)) {
			return NULL;
		}
		p_next = x_restobj(p_obj);
		p_obj = x_firstobj(p_obj);
	}

	return x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj,
		x_mkiter(p_base, x_iterprim(p_iter), p_next));
}
