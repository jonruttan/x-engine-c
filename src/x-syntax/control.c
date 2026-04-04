/** @file control.c
 *  @brief Syntax - Control Flow (match, guard, error, %seq)
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2024 Jon Ruttan
 *  @license MIT No Attribution (MIT-0)
 */

/*     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
#include "x-prim.h"
#include "x-base-typesystem.h"
#include "x-type/ptr.h"
#include "x-type/str.h"
#include <setjmp.h>

/**
 * Conditional dispatch form. x-lang: (match (test body) ...)
 *
 * Evaluates each test in order (fexpr -- clause structure is not
 * evaluated, but each test expression is explicitly evaluated).  Returns
 * the body of the first clause whose test is truthy, via tail-call
 * evaluation.  Returns nil if no clause matches.
 *
 * @param p_base  Execution context.
 * @param p_args  Unevaluated argument list; expects (caller (test body) ...).
 * @return NULL; result delivered via TCO expr slot.
 *
 * @details **Tail position.**  The matching clause's body expression is
 *          stored in tco_expr rather than evaluated directly.  This
 *          makes the last clause a proper tail call -- x_eval's
 *          trampoline will pick it up and evaluate it without growing
 *          the C stack.  Because match does not alter the environment,
 *          tco_env is left nil (simple TCO); the trampoline skips env
 *          restore.
 *
 * @note Only the body of the FIRST matching clause is deferred.
 *       Subsequent clauses are never evaluated.  If no clause matches,
 *       tco_expr remains nil and x_eval returns NULL.
 *
 * @see x_eval                 -- trampoline that consumes tco_expr
 * @see x_eval_body_tco_simple -- similar simple-TCO pattern used by if/do
 */
static x_obj_t *x_prim_match(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_clause, *p_test;
	p_args = x_1(p_args);
	while ( ! x_obj_isnil(p_base, p_args)) {
		p_clause = x_firstobj(p_args);
		p_test = x_eval_arg(p_base, x_firstobj(p_clause));

		if ( ! x_obj_isnil(p_base, p_test)
				&& p_test != x_firstobj(x_base_field_false(p_base))) {
			x_firstobj(x_base_field_tco_expr(p_base)) =
				x_firstobj(x_restobj(p_clause));

			return NULL;
		}

		p_args = x_restobj(p_args);
	}

	return NULL;
}

/**
 * Error recovery form. x-lang: (guard (var handler-body ...) body ...)
 *
 * Installs an error handler, evaluates body, and catches errors (fexpr --
 * clause and body forms are not evaluated up front).  On error, restores
 * the save stack and environment boundary to the guard point, binds the
 * error value to var, and evaluates handler-body.
 *
 * @param p_base  Execution context.
 * @param p_args  Unevaluated argument list; expects (caller (var handler-body ...) body ...).
 * @return Result of body on success, or result of handler-body on error.
 *
 * @details **Handler pair tree structure.**  The handler object is a
 *          nested pair tree:
 *          @code
 *          (jmp-ptr . ((saved-env . saved-boundary) . (error-value . nil)))
 *          @endcode
 *          - jmp-ptr: x_ptr wrapping a jmp_buf on the C stack
 *          - saved-env: env-alist snapshot at guard installation
 *          - saved-boundary: local-boundary snapshot at guard installation
 *          - error-value: initially nil, filled by x_base_error or
 *            x_prim_error before longjmp
 *
 * @details **Handler stack.**  The handler is pushed onto
 *          error_handler (a single-slot stack on p_base) and the
 *          previous handler is saved in p_prev_handler.  On exit
 *          (normal or error), the previous handler is restored.  This
 *          provides nested guard support -- inner guards catch first.
 *
 * @details **setjmp/longjmp protocol.**  setjmp(jmp) returns 0 on
 *          installation (normal path: evaluate body).  When
 *          x_base_error or x_prim_error calls longjmp, setjmp returns
 *          non-zero (error path).  On the error path:
 *          1. save_stack is restored to the guard point (unwinding
 *             any fn/let frames entered since guard)
 *          2. error value is bound to var in the current env
 *          3. handler-body is evaluated via x_eval_body (no TCO --
 *             the guard frame must remain on the C stack)
 *          4. env-alist and local-boundary are restored from the
 *             handler's saved copies
 *
 * @note Uses setjmp/longjmp for non-local error transfer.  The jmp_buf
 *       lives on this C frame, so the handler is only valid while this
 *       function is on the call stack.  Capturing and invoking it after
 *       return would be undefined behavior.
 *
 * @note The body is evaluated with x_eval_body (no TCO), not
 *       x_eval_body_tco.  This ensures the guard's C frame (and its
 *       jmp_buf) remains valid throughout body execution.
 *
 * @see x_base_error   -- C-level error that longjmps to this handler
 * @see x_prim_error   -- x-lang (error msg) that longjmps to this handler
 */
