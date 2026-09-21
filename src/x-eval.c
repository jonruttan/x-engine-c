/** @file x-eval.c
 *  @brief Evaluator with TCO trampoline
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
#include "x-eval.h"
#include "x-env.h"
#include "x-tco.h"
#include "x-toplevel.h"
#include "x-obj.h"
#include "x-prim.h"
#include "x-type.h"
#include "x-alist.h"
#include "x-type/ptr.h"
#include "x-type/str.h"
#include "x-type/list.h"
#include "x-type/err.h"
#include "x-type/symbol.h"
#include "x-token.h"
#include <setjmp.h>

#include "x-type/prim.h"

/* Evaluator engine (x_eval + the TCO/operative trampolines).  Unit tests that
 * exercise only the base layer omit it by defining STUB_X_EVAL (then take
 * x_eval from helper-stubs) or X_EVAL_OWN (provide their own double) before
 * #including this file -- the base construction/IO/error code below stays. */
#if !defined(STUB_X_EVAL) && !defined(X_EVAL_OWN)

/**
 * Defer an operative body's tail to the outer trampoline (TCO).
 *
 * Evaluates the non-tail body forms synchronously, then stores the tail form
 * in tco_expr and the caller's environment in tco_env.  Deliberately does NOT
 * push the save-stack -- operatives stay invisible to it, so a procedure
 * whose tail is an operative call still owns its own restore.  The
 * trampoline keeps the outermost environment it is handed and makes it
 * current at exit, which for an operative is the caller's: a `def` the body
 * evaluated in the caller's environment is IN that environment, so there is
 * nothing to decide about keeping or shedding a chain head.
 *
 * @param p_base    x_obj_t* -- Base (execution context)
 * @param p_body    x_obj_t* -- Operative body (sequence of forms)
 * @param p_caller  x_obj_t* -- The caller's environment, current on return
 * @return x_obj_t* -- NULL (result delivered via the trampoline)
 */
x_obj_t *x_eval_op_body(x_obj_t *p_base, x_obj_t *p_body, x_obj_t *p_caller)
{
	x_obj_t **p_cell = x_heap_root_slot(p_base);
	x_spair_t root = x_obj_set((x_obj_t *)x_type_pair_obj, X_OBJ_FLAG_NONE,
		{ NULL }, { NULL });

	/* Root the caller's environment -- held only by this frame across
	 * every body eval until it reaches the tco-env field -- and the
	 * advancing body (one registered cell; popped on every exit path). */
	x_firstobj((x_obj_t *)root) = p_caller;
	x_heap_root_push(p_cell, root);

	while ( ! x_obj_isnil(p_base, p_body)) {
		/* A body is user-supplied: nil ends a proper one, an atom ends
		 * a dotted one and must not be read as a cell (#487). */
		x_eval_spine_guard(p_base, p_body);
		if (x_obj_isnil(p_base, x_restobj(p_body))) {
			x_firstobj(x_eval_field_tco_expr(p_base)) = x_firstobj(p_body);

			/* Nil tail: no trampoline will run -- restore synchronously. */
			if (x_obj_isnil(p_base,
				x_firstobj(x_eval_field_tco_expr(p_base)))) {
				x_tco_restore(p_base, p_caller);
				x_heap_root_pop(p_cell);
				return NULL;
			}

			x_firstobj(x_eval_field_tco_env(p_base)) = p_caller;

			x_heap_root_pop(p_cell);
			return NULL;
		}

		x_restobj((x_obj_t *)root) = p_body;
		x_eval_arg(p_base, x_firstobj(p_body));

		p_body = x_restobj(p_body);
	}

	/* Empty body: restore synchronously. */
	x_tco_restore(p_base, p_caller);

	x_heap_root_pop(p_cell);

	return NULL;
}

/*
 * Shared TCO keep/restore for the two trampoline loops -- x_eval's inline
 * loop and x_eval_tco_trampoline.  Both keep the first (outermost)
 * environment the tco_env channel hands them and make it current on exit.
 * Extracted so the two copies cannot drift.
 */

/* Keep the outermost environment from a tco_env value.  The kept
 * environment is also stored into @p p_tco_root's slot so the GC roots it
 * across the arbitrary evaluation between capture and restore (#243 -- a C
 * local is not a root; the environment is already off the save-stack). */
static void x_eval_tco_keep(x_obj_t *p_base, x_obj_t *p_te, x_obj_t *p_tco_root,
	x_obj_t **pp_save)
{
	if (x_obj_isnil(p_base, p_te))
		return;

	if (*pp_save == NULL || x_obj_isnil(p_base, *pp_save)) {
		*pp_save = p_te;
		x_firstobj(p_tco_root) = p_te;
	}
}

/* Make the kept environment current, if one was kept.  The outermost is the
 * one kept, so an inner call's environment never survives its caller's
 * return whatever the tail chain did in between. */
static void x_eval_tco_apply(x_obj_t *p_base, x_obj_t *p_save)
{
	if (p_save != NULL && ! x_obj_isnil(p_base, p_save))
		x_tco_restore(p_base, p_save);
}

