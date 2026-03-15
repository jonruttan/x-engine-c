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
			{ "to",      offsetof(struct x_type_t, p_to) }
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
	x_obj_t *p_name_str = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_handlers = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));
	x_char_t *name = x_lib_strndup(x_strval(p_name_str),
		x_lib_strlen(x_strval(p_name_str)));
	x_obj_t *p_name_atom = x_obj_make(p_base, x_type_atom_obj,
		X_OBJ_FLAG_OWN, X_OBJ_LENGTH_ATOM, name),
		*p_type;

	p_type = x_prim_type_build_struct(p_base, p_name_atom, p_handlers);
	x_base_type_alist_extend(p_base, p_type);

	return p_name_atom;
}

/* base-make-type: (base-make-type base name handlers) -> register type on target base
 *
 * Evaluates args on the calling base (where closures are valid),
 * builds the type struct, and registers it on the target base.
 *
 * The type struct is allocated on the calling base's heap because:
 *   1. Symbol lookups for handler names need the calling base's types.
 *   2. make-token-base targets may lack symbol creation infrastructure.
 *   3. GC safety: the type struct is reachable from the calling base
 *      via: env -> target base binding -> type alist -> type struct.
 * Constraint: the calling base must retain a reference to the target
 * base for the lifetime of the registered types.
 *
 * Marks the target base tree with SHARED so the calling base's GC
 * won't sweep handler closures referenced cross-base.
 *
 * Alist navigation from Scheme:
 *   (first (first (first base)))  = type alist (list of entries)
 *   Each entry = (handle . type-struct)
 *   type-struct has 6 elements: name, data, heap, proc, cvt, io
 *   io = (analyse-stack delimit-stack write-stack display-stack error-stack)
 *   Each stack = (current-fn . saved)
 *   Push new fn: (set-first (first analyse-stack) new-fn)
 *
 * type? limitation: pair?/symbol?/atom? use x_type_field_name on
 * static type objects (UB), so they may return false for objects
 * allocated on child bases. Use null?/not-null? checks instead. */
static x_obj_t *x_prim_base_make_type(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_target = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_name_str = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args))),
		*p_handlers = x_prim_eval_arg(p_base,
			x_firstobj(x_restobj(x_restobj(p_args))));
	x_char_t *name = x_lib_strndup(x_strval(p_name_str),
		x_lib_strlen(x_strval(p_name_str)));
	x_obj_t *p_name_atom = x_obj_make(p_base, x_type_atom_obj,
		X_OBJ_FLAG_OWN, X_OBJ_LENGTH_ATOM, name),
		*p_type;

	/* Build type using calling base; register on target. */
	p_type = x_prim_type_build_struct(p_base, p_name_atom, p_handlers);
	x_base_type_alist_extend(p_target, p_type);

	/* Mark target base and its tree (including handler closures on calling
	 * base's heap) with INUSE so they survive the calling base's GC sweep. */
	x_obj_flags(p_target) |= X_OBJ_FLAG_SHARED;
	x_heap_mark(p_base, x_atomobj(p_target), X_OBJ_FLAG_SHARED,
		x_type_heap_mark);

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
		return NULL;
	}

	return x_obj_make(p_base, p_type, 0, X_OBJ_LENGTH_PAIR, p_data, NULL);
}

/* type?: (type? obj type-handle) -> t if obj's type matches handle */
static x_obj_t *x_prim_typep(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_handle = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));

	if (x_obj_isnil(p_base, p_obj) || x_obj_isnil(p_base, x_obj_type(p_obj))) {
		return x_base_field_false(p_base);
	}

	return x_type_field_name(x_obj_type(p_obj)) == p_handle
		? x_base_field_true(p_base) : x_base_field_false(p_base);
}

/* type-of: (type-of obj) -> type handle (name atom) for obj's type */
static x_obj_t *x_prim_type_of(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_prim_eval_arg(p_base, x_firstobj(p_args));
	x_spair_t name_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_obj }, { p_base })
	};

	return x_type_prim_type_name(p_base, (x_obj_t *)name_args);
}

/* type-name: (type-name obj) -> name string of obj's type */
static x_obj_t *x_prim_type_name(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_name;

	if (x_obj_isnil(p_base, p_obj) || x_obj_isnil(p_base, x_obj_type(p_obj))) {
		return NULL;
	}

	p_name = x_type_field_name(x_obj_type(p_obj));

	if (x_obj_isnil(p_base, p_name)) {
		return NULL;
	}

	return x_mkstr(p_base, x_atomstr(p_name));
}

/* make-token-base: (make-token-base) -> bare base for tokenization (no sexp types)
 *
 * Uses NULL parent so the base is off-heap, like make-base.
 * Copies the true symbol from the calling base for correct
 * boolean semantics in handler closures. */
