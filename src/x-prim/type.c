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
#include "x-base-typesystem.h"
#include "x-heap.h"
#include "x-type.h"
#include <stddef.h>
#include <setjmp.h>
#include "x-token.h"
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

/* Helper: build type struct from handlers alist.
 * p_base is used for symbol lookup and allocation. */
static x_obj_t *x_prim_type_build_struct(x_obj_t *p_base,
	x_obj_t *p_name_atom, x_obj_t *p_handlers)
{
	struct x_type_t type = { 0 };
	x_obj_t *p_sym, *p_entry;
	x_spair_t assoc_args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { (x_obj_t *)(assoc_args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_handlers }, { NULL })
	};

	type.p_name = p_name_atom;
	type.p_units = (x_obj_t *)&x_type_units_pair_obj;

	/* Look up handler closures from the alist. */
	{
		static const struct { x_char_t *name; size_t offset; } fields[] = {
			{ "call",    offsetof(struct x_type_t, p_call) },
			{ "eval",    offsetof(struct x_type_t, p_eval) },
			{ "write",   offsetof(struct x_type_t, p_write) },
			{ "display", offsetof(struct x_type_t, p_display) },
			{ "length",  offsetof(struct x_type_t, p_length) },
			{ "analyse", offsetof(struct x_type_t, p_analyse) },
			{ "delimit", offsetof(struct x_type_t, p_delimit) },
			{ "read",    offsetof(struct x_type_t, p_read) },
			{ "error",   offsetof(struct x_type_t, p_error) },
			{ "from",    offsetof(struct x_type_t, p_from) },
			{ "to",      offsetof(struct x_type_t, p_to) },
			{ "units",   offsetof(struct x_type_t, p_units) },
			{ "free",    offsetof(struct x_type_t, p_free) },
			{ "mark",    offsetof(struct x_type_t, p_mark) },
			{ "first-chars", offsetof(struct x_type_t, p_data) },
			{ "iter",    offsetof(struct x_type_t, p_iter) }
		};
		int i;

		for (i = 0; i < (int)(sizeof(fields) / sizeof(fields[0])); i++) {
			p_sym = x_mksymbol(p_base, fields[i].name);
			x_firstobj((x_obj_t *)assoc_args) = p_sym;
			p_entry = x_alist_assoc(p_base, (x_obj_t *)assoc_args);
			if ( ! x_obj_isnil(p_base, p_entry))
				*(x_obj_t **)((char *)&type + fields[i].offset) = x_restobj(p_entry);
		}
	}

	return x_type_struct_make(p_base, type);
}

/* make-type: (make-type name handlers-alist) -> create and register runtime type */
static x_obj_t *x_prim_make_type(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_name_str, *p_handlers;
	x_char_t *name;
	x_obj_t *p_name_atom, *p_type;

	x_eargs(p_base, p_args, 3, NULL, &p_name_str, &p_handlers);
	name = x_lib_strndup(x_strval(p_name_str),
		x_lib_strlen(x_strval(p_name_str)));
	p_name_atom = x_obj_make(p_base, x_type_atom_obj,
		X_OBJ_FLAG_OWN, X_OBJ_LENGTH_ATOM, name);

	p_type = x_prim_type_build_struct(p_base, p_name_atom, p_handlers);
	x_base_type_alist_extend(p_base, p_type);

	return p_name_atom;
}

/* base-make-type: (base-make-type base name handlers) -> register type on target base */
static x_obj_t *x_prim_base_make_type(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_target, *p_name_str, *p_handlers;
	x_char_t *name;
	x_obj_t *p_name_atom, *p_type;

	x_eargs(p_base, p_args, 4, NULL, &p_target, &p_name_str, &p_handlers);
	name = x_lib_strndup(x_strval(p_name_str),
		x_lib_strlen(x_strval(p_name_str)));
	p_name_atom = x_obj_make(p_base, x_type_atom_obj,
		X_OBJ_FLAG_OWN, X_OBJ_LENGTH_ATOM, name);

	/* Build type using calling base; register on target. */
	p_type = x_prim_type_build_struct(p_base, p_name_atom, p_handlers);
	x_base_type_alist_extend(p_target, p_type);

	/* Mark target base and its tree with SHARED so calling base's GC
	 * won't sweep handler closures referenced cross-base. */
	x_obj_flags(p_target) |= X_OBJ_FLAG_SHARED;
	x_heap_tree_mark(p_base, x_atomobj(p_target), X_OBJ_FLAG_SHARED);

	return p_name_atom;
}