/**
 * Evaluate an expression with tail-call optimization.
 *
 * Dispatches to the expression's type-level eval handler. If the handler
 * sets a TCO tail expression on p_base, the trampoline loop re-evaluates
 * without growing the C stack. On exit, restores the environment from
 * the saved TCO snapshot.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- (expression . env) pair
 * @return x_obj_t* -- Evaluated result, or NULL for nil
 *
 * @details **Outermost detection.**  The local @c trampolining flag starts
 *          at 0.  When a TCO tail expression is first detected on p_base,
 *          this x_eval instance sets @c trampolining = 1, claiming
 *          ownership of the trampoline loop.  Any nested x_eval called
 *          during handler dispatch will see tco_expr as cleared (this
 *          instance clears it before goto) and will therefore NOT enter
 *          the trampoline -- it returns normally and its result is
 *          discarded in favor of the deferred tail expression.
 *
 * @details **tco_expr / tco_env lifecycle.**
 *          - **Set by:** x_eval_body_tco (full TCO) stores the tail
 *            expression in tco_expr and the environment to restore in
 *            tco_env.  x_prim_match stores
 *            only tco_expr (tco_env stays nil -- no env change needed).
 *          - **Consumed by:** This function's trampoline loop.  On each
 *            iteration it copies tco_expr into the eval args, clears
 *            tco_expr on p_base, and jumps to eval_start.
 *          - **tco_env cleared:** Each iteration clears tco_env on p_base
 *            after snapshotting it into the local p_tco_env_save.  This
 *            prevents nested x_eval calls from seeing stale env state.
 *
 * @details **p_tco_env_save snapshot.**
 *          - Captured on first trampoline entry from tco_env on p_base.
 *          - On later iterations, if the initial snapshot was nil (set by
 *            simple forms like if/do/match) but an inner form (fn/let/op)
 *            now provides a non-nil tco_env, the snapshot is upgraded.
 *          - Used only at exit: the outermost x_eval makes it current.
 *
 * @details **Nested x_eval calls do NOT restore env.**  Only the
 *          instance where @c trampolining == 1 executes the env restore
 *          block.  This is critical: a recursive x_eval (e.g. from
 *          evaluating a sub-expression inside a primitive) must not
 *          interfere with the outer trampoline's env management.
 *
 * @note Uses goto-based trampoline; only the outermost x_eval in
 *       a call chain performs env restoration.
 *
 * @see x_eval_body_tco      -- full TCO body evaluator (sets tco_expr + tco_env)
 * @see x_eval_tco_trampoline -- standalone trampoline used by closure call paths
 */
x_obj_t *x_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_exp;
	x_obj_t *p_tco_env_save = NULL;   /* the outermost environment kept */
	x_obj_t *p_te;                    /* tco_env fetched per trampoline pass */
	x_spair_t prim_args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL });
	/* Root for the kept environment: it is popped off the save-stack, the
	 * tco-env field is cleared, and it lives only in the local above across
	 * every trampoline iteration -- arbitrary evaluation -- until the exit
	 * restore makes it current. */
	x_obj_t **p_cell = x_heap_root_slot(p_base);
	x_spair_t tco_root = x_obj_set((x_obj_t *)x_type_pair_obj,
		X_OBJ_FLAG_NONE, { NULL }, { NULL });
	int trampolining = 0;
#ifdef X_SIGNAL
	/* Interrupt-flag pointer, resolved once from the base (signal-register
	 * publishes signal.c's static atom here).  Cached so the trampoline pays
	 * a single load per iteration, and so a GC relocation of the base spine
	 * mid-eval can't invalidate it -- the target is a non-heap static. */
	x_obj_t *p_sigint = x_base_isset(p_base) ? x_firstobj(x_eval_field_sigint(p_base)) : NULL;
#endif

	x_heap_root_push(p_cell, tco_root);

eval_start:
#ifdef X_SIGNAL
	/* SIGINT: throw STOP if a guard is active.  Volatile cast forces a
	 * re-read each iteration; without it -O2 hoists it out of the loop. */
	if (p_sigint != NULL
		&& *(volatile x_int_t *)&x_atomint(p_sigint)
		&& ! x_obj_isnil(p_base, x_firstobj(x_eval_field_error_handler(p_base))))
	{
		x_atomint(p_sigint) = 0;
		x_eval_error(p_base, "STOP", NULL);
	}
#endif
	if (x_base_isset(p_base)) {
		x_atomint(x_firstobj(x_eval_field_profile_evals(p_base)))++;
	}

	p_exp = x_firstobj(x_eval_arg_exp(p_args));

	/* Update base line/file counters from the expression's source metadata
	 * (slot 0 = line, slot 1 = file id).  After this, current-line/current-file
	 * reflect the eval site, so an error here snapshots the right location. */
	if (p_exp != NULL && (x_obj_flags(p_exp) & X_OBJ_FLAG_META)) {
		x_atomint(x_firstobj(x_eval_field_line(p_base))) = x_obj_meta_i(p_exp, 0).i;
		if (x_atomint(x_firstobj(x_base_field_obj_meta_extra(p_base))) > 1) {
			x_atomint(x_firstobj(x_eval_field_file(p_base))) = x_obj_meta_i(p_exp, 1).i;
		}
	}

#ifdef X_COV
	if (p_exp != NULL) {
		x_obj_flags(p_exp) |= X_OBJ_FLAG_COV;
	}
#endif

	if (x_obj_isnil(p_base, p_exp)) {
		x_heap_root_pop(p_cell);
		return NULL;
	}

	/* Differentiate simple from complex types.
	 * Guard: NULL-typed (raw stack) objects self-evaluate. */
	if (x_obj_type(p_exp) == NULL || x_obj_isnil(p_base, x_obj_type(x_obj_type(p_exp)))) {
		x_heap_root_pop(p_cell);
		return p_exp;
	}

	x_firstobj((x_obj_t *)prim_args) = x_type_field_eval(x_obj_type(p_exp));

	if ( ! x_obj_isnil(p_base, x_firstobj((x_obj_t *)prim_args))) {
		x_restobj((x_obj_t *)prim_args) = p_args;
		p_exp = x_callable_call(p_base, (x_obj_t *)prim_args);

		if (p_exp == p_args) {
			goto eval_start;
		}
	}

	/* TCO trampoline: re-evaluate tail expression if set. */
	if (x_base_isset(p_base) && ! x_obj_isnil(p_base, x_firstobj(x_eval_field_tco_expr(p_base)))) {
		p_te = x_firstobj(x_eval_field_tco_env(p_base));

		trampolining = 1;

		/* Keep the first (outermost) environment: a procedure hands over
		 * its caller's, an operative its caller's.  if/do/match/and/or set
		 * none (tco_env nil) -- an inner fn/let/op fills it later. */
		x_eval_tco_keep(p_base, p_te, (x_obj_t *)tco_root, &p_tco_env_save);

		x_firstobj(x_eval_field_tco_env(p_base)) = NULL;
		x_firstobj(x_eval_arg_exp(p_args)) = x_firstobj(x_eval_field_tco_expr(p_base));
		x_firstobj(x_eval_field_tco_expr(p_base)) = NULL;
		x_atomint(x_firstobj(x_eval_field_profile_tco(p_base)))++;

		goto eval_start;
	}

	/* TCO env restore: only the x_eval that trampolined restores env. */
	if (trampolining && x_base_isset(p_base)) {
		x_firstobj(x_eval_field_tco_env(p_base)) = NULL;
		x_eval_tco_apply(p_base, p_tco_env_save);
	}

	x_heap_root_pop(p_cell);

	return p_exp;
}


