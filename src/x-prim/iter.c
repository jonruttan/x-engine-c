/** @file x-prim/iter.c
 *  @brief Iterator primitives -- make-iter, iter-next/step/empty?.
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
#include "x-prim.h"
#include "x-eval.h"
#include "x-heap.h"
#include "x-type.h"
#include "x-token.h"
#include "x-type/buffer.h"
#include "x-type/char.h"
#include "x-type/int.h"
#include "x-type/iter.h"
#include "x-type/prim.h"
#include "x-type/str.h"

/** x-lang (make-iter step-fn value): build an iterator -- a boxed generator.
 *  Steps are PURE: an x-lang step-fn is (step state) -> (value . next-state)
 *  or () when exhausted; iter-next owns the box write-back -- value going
 *  nil marks exhaustion (iter-empty?). */
static x_obj_t *x_prim_make_iter(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_fn, *p_val;

	x_eargs(p_base, p_args, 3, NULL, &p_fn, &p_val);

	return x_make_iter(p_base, X_OBJ_FLAG_NONE, p_fn, p_val);
}

/** x-lang (iter-next iter): advance an iterator, returning its next element. */
static x_obj_t *x_prim_iter_next(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_prim_op1(p_base, p_args, x_type_iter_next);
}

/** x-lang (iter-step it): step an iterator FUNCTIONALLY -- returns
 *  (value . next-iterator) with the given iterator untouched, or () when
 *  exhausted.  The generator view of an iterator: Gen pipelines drive C
 *  steps through this door. */
static x_obj_t *x_prim_iter_step(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_prim_op1(p_base, p_args, x_type_iter_step);
}

/** x-lang (iter-empty? iter): #t when the iterator is exhausted, else #f.
 *  Reads x_iterempty directly and maps it to a real boolean. */
static x_obj_t *x_prim_iter_empty(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_iter;

	x_eargs(p_base, p_args, 2, NULL, &p_iter);

	return x_iterempty(p_base, p_iter)
		? x_firstobj(x_eval_field_true(p_base))
		: x_firstobj(x_eval_field_false(p_base));
}


/** Register the iterator primitives. */
x_obj_t *x_prim_iter_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_prim_entry_t entries[] = {
		{ "make-iter",         x_prim_make_iter,         "iter",   "make"          },
		{ "iter-next",         x_prim_iter_next,         "iter",   "next"          },
		{ "iter-step",         x_prim_iter_step,         "iter",   "step"          },
		{ "iter-empty?",       x_prim_iter_empty,        "iter",   "empty?"        }
	};

	x_prims_bind_table(p_base, entries, sizeof(entries) / sizeof(entries[0]));
	return p_base;
}