/* make-instance: (make-instance type-handle data) -> create typed instance */
static x_obj_t *x_prim_make_instance(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_handle, *p_data;
	x_spair_t lookup_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL })
	};
	x_obj_t *p_type;

	x_eargs(p_base, p_args, 3, NULL, &p_handle, &p_data);
	x_firstobj((x_obj_t *)lookup_args) = p_handle;
	p_type = x_base_type_alist_assoc(p_base, (x_obj_t *)lookup_args);

	if (x_obj_isnil(p_base, p_type)) {
		return NULL;
	}

	return x_obj_make(p_base, p_type, 0, X_OBJ_LENGTH_PAIR, p_data, NULL);
}

/* type?: (type? obj type-handle) -> t if obj's type matches handle */
static x_obj_t *x_prim_typep(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj, *p_handle;

	x_eargs(p_base, p_args, 3, NULL, &p_obj, &p_handle);

	if (x_obj_isnil(p_base, p_obj) || x_obj_isnil(p_base, x_obj_type(p_obj))) {
		return x_firstobj(x_base_field_false(p_base));
	}

	return x_type_field_name(x_obj_type(p_obj)) == p_handle
		? x_firstobj(x_base_field_true(p_base)) : x_firstobj(x_base_field_false(p_base));
}

/* type-of: (type-of obj) -> type handle (name atom) for obj's type */
static x_obj_t *x_prim_type_of(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj;
	x_spair_t name_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL })
	};

	x_eargs(p_base, p_args, 2, NULL, &p_obj);
	x_firstobj((x_obj_t *)name_args) = p_obj;
	x_restobj((x_obj_t *)name_args) = p_base;

	return x_type_prim_type_name(p_base, (x_obj_t *)name_args);
}

/* type-name: (type-name obj) -> name string of obj's type */
static x_obj_t *x_prim_type_name(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj, *p_name;

	x_eargs(p_base, p_args, 2, NULL, &p_obj);

	if (x_obj_isnil(p_base, p_obj) || x_obj_isnil(p_base, x_obj_type(p_obj))) {
		return NULL;
	}

	p_name = x_type_field_name(x_obj_type(p_obj));

	if (x_obj_isnil(p_base, p_name)) {
		return NULL;
	}

	return x_mkstr(p_base, x_atomstr(p_name));
}

/* make-token-base: (make-token-base) -> bare base for tokenization */
static x_obj_t *x_prim_make_token_base(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_new = x_base_ts_make(NULL, NULL);
	(void)p_args;

	/* Inherit boolean singletons from calling base. */
	x_base_field_true(p_new) = x_firstobj(x_base_field_true(p_base));
	x_base_field_false(p_new) = x_firstobj(x_base_field_false(p_base));

	return p_new;
}

/* make-base: (make-base) -> create fresh sandboxed interpreter */
static x_obj_t *x_prim_make_base(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_new_base, *p_buffer;
	x_char_t *buffer;
	(void)p_args;

	buffer = (x_char_t *)x_sys_malloc(256);
	p_new_base = x_base_ts_make(NULL, NULL);

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
	x_base_field_buffer(p_new_base) = x_mkspair(p_new_base, X_OBJ_FLAG_NONE,
		p_buffer, x_base_field_buffer(p_new_base));

	/* Register primitives. */
	x_prim_register(p_new_base, p_new_base);

	return p_new_base;
}

