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
 * Push the current environment state as a TCO restore compound.
 *
 * Snapshots env-alist, local-boundary, global tree (a BST), and shadow-head into a
 * compound @c ((env . boundary) . (bst . shadow)) and pushes it onto the
 * save-stack.  Procedure calls and eval-with-env use this to capture the
 * environment before extending it; the trampoline (or x_eval_body_tco's
 * early-exit paths) restores from it via x_tco_restore().
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @return x_obj_t* -- The pushed compound
 * @see x_tco_restore
 */
x_obj_t *x_tco_compound_save(x_obj_t *p_base)
{
	x_obj_t *p_compound = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_firstobj(x_eval_field_env_alist(p_base)),
			x_eval_field_env_local_boundary(p_base)),
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_eval_field_env_global_tree(p_base),
			x_eval_field_shadow_list(p_base)));

	x_eval_field_save_stack(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		p_compound, x_eval_field_save_stack(p_base));

	return p_compound;
}

/**
 * Restore env-alist, local-boundary, global tree (a BST), and shadow list from a TCO
 * compound @c ((env . boundary) . (bst . shadow)).
 *
 * Does NOT touch the save-stack -- callers that took the compound from the
 * save-stack top pop it separately.  This is the single restore used by both
 * trampoline exit points (x_eval, x_eval_tco_trampoline), x_eval_body_tco's
 * early-exit paths, and eval-with-env.
 *
 * @param p_base      x_obj_t* -- Base (execution context)
 * @param p_compound  x_obj_t* -- Compound built by x_tco_compound_save()
 * @see x_tco_compound_save
 */
void x_tco_restore(x_obj_t *p_base, x_obj_t *p_compound)
{
	x_firstobj(x_eval_field_env_alist(p_base))
		= x_firstobj(x_firstobj(p_compound));
	x_eval_field_env_local_boundary(p_base)
		= x_restobj(x_firstobj(p_compound));
	x_eval_field_env_global_tree(p_base)
		= x_firstobj(x_restobj(p_compound));
	x_prim_clear_shadows_to(p_base, x_restobj(x_restobj(p_compound)));
}

/** Discriminator whose address tags a tco_env value as an operative restore
 *  record @c (TAG . ((caller . op_head) . (boundary . shadow))) rather than a
 *  procedure env compound.  The trampolines route a tco_env by testing
 *  @c x_firstobj(tco_env) == &x_tco_op_tag. */
x_satom_t x_tco_op_tag = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { NULL });

/**
 * Restore env-alist, local-boundary, and shadow from an operative record
 * @c (TAG . ((caller . op_head) . (boundary . shadow))).
 *
 * The env-alist restore is CONDITIONAL on whether the op's formal frame
 * (op_head) is still reachable from the current head (see the body).  Boundary
 * and shadow always restore; the global BST is never touched (procedures own
 * it, and the trampoline applies the proc compound around this call).
 *
 * @param p_base        x_obj_t* -- Base (execution context)
 * @param p_record      x_obj_t* -- Operative record built by x_eval_op_body()
 * @param force_caller  int -- Non-zero when a procedure compound was also
 *                      captured here (the op's tail resolved to an applied
 *                      procedure, e.g. let): restore env to the caller
 *                      unconditionally rather than walking to the formal
 *                      frame.  See the body.
 * @see x_eval_op_body
 */