/**
 * Evaluate a single expression.
 *
 * Wraps @p p_arg in a stack-allocated (atom . nil) pair and passes it
 * through x_eval, which unwraps and evaluates the inner expression.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_arg   x_obj_t* -- Expression to evaluate
 * @return x_obj_t* -- Evaluation result
 */
x_obj_t *x_eval_arg(x_obj_t *p_base, x_obj_t *p_arg)
{
	x_satom_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_arg });
	x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { wrap }, { NULL });

	return x_eval(p_base, (x_obj_t *)args);
}

/**
 * Raise unless @p p_obj is a spine cell that first/rest may navigate.
 *
 * @param p_base  Base (execution context).
 * @param p_obj   The spine position about to be walked (never nil).
 *
 * @details **Improper-spine guard (#69, ruled).**  A first/rest walk is
 *          only meaningful for an object whose TYPE DECLARES pair units
 *          -- the same shape contract the collector's payload walk
 *          trusts (x_type_prim_heap_mark).  The test is STRUCTURAL, not
 *          a type-identity list: any reader personality's spine type
 *          participates by declaring pair units (the reader and the
 *          evaluator need not be symmetric), and two shapes are cells
 *          by construction -- raw stack cells (NULL type slot) and heap
 *          pairs tagged with the built-in pair static (the x_mkspair
 *          product; #296).  The static's own type slot is NULL, so the
 *          registered-type probe could never accept it -- omitting it
 *          made every C-built spine handed to an applicative in a
 *          minimal base raise spuriously.  A dotted tail lands here as
 *          a non-cell and raises a catchable error in place of the
 *          segfault it replaces -- (list 1 . 5), and bare-x-core
 *          (f 1.5) where the float module is absent and 1.5 reads as a
 *          dotted pair; the tail atom is atom-tagged or registered-typed,
 *          so neither shape re-admits it.
 *
 * @note Every C consumer of an argument spine funnels through here:
 *       x_eval_list for applicatives, and x_args/x_eargs for the prims
 *       (#487 -- those walked past a dotted tail into x_firstobj on an
 *       atom, reading its value word as a pair pointer, which no guard
 *       could catch because a prim call never enters x_eval_list).
 *       Ops still receive their spines RAW and bind dotted tails
 *       legitimately, so they remain untouched.
 *
 * @see x_eval_list -- the applicative argument walk
 * @see x_eargs     -- the prim argument walk (include/x-prim.h)
 */
void x_eval_spine_guard(x_obj_t *p_base, x_obj_t *p_obj)
{
	x_obj_t *p_t, *p_units;
	int is_cell;

	p_t = x_obj_type(p_obj);
	is_cell = p_t == NULL || x_obj_type_isspair(p_obj);

	if ( ! is_cell && ! x_obj_type_issatom(p_obj)
		&& ! x_obj_isnil(p_base, p_t) && x_obj_type_isspair(p_t)) {
		p_units = x_type_field_units(p_t);
		is_cell = p_units != NULL
			&& x_type_units_count(p_units) == X_OBJ_UNITS_PAIR;
	}

	if ( ! is_cell) {
		x_eval_error(p_base,
			(x_char_t *)"call: improper argument list (dotted tail)",
			NULL);
	}
}

/**
 * Read the argument at an already-navigated spine position.
 *
 * @param p_base  Base (execution context).
 * @param p_pos   A spine position (the result of an x_1/x_11 peek).
 * @return The element at @p p_pos, or nil if the list ended there.
 *
 * @details The raw positional macros (x_011 and friends) navigate
 *          first/rest UNCHECKED, which is their contract -- so a caller
 *          peeking PAST the arity a guarded walk covered used to read
 *          the atom a dotted tail ends with as a pair (#487).  This is
 *          that peek, done safely: nil when the list ended, the element
 *          when the position is a cell, and the ruled catchable raise
 *          on anything else.
 *
 * @see x_eval_spine_guard -- the structural test
 */
x_obj_t *x_eval_spine_first(x_obj_t *p_base, x_obj_t *p_pos)
{
	if (x_obj_isnil(p_base, p_pos)) {
		return NULL;
	}
	x_eval_spine_guard(p_base, p_pos);

	return x_firstobj(p_pos);
}

/**
 * Evaluate each element of a list, returning a new list of results.
 *
 * Recursively evaluates via x_eval_arg, rooting the tail on the
 * eval-list GC root so the garbage collector does not free remaining
 * arguments while evaluating the current one.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- List of unevaluated expressions
 * @return x_obj_t* -- New list of evaluated results, or NULL if empty
 *
 * @details **GC rooting protocol.**  Before evaluating the current
 *          element, the entire remaining arg list is pushed onto
 *          eval_list (a GC root on p_base) as a stack-allocated pair.
 *          This prevents the collector from freeing the rest of the
 *          list while x_eval_arg runs (which may trigger GC).  After
 *          evaluation, the root is popped.  The push/pop is O(1) per
 *          element, but the recursion itself is O(n) in C stack depth
 *          -- one frame per list element.  This is acceptable for
 *          argument lists (typically short) but would overflow on
 *          very long lists.
 *
 * @note Returns NULL for nil input (empty arg list), which is the
 *       identity for list construction.
 *
 * @see x_eval_arg  -- evaluates a single expression
 * @see x_eval_body -- iterative body evaluator (same GC rooting pattern)
 */
