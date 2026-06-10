/** @file x-type/list.c
 *  @brief List type -- construction, indexing/slicing, eval dispatch, and iteration.
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2024 Jon Ruttan
 *  @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */

#include "x-type/list.h"
#include "x-type/iter.h"
#include "x-type/prim.h"
#include "x-prim.h"
#include "x-eval.h"
#include "x-token/sexp/list.h"

x_satom_t x_type_list_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_LIST_NAME }),
	x_type_list_struct_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_list_struct }),
	x_type_list_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_list_make }),
	x_type_list_length_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_list_length }),
	x_type_list_call_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_list_call }),
	x_type_list_eval_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_list_eval }),
	x_type_list_iter_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_type_list_iter });

/**
 * Allocate a LIST pair with first = @p p1 and rest = @p p2.
 *
 * @param p_base  Execution context.
 * @param flags   Object flags.
 * @param p1      First element (head).
 * @param p2      Rest element (tail).
 * @return Newly allocated LIST object.
 */
x_obj_t *x_make_list(x_obj_t *p_base, x_obj_flag_t flags, void *p1, void *p2)
{
	x_satom_t o_flags = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = flags });
	x_spair_t o_list = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .v = p1 }, { .v = p2 }),
		args[2] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_list }, { (x_obj_t *)(args + 1) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { o_flags }, { NULL })
		};

	return x_type_list_make(p_base, (x_obj_t *)args);
}

/**
 * Build the LIST type struct descriptor.
 *
 * Populates name, units (pair), make, length, call, eval, analyse,
 * delimit, read, write, and display hooks.
 *
 * @param p_base  Execution context.
 * @param p_args  Unused.
 * @return Type struct pair-tree for LIST.
 */
x_obj_t *x_type_list_struct(x_obj_t *p_base, x_obj_t *p_args)
{
	struct x_type_t type = {
		.p_name = x_type_list_name,
		.p_units = (x_obj_t *)&x_type_units_pair_obj,
		.p_make = x_type_list_make_prim,
		.p_length = x_type_list_length_prim,
		.p_call = x_type_list_call_prim,
		.p_eval = x_type_list_eval_prim,
		.p_analyse = x_sexp_list_analyse_prim,
		.p_delimit = x_sexp_list_delimit_prim,
		.p_read = x_sexp_list_read_prim,
		.p_write = x_sexp_list_write_prim,
		.p_display = x_sexp_list_display_prim
	};

	return x_type_struct_make(p_base, type);
}

/**
 * Register (or retrieve) the LIST type in the type alist.
 *
 * @param p_base  Execution context.
 * @param p_args  Unused.
 * @return The registered LIST type object.
 */
x_obj_t *x_type_list_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_list_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_list_struct_prim }, { NULL })
	};

	return x_type_struct_get(p_base, (x_obj_t *)args);
}

/**
 * Type-system make handler for LIST objects.
 *
 * Extracts first/rest from the pair in @c p_args[0] and optional flags
 * from @c p_args[1].
 *
 * @param p_base  Execution context.
 * @param p_args  Argument list: (pair-atom [flags-atom]).
 * @return Newly allocated LIST object.
 */
x_obj_t *x_type_list_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = x_type_list_register(p_base, p_base),
		*p_list = x_0(p_args);
	x_obj_flag_t flags = x_obj_isnil(p_base, x_1(p_args))
		? 0 : x_firstint(x_01(p_args));

	return x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_PAIR, x_0(p_list), x_1(p_list));
}

/**
 * Compute the length of a list by walking to its end.
 *
 * @param p_base  Execution context.
 * @param p_args  Argument list whose first element is the list.
 * @return An satom containing the integer length.
 */
x_obj_t *x_type_list_length(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args);
	x_int_t len = 0;

	while (! x_obj_isnil(p_base, p_obj)) {
		len++;
		p_obj = x_restobj(p_obj);
	}

	return x_mksatom(p_base, X_OBJ_FLAG_NONE, len);
}

/**
 * Call handler -- index or slice a list.
 *
 * With one argument: @c (list index) returns the element at @p index
 * (negative indices count from the end).  With two arguments:
 * @c (list start len) returns a sublist of @p len elements starting
 * at position @p start.
 *
 * @param p_base  Execution context.
 * @param p_args  Argument list: (list index) or (list start len).
 * @return The indexed element, the sliced sublist, or NULL.
 */