void x_op_restore(x_obj_t *p_base, x_obj_t *p_record, int force_caller)
{
	x_obj_t *p_rest = x_restobj(p_record),
		*p_caller = x_firstobj(x_firstobj(p_rest)),
		*p_head = x_restobj(x_firstobj(p_rest)),
		*p_boundary = x_firstobj(x_restobj(p_rest)),
		*p_shadow = x_restobj(x_restobj(p_rest)),
		*p_walk;

	/* Env-alist restore.  @p force_caller (set by the trampoline iff a procedure
	 * compound was ALSO captured here) means the op's tail resolved to an
	 * APPLIED procedure -- e.g. let, which expands to (apply (fn ...) ...).  That
	 * tail leaves env on a branched closure frame that must be shed, so restore
	 * to the caller unconditionally.
	 *
	 * Otherwise the op tail-eval'd into the caller's `e`.  Walk toward the op's
	 * formal frame: still on the chain -> the body computed a value in the
	 * formals without tail-eval'ing away, restore to caller to shed them; gone
	 * -> the body tail-eval'd and may have grown `e` with a (def ...) the caller
	 * must keep seeing (define-sugar, do-sequenced defs), so keep the head.
	 *
	 * Boundary and shadow always restore; the BST is never touched (procedures
	 * own it, and the trampoline applies the proc compound around this call). */
	if (force_caller) {
		x_firstobj(x_eval_field_env_alist(p_base)) = p_caller;
	} else {
		p_walk = x_firstobj(x_eval_field_env_alist(p_base));
		while ( ! x_obj_isnil(p_base, p_walk) && p_walk != p_head) {
			p_walk = x_restobj(p_walk);
		}
		if (p_walk == p_head) {
			x_firstobj(x_eval_field_env_alist(p_base)) = p_caller;
		} else {
			/* op_head is gone from the chain.  Two ways that happens:
			 * (1) the body tail-eval'd a top-level (def ...) into the
			 *     caller, growing the caller's env in place -- the head now
			 *     chains DOWN TO the caller, and we must keep it so the new
			 *     binding survives (define-sugar, do-sequenced defs);
			 * (2) the body's tail left the env-alist head on an unrelated
			 *     frame -- e.g. a nested TCO recursion inside (eval expr e)
			 *     (the interpolation operative parses holes that way) whose
			 *     own restore was suppressed as a non-outermost trampoline.
			 * Distinguish by walking for the caller: reachable -> case (1),
			 * keep; not reachable -> case (2), the head is foreign, so
			 * restore to the caller.  Without this an operative in if-tail
			 * (simple-TCO) position leaks that foreign frame, and the next
			 * form the caller evaluates sees the wrong scope (Unbound). */
			p_walk = x_firstobj(x_eval_field_env_alist(p_base));
			while ( ! x_obj_isnil(p_base, p_walk) && p_walk != p_caller) {
				p_walk = x_restobj(p_walk);
			}
			if (p_walk != p_caller) {
				x_firstobj(x_eval_field_env_alist(p_base)) = p_caller;
			}
		}
	}

	x_eval_field_env_local_boundary(p_base) = p_boundary;
	x_prim_clear_shadows_to(p_base, p_shadow);
}

/**
 * Defer an operative body's tail to the outer trampoline (TCO).
 *
 * Evaluates the non-tail body forms synchronously, then stores the tail form in
 * tco_expr and a tagged operative restore record in tco_env.  Deliberately does
 * NOT push the save-stack -- operatives stay invisible to it, so a top-level
 * (def ...) run by tail-eval'd body code still classifies as top-level (BST),
 * and the operative does not block the procedure env channel.  The trampoline
 * keeps the first procedure compound and the first operative record separately,
 * applying x_tco_restore then x_op_restore at exit.
 *
 * @param p_base      x_obj_t* -- Base (execution context)
 * @param p_body      x_obj_t* -- Operative body (sequence of forms)
 * @param p_caller    x_obj_t* -- env-alist head before the op extended it
 * @param p_op_head   x_obj_t* -- the op's installed formal-frame head
 * @param p_boundary  x_obj_t* -- local-boundary to restore
 * @param p_shadow    x_obj_t* -- shadow-list head to clear back to
 * @return x_obj_t* -- NULL (result delivered via the trampoline)
 * @see x_op_restore
 */
x_obj_t *x_eval_op_body(x_obj_t *p_base, x_obj_t *p_body,
	x_obj_t *p_caller, x_obj_t *p_op_head,
	x_obj_t *p_boundary, x_obj_t *p_shadow)
{
	x_obj_t *p_record = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		(x_obj_t *)&x_tco_op_tag,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, p_caller, p_op_head),
			x_mkspair(p_base, X_OBJ_FLAG_NONE, p_boundary, p_shadow)));
	x_obj_t **p_cell = x_heap_root_slot(p_base);
	x_spair_t root = x_obj_set((x_obj_t *)x_type_pair_obj, X_OBJ_FLAG_NONE,
		{ NULL }, { NULL });

	/* Root the restore record -- held only by this frame across every
	 * body eval until it reaches the tco-env field -- and the advancing
	 * body (one registered cell; popped on every exit path). */
	x_firstobj((x_obj_t *)root) = p_record;
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
				x_op_restore(p_base, p_record, 0);
				x_heap_root_pop(p_cell);
				return NULL;
			}

			x_firstobj(x_eval_field_tco_env(p_base)) = p_record;

			x_heap_root_pop(p_cell);
			return NULL;
		}

		x_restobj((x_obj_t *)root) = p_body;
		x_eval_arg(p_base, x_firstobj(p_body));

		p_body = x_restobj(p_body);
	}

	/* Empty body: restore synchronously. */
	x_op_restore(p_base, p_record, 0);

	x_heap_root_pop(p_cell);

	return NULL;
}