x_obj_t *x_eval_list(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_val, *p_rest;
	x_obj_t **p_cell = x_heap_root_slot(p_base);
	x_spair_t root = x_obj_set((x_obj_t *)x_type_pair_obj, X_OBJ_FLAG_NONE,
		{ NULL }, { NULL });

	if (x_obj_isnil(p_base, p_args)) {
		return NULL;
	}

	x_eval_spine_guard(p_base, p_args);

	/* Root p_args so GC doesn't free rest while evaluating first; the
	 * cell's rest slot then keeps the fresh result alive across the
	 * recursion (a hold the eval-list idiom never covered -- only the
	 * conservative scan did). */
	x_firstobj((x_obj_t *)root) = p_args;
	x_heap_root_push(p_cell, root);

	p_val = x_eval_arg(p_base, x_firstobj(p_args));
	x_restobj((x_obj_t *)root) = p_val;

	p_rest = x_eval_list(p_base, x_restobj(p_args));

	x_heap_root_pop(p_cell);

	return x_mklist(p_base, p_val, p_rest);
}

/**
 * Evaluate a body (list of expressions) sequentially, returning the last result.
 *
 * Each expression is rooted on the eval-list before evaluation so the
 * GC does not collect the remaining body. No tail-call optimization.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_body  x_obj_t* -- List of body expressions
 * @return x_obj_t* -- Result of the last expression, or NULL if empty
 *
 * @note When X_COV is defined, marks each body cell with X_OBJ_FLAG_COV.
 */
x_obj_t *x_eval_body(x_obj_t *p_base, x_obj_t *p_body)
{
	x_obj_t *p_result = NULL;
	x_obj_t **p_cell = x_heap_root_slot(p_base);
	x_spair_t root = x_obj_set((x_obj_t *)x_type_pair_obj, X_OBJ_FLAG_NONE,
		{ NULL }, { NULL });

	/* Root the advancing body so GC doesn't free remaining exprs --
	 * one registered cell for the whole walk instead of an eval-list
	 * cons per element. */
	x_heap_root_push(p_cell, root);

	while ( ! x_obj_isnil(p_base, p_body)) {
		/* A body is user-supplied: nil ends a proper one, an atom ends
		 * a dotted one and must not be read as a cell (#487). */
		x_eval_spine_guard(p_base, p_body);
#ifdef X_COV
		x_obj_flags(p_body) |= X_OBJ_FLAG_COV;
#endif
		x_firstobj((x_obj_t *)root) = p_body;

		p_result = x_eval_arg(p_base, x_firstobj(p_body));

		p_body = x_restobj(p_body);
	}

	x_heap_root_pop(p_cell);

	return p_result;
}

/**
 * Evaluate a body with full tail-call optimization.
 *
 * Non-tail expressions are evaluated normally. The tail (last)
 * expression is stored in the TCO expr slot instead of being evaluated
 * directly, and the caller's saved environment is captured in
 * tco-env so the trampoline can restore it after the tail call.
 *
 * On early exit (nil tail) or empty body, pops the save-stack and makes
 * the environment it held current again.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_body  x_obj_t* -- List of body expressions
 * @return x_obj_t* -- Result of non-tail expressions, or NULL when
 *                      tail expression is deferred to the trampoline
 *
 * @details **Save-stack protocol.**  The caller (fn/let dispatch)
 *          pushes the environment it is leaving onto save_stack BEFORE
 *          calling this function (x_tco_env_save), so it can be made
 *          current again after the tail call completes.
 *
 * @details **tco_env capture.**  When the tail expression is reached
 *          (last element of body), this function checks whether
 *          tco_env is still nil.  If so, it copies the save-stack top
 *          into tco_env, providing the env snapshot that x_eval's
 *          trampoline will use for restoration.  If tco_env is already
 *          set (by a prior TCO iteration), the existing value is kept.
 *
 * @details **Save-stack pop.**  After capturing tco_env (or on early
 *          exit), the save-stack is popped.  On the normal tail-call
 *          path this is a simple pop (the trampoline in x_eval handles
 *          restore).  On early exit (nil tail or empty body), this
 *          function does a full restore from the popped frame before
 *          returning, since no trampoline iteration will follow.
 *
 * @note When X_COV is defined, marks each body cell with X_OBJ_FLAG_COV.
 *
 * @see x_eval                  -- outermost trampoline that consumes tco_expr/tco_env
 * @see x_eval_tco_trampoline   -- standalone trampoline for closure call paths
 */
