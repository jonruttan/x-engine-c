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
#include "x-interp.h"
#include "x-obj.h"
#include "x-prim.h"
#include "x-type.h"

#include "x-type/prim.h"

/**
 * Push the current environment state as a TCO restore compound.
 *
 * Snapshots env-alist, local-boundary, global-bst, and shadow-head into a
 * compound @c ((env . boundary) . (bst . shadow)) and pushes it onto the
 * save-stack.  Procedure calls and eval-with-env use this to capture the
 * environment before extending it; the trampoline (or x_eval_body_tco's
 * early-exit paths) restores from it via x_tco_restore().
 *
 * @param p_base  x_obj_t* -- Execution context
 * @return x_obj_t* -- The pushed compound
 * @see x_tco_restore
 */
x_obj_t *x_tco_compound_save(x_obj_t *p_base)
{
	x_obj_t *p_compound = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_firstobj(x_interp_field_env_alist(p_base)),
			x_interp_field_env_local_boundary(p_base)),
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_interp_field_env_global_tree(p_base),
			x_interp_field_shadow_list(p_base)));

	x_interp_field_save_stack(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		p_compound, x_interp_field_save_stack(p_base));

	return p_compound;
}

/**
 * Restore env-alist, local-boundary, global-bst, and shadow list from a TCO
 * compound @c ((env . boundary) . (bst . shadow)).
 *
 * Does NOT touch the save-stack -- callers that took the compound from the
 * save-stack top pop it separately.  This is the single restore used by both
 * trampoline exit points (x_eval, x_eval_tco_trampoline), x_eval_body_tco's
 * early-exit paths, and eval-with-env.
 *
 * @param p_base      x_obj_t* -- Execution context
 * @param p_compound  x_obj_t* -- Compound built by x_tco_compound_save()
 * @see x_tco_compound_save
 */
void x_tco_restore(x_obj_t *p_base, x_obj_t *p_compound)
{
	x_firstobj(x_interp_field_env_alist(p_base))
		= x_firstobj(x_firstobj(p_compound));
	x_interp_field_env_local_boundary(p_base)
		= x_restobj(x_firstobj(p_compound));
	x_interp_field_env_global_tree(p_base)
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
 * @param p_base    x_obj_t* -- Execution context
 * @param p_record  x_obj_t* -- Operative record built by x_eval_op_body()
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
		x_firstobj(x_interp_field_env_alist(p_base)) = p_caller;
	} else {
		p_walk = x_firstobj(x_interp_field_env_alist(p_base));
		while ( ! x_obj_isnil(p_base, p_walk) && p_walk != p_head) {
			p_walk = x_restobj(p_walk);
		}
		if (p_walk == p_head) {
			x_firstobj(x_interp_field_env_alist(p_base)) = p_caller;
		}
	}

	x_interp_field_env_local_boundary(p_base) = p_boundary;
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
 * @param p_base      x_obj_t* -- Execution context
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

	while ( ! x_obj_isnil(p_base, p_body)) {
		if (x_obj_isnil(p_base, x_restobj(p_body))) {
			x_firstobj(x_interp_field_tco_expr(p_base)) = x_firstobj(p_body);

			/* Nil tail: no trampoline will run -- restore synchronously. */
			if (x_obj_isnil(p_base,
				x_firstobj(x_interp_field_tco_expr(p_base)))) {
				x_op_restore(p_base, p_record, 0);
				return NULL;
			}

			x_firstobj(x_interp_field_tco_env(p_base)) = p_record;

			return NULL;
		}

		x_obj_push_field(p_base, &x_interp_field_eval_list(p_base),
			p_body, X_OBJ_FLAG_NONE);
		x_eval_arg(p_base, x_firstobj(p_body));
		x_obj_pop_field(p_base, &x_interp_field_eval_list(p_base));

		p_body = x_restobj(p_body);
	}

	/* Empty body: restore synchronously. */
	x_op_restore(p_base, p_record, 0);

	return NULL;
}

/**
 * Evaluate an expression with tail-call optimization.
 *
 * Dispatches to the expression's type-level eval handler. If the handler
 * sets a TCO tail expression on p_base, the trampoline loop re-evaluates
 * without growing the C stack. On exit, restores the environment from
 * the saved TCO snapshot.
 *
 * @param p_base  x_obj_t* -- Execution context
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
 *            tco_env.  x_eval_body_tco_simple and x_prim_match store
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
 * @see x_eval_body_tco_simple -- lightweight TCO (sets tco_expr only)
 * @see x_eval_tco_trampoline -- standalone trampoline used by closure call paths
 * @see x_prim_clear_shadows_to -- called during env restore to unwind shadow flags
 */
x_obj_t *x_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_exp;
	x_obj_t *p_tco_env_save = NULL;   /* first procedure env compound */
	x_obj_t *p_op_save = NULL;        /* first operative restore record */
	x_spair_t prim_args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL });
	int trampolining = 0;
	int op_outermost = 0;             /* the first record kept is an op record */
	int kept_any = 0;                 /* a tco_env (either channel) was kept */