/*
 * Shared TCO keep/restore for the two trampoline loops -- x_eval's inline
 * loop and x_eval_tco_trampoline.  Both keep the first (outermost) proc env
 * compound and first operative record from the tco_env channel, then on exit
 * apply them in reverse capture order.  Extracted so the two copies cannot
 * drift (they were line-for-line duplicates).
 */

/* Keep the outermost record of each channel from a tco_env value.  The kept
 * records are also stored into @p p_tco_root's slots so the GC roots them
 * across the arbitrary evaluation between capture and restore (#243 -- a C
 * local is not a root; the records are already off the save-stack). */
static void x_tco_keep(x_obj_t *p_base, x_obj_t *p_te, x_obj_t *p_tco_root,
	x_obj_t **pp_op_save, x_obj_t **pp_proc_save,
	int *p_op_outermost, int *p_kept_any)
{
	int is_op;

	if (x_obj_isnil(p_base, p_te))
		return;

	is_op = (x_firstobj(p_te) == (x_obj_t *)&x_tco_op_tag);

	if ( ! *p_kept_any) {
		*p_op_outermost = is_op;
		*p_kept_any = 1;
	}

	if (is_op) {
		if (*pp_op_save == NULL || x_obj_isnil(p_base, *pp_op_save)) {
			*pp_op_save = p_te;
			x_restobj(p_tco_root) = p_te;
		}
	} else if (*pp_proc_save == NULL || x_obj_isnil(p_base, *pp_proc_save)) {
		*pp_proc_save = p_te;
		x_firstobj(p_tco_root) = p_te;
	}
}

/* Apply the kept records in REVERSE capture order so the OUTERMOST frame wins
 * env-alist (inner applied first, then overridden).  A proc compound present
 * alongside an op record means the op's tail resolved to an applied procedure
 * (let), whose closure frame must be shed -- force_caller carries that.  The
 * proc compound always restores the BST; ops leave it alone. */
static void x_tco_apply(x_obj_t *p_base, x_obj_t *p_op_save,
	x_obj_t *p_proc_save, int op_outermost)
{
	int has_proc = (p_proc_save != NULL && ! x_obj_isnil(p_base, p_proc_save));
	int has_op = (p_op_save != NULL && ! x_obj_isnil(p_base, p_op_save));

	if (op_outermost) {
		if (has_proc)
			x_tco_restore(p_base, p_proc_save);
		if (has_op)
			x_op_restore(p_base, p_op_save, has_proc);
	} else {
		if (has_op)
			x_op_restore(p_base, p_op_save, has_proc);
		if (has_proc)
			x_tco_restore(p_base, p_proc_save);
	}
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
 *            expression in tco_expr and the compound env snapshot in
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
 *            simple forms like if/do/match) but an inner form (fn/let)
 *            now provides a non-nil tco_env, the snapshot is upgraded.
 *          - Used only at exit: the outermost x_eval restores env-alist,
 *            local-boundary, global-BST, and shadow-list from the
 *            compound pair ((env . boundary) . (bst . shadow_head)).
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
 * @see x_prim_clear_shadows_to -- called during env restore to unwind shadow flags
 */
