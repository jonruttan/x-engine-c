/*
 * # Computational Expressions in C
 *
 * ## x-prim/type.c -- Implementation - Primitives - Types & Sandboxing
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
#include "x-alist.h"
#include "x-base.h"
#include "x-type/char.h"
#include "x-type/comment.h"
#include "x-type/buffer.h"
#include "x-type/int.h"
#include "x-type/list.h"
#include "x-type/operative.h"
#include "x-type/prim.h"
#include "x-type/procedure.h"
#include "x-type/ptr.h"
#include "x-type/str.h"
#include "x-type/symbol.h"
#include "x-type/whitespace.h"

/* make-type: (make-type name handlers-alist) -> create runtime type */
static x_obj_t *x_prim_make_type(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_name_str = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_handlers = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));
	x_char_t *name = x_lib_strndup(x_strval(p_name_str),
		x_lib_strlen(x_strval(p_name_str)));
	x_obj_t *p_name_atom = x_obj_make(p_base, x_type_atom_obj,
		X_OBJ_FLAG_OWN, X_OBJ_LENGTH_ATOM, name);
	struct x_type_t type = { 0 };
	x_obj_t *p_sym, *p_entry, *p_type;
	x_spair_t assoc_args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { (x_obj_t *)(assoc_args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_handlers }, { NULL })
	};

	type.p_name = p_name_atom;

	/* Look up handler closures from the alist. */
	p_sym = x_mksymbol(p_base, (x_char_t *)"call");
	x_firstobj((x_obj_t *)assoc_args) = p_sym;
	p_entry = x_alist_assoc(p_base, (x_obj_t *)assoc_args);
	if ( ! x_obj_isnil(p_base, p_entry)) {
		type.p_call = x_restobj(p_entry);
	}

	p_sym = x_mksymbol(p_base, (x_char_t *)"write");
	x_firstobj((x_obj_t *)assoc_args) = p_sym;
	p_entry = x_alist_assoc(p_base, (x_obj_t *)assoc_args);
	if ( ! x_obj_isnil(p_base, p_entry)) {
		type.p_write = x_restobj(p_entry);
	}

	p_sym = x_mksymbol(p_base, (x_char_t *)"length");
	x_firstobj((x_obj_t *)assoc_args) = p_sym;
	p_entry = x_alist_assoc(p_base, (x_obj_t *)assoc_args);
	if ( ! x_obj_isnil(p_base, p_entry)) {
		type.p_length = x_restobj(p_entry);
	}

	p_type = x_type_struct_make(p_base, type);
	x_base_type_alist_extend(p_base, p_type);

	return p_name_atom;
}

/* make-instance: (make-instance type-handle data) -> create typed instance */
static x_obj_t *x_prim_make_instance(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_handle = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_data = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));
	x_spair_t lookup_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_handle }, { NULL })
	};
	x_obj_t *p_type = x_base_type_alist_assoc(p_base, (x_obj_t *)lookup_args);

	if (x_obj_isnil(p_base, p_type)) {
		return p_base;
	}

	return x_obj_make(p_base, p_type, 0, X_OBJ_LENGTH_PAIR, p_data, p_base);
}

/* type?: (type? obj type-handle) -> t if obj's type matches handle */
static x_obj_t *x_prim_typep(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_handle = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	if (x_obj_isnil(p_base, p_obj) || x_obj_isnil(p_base, x_obj_type(p_obj))) {
		return p_base;
	}

	return x_type_field_name(x_obj_type(p_obj)) == p_handle
		? x_mksymbol(p_base, (x_char_t *)X_PRIM_TRUE) : p_base;
}

/* type-name: (type-name obj) -> name string of obj's type */
static x_obj_t *x_prim_type_name(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_name;

	if (x_obj_isnil(p_base, p_obj) || x_obj_isnil(p_base, x_obj_type(p_obj))) {
		return p_base;
	}

	p_name = x_type_field_name(x_obj_type(p_obj));

	if (x_obj_isnil(p_base, p_name)) {
		return p_base;
	}

	return x_mkstr(p_base, x_atomstr(p_name));
}