x_obj_t *x_type_list_call(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *proc = x_firstobj(p_args), *vals = x_restobj(p_args);
	x_obj_t *arg1, *arg2;
	x_obj_t *p_result, *p_tail, *p_new, *p_walk;
	x_int_t n, i;
	x_int_t start, len;

	/* Evaluate first arg. */
	if (x_obj_isnil(p_base, vals)) {
		return NULL;
	}

	arg1 = x_eval_arg(p_base, x_firstobj(vals));
	vals = x_restobj(vals);

	if (! x_obj_isnil(p_base, vals)) {
		/* Slice: (list start len) -> sublist */
		start = x_atomint(arg1);
		p_result = NULL;
		p_tail = NULL;

		arg2 = x_eval_arg(p_base, x_firstobj(vals));
		len = x_atomint(arg2);

		/* Walk to start position. */
		for (i = 0; i < start && ! x_obj_isnil(p_base, proc); i++) {
			proc = x_restobj(proc);
		}

		/* Collect len elements. */
		for (i = 0; i < len && ! x_obj_isnil(p_base, proc); i++) {
			p_new = x_mklist(p_base, x_firstobj(proc), NULL);

			if (x_obj_isnil(p_base, p_result)) {
				p_result = p_new;
			} else {
				x_restobj(p_tail) = p_new;
			}

			p_tail = p_new;
			proc = x_restobj(proc);
		}

		return p_result;
	}

	/* Single index. */
	n = x_atomint(arg1);

	/* Negative index: count from end. */
	if (n < 0) {
		p_walk = proc;
		len = 0;

		while (! x_obj_isnil(p_base, p_walk)) {
			len++;
			p_walk = x_restobj(p_walk);
		}

		n += len;
	}

	/* Walk to index. */
	for (i = 0; i < n && ! x_obj_isnil(p_base, proc); i++) {
		proc = x_restobj(proc);
	}

	return x_obj_isnil(p_base, proc) ? NULL : x_firstobj(proc);
}

/**
 * Eval handler -- resolve operator and dispatch through callable protocol.
 *
 * Evaluates the first element of the list expression to obtain a
 * procedure or operative, then dispatches the rest of the list as
 * arguments through x_callable_call().  The expression is rooted on the
 * eval list to protect it from GC during evaluation.
 *
 * @param p_base  Execution context.
 * @param p_args  Eval argument frame containing the list expression.
 * @return Result of the operator application, or the original expression
 *         if the operator resolves to nil.
 *
 * @see x_callable_call
 */
x_obj_t *x_type_list_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_exp = x_firstobj(x_eval_arg_exp(p_args)), *p_proc, *p_result;
	x_satom_t first_atom = x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_firstobj(p_exp) });
	x_spair_t eval_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { first_atom }, { NULL })
	},
	proc_exp = x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { x_restobj(p_exp) }),
	prim_args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { (x_obj_t *)proc_exp });

	/* Root p_exp so GC doesn't free the arg list during eval/call */
	x_obj_push_field(p_base, &x_eval_field_eval_list(p_base), p_exp, X_OBJ_FLAG_NONE);

	/* Eval first to resolve operator (e.g. symbol -> prim). */
	p_proc = x_eval(p_base, (x_obj_t *)eval_args);

	if (x_obj_isnil(p_base, p_proc)) {
		x_obj_pop_field(p_base, &x_eval_field_eval_list(p_base));
		return p_exp;
	}

	x_firstobj((x_obj_t *)proc_exp) = p_proc;
	x_firstobj((x_obj_t *)prim_args) = x_type_field_call(x_obj_type(p_proc));

	if (x_obj_isnil(p_base, x_firstobj((x_obj_t *)prim_args))) {
		x_obj_pop_field(p_base, &x_eval_field_eval_list(p_base));
		return p_exp;
	}

	p_result = x_callable_call(p_base, (x_obj_t *)prim_args);
	x_obj_pop_field(p_base, &x_eval_field_eval_list(p_base));
	return p_result;
}

/**
 * Iterator step -- advance a list iterator by one element.
 *
 * Returns the current element and advances the iterator's internal
 * cursor to the next cell.  Returns NULL when exhausted.
 *
 * @param p_base  Execution context.
 * @param p_args  Argument list whose first element is the iterator.
 * @return Current element, or NULL at end of list.
 */
x_obj_t *x_type_list_iter(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_iter = x_firstobj(p_args), *p_obj;

	if (x_obj_isnil(p_base, x_iterval(p_iter))) {
		return NULL;
	}

	p_obj = x_firstobj(x_iterval(p_iter));
	x_iterval(p_iter) = x_restobj(x_iterval(p_iter));

	return p_obj;
}
