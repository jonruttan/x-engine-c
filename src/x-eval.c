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
 * @param p_base    x_obj_t* -- Base (execution context)
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
	int is_op;
	int has_proc, has_op;
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

	/* Update base line counter from expression's source line metadata.
	 * After this, current-line reflects the eval site (useful for errors). */
	if (p_exp != NULL && (x_obj_flags(p_exp) & X_OBJ_FLAG_META)) {
		x_atomint(x_firstobj(x_eval_field_line(p_base))) = x_obj_meta_i(p_exp, 0).i;
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
		 * set neither (tco_env nil) -- an inner fn/let/op fills it later.
		 * op_outermost records whether the very first kept record is an op, so
		 * the exit can apply the records in reverse capture order. */
		if ( ! x_obj_isnil(p_base, p_te)) {
			is_op = (x_firstobj(p_te) == (x_obj_t *)&x_tco_op_tag);

			if ( ! kept_any) {
				op_outermost = is_op;
				kept_any = 1;
			}

			if (is_op) {
				if (p_op_save == NULL || x_obj_isnil(p_base, p_op_save)) {
					p_op_save = p_te;
					x_restobj((x_obj_t *)tco_root) = p_op_save;
				}
			} else if (p_tco_env_save == NULL || x_obj_isnil(p_base, p_tco_env_save)) {
				p_tco_env_save = p_te;
				x_firstobj((x_obj_t *)tco_root) = p_tco_env_save;
			}
		}

		x_firstobj(x_eval_field_tco_env(p_base)) = NULL;
		x_firstobj(x_eval_arg_exp(p_args)) = x_firstobj(x_eval_field_tco_expr(p_base));
		x_firstobj(x_eval_field_tco_expr(p_base)) = NULL;
		x_atomint(x_firstobj(x_eval_field_profile_tco(p_base)))++;

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
		has_proc = (p_tco_env_save != NULL
			&& ! x_obj_isnil(p_base, p_tco_env_save));
		has_op = (p_op_save != NULL
			&& ! x_obj_isnil(p_base, p_op_save));

		x_firstobj(x_eval_field_tco_env(p_base)) = NULL;

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

	x_heap_root_pop(p_cell);

	return p_exp;
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
	/* Backing store for the error-message atom.  Static lifetime so it
	 * survives the longjmp out of x_eval_error, but it is reached only
	 * through the base (x_eval_field_error_str), never by this name. */
	static x_char_t err_buf[X_ERROR_BUF_SIZE];
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
	 * descriptor (tools/base-layout.x) via the generated x-eval-layout.h.
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
	x_firstobj(x_eval_field_error_str(p_base)) = atom(nil);

	/* #t/#f and the sigint flag are inherited from a parent base so every
	 * base in a tree shares the singletons; the root base sets its own
	 * booleans during primitive registration. */
	if (p_parent != nil) {
		x_firstobj(x_eval_field_true(p_base)) = x_firstobj(x_eval_field_true(p_parent));
		x_firstobj(x_eval_field_false(p_base)) = x_firstobj(x_eval_field_false(p_parent));
		x_firstobj(x_eval_field_sigint(p_base)) = x_firstobj(x_eval_field_sigint(p_parent));
	}

	/* Point the error-message atom at the scratch buffer.  From here on the
	 * buffer is reached only through the base (x_eval_field_error_str). */
	x_atomstr(x_firstobj(x_eval_field_error_str(p_base))) = err_buf;


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
	x_char_t *buf;
	x_char_t *p_src;
	int n;
	int cap;

	/* Extract symbol string from object if possible. */
	if (p_obj != NULL && x_obj_type_issatom(p_obj)) {
		symbol = x_atomstr(p_obj);
	}

	/* If an error handler is installed, store message and longjmp. */
	if (x_base_isset(p_base)
		&& ! x_obj_isnil(p_base, x_firstobj(x_eval_field_error_handler(p_base)))) {
		p_handler = x_firstobj(x_eval_field_error_handler(p_base));
		/* The base-resident error atom; its string is the scratch buffer
		 * (X_ERROR_BUF_SIZE), reached only through the base. */
		p_err = x_firstobj(x_eval_field_error_str(p_base));
		buf = x_atomstr(p_err);
		n = 0;
		cap = X_ERROR_BUF_SIZE - 2;		/* room for closing "'" + '\0' */
		p_src = message;

		/* Copy the message; when the error names a symbol, append " '<symbol>'"
		 * so the guard reads e.g. "Unbound SYMBOL 'str'".  Formatting in place
		 * means no allocation, so it is safe even for out-of-memory errors. */
		while (*p_src != '\0' && n < cap) {
			buf[n++] = *p_src++;
		}
		if (symbol != NULL) {
			if (n < cap) {
				buf[n++] = ' ';
			}
			if (n < cap) {
				buf[n++] = '\'';
			}
			p_src = symbol;
			while (*p_src != '\0' && n < cap) {
				buf[n++] = *p_src++;
			}
			buf[n++] = '\'';
		}
		buf[n] = '\0';

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

	if ( ! x_base_isset(p_base)) {
		return NULL;
	}

	x_restobj((x_obj_t *)args) = x_firstobj(x_eval_field_env_alist(p_base));

	return x_firstobj(x_eval_field_env_alist(p_base)) = x_alist_extend(p_base, (x_obj_t *)args);
}

/**
 * Push a file descriptor onto the input file stack.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param fd      x_int_t -- File descriptor to push
 * @return x_obj_t* -- The new top-of-stack atom
 */
x_obj_t *x_eval_filein_push(x_obj_t *p_base, x_int_t fd)
{
	x_base_field_filein(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mksatom(p_base, X_OBJ_FLAG_NONE, fd), x_base_field_filein(p_base));
	return x_firstobj(x_base_field_filein(p_base));
}

/**
 * Pop the top file descriptor from the input file stack.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @return x_obj_t* -- The popped top-of-stack atom
 */
x_obj_t *x_eval_filein_pop(x_obj_t *p_base)
{
	x_obj_t *p_top = x_firstobj(x_base_field_filein(p_base));
	x_base_field_filein(p_base) =
		x_restobj(x_base_field_filein(p_base));
	return p_top;
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
 * Pop the top buffer from the buffer stack.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @return x_obj_t* -- The popped buffer object
 */
x_obj_t *x_eval_buffer_pop(x_obj_t *p_base)
{
	x_obj_t *p_top = x_firstobj(x_base_field_buffer(p_base));
	x_base_field_buffer(p_base) =
		x_restobj(x_base_field_buffer(p_base));
	return p_top;
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
 *       The shell driver pipes library source via stdin:
 *       @code
 *       cat lib/x.x - | ./x
 *       @endcode
 *       There is no file I/O in the C interpreter; all loading goes
 *       through the buffer/fd mechanism.
 *
 * @see x_eval_buffer_push -- push buffer before calling
 * @see x_eval_buffer_pop  -- pop buffer after calling
 * @see x_eval             -- evaluator called for each expression
 */
x_obj_t *x_eval_load(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_firstobj(x_base_field_buffer(p_base));
	x_obj_t *p_exp, *p_result = NULL;
	x_obj_t *p_saved_stack;
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

	for (;;) {
		p_exp = x_token_read(p_base, (x_obj_t *)read_args);
		if (x_obj_isnil(p_base, p_exp)) break;

		x_firstobj((x_obj_t *)exp_wrap) = p_exp;
		p_result = x_eval(p_base, (x_obj_t *)eval_args);
	}

	x_eval_field_save_stack(p_base) = p_saved_stack;

	return p_result;
}

/**
 * Write a string atom to the base's output file descriptor.
 *
 * Extracts the string pointer and optional length from @p p_args,
 * then delegates to x_base_write. If no length is provided, uses
 * strlen.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- (string-atom . optional-length)
 * @return x_obj_t* -- Result of x_base_write
 */
x_obj_t *x_eval_write_str(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_atom = x_firstobj(p_args);
	x_char_t *str = x_atomstr(p_atom);
	x_int_t len = x_obj_isnil(p_base, x_restobj(p_args))
		? (x_int_t)x_lib_strlen(str)
		: x_atomint(x_firstobj(x_restobj(p_args)));
	x_satom_t data = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .s = str }),
		sz = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .i = len });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE,
			{ data }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { sz }, { NULL })
	};

	return x_base_write(p_base, (x_obj_t *)args);
}