/* base-eval: (base-eval base expr) -> eval expr in target base */
static x_obj_t *x_prim_base_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	jmp_buf jmp;
	x_obj_t *p_target, *p_expr;
	x_obj_t *p_handler, *p_result;

	x_eargs(p_base, p_args, 3, NULL, &p_target, &p_expr);

	/* Build handler pair tree: (jmp-ptr saved-env error-value) */
	p_handler = x_mkspair(p_target, X_OBJ_FLAG_NONE,
		x_mkptr(p_target, &jmp),
		x_mkspair(p_target, X_OBJ_FLAG_NONE,
			x_firstobj(x_base_field_env_alist(p_target)),
			x_mkspair(p_target, X_OBJ_FLAG_NONE, NULL, NULL)));

	/* Push handler onto error_handler_stack */
	x_base_field_error_handler(p_target) = x_mkspair(p_target, X_OBJ_FLAG_NONE,
		p_handler, x_base_field_error_handler(p_target));

	if (setjmp(jmp) == 0) {
		p_result = x_eval_arg(p_target, p_expr);
	} else {
		x_obj_t *p_err = x_error_handler_error(p_handler);

		/* Error caught from target: pop handler, restore env, re-signal. */
		x_base_field_error_handler(p_target)
			= x_restobj(x_base_field_error_handler(p_target));
		x_firstobj(x_base_field_env_alist(p_target))
			= x_error_handler_saved_env(p_handler);

		if ( ! x_obj_isnil(p_base, x_firstobj(x_base_field_error_handler(p_base)))) {
			x_obj_t *p_parent = x_firstobj(x_base_field_error_handler(p_base));

			x_error_handler_error(p_parent) = p_err;
			x_firstobj(x_base_field_env_alist(p_base))
				= x_error_handler_saved_env(p_parent);
			longjmp(*(jmp_buf *)x_error_handler_jmp(p_parent), 1);
		}

		x_obj_error(p_base, "error", p_err);

		return NULL;
	}

	/* Pop error_handler_stack */
	x_base_field_error_handler(p_target)
		= x_restobj(x_base_field_error_handler(p_target));

	return p_result;
}

/* base-bind: (base-bind base name value) -> bind in target base */
static x_obj_t *x_prim_base_bind(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_target, *p_name, *p_val;
	x_obj_t *p_pair;

	x_eargs(p_base, p_args, 4, NULL, &p_target, &p_name, &p_val);

	p_pair = x_mkspair(p_target, X_OBJ_FLAG_NONE, p_name, p_val);
	x_base_env_alist_extend(p_target, p_pair);

	return p_val;
}

/* buffer-token: (buffer-token buffer) -> extract consumed portion as string */
static x_obj_t *x_prim_buffer_token(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer;
	x_int_t len;
	x_char_t *str;

	x_eargs(p_base, p_args, 2, NULL, &p_buffer);
	len = x_bufferlen(p_buffer);
	str = x_lib_strndup(x_bufferval(p_buffer), len);

	return x_mkstrown(p_base, str);
}

/* token-read-string: (token-read-string token-base string) -> list of tokens */
static x_obj_t *x_prim_token_read_string(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_token_base, *p_str;
	x_char_t *str;
	x_int_t len;
	x_char_t *buf;
	x_obj_t *p_buffer, *p_token, *p_result, *p_tail, *p_node;

	x_eargs(p_base, p_args, 3, NULL, &p_token_base, &p_str);
	str = x_strval(p_str);
	len = x_lib_strlen(str);
	buf = (x_char_t *)x_sys_malloc(len + 1);

	x_lib_memcpy(buf, str, len);
	buf[len] = '\0';

	p_buffer = x_mkfbufferown(p_token_base, X_OBJ_FLAG_RO, buf);
	x_bufferwrite(p_buffer) = x_bufferval(p_buffer) + len;

	if (x_atomint(x_firstobj(x_base_field_obj_meta_extra(p_base))) > 0
			&& (x_obj_flags(p_buffer) & X_OBJ_FLAG_META)) {
		x_obj_meta_i(p_buffer, 0).i = 1;
	}

	p_result = NULL;
	p_tail = NULL;

	{
		x_spair_t read_args[1] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_buffer }, { p_token_base })
		};

		for (;;) {
			p_token = x_token_read(p_token_base, (x_obj_t *)read_args);

			if (x_obj_isnil(p_token_base, p_token)) {
				break;
			}

			p_node = x_mklist(p_base, p_token, NULL);

			if (x_obj_isnil(p_base, p_result)) {
				p_result = p_node;
			} else {
				x_restobj(p_tail) = p_node;
			}

			p_tail = p_node;
		}
	}

	return p_result;
}

