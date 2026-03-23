/*
 * # Computational Expressions in C
 *
 * ## x-syntax/control.c -- Syntax - Control Flow (match, guard, %seq)
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
#include "x-prim.h"
#include "x-base.h"
#include "x-type/ptr.h"
#include "x-type/str.h"
#include <setjmp.h>

/* match: (match (test body)...) -> first truthy test's body (tail-eval) */
static x_obj_t *x_prim_match(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_clause, *p_test;
	p_args = x_1(p_args);
	while ( ! x_obj_isnil(p_base, p_args)) {
		p_clause = x_firstobj(p_args);
		p_test = x_eval_arg(p_base, x_firstobj(p_clause));

		if ( ! x_obj_isnil(p_base, p_test)
				&& p_test != x_base_field_false(p_base)) {
			x_base_field_tco_expr(p_base) =
				x_firstobj(x_restobj(p_clause));

			return NULL;
		}

		p_args = x_restobj(p_args);
	}

	return NULL;
}

/* guard: (guard (var handler-body...) body...) -> error recovery */
static x_obj_t *x_prim_guard(x_obj_t *p_base, x_obj_t *p_args)
{
	jmp_buf jmp;
	x_obj_t *p_clause, *p_var, *p_handler_body, *p_body,
		*p_prev_handler = x_base_field_error_handler(p_base),
		*p_saved_save_stack = x_base_field_save_stack(p_base),
		*p_handler, *p_result = NULL;
	x_args(p_args, 2, NULL, &p_clause);
	p_var = x_firstobj(p_clause);
	p_handler_body = x_restobj(p_clause);
	p_body = x_11(p_args);

	/* Build handler: (jmp-ptr (saved-env . saved-boundary) error-value) */
	p_handler = x_mkspair(p_base,
		x_mkptr(p_base, &jmp),
		x_mkspair(p_base,
			x_mkspair(p_base, x_base_field_env_alist(p_base),
			                   x_base_field_env_local_boundary(p_base)),
			x_mkspair(p_base, NULL, NULL)));
	x_base_field_error_handler(p_base) = p_handler;

	if (setjmp(jmp) == 0) {
		/* Normal execution: evaluate body. */
		p_result = x_eval_body(p_base, p_body);
	} else {
		/* Error caught: restore save-stack and boundary to guard point. */
		x_obj_t *p_err = x_error_handler_error(p_handler);
		x_obj_t *p_pair = x_mkspair(p_base, p_var, p_err);

		x_base_field_save_stack(p_base) = p_saved_save_stack;
		x_base_env_alist_extend(p_base, p_pair);
		p_result = x_eval_body(p_base, p_handler_body);
		x_base_field_env_alist(p_base)
			= x_error_handler_saved_env(p_handler);
		x_base_field_env_local_boundary(p_base)
			= x_error_handler_saved_boundary(p_handler);
	}

	/* Pop handler. */
	x_base_field_error_handler(p_base) = p_prev_handler;

	return p_result;
}

/* error: (error message) -> signal an error */
static x_obj_t *x_prim_error(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_msg, *p_handler = x_base_field_error_handler(p_base);
	x_eargs(p_base, p_args, 2, NULL, &p_msg);

	/* If handler installed, use it. */
	if ( ! x_obj_isnil(p_base, p_handler)) {
		x_error_handler_error(p_handler) = p_msg;
		x_base_field_env_alist(p_base)
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

/* %seq: (%seq a b) -> eval a (blocking), tco-eval b */
static x_obj_t *x_prim_seq(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_a, *p_b;
	x_args(p_args, 3, NULL, &p_a, &p_b);

	/* Root args so GC doesn't free them during eval of first arg */
	x_base_field_eval_list_stack(p_base) = x_mkspair(p_base,
		x_1(p_args), x_base_field_eval_list_stack(p_base));

	x_eval_arg(p_base, p_a);
	x_base_field_tco_expr(p_base) = p_b;

	/* Unroot */
	x_base_field_eval_list_stack(p_base)
		= x_restobj(x_base_field_eval_list_stack(p_base));

	return NULL;
}

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