static x_obj_t *x_prim_guard(x_obj_t *p_base, x_obj_t *p_args)
{
	jmp_buf jmp;
	x_obj_t *p_clause, *p_var, *p_handler_body, *p_body,
		*p_prev_handler = x_firstobj(x_base_field_error_handler(p_base)),
		*p_saved_save_stack = x_base_field_save_stack(p_base),
		*p_handler, *p_result = NULL;
	x_args(p_args, 2, NULL, &p_clause);
	p_var = x_firstobj(p_clause);
	p_handler_body = x_restobj(p_clause);
	p_body = x_11(p_args);

	/* Build handler: (jmp-ptr (saved-env . saved-boundary) error-value) */
	p_handler = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_mkptr(p_base, &jmp),
		x_mkspair(p_base, X_OBJ_FLAG_NONE,
			x_mkspair(p_base, X_OBJ_FLAG_NONE, x_firstobj(x_base_field_env_alist(p_base)),
			                   x_base_field_env_local_boundary(p_base)),
			x_mkspair(p_base, X_OBJ_FLAG_NONE, NULL, NULL)));
	x_firstobj(x_base_field_error_handler(p_base)) = p_handler;

	if (setjmp(jmp) == 0) {
		/* Normal execution: evaluate body. */
		p_result = x_eval_body(p_base, p_body);
	} else {
		/* Error caught: restore save-stack and boundary to guard point. */
		x_obj_t *p_err = x_error_handler_error(p_handler);
		x_obj_t *p_pair = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_var, p_err);

		x_base_field_save_stack(p_base) = p_saved_save_stack;
		x_base_env_alist_extend(p_base, p_pair);
		p_result = x_eval_body(p_base, p_handler_body);
		x_firstobj(x_base_field_env_alist(p_base))
			= x_error_handler_saved_env(p_handler);
		x_base_field_env_local_boundary(p_base)
			= x_error_handler_saved_boundary(p_handler);
	}

	/* Pop handler. */
	x_firstobj(x_base_field_error_handler(p_base)) = p_prev_handler;

	return p_result;
}

/**
 * Error signalling form. x-lang: (error message)
 *
 * Signals an error with the given message (fexpr -- message is explicitly
 * evaluated via x_eargs).  If a guard handler is installed, transfers
 * control to it via longjmp.  Otherwise falls through to a fatal error.
 *
 * @param p_base  Execution context.
 * @param p_args  Unevaluated argument list; expects (caller message).
 * @return Does not return normally when a handler is installed.
 * @note Falls through to x_obj_error for fatal output when no handler exists.
 * @see x_prim_guard
 */
static x_obj_t *x_prim_error(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_msg, *p_handler = x_firstobj(x_base_field_error_handler(p_base));
	x_eargs(p_base, p_args, 2, NULL, &p_msg);

	/* If handler installed, use it. */
	if ( ! x_obj_isnil(p_base, p_handler)) {
		x_error_handler_error(p_handler) = p_msg;
		x_firstobj(x_base_field_env_alist(p_base))
			= x_error_handler_saved_env(p_handler);
		x_base_field_env_local_boundary(p_base)
			= x_error_handler_saved_boundary(p_handler);
		longjmp(*(jmp_buf *)x_error_handler_jmp(p_handler), 1);
	}

	/* No handler: fall through to fatal error. */
	if (x_obj_type_isstr(p_base, p_msg)) {
		x_obj_error(p_base, x_strval(p_msg), NULL);
	} else {
		x_obj_error(p_base, "error", p_msg);
	}

	return NULL;
}

/**
 * Sequence form. x-lang: (%seq a b)
 *
 * Evaluates the first expression for its side effects, then tail-call
 * evaluates the second (fexpr -- both arguments are explicitly evaluated
 * internally).  Roots the argument list during evaluation of the first
 * expression to protect it from GC.
 *
 * @param p_base  Execution context.
 * @param p_args  Unevaluated argument list; expects (caller a b).
 * @return NULL; result of b delivered via TCO expr slot.
 * @note Internal primitive; used by the compiler to sequence body forms.
 */
static x_obj_t *x_prim_seq(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_a, *p_b;
	x_args(p_args, 3, NULL, &p_a, &p_b);

	/* Root args so GC doesn't free them during eval of first arg */
	x_base_field_eval_list(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		x_1(p_args), x_base_field_eval_list(p_base));

	x_eval_arg(p_base, p_a);
	x_firstobj(x_base_field_tco_expr(p_base)) = p_b;

	/* Unroot */
	x_base_field_eval_list(p_base)
		= x_restobj(x_base_field_eval_list(p_base));

	return NULL;
}

/**
 * Register control flow syntax primitives.
 *
 * Binds: match, guard, error, %seq.
 *
 * @param p_base  Execution context.
 * @param p_args  Unused.
 * @return p_base.
 */
x_obj_t *x_syntax_control_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_callable_entry_t entries[] = {
		{ "match", x_prim_match },
		{ "guard", x_prim_guard },
		{ "error", x_prim_error },
		{ "%seq", x_prim_seq }
	};

	x_callable_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