/* make-obj: (make-obj type-handle n) -> allocate typed object with n slots */
static x_obj_t *x_prim_make_obj(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_handle, *p_n;
	x_spair_t lookup_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL })
	};
	x_obj_t *p_type;
	x_int_t n, i;
	x_obj_t *p_obj;

	x_eargs(p_base, p_args, 3, NULL, &p_handle, &p_n);
	x_firstobj((x_obj_t *)lookup_args) = p_handle;
	p_type = x_base_type_alist_assoc(p_base, (x_obj_t *)lookup_args);

	if (x_obj_isnil(p_base, p_type)) {
		return NULL;
	}

	n = x_intval(p_n);
	p_obj = x_obj_alloc(p_base, p_type, 0, (size_t)n);

	for (i = 0; i < n; i++) {
		(&x_firstobj(p_obj))[i] = NULL;
	}

	return p_obj;
}

/* obj-ref: (obj-ref obj i) -> value at slot i */
static x_obj_t *x_prim_obj_ref(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj, *p_i;

	x_eargs(p_base, p_args, 3, NULL, &p_obj, &p_i);

	return (&x_firstobj(p_obj))[x_intval(p_i)];
}

/* obj-set!: (obj-set! obj i val) -> val */
static x_obj_t *x_prim_obj_set(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj, *p_i, *p_val;

	x_eargs(p_base, p_args, 4, NULL, &p_obj, &p_i, &p_val);

	(&x_firstobj(p_obj))[x_intval(p_i)] = p_val;

	return p_val;
}

/* iter: (iter obj) -> call type's iter handler to get an iterator */
static x_obj_t *x_prim_iter(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj;
	x_obj_t *p_type;
	x_obj_t *p_iter_fn;

	x_eargs(p_base, p_args, 2, NULL, &p_obj);
	p_type = x_obj_type(p_obj);

	if (x_obj_isnil(p_base, p_type) || ! x_obj_type_isspair(p_type)) {
		return NULL;
	}

	p_iter_fn = x_type_field_iter(p_type);
	if (x_obj_isnil(p_base, p_iter_fn)) {
		return NULL;
	}

	/* Call the iter handler as (iter-fn obj) */
	{
		x_spair_t iter_call[1] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_iter_fn }, { NULL })
		};
		x_restobj((x_obj_t *)iter_call) = x_mkspair(p_base, X_OBJ_FLAG_NONE, p_obj, NULL);
		return x_callable_call(p_base, (x_obj_t *)iter_call);
	}
}

x_obj_t *x_prim_type_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_callable_entry_t entries[] = {
		{ "make-type", x_prim_make_type },
		{ "base-make-type", x_prim_base_make_type },
		{ "make-instance", x_prim_make_instance },
		{ "make-obj", x_prim_make_obj },
		{ "obj-ref", x_prim_obj_ref },
		{ "obj-set!", x_prim_obj_set },
		{ "type?", x_prim_typep },
		{ "type-of", x_prim_type_of },
		{ "type-name", x_prim_type_name },
		{ "buffer-token", x_prim_buffer_token },
		{ "make-token-base", x_prim_make_token_base },
		{ "make-base", x_prim_make_base },
		{ "base-eval", x_prim_base_eval },
		{ "base-bind", x_prim_base_bind },
		{ "token-read-string", x_prim_token_read_string },
		{ "iter", x_prim_iter }
	};

	x_callable_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