static x_obj_t *x_prim_make_token_base(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_new = x_base_make(NULL, NULL);

	/* Inherit boolean singletons from calling base. */
	x_base_field_true(p_new) = x_base_field_true(p_base);
	x_base_field_false(p_new) = x_base_field_false(p_base);

	return p_new;
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
	x_base_field_buffer_stack(p_new_base) = x_mkspair(p_new_base,
		p_buffer, x_base_field_buffer_stack(p_new_base));

	/* Register primitives. */
	x_prim_register(p_new_base, p_new_base);

	return p_new_base;
}

/* base-eval: (base-eval base expr) -> eval expr in target base */
static x_obj_t *x_prim_base_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	jmp_buf jmp;
	x_obj_t *p_target = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_expr = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args))),
		*p_handler, *p_result;

	/* Build handler pair tree: (jmp-ptr saved-env error-value) */
	p_handler = x_mkspair(p_target,
		x_mkptr(p_target, &jmp),
		x_mkspair(p_target,
			x_base_field_env_alist(p_target),
			x_mkspair(p_target, NULL, NULL)));

	/* Push handler onto error_handler_stack */
	x_base_field_error_handler_stack(p_target) = x_mkspair(p_target,
		p_handler, x_base_field_error_handler_stack(p_target));

	if (setjmp(jmp) == 0) {
		p_result = x_prim_eval_arg(p_target, p_expr);
	} else {
		x_obj_t *p_err = x_error_handler_error(p_handler);

		/* Error caught from target: pop handler, restore env, re-signal in parent. */
		x_base_field_error_handler_stack(p_target)
			= x_restobj(x_base_field_error_handler_stack(p_target));
		x_base_field_env_alist(p_target)
			= x_error_handler_saved_env(p_handler);

		if ( ! x_obj_isnil(p_base, x_base_field_error_handler(p_base))) {
			x_obj_t *p_parent = x_base_field_error_handler(p_base);

			x_error_handler_error(p_parent) = p_err;
			x_base_field_env_alist(p_base)
				= x_error_handler_saved_env(p_parent);
			longjmp(*(jmp_buf *)x_error_handler_jmp(p_parent), 1);
		}

		x_obj_error(p_base, "error", p_err);

		return NULL;
	}

	/* Pop error_handler_stack */
	x_base_field_error_handler_stack(p_target)
		= x_restobj(x_base_field_error_handler_stack(p_target));

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

	p_pair = x_mkspair(p_target, p_name, p_val);

	x_base_env_alist_extend(p_target, p_pair);

	return p_val;
}

/* convert_alist_find: walk alist for matching key (pointer equality) */
static x_obj_t *x_convert_alist_find(x_obj_t *p_base,
	x_obj_t *p_alist, x_obj_t *p_key)
{
	while ( ! x_obj_isnil(p_base, p_alist)) {
		if (x_firstobj(x_firstobj(p_alist)) == p_key) {
			return x_firstobj(p_alist);
		}
		p_alist = x_restobj(p_alist);
	}

	return NULL;
}

/* convert: (convert value target-type-handle) -> converted value or ()
 *
 * Lookup order:
 * 1. Short-circuit: value already target type
 * 2. Exact match: source handle in target's 'from' alist
 * 3. Wildcard: 't' symbol key in target's 'from' alist
 * 4. Outbound: target handle in source's 'to' alist */