x_obj_t *x_eval_body_tco(x_obj_t *p_base, x_obj_t *p_body)
{
	x_obj_t *p_result = NULL;
	x_obj_t **p_cell = x_heap_root_slot(p_base);
	x_spair_t root = x_obj_set((x_obj_t *)x_type_pair_obj, X_OBJ_FLAG_NONE,
		{ NULL }, { NULL });

	/* Root the advancing body so GC doesn't free remaining exprs (one
	 * cell for the walk; popped on every exit path). */
	x_heap_root_push(p_cell, root);

	while ( ! x_obj_isnil(p_base, p_body)) {
		/* A body is user-supplied: nil ends a proper one, an atom ends
		 * a dotted one and must not be read as a cell (#487). */
		x_eval_spine_guard(p_base, p_body);
#ifdef X_COV
		x_obj_flags(p_body) |= X_OBJ_FLAG_COV;
#endif
		if (x_obj_isnil(p_base, x_restobj(p_body))) {
			x_firstobj(x_eval_field_tco_expr(p_base)) = x_firstobj(p_body);

			if (x_obj_isnil(p_base,
				x_firstobj(x_eval_field_tco_expr(p_base)))) {
				/* Nil tail: restore from save-stack top and pop. */
				x_tco_restore(p_base,
					x_firstobj(x_eval_field_save_stack(p_base)));
				x_eval_field_save_stack(p_base)
					= x_restobj(x_eval_field_save_stack(p_base));
				x_heap_root_pop(p_cell);
				return NULL;
			}

			if (x_obj_isnil(p_base,
				x_firstobj(x_eval_field_tco_env(p_base)))) {
				/* Hand the trampoline the environment to restore. */
				x_firstobj(x_eval_field_tco_env(p_base))
					= x_firstobj(x_eval_field_save_stack(p_base));
			}

			/* Pop save-stack */
			x_eval_field_save_stack(p_base)
				= x_restobj(x_eval_field_save_stack(p_base));

			x_heap_root_pop(p_cell);
			return NULL;
		}

		x_firstobj((x_obj_t *)root) = p_body;

		p_result = x_eval_arg(p_base, x_firstobj(p_body));

		p_body = x_restobj(p_body);
	}

	/* Empty body: restore from save-stack top and pop. */
	x_tco_restore(p_base, x_firstobj(x_eval_field_save_stack(p_base)));
	x_eval_field_save_stack(p_base)
		= x_restobj(x_eval_field_save_stack(p_base));

	x_heap_root_pop(p_cell);

	return p_result;
}


/**
 * TCO trampoline: repeatedly evaluate deferred tail expressions.
 *
 * After a TCO-aware body defers its tail expression, this loop
 * evaluates it. If that evaluation itself defers another tail call,
 * the loop continues until no more TCO expressions remain.
 *
 * On exit, makes the environment saved in tco-env current again.
 *
 * @param p_base   x_obj_t* -- Base (execution context)
 * @param p_result x_obj_t* -- Initial result (from non-tail evaluation)
 * @return x_obj_t* -- Final evaluation result
 *
 * @see x_eval_body_tco
 */
x_obj_t *x_eval_tco_trampoline(x_obj_t *p_base, x_obj_t *p_result)
{
	x_obj_t *p_tco, *p_te, *p_tco_env = NULL;
	/* Root for the kept environment (#243, mirrors x_eval): it is popped
	 * off the save-stack and the tco-env field is cleared, so across the
	 * arbitrary evaluation below this local holds the only reference --
	 * and a C local is not a root (x-heap.h).  An argument eval that
	 * triggers (heap-collect) would otherwise sweep the environment the
	 * exit restore then reads. */
	x_obj_t **p_cell = x_heap_root_slot(p_base);
	x_spair_t tco_root = x_obj_set((x_obj_t *)x_type_pair_obj,
		X_OBJ_FLAG_NONE, { NULL }, { NULL });

	x_heap_root_push(p_cell, tco_root);

	while ( ! x_obj_isnil(p_base, x_firstobj(x_eval_field_tco_expr(p_base)))) {
		p_tco = x_firstobj(x_eval_field_tco_expr(p_base));

		/* Keep the outermost environment (mirrors x_eval). */
		p_te = x_firstobj(x_eval_field_tco_env(p_base));
		x_eval_tco_keep(p_base, p_te, (x_obj_t *)tco_root, &p_tco_env);

		x_firstobj(x_eval_field_tco_expr(p_base)) = NULL;
		x_firstobj(x_eval_field_tco_env(p_base)) = NULL;
		p_result = x_eval_arg(p_base, p_tco);
	}

	x_eval_tco_apply(p_base, p_tco_env);

	x_heap_root_pop(p_cell);

	return p_result;
}

#endif /* !STUB_X_EVAL && !X_EVAL_OWN -- evaluator engine */

/* ===== merged from x-interp.c: base construction, error handling, env/io ===== */

#define nil			NULL
#define pair(X,Y)	(x_mkspair(p_base, X_OBJ_FLAG_NONE, (X), (Y)))
#define atom(X)		(x_mksatom(p_base, X_OBJ_FLAG_NONE, (X)))

static x_satom_t x_type_prim_type_name_hook =
	x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = x_type_prim_type_name });
static x_satom_t x_type_prim_units_hook =
	x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = x_type_prim_units });
static x_satom_t x_type_prim_length_hook =
	x_obj_set(NULL, X_OBJ_FLAG_NONE, { .fn = x_type_prim_length });
/* The pre-registration error value: an ERR-SHAPED (code . subject) pair
 * with no type tag, for bases built before the type registry exists.
 * See x_eval_error's else branch. */
static x_satom_t s_bare_code = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .s = NULL });
static x_satom_t s_bare_subject = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .s = NULL });
static x_spair_t s_bare_err = x_obj_set(NULL, X_OBJ_FLAG_NONE,
	{ (x_obj_t *)&s_bare_code }, { (x_obj_t *)&s_bare_subject });
static x_satom_t x_eval_error_hook =
	x_obj_set(NULL, X_OBJ_FLAG_NONE, { .v = (void *)x_eval_error });
static x_satom_t x_type_heap_mark_hook =
	x_obj_set(NULL, X_OBJ_FLAG_NONE, { .v = (void *)x_type_heap_mark });
static x_satom_t x_type_heap_free_hook =
	x_obj_set(NULL, X_OBJ_FLAG_NONE, { .v = (void *)x_type_heap_free });