x_obj_t *x_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_exp;
	x_obj_t *p_tco_env_save = NULL;   /* first procedure env compound */
	x_obj_t *p_op_save = NULL;        /* first operative restore record */
	x_obj_t *p_te;                    /* tco_env fetched per trampoline pass */
	x_spair_t prim_args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL });
	/* Roots for the kept TCO restore records: they are popped off the
	 * save-stack, the tco-env field is cleared, and the records live only
	 * in the two locals above across every trampoline iteration --
	 * arbitrary evaluation -- until the exit restores apply them. */
	x_obj_t **p_cell = x_heap_root_slot(p_base);
	x_spair_t tco_root = x_obj_set((x_obj_t *)x_type_pair_obj,
		X_OBJ_FLAG_NONE, { NULL }, { NULL });
	int trampolining = 0;
	int op_outermost = 0;             /* the first record kept is an op record */
	int kept_any = 0;                 /* a tco_env (either channel) was kept */
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

		/* Keep the first (outermost) of each channel: procedures provide an
		 * env compound, operatives a tagged restore record.  if/do/match/and/or
		 * set neither (tco_env nil) -- an inner fn/let/op fills it later. */
		x_tco_keep(p_base, p_te, (x_obj_t *)tco_root,
			&p_op_save, &p_tco_env_save, &op_outermost, &kept_any);

		x_firstobj(x_eval_field_tco_env(p_base)) = NULL;
		x_firstobj(x_eval_arg_exp(p_args)) = x_firstobj(x_eval_field_tco_expr(p_base));
		x_firstobj(x_eval_field_tco_expr(p_base)) = NULL;
		x_atomint(x_firstobj(x_eval_field_profile_tco(p_base)))++;

		goto eval_start;
	}

	/* TCO env restore: only the x_eval that trampolined restores env
	 * (see x_tco_apply for the reverse-capture-order rationale). */
	if (trampolining && x_base_isset(p_base)) {
		x_firstobj(x_eval_field_tco_env(p_base)) = NULL;
		x_tco_apply(p_base, p_op_save, p_tco_env_save, op_outermost);
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
 * Extend an environment by binding parameters to values.
 *
 * Handles three cases: (1) variadic -- a bare symbol binds to the
 * entire remaining value list, (2) base -- no more params returns
 * the environment unchanged, (3) recursive -- binds first param to
 * first value, then recurses on the rest.
 *
 * The new spine cells carry X_OBJ_FLAG_FRAME, marking them as local
 * frame bindings: symbol lookup walks the frame region of the chain
 * before consulting the global BST, so locals -- including enclosing-
 * frame captures -- shadow globals with correct lexical semantics
 * (GH #47).
 *
 * @param p_base   x_obj_t* -- Base (execution context)
 * @param p_env    x_obj_t* -- Current environment alist
 * @param p_params x_obj_t* -- Parameter list (or single symbol for variadic)
 * @param p_vals   x_obj_t* -- Value list
 * @return x_obj_t* -- Extended environment alist (newly consed pairs)
 *
 * @details **No in-place mutation.**  Each binding creates a new
 *          (symbol . value) pair and a new alist cons cell prepended
 *          to @p p_env.  The original environment is never modified,
 *          which is essential for the TCO env-restore protocol: the
 *          saved env snapshot remains valid even after extension.
 *
 * @note The variadic case (bare symbol for p_params) binds the ENTIRE
 *       remaining value list, not just one value.  This implements
 *       rest-parameter semantics: @c (fn (a . rest) ...).
 *
 * @see x_type_symbol_eval -- the 3-step lookup that honours FRAME cells
 * @see x_prim_define      -- marks closure-scope def cells the same way
 * @see x_eval_body_tco    -- saves/restores env around extended scopes
 */
x_obj_t *x_env_extend(x_obj_t *p_base, x_obj_t *p_env,
	x_obj_t *p_params, x_obj_t *p_vals)
{
	x_obj_t *p_pair;
	x_obj_t *p_val;
	x_obj_t *p_rest;
	x_obj_t **pp_spine;

	/* Variadic: single symbol binds to entire remaining arg list. */
	if ( ! x_obj_isnil(p_base, p_params)
		&& x_obj_type_issymbol(p_base, p_params)) {
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

		return x_mkspair(p_base, X_OBJ_FLAG_FRAME, p_pair, p_env);
	}

	/* Base case: no more params. */
	if (x_obj_isnil(p_base, p_params)) {
		return p_env;
	}

	/* Recursive case: bind first param to first val, continue.
	 * When the args run out before the params do (fewer args than params),
	 * bind the remaining params to nil -- symmetric with surplus args, which
	 * are ignored once params run out.  Without this guard x_firstobj/
	 * x_restobj would dereference a nil p_vals and crash. */
	p_val  = x_obj_isnil(p_base, p_vals)
		? NULL : x_firstobj(p_vals);
	p_rest = x_obj_isnil(p_base, p_vals)
		? NULL : x_restobj(p_vals);
	p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_firstobj(p_params), p_val);

	return x_env_extend(p_base,
		x_mkspair(p_base, X_OBJ_FLAG_FRAME, p_pair, p_env),
		x_restobj(p_params),
		p_rest);
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
 * On early exit (nil tail) or empty body, pops and restores the
 * compound save-stack frame (env, boundary, BST, shadow list).
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_body  x_obj_t* -- List of body expressions
 * @return x_obj_t* -- Result of non-tail expressions, or NULL when
 *                      tail expression is deferred to the trampoline
 *
 * @details **Save-stack protocol.**  The caller (fn/let dispatch)
 *          pushes a compound pair onto save_stack BEFORE calling this
 *          function.  The compound has the shape:
 *          @code
 *          ((env-alist . local-boundary) . (global tree (a BST) . shadow-head))
 *          @endcode
 *          This captures the full env state prior to extension so it
 *          can be restored after the tail call completes.
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
 * @see x_prim_clear_shadows_to -- called during early-exit restore
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
				/* Save compound (env . boundary) for TCO restore */
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
 * On exit, restores the environment, local boundary, global BST, and
 * shadow list from the compound saved in tco-env.
 *
 * @param p_base   x_obj_t* -- Base (execution context)
 * @param p_result x_obj_t* -- Initial result (from non-tail evaluation)
 * @return x_obj_t* -- Final evaluation result
 *
 * @see x_eval_body_tco
 */
x_obj_t *x_eval_tco_trampoline(x_obj_t *p_base, x_obj_t *p_result)
{
	x_obj_t *p_tco, *p_te, *p_tco_env = NULL, *p_op_save = NULL;
	int op_outermost = 0, kept_any = 0;
	/* Roots for the kept restore records (#243, mirrors x_eval): they
	 * are popped off the save-stack and the tco-env field is cleared,
	 * so across the arbitrary evaluation below these locals hold the
	 * only references -- and a C local is not a root (x-heap.h).  An
	 * argument eval that triggers (heap-collect) would otherwise sweep
	 * the records the exit restores then read. */
	x_obj_t **p_cell = x_heap_root_slot(p_base);
	x_spair_t tco_root = x_obj_set((x_obj_t *)x_type_pair_obj,
		X_OBJ_FLAG_NONE, { NULL }, { NULL });

	x_heap_root_push(p_cell, tco_root);

	while ( ! x_obj_isnil(p_base, x_firstobj(x_eval_field_tco_expr(p_base)))) {
		p_tco = x_firstobj(x_eval_field_tco_expr(p_base));

		/* Keep the outermost record of each channel (mirrors x_eval). */
		p_te = x_firstobj(x_eval_field_tco_env(p_base));
		x_tco_keep(p_base, p_te, (x_obj_t *)tco_root,
			&p_op_save, &p_tco_env, &op_outermost, &kept_any);

		x_firstobj(x_eval_field_tco_expr(p_base)) = NULL;
		x_firstobj(x_eval_field_tco_env(p_base)) = NULL;
		p_result = x_eval_arg(p_base, p_tco);
	}

	x_tco_apply(p_base, p_op_save, p_tco_env, op_outermost);

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
 * (alist, local-boundary, global-tree, shadow-list), ctrl-group
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
 *          (env-alist . (local-boundary . (global-tree . shadow-list)))
 *          @endcode
 *          - env-alist: linear list of (symbol . value) bindings
 *          - local-boundary: pointer into alist separating locals from globals
 *          - global-tree: BST index over global bindings for O(log n) lookup
 *          - shadow-list: symbols with X_OBJ_FLAG_SHADOW for scope unwinding
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
 *          handler's error slot, then the env-alist and local-boundary
 *          are restored from the handler's saved copies (captured at
 *          guard installation time).  Finally, longjmp transfers control
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

		x_firstobj(x_eval_field_env_alist(p_base))
			= x_error_handler_saved_env(p_handler);
		x_eval_field_env_local_boundary(p_base)
			= x_error_handler_saved_boundary(p_handler);
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
 * Prepend a binding pair to the base's environment alist.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- (symbol . value) pair to prepend
 * @return x_obj_t* -- The new env alist head, or NULL if base is unset
 */
x_obj_t *x_eval_env_alist_extend(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_args }, { NULL });
	x_obj_t *p_old, *p_new;

	if ( ! x_base_isset(p_base)) {
		return NULL;
	}

	p_old = x_firstobj(x_eval_field_env_alist(p_base));
	x_restobj((x_obj_t *)args) = p_old;

	p_new = x_firstobj(x_eval_field_env_alist(p_base))
		= x_alist_extend(p_base, (x_obj_t *)args);

	/* Frame-region invariant (GH #47): symbol lookup's step 1 walks the
	 * LEADING run of FRAME-marked cells, so an unmarked cell must never
	 * be consed onto a frame head -- it would hide the frame cells below
	 * it.  A top-level-classified def CAN run with a frame head current
	 * (the TCO tail-def leak keeps op/printer frames on the caller's
	 * chain); it still binds globally through the BST, but its chain
	 * cell inherits the FRAME mark so the region stays contiguous. */
	if ( ! x_obj_isnil(p_base, p_old)
		&& (x_obj_flags(p_old) & X_OBJ_FLAG_FRAME)) {
		x_obj_flags(p_new) |= X_OBJ_FLAG_FRAME;
	}

	return p_new;
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
	x_obj_t *p_saved_stack;
	x_obj_t *p_saved_env, *p_saved_boundary, *p_env;
	x_satom_t exp_wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL });
	x_spair_t eval_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { exp_wrap }, { NULL })
	};
	x_spair_t read_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_buffer }, { p_base })
	};

	/* Each form read from the file is a TOP-LEVEL form: its top-level `def`s
	 * must bind globally (BST), not as locals of whatever was being evaluated
	 * when this load was triggered.  x_prim_define decides global-vs-local by
	 * testing whether the save-stack is empty, so when `include` runs under a
	 * (eval form env) -- which leaves a restore compound on the save-stack --
	 * a loaded file's defs would otherwise land in a transient local scope and
	 * vanish on restore (e.g. a module whose `def-class` is then Unbound).
	 * Hide the outer save-stack for the duration of the load so each form sees
	 * an empty stack, exactly as at the true top level.  Each x_eval call below
	 * balances its own pushes, so the stack is back to nil between iterations.
	 * On error the loaded form longjmps to its guard, which restores the
	 * interpreter state from guard's own snapshot -- this abandoned C frame's
	 * saved value is moot -- so a plain save/restore around the loop is safe. */
	p_saved_stack = x_eval_field_save_stack(p_base);
	x_eval_field_save_stack(p_base) = NULL;

	/* The same top-level contract, for CLOSURES: a closure a loaded file
	 * defines captures the env-alist head, so with the includer's lexical
	 * frames still on the chain it captures them permanently -- the x-level
	 * loader wrappers' formals (`path`, `name`, ...) then shadow the global
	 * env inside every loaded fn/op forever (the Logo server read its own
	 * module path where its request path should have been).  Strip the
	 * leading FRAME run -- exactly the frame region symbol lookup's step 1
	 * walks -- so each form evaluates against, and each closure captures,
	 * the true top-level chain (base-bind and global cells stay).  The
	 * boundary is cleared with it; both restore after the loop, and on
	 * error the longjmp target's own snapshot supersedes these saves, the
	 * same argument as the save-stack above. */
	p_saved_env = x_firstobj(x_eval_field_env_alist(p_base));
	p_saved_boundary = x_eval_field_env_local_boundary(p_base);
	p_env = p_saved_env;
	while ( ! x_obj_isnil(p_base, p_env)
		&& (x_obj_flags(p_env) & X_OBJ_FLAG_FRAME)) {
		p_env = x_restobj(p_env);
	}
	x_firstobj(x_eval_field_env_alist(p_base)) = p_env;
	x_eval_field_env_local_boundary(p_base) = NULL;

	for (;;) {
		p_exp = x_token_read(p_base, (x_obj_t *)read_args);
		/* Break on the EOF SENTINEL, not on nil: nil is the value a
		 * top-level `()` reads as, and breaking on it used to end the
		 * load there, silently skipping the rest of the file. */
		if (p_exp == (x_obj_t *)x_token_eof_prim) break;

		x_firstobj((x_obj_t *)exp_wrap) = p_exp;
		p_result = x_eval(p_base, (x_obj_t *)eval_args);
	}

	x_eval_field_save_stack(p_base) = p_saved_stack;
	x_firstobj(x_eval_field_env_alist(p_base)) = p_saved_env;
	x_eval_field_env_local_boundary(p_base) = p_saved_boundary;

	return p_result;
}