#ifdef X_SIGNAL
	/* Interrupt-flag pointer, resolved once from the base (signal-register
	 * publishes signal.c's static atom here).  Cached so the trampoline pays
	 * a single load per iteration, and so a GC relocation of the base spine
	 * mid-eval can't invalidate it -- the target is a non-heap static. */
	x_obj_t *p_sigint = x_base_isset(p_base)
		? x_firstobj(x_interp_field_sigint(p_base)) : NULL;
#endif

eval_start:
#ifdef X_SIGNAL
	/* SIGINT: throw STOP if a guard is active.  Volatile cast forces a
	 * re-read each iteration; without it -O2 hoists it out of the loop. */
	if (p_sigint != NULL
		&& *(volatile x_int_t *)&x_atomint(p_sigint)
		&& ! x_obj_isnil(p_base,
			x_firstobj(x_interp_field_error_handler(p_base)))) {
		x_atomint(p_sigint) = 0;
		x_interp_error(p_base, "STOP", NULL);
	}
#endif
	if (x_base_isset(p_base))
		x_atomint(x_firstobj(x_interp_field_profile_evals(p_base)))++;
	p_exp = x_firstobj(x_eval_arg_exp(p_args));

	/* Update base line counter from expression's source line metadata.
	 * After this, current-line reflects the eval site (useful for errors). */
	if (p_exp != NULL && (x_obj_flags(p_exp) & X_OBJ_FLAG_META))
		x_atomint(x_firstobj(x_interp_field_line(p_base)))
			= x_obj_meta_i(p_exp, 0).i;

#ifdef X_COV
	if (p_exp != NULL)
		x_obj_flags(p_exp) |= X_OBJ_FLAG_COV;
#endif

	if (x_obj_isnil(p_base, p_exp)) {
		return NULL;
	}

	/* Differentiate simple from complex types.
	 * Guard: NULL-typed (raw stack) objects self-evaluate. */
	if (x_obj_type(p_exp) == NULL
		|| x_obj_isnil(p_base, x_obj_type(x_obj_type(p_exp)))) {
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
	if (x_base_isset(p_base)
		&& ! x_obj_isnil(p_base, x_firstobj(x_interp_field_tco_expr(p_base)))) {
		x_obj_t *p_te = x_firstobj(x_interp_field_tco_env(p_base));

		trampolining = 1;

		/* Keep the first (outermost) of each channel: procedures provide an
		 * env compound, operatives a tagged restore record.  if/do/match/and/or
		 * set neither (tco_env nil) -- an inner fn/let/op fills it later.
		 * op_outermost records whether the very first kept record is an op, so
		 * the exit can apply the records in reverse capture order. */
		if ( ! x_obj_isnil(p_base, p_te)) {
			int is_op = (x_firstobj(p_te) == (x_obj_t *)&x_tco_op_tag);

			if ( ! kept_any) {
				op_outermost = is_op;
				kept_any = 1;
			}
			if (is_op) {
				if (p_op_save == NULL || x_obj_isnil(p_base, p_op_save))
					p_op_save = p_te;
			} else if (p_tco_env_save == NULL
				|| x_obj_isnil(p_base, p_tco_env_save)) {
				p_tco_env_save = p_te;
			}
		}

		x_firstobj(x_interp_field_tco_env(p_base)) = NULL;
		x_firstobj(x_eval_arg_exp(p_args)) = x_firstobj(x_interp_field_tco_expr(p_base));
		x_firstobj(x_interp_field_tco_expr(p_base)) = NULL;
		x_atomint(x_firstobj(x_interp_field_profile_tco(p_base)))++;

		goto eval_start;
	}

	/* TCO env restore: only the x_eval that trampolined restores env.
	 * Apply the two channels in REVERSE capture order so the OUTERMOST frame
	 * (captured first) wins env-alist: the inner record is applied first and
	 * then overridden.  This matters because a procedure's tail can be an
	 * operative (proc outer) or an operative's tail a procedure (op outer);
	 * a fixed order would leave one of those with the inner frame.  The proc
	 * compound always restores the BST (ops leave it alone), so whichever
	 * order, the inner proc still re-establishes the correct BST.
	 *
	 * has_proc -> force the op record's env restore to the caller: a proc
	 * compound here means the op's tail resolved to an applied procedure (let),
	 * whose closure frame must be shed rather than kept. */
	if (trampolining && x_base_isset(p_base)) {
		int has_proc = (p_tco_env_save != NULL
			&& ! x_obj_isnil(p_base, p_tco_env_save));
		int has_op = (p_op_save != NULL
			&& ! x_obj_isnil(p_base, p_op_save));

		x_firstobj(x_interp_field_tco_env(p_base)) = NULL;

		if (op_outermost) {
			/* op outermost -> apply inner proc first, outer op last. */
			if (has_proc)
				x_tco_restore(p_base, p_tco_env_save);
			if (has_op)
				x_op_restore(p_base, p_op_save, has_proc);
		} else {
			/* proc outermost (or no op) -> apply inner op first, outer proc last. */
			if (has_op)
				x_op_restore(p_base, p_op_save, has_proc);
			if (has_proc)
				x_tco_restore(p_base, p_tco_env_save);
		}
	}

	return p_exp;
}