/**
 * Create and initialize a full x-lang base object atop x-expr.
 *
 * Calls x_base_make (x-expr layer) with default file descriptors and
 * hooks, then fills in the type-system-specific slots: env-group
 * (the current environment and the root), ctrl-group
 * (save-stack, error-handler, TCO slots), io-state (line counter,
 * boolean caches), extended profile counters, and project extras
 * (eval-list, token-cache, mark/free hooks, mark-roots).
 *
 * @param p_base  x_obj_t* -- Parent base (or NULL for root)
 * @param p_args  x_obj_t* -- Unused
 * @return x_obj_t* -- Newly constructed base object
 *
 * @details **x-expr vs x-lang layers.**  x_base_make (x-expr) allocates
 *          the base tree skeleton: heap group (pools, GC state), file
 *          descriptors, buffer stack, type-alist slot, profile head
 *          (1 counter for GC cycles), and hook slots.  It leaves env,
 *          ctrl, io-state, and extras as nil.  This function fills all
 *          of those in, giving the base its full evaluator personality.
 *
 * @details **Base tree nodes carry X_OBJ_FLAG_SHARED** (set by x-expr's
 *          x_base_make).  The SHARED flag tells the GC mark phase that
 *          these spine nodes are allocated from the base's own pool and
 *          must be marked but never freed -- they are structurally
 *          permanent for the lifetime of the base.
 *
 * @details **Env-group layout:**
 *          @code
 *          (env . env-root)
 *          @endcode
 *          - env: the current environment, a (bindings . parent) pair
 *          - env-root: the base's root environment, whose bindings are
 *            a tree and whose parent is nil
 *
 * @details **Ctrl-group layout:**
 *          @code
 *          ((save-stack . (error-handler-slot . nil)) .
 *           ((tco-expr-slot . nil) . (tco-env-slot . nil)))
 *          @endcode
 *
 * @details **Profile counters** (9 additional beyond x-expr's GC counter):
 *          evals, TCO hits, lookups, BST lookups, and internal metrics.
 *
 * @note When @p p_base is non-NULL (child base), boolean caches (#t/#f)
 *       are inherited from the parent so all bases in a tree share the
 *       same singleton boolean objects.
 *
 * @see x_eval_error  -- uses the error-handler from ctrl-group
 * @see x_eval        -- uses tco-expr/tco-env from ctrl-group
 */
x_obj_t *x_eval_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_parent = p_base;
	struct x_base_t base_cfg;

	base_cfg.filein = STDIN_FILENO;
	base_cfg.fileout = STDOUT_FILENO;
	base_cfg.fileerr = STDERR_FILENO;
	base_cfg.p_hook_type_name = (x_obj_t *)x_type_prim_type_name_hook;
	base_cfg.p_hook_units = (x_obj_t *)x_type_prim_units_hook;
	base_cfg.p_hook_length = (x_obj_t *)x_type_prim_length_hook;
	base_cfg.p_hook_error = (x_obj_t *)x_eval_error_hook;
	base_cfg.obj_meta_extra = 0;
	base_cfg.p_heap_mark = (x_obj_t *)x_type_heap_mark_hook;
	base_cfg.p_heap_free = (x_obj_t *)x_type_heap_free_hook;

	p_base = x_base_make(p_base, base_cfg);

	/* Set base type (x-expr uses NULL). */
	x_obj_type(p_base) = x_eval_obj;

	/* Build the empty pair-tree skeleton -- env+ctrl, the type-alist cell,
	 * io-state, the profile counters, and the state group -- from the
	 * descriptor (tools/contract/base-layout.x) via the generated x-eval-layout.h.
	 * Every leaf cell's car comes out nil; initial values are set just below. */
#define X_EVAL_BUILD_TREE
#include "x-eval-layout.h"
#undef X_EVAL_BUILD_TREE

	/* The root environment: an empty tree with no parent.  It is both the
	 * base's root and the environment evaluation starts in; every child
	 * environment made later reaches it through its parents. */
	x_eval_field_env_root(p_base) = pair(nil, nil);
	x_eval_field_env(p_base) = x_eval_field_env_root(p_base);

	/* Initial values (the skeleton leaves every cell's car nil). */
	x_firstobj(x_eval_field_line(p_base)) = atom(1);
	x_firstobj(x_eval_field_profile_evals(p_base)) = atom(0);
	x_firstobj(x_eval_field_profile_tco(p_base)) = atom(0);
	x_firstobj(x_eval_field_profile_assoc_calls(p_base)) = atom(0);
	x_firstobj(x_eval_field_profile_assoc_steps(p_base)) = atom(0);
	x_firstobj(x_eval_field_profile_sym_find_calls(p_base)) = atom(0);
	x_firstobj(x_eval_field_profile_sym_find_steps(p_base)) = atom(0);
	x_firstobj(x_eval_field_profile_gc_runs(p_base)) = atom(0);
	x_firstobj(x_eval_field_profile_bst_hits(p_base)) = atom(0);
	x_firstobj(x_eval_field_profile_bst_misses(p_base)) = atom(0);
	x_firstobj(x_eval_field_profile_env_steps(p_base)) = atom(0);
	/* The err cell stays NIL here.  The ERR the raise path fills is built
	 * by x_type_err_register, because building it needs the type registry
	 * and this function runs before there is one -- x-eval must not depend
	 * on the type layer (tests/c/src/2.x-base.spec.c links without it, and
	 * that is the layering being kept, not an accident of the test). */

	/* Source-location tracking.  `file` mirrors `line` (the live id of the
	 * form being evaluated, 0 = no file / REPL input); err-line/err-file hold
	 * the raise-time snapshot -- all three are int atoms.  file-registry is the
	 * id->path alist; the skeleton already leaves its car nil (empty list), so
	 * it is NOT re-initialized here (an int atom there would be walked as a
	 * pair by `include` and the lookup, and segfault). */
	x_firstobj(x_eval_field_file(p_base)) = atom(0);
	x_firstobj(x_eval_field_err_line(p_base)) = atom(0);
	x_firstobj(x_eval_field_err_file(p_base)) = atom(0);

	/* #t/#f and the sigint flag are inherited from a parent base so every
	 * base in a tree shares the singletons; the root base sets its own
	 * booleans during primitive registration. */
	if (p_parent != nil) {
		x_firstobj(x_eval_field_true(p_base)) = x_firstobj(x_eval_field_true(p_parent));
		x_firstobj(x_eval_field_false(p_base)) = x_firstobj(x_eval_field_false(p_parent));
		x_firstobj(x_eval_field_sigint(p_base)) = x_firstobj(x_eval_field_sigint(p_parent));
	}

	return p_base;
}