static x_obj_t *x_prim_convert(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_val, *p_handle, *p_type, *p_from_alist,
		*p_source_handle, *p_entry, *p_converter;
	x_spair_t lookup_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL })
	};
	x_spair_t call_args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL },
			{ (x_obj_t *)(call_args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL })
	};

	p_val = x_prim_eval_arg(p_base, x_firstobj(p_args));

	if (x_obj_isnil(p_base, p_val)) {
		return NULL;
	}

	p_handle = x_prim_eval_arg(p_base,
		x_firstobj(x_restobj(p_args)));

	/* 1. Short-circuit: already the target type. */
	if ( ! x_obj_isnil(p_base, x_obj_type(p_val))
		&& ! x_obj_isnil(p_base, x_obj_type(p_val))
		&& x_type_field_name(x_obj_type(p_val)) == p_handle) {
		return p_val;
	}

	/* Get source type handle. */
	{
		x_spair_t name_args[1] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE,
				{ p_val }, { p_base })
		};
		p_source_handle = x_type_prim_type_name(p_base,
			(x_obj_t *)name_args);
	}

	/* Look up target type struct. */
	x_firstobj((x_obj_t *)lookup_args) = p_handle;
	p_type = x_base_type_alist_assoc(p_base,
		(x_obj_t *)lookup_args);

	if ( ! x_obj_isnil(p_base, p_type)) {
		p_from_alist = x_type_field_from(p_type);

		if ( ! x_obj_isnil(p_base, p_from_alist)) {
			/* 2. Exact match: source handle in target's from alist. */
			p_entry = NULL;
			if ( ! x_obj_isnil(p_base, p_source_handle)) {
				p_entry = x_convert_alist_find(p_base,
					p_from_alist, p_source_handle);
			}

			/* 3. Wildcard: 't' symbol key. */
			if (x_obj_isnil(p_base, p_entry)) {
				p_entry = x_convert_alist_find(p_base,
					p_from_alist, x_base_field_true(p_base));
			}

			if ( ! x_obj_isnil(p_base, p_entry)) {
				goto call_converter;
			}
		}
	}

	/* 4. Outbound: target handle in source's 'to' alist. */
	if ( ! x_obj_isnil(p_base, p_source_handle)) {
		x_obj_t *p_source_type, *p_to_alist;

		x_firstobj((x_obj_t *)lookup_args) = p_source_handle;
		p_source_type = x_base_type_alist_assoc(p_base,
			(x_obj_t *)lookup_args);

		if ( ! x_obj_isnil(p_base, p_source_type)) {
			p_to_alist = x_type_field_to(p_source_type);

			if ( ! x_obj_isnil(p_base, p_to_alist)) {
				p_entry = x_convert_alist_find(p_base,
					p_to_alist, p_handle);

				if ( ! x_obj_isnil(p_base, p_entry)) {
					goto call_converter;
				}
			}
		}
	}

	return NULL;

call_converter:
	/* Call: (converter-fn value) */
	p_converter = x_restobj(p_entry);
	x_firstobj((x_obj_t *)call_args) = p_converter;
	x_firstobj((x_obj_t *)(call_args + 1)) = p_val;

	return x_type_prim_apply(p_base, (x_obj_t *)call_args);
}

/* buffer-token: (buffer-token buffer) -> extract consumed portion as string */
static x_obj_t *x_prim_buffer_token(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_prim_eval_arg(p_base, x_firstobj(p_args));
	x_int_t len = x_bufferlen(p_buffer);
	x_char_t *str = x_lib_strndup(x_bufferval(p_buffer), len);

	return x_mkstrown(p_base, str);
}

/* token-read-string: (token-read-string token-base string) -> list of tokens
 *
 * Tokenizes a string using the given base for token dispatch.
 * The token-base has shell types registered on its type-alist.
 * Closures from the calling base work because objects with a NULL heap
 * makes any base object recognized as nil across bases, and
 * the token-base has its own TCO state (no interference). */
static x_obj_t *x_prim_token_read_string(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_token_base = x_prim_eval_arg(p_base, x_firstobj(p_args)),
		*p_str = x_prim_eval_arg(p_base, x_firstobj(x_restobj(p_args)));
	x_char_t *str = x_strval(p_str);
	x_int_t len = x_lib_strlen(str);
	x_char_t *buf = (x_char_t *)x_sys_malloc(len + 1);
	x_obj_t *p_buffer, *p_token, *p_result, *p_tail, *p_node;

	/* Readonly buffer: string content only, no sentinel.
	 * EOF auto-scoring in x_token_analyse handles the final
	 * token when the buffer is exhausted. */
	x_lib_memcpy(buf, str, len);
	buf[len] = '\0';

	p_buffer = x_mkfbufferown(p_token_base, X_OBJ_FLAG_RO, buf);
	x_bufferwrite(p_buffer) = x_bufferval(p_buffer) + len;

	/* Initialize line counter in buffer's extra metadata slot. */
	if (x_obj_meta_extra > 0
			&& (x_obj_flags(p_buffer) & X_OBJ_FLAG_EXT)) {
		x_obj_meta_slot(p_buffer, 0).i = 1;
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

			/* Build result list on the calling base. */
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

x_obj_t *x_prim_type_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_prim_entry_t entries[] = {
		{ "make-type", x_prim_make_type },
		{ "base-make-type", x_prim_base_make_type },
		{ "make-instance", x_prim_make_instance },
		{ "type?", x_prim_typep },
		{ "type-of", x_prim_type_of },
		{ "type-name", x_prim_type_name },
		{ "convert", x_prim_convert },
		{ "buffer-token", x_prim_buffer_token },
		{ "make-token-base", x_prim_make_token_base },
		{ "make-base", x_prim_make_base },
		{ "base-eval", x_prim_base_eval },
		{ "base-bind", x_prim_base_bind },
		{ "token-read-string", x_prim_token_read_string }
	};

	x_prim_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