/* make-base: (make-base) -> create fresh sandboxed interpreter */
static x_obj_t *x_prim_make_base(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_new_base, *p_buffer;
	x_char_t *buffer = (x_char_t *)x_sys_malloc(256);

	p_new_base = x_base_make(NULL, NULL);

	/* Register types. */
	x_type_prim_register(p_new_base, p_new_base);
	x_type_operative_register(p_new_base, p_new_base);
	x_type_procedure_register(p_new_base, p_new_base);
	x_type_symbol_register(p_new_base, p_new_base);
	x_type_list_register(p_new_base, p_new_base);
	x_type_int_register(p_new_base, p_new_base);
	x_type_str_register(p_new_base, p_new_base);
	x_type_char_register(p_new_base, p_new_base);
	x_type_whitespace_register(p_new_base, p_new_base);
	x_type_comment_register(p_new_base, p_new_base);

	/* Set up read buffer. */
	p_buffer = x_mkbuffer(p_new_base, buffer);
	x_base_field_buffer(p_new_base) = p_buffer;

	/* Register primitives. */
	x_prim_register(p_new_base, p_new_base);

	return p_new_base;
}

/* Rebuild expression tree replacing source nil with target nil. */
static x_obj_t *x_prim_renil(x_obj_t *p_src, x_obj_t *p_dst,
	x_obj_t *p_exp)
{
	if (x_obj_isnil(p_src, p_exp)) {
		return p_dst;
	}

	if (x_obj_type_islist(p_src, p_exp)) {
		return x_mklist(p_dst,
			x_prim_renil(p_src, p_dst, x_firstobj(p_exp)),
			x_prim_renil(p_src, p_dst, x_restobj(p_exp)));
	}

	return p_exp;
}

/* base-eval: (base-eval base expr) -> eval expr in target base */
static x_obj_t *x_prim_base_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	x_error_handler_t handler;
	x_obj_t *p_target = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_expr = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args))),
		*p_prev_handler = x_base_field_error_handler(p_target),
		*p_result;

	/* Re-nil: replace parent nil terminators with target nil. */
	p_expr = x_prim_renil(p_base, p_target, p_expr);

	/* Install bridge handler in target to propagate errors to parent. */
	handler.p_error = p_target;
	handler.error_msg = NULL;
	handler.p_saved_env = x_base_field_env_alist(p_target);
	handler.prev = x_obj_isnil(p_target, p_prev_handler)
		? NULL : (x_error_handler_t *)x_ptrval(p_prev_handler);
	x_base_field_error_handler(p_target) = x_mkptr(p_target, &handler);

	if (setjmp(handler.jmp) == 0) {
		p_result = x_prim_eval_arg(p_target, p_expr);
	} else {
		/* Error caught from target: restore and re-signal in parent. */
		x_base_field_error_handler(p_target) = handler.prev
			? x_mkptr(p_target, handler.prev) : p_target;
		x_base_field_env_alist(p_target) = handler.p_saved_env;

		if ( ! x_obj_isnil(p_base, x_base_field_error_handler(p_base))) {
			x_error_handler_t *p_parent =
				(x_error_handler_t *)x_ptrval(
					x_base_field_error_handler(p_base));

			p_parent->p_error = handler.p_error;
			p_parent->error_msg = handler.error_msg;
			x_base_field_env_alist(p_base) = p_parent->p_saved_env;
			longjmp(p_parent->jmp, 1);
		}

		x_obj_error(p_base, "error", NULL);

		return p_base;
	}

	/* Pop handler. */
	x_base_field_error_handler(p_target) = handler.prev
		? x_mkptr(p_target, handler.prev) : p_target;

	return p_result;
}

/* base-bind: (base-bind base name value) -> bind in target base */
static x_obj_t *x_prim_base_bind(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_target = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_name = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args))),
		*p_val = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(x_restobj(p_args)))),
		*p_pair;

	/* Re-nil list values for cross-base compatibility. */
	p_val = x_prim_renil(p_base, p_target, p_val);
	p_pair = x_mkspair(p_target, p_name, p_val);

	x_base_env_alist_extend(p_target, p_pair);

	return p_val;
}

x_obj_t *x_prim_type_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_prim_bind(p_base, "make-type", x_prim_make_type);
	x_prim_bind(p_base, "make-instance", x_prim_make_instance);
	x_prim_bind(p_base, "type?", x_prim_typep);
	x_prim_bind(p_base, "type-name", x_prim_type_name);
	x_prim_bind(p_base, "make-base", x_prim_make_base);
	x_prim_bind(p_base, "base-eval", x_prim_base_eval);
	x_prim_bind(p_base, "base-bind", x_prim_base_bind);

	return p_base;
}