#undef nil
#undef pair
#undef atom

/**
 * Signal an error with a message and optional object context.
 *
 * If an error handler is installed (via @c guard), builds a combined
 * error string with line number, restores the saved environment, and
 * longjmps to the handler. Otherwise, writes the error to stderr via
 * the low-level x_error function.
 *
 * @param p_base   x_obj_t* -- Base (execution context)
 * @param message  x_char_t* -- Error message string
 * @param p_obj    x_obj_t* -- Object associated with the error (may be NULL)
 *
 * @details **Zero-allocation error path.**  When a handler is installed,
 *          the message string pointer is stored directly in a static
 *          atom (no malloc, no x_mkstrown).  Message strings from C
 *          callers are always string literals (static storage), so they
 *          survive the longjmp.  The guard handler in x-lang receives
 *          the bare message; x-lang code can add line/symbol context
 *          via (%base) if needed.
 *
 * @details **longjmp protocol.**  The error value is stored in the
 *          handler's error slot, then the environment the handler saved
 *          at guard installation time is made current again.  Finally, longjmp transfers control
 *          to the setjmp site in x_prim_guard.  This unwinds all C
 *          frames between the error site and the guard -- any local
 *          state in those frames is lost.
 *
 * @note When no handler is installed, writes the error via x_error and
 *       terminates the process (docs/spec.md pins this contract for
 *       `error`).  Returning instead would resume the raising primitive
 *       mid-operation with a garbage value -- at boot, where no guard is
 *       installed yet and the harness discards stderr, that silently
 *       corrupted the load (the class-call trap).
 *
 * @see x_prim_guard  -- installs the handler and setjmp site
 * @see x_prim_error  -- x-lang (error msg) primitive that calls this
 */
#ifndef STUB_X_BASE_ERROR
void x_eval_error(x_obj_t *p_base, x_char_t *message, x_obj_t *p_obj)
{
	int fd;
	x_char_t *symbol = NULL;
	x_obj_t *p_handler;
	x_obj_t *p_err;

	/* Extract symbol string from object if possible.  Still needed by the
	 * UNCAUGHT path below, which words its own report in C: it runs before
	 * any library is loaded, so there is no x-lang there to ask. */
	if (p_obj != NULL && x_obj_type_issatom(p_obj)) {
		symbol = x_atomstr(p_obj);
	}

	/* Snapshot the raise-site source location into the stable err-line/err-file
	 * cells.  The live line/file counters are overwritten as the handler body
	 * evaluates, and the caught handler is popped before its body runs, so
	 * (io error-line)/(io error-file) must read this frozen copy, not the
	 * handler or the live counter. */
	if (x_base_isset(p_base)) {
		x_atomint(x_firstobj(x_eval_field_err_line(p_base)))
			= x_atomint(x_firstobj(x_eval_field_line(p_base)));
		x_atomint(x_firstobj(x_eval_field_err_file(p_base)))
			= x_atomint(x_firstobj(x_eval_field_file(p_base)));
	}

	/* If an error handler is installed, store message and longjmp. */
	if (x_base_isset(p_base)
		&& ! x_obj_isnil(p_base, x_firstobj(x_eval_field_error_handler(p_base)))) {
		p_handler = x_firstobj(x_eval_field_error_handler(p_base));

		/* The base-resident ERR, filled IN PLACE: the code slot's atom is
		 * repointed at this raise's message literal (static storage, so it
		 * survives the longjmp) and the offending object goes in the obj
		 * slot whole.  Two pointer stores -- no copy, no allocation, no
		 * truncation, and safe even when the failure IS out of memory.
		 *
		 * The engine stops here.  It does not concatenate the symbol into
		 * the message and it does not word anything: an ERR's write/display
		 * stacks boot empty and x-lang's err-io.x pushes the prose, which is
		 * what lets a lang push its own over that.  What used to be one
		 * flattened English string is now the two facts it was flattened
		 * from. */
		/* Fill the base's ERR in place: the code slot's atom is repointed
		 * at this raise's message literal (static storage, so it survives
		 * the longjmp) and the subject beside it.  Two pointer stores --
		 * no copy, no allocation, safe even when the failure IS OOM.
		 *
		 * A base whose ERR type is not registered yet (x_eval_make runs
		 * before the registry exists, and the low-level C harnesses never
		 * build one) has nil here.  THE JUMP STILL HAPPENS -- an installed
		 * handler means the caller is prepared to be jumped to, and
		 * turning that into a fatal exit because the type layer is absent
		 * would be a silent semantic change.  Only the value differs. */
		p_err = x_firstobj(x_eval_field_err(p_base));
		if (p_err != NULL && ! x_obj_isnil(p_base, p_err)) {
			x_atomstr(x_err_code(p_err)) = message;
			x_atomstr(x_err_subject(p_err))
				= (symbol != NULL) ? symbol : (x_char_t *)"";
		} else {
			/* No type registry on this base, so no ERR instance to fill:
			 * the bare-C harnesses build an evaluator without the type
			 * layer on purpose (tests/c/src/2.x-base.spec.c pins that
			 * layering, and it is worth keeping -- x-eval must not depend
			 * on x-type).
			 *
			 * The fallback has the SAME SHAPE, (code . subject), and only
			 * lacks the type tag.  That matters: every C consumer reads a
			 * raised error through x_err_code/x_err_subject and none of
			 * them should have to ask which window it came from.  x-lang
			 * never observes this one -- it closes when
			 * x_type_err_register runs, before any library loads -- so the
			 * missing tag costs nothing that can be seen from up there.
			 *
			 * File-static, like the scratch buffer it replaces, and safe
			 * for the same reason: no second base can exist this early. */
			x_atomstr((x_obj_t *)&s_bare_code) = message;
			x_atomstr((x_obj_t *)&s_bare_subject)
				= (symbol != NULL) ? symbol : (x_char_t *)"";
			p_err = (x_obj_t *)&s_bare_err;
		}

		x_error_handler_error(p_handler) = p_err;

		/* Save error line — raw int in rest slot, zero allocation */
		x_error_handler_line(p_handler)
			= (x_obj_t *)(x_int_t)x_atomint(x_firstobj(x_eval_field_line(p_base)));

		x_eval_field_env(p_base) = x_error_handler_saved_env(p_handler);
		longjmp(*(jmp_buf *)x_error_handler_jmp(p_handler), 1);
	}

	fd = x_base_isset(p_base) ? x_atomint(x_firstobj(x_base_field_fileerr(p_base))) : STDERR_FILENO;

	x_error(fd, message, symbol);
	x_sys_write(fd, X_STR_LITERAL("\n"));

	/* Uncaught errors are fatal (spec: "Without a handler, `error`
	 * terminates the process").  The raising primitive cannot be resumed
	 * -- returning here used to continue it with a garbage value, so an
	 * unbound head mid-boot yielded nil and x_type_list_eval silently
	 * passed the form through unevaluated. */
	x_sys_exit(X_SYS_EXIT_FAILURE);
}
#endif /* !STUB_X_BASE_ERROR */

/**
 * Add a type struct to the base's type alist.
 *
 * Wraps the type struct as a (name . type_struct) pair for alist
 * keying and prepends it to the type alist.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Type struct to register
 * @return x_obj_t* -- The new type alist head, or NULL if base is unset
 */
x_obj_t *x_eval_type_alist_extend(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_entry;
	x_spair_t args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL })
	};

	if ( ! x_base_isset(p_base)) {
		return NULL;
	}

	/* Wrap type struct as (name . type_struct) for alist keying */
	p_entry = x_mkspair(p_base, X_OBJ_FLAG_NONE, x_type_field_name(p_args), p_args);
	x_firstobj((x_obj_t *)args) = p_entry;
	x_restobj((x_obj_t *)args) = x_firstobj(x_eval_field_type_alist(p_base));

	return x_firstobj(x_eval_field_type_alist(p_base)) = x_alist_extend(p_base, (x_obj_t *)args);
}

/**
 * Look up a type struct in the base's type alist by name.
 *
 * Searches for a (name . type_struct) entry matching the first element
 * of @p p_args. Returns the bare type struct (unwrapped from the
 * alist entry), or NULL if not found.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Pair whose first is the type name to look up
 * @return x_obj_t* -- Type struct, or NULL
 */
x_obj_t *x_eval_type_alist_assoc(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_result;
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_firstobj(p_args) }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL })
	};

	if ( ! x_base_isset(p_base)) {
		return NULL;
	}

	x_firstobj((x_obj_t *)args[1]) = x_firstobj(x_eval_field_type_alist(p_base));

	p_result = x_alist_assoc(p_base, (x_obj_t *)args);

	/* Unwrap (name . type_struct) entry to return bare type struct */
	return x_obj_isnil(p_base, p_result) ? NULL : x_restobj(p_result);
}

/**
 * Push a buffer onto the buffer stack.
 *
 * @param p_base   x_obj_t* -- Base (execution context)
 * @param p_buffer x_obj_t* -- Buffer object to push
 * @return x_obj_t* -- The pushed buffer
 */
x_obj_t *x_eval_buffer_push(x_obj_t *p_base, x_obj_t *p_buffer)
{
	x_base_field_buffer(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		p_buffer, x_base_field_buffer(p_base));
	return p_buffer;
}

/**
 * Read and evaluate all expressions from the current buffer.
 *
 * Loops calling x_token_read until EOF, evaluating each expression
 * via x_eval. Returns the result of the last expression.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Unused
 * @return x_obj_t* -- Result of the last evaluated expression, or NULL
 *
 * @details Reads from the buffer at the top of the buffer stack
 *          (x_base_field_buffer).  The caller is responsible for
 *          pushing the desired buffer before calling this function
 *          (via x_eval_buffer_push) and popping it afterward.  Each
 *          read expression is wrapped in a stack-allocated (atom . nil)
 *          eval-args pair and passed to x_eval, which runs the full
 *          evaluator including the TCO trampoline.  The result of each
 *          expression is discarded except the last.
 *
 * @note This is the primary entry point for loading library files.
 *       The shell driver pipes library source via stdin
 *       (@c cat lib/x.x - | ./x-bin).  The core loop reads through the
 *       buffer/fd stack; the optional @c include primitive
 *       (X_INCLUDE, x-cli.c) additionally opens files via x_sys_open
 *       and pushes them onto the same stack.
 *
 * @see x_eval  -- evaluator called for each expression
 */
x_obj_t *x_eval_load(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_firstobj(x_base_field_buffer(p_base));
	x_obj_t *p_exp, *p_result = NULL;
	x_toplevel_t top;
	x_satom_t exp_wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL });
	x_spair_t eval_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { exp_wrap }, { NULL })
	};
	x_spair_t read_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_buffer }, { p_base })
	};

	/* Each form read from the file is a TOP-LEVEL form -- what that means
	 * is x_toplevel_enter's to say, once, for this door and for eval!'s.
	 * One bracket around the whole file, not one per form: a form's defs
	 * stay on the chain for the forms after it, as they always have. */
	x_toplevel_enter(p_base, &top);

	for (;;) {
		p_exp = x_token_read(p_base, (x_obj_t *)read_args);
		/* Break on the EOF SENTINEL, not on nil: nil is the value a
		 * top-level `()` reads as, and breaking on it used to end the
		 * load there, silently skipping the rest of the file. */
		if (p_exp == (x_obj_t *)x_token_eof_prim) break;

		x_firstobj((x_obj_t *)exp_wrap) = p_exp;
		p_result = x_eval(p_base, (x_obj_t *)eval_args);
	}

	x_toplevel_leave(p_base, &top);

	return p_result;
}

