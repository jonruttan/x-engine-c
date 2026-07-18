/**
 * @file type.c
 * @brief Type system and sandboxing primitives for x-lang.
 *
 * Provides runtime type creation (make-type), type introspection (type-of,
 * type?, type-name), object allocation (make-obj, make-instance),
 * sandboxed interpreter creation (make-base), cross-base evaluation
 * (base-eval, base-bind), tokenization helpers (make-token-base,
 * token-read-string, buffer-token), and iteration (iter).  Slot access
 * (obj ref / obj set!) is pure x-lang now: boot/data.x + boot/reflect.x
 * implement it reflectively over tools/obj-layout.x.
 *
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2026 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
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
#include "x-prim.h"
#include "x-alist.h"
#include "x-eval.h"
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
#include "x-type/iter.h"
#include "x-type/str.h"
#include "x-type/symbol.h"
#include "x-type/whitespace.h"

/**
 * @brief Build a type struct from a handlers alist.
 *
 * Iterates a table of known handler field names (call, eval, write, display,
 * length, analyse, delimit, read, error, from, to, units, free, mark,
 * iter), looks each up in @p p_handlers via alist association,
 * and populates the corresponding x_type_t slot.
 *
 * @param p_base  Base (execution context) used for symbol lookup and allocation.
 * @param p_name_atom  Atom for the type name.
 * @param p_handlers   Alist mapping handler name symbols to closures.
 * @return Heap-allocated type struct object.
 */
static x_obj_t *x_prim_type_build_struct(x_obj_t *p_base,
	x_obj_t *p_name_atom, x_obj_t *p_handlers)
{
	struct x_type_t type = { 0 };
	x_obj_t *p_sym, *p_entry;
	x_spair_t assoc_args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { (x_obj_t *)(assoc_args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_handlers }, { NULL })
	};

	static const struct { x_char_t *name; size_t offset; } fields[] = {
		{ "call",    offsetof(struct x_type_t, p_call) },
		{ "eval",    offsetof(struct x_type_t, p_eval) },
		{ "write",   offsetof(struct x_type_t, p_write) },
		{ "display", offsetof(struct x_type_t, p_display) },
		{ "length",  offsetof(struct x_type_t, p_length) },
		{ "analyse", offsetof(struct x_type_t, p_analyse) },
		{ "delimit", offsetof(struct x_type_t, p_delimit) },
		{ "read",    offsetof(struct x_type_t, p_read) },
		{ "from",    offsetof(struct x_type_t, p_from) },
		{ "to",      offsetof(struct x_type_t, p_to) },
		{ "units",   offsetof(struct x_type_t, p_units) },
		{ "free",    offsetof(struct x_type_t, p_free) },
		{ "mark",    offsetof(struct x_type_t, p_mark) },
		{ "iter",    offsetof(struct x_type_t, p_iter) },
		{ "ops",     offsetof(struct x_type_t, p_ops) }
	};
	int i;

	type.p_name = p_name_atom;
	type.p_units = (x_obj_t *)&x_type_units_pair_obj;

	/* Look up handler closures from the alist. */
	for (i = 0; i < (int)(sizeof(fields) / sizeof(fields[0])); i++) {
		p_sym = x_mksymbol(p_base, fields[i].name);
		x_firstobj((x_obj_t *)assoc_args) = p_sym;
		p_entry = x_alist_assoc(p_base, (x_obj_t *)assoc_args);
		if ( ! x_obj_isnil(p_base, p_entry))
			*(x_obj_t **)((char *)&type + fields[i].offset) = x_restobj(p_entry);
	}

	return x_type_struct_make(p_base, type);
}

/**
 * @brief Create and register a runtime type.
 *
 * x-lang form: @code (make-type name handlers-alist) @endcode
 *
 * Duplicates the name string into an owned atom, builds the type struct
 * from the handlers alist, and prepends it to the base's type alist.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self name-string handlers-alist).
 * @return The type name atom (handle for type? / make-instance lookups).
 * @see x_prim_type_build_struct
 */
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
	x_eval_type_alist_extend(p_base, p_type);

	return p_name_atom;
}

/**
 * @brief Create a type on a target base (cross-base type registration).
 *
 * x-lang form: @code (base-make-type base name handlers) @endcode
 *
 * Like make-type, but registers the type on @p p_target rather than the
 * calling base. Marks the target base tree as SHARED so the calling
 * base's GC will not sweep handler closures referenced across bases.
 *
 * @param p_base  Calling execution context (used for handler closure allocation).
 * @param p_args  Unevaluated: (self target-base name-string handlers-alist).
 * @return The type name atom.
 * @note Sets X_OBJ_FLAG_SHARED on the target base to prevent cross-base GC.
 * @see x_prim_make_type
 */
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
	x_eval_type_alist_extend(p_target, p_type);

	/* Mark target base and its tree with SHARED so calling base's GC
	 * won't sweep handler closures referenced cross-base. */
	x_obj_flags(p_target) |= X_OBJ_FLAG_SHARED;
	x_heap_tree_mark(p_base, x_atomobj(p_target), X_OBJ_FLAG_SHARED);

	return p_name_atom;
}

/**
 * @brief Create an instance of a runtime-defined type.
 *
 * x-lang form: @code (make-instance type-handle data) @endcode
 *
 * Looks up the type by its handle atom in the base's type alist and
 * allocates a pair-sized object of that type with @p p_data as its first slot.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self type-handle data).
 * @return New typed instance, or NULL if the type handle is not found.
 */
static x_obj_t *x_prim_make_instance(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_handle, *p_data;
	x_spair_t lookup_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL })
	};
	x_obj_t *p_type;

	x_eargs(p_base, p_args, 3, NULL, &p_handle, &p_data);
	x_firstobj((x_obj_t *)lookup_args) = p_handle;
	p_type = x_eval_type_alist_assoc(p_base, (x_obj_t *)lookup_args);

	if (x_obj_isnil(p_base, p_type)) {
		return NULL;
	}

	return x_obj_make(p_base, p_type, 0, X_OBJ_LENGTH_PAIR, p_data, NULL);
}

/**
 * @brief Test whether an object's type matches a given handle.
 *
 * x-lang form: @code (type? obj type-handle) @endcode
 *
 * Compares the name atom pointer of the object's type against @p p_handle.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self obj type-handle).
 * @return The @c t symbol if the type matches, @c f otherwise.
 */
static x_obj_t *x_prim_typep(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj, *p_handle;

	x_eargs(p_base, p_args, 3, NULL, &p_obj, &p_handle);

	/* Guard the non-pair-tree tags before navigating type fields: a child
	 * base's type is the x_eval_obj sentinel (a static atom tagging the
	 * raw string "BASE"), and x_type_field_name on it reads past the tag
	 * string (ASan global-buffer-overflow). Same rule as x_type_op_try:
	 * only a pair-tree type has fields. No handle can match -> #f. */
	if (x_obj_isnil(p_base, p_obj) || x_obj_isnil(p_base, x_obj_type(p_obj))
			|| ! x_obj_type_isspair(x_obj_type(p_obj))) {
		return x_firstobj(x_eval_field_false(p_base));
	}

	return x_type_field_name(x_obj_type(p_obj)) == p_handle
		? x_firstobj(x_eval_field_true(p_base)) : x_firstobj(x_eval_field_false(p_base));
}

/**
 * @brief Return the type handle (name atom) for an object.
 *
 * x-lang form: @code (type-of obj) @endcode
 *
 * Delegates to the C-level x_type_prim_type_name to retrieve the type's
 * name atom, which serves as the canonical handle for type operations.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self obj).
 * @return Type name atom, or NULL for nil/untyped objects.
 */
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

/* (type name obj-or-handle) is pure x-lang now: boot/reflect.x mirrors the
 * handle/object/nil branches over the layout contracts (the sentinel tags
 * come from live probes at boot, the name walk from tools/base-paths.x's
 * type-rooted entries). */

/**
 * @brief Create a bare base suitable for tokenization only.
 *
 * x-lang form: @code (make-token-base) @endcode
 *
 * Allocates a minimal base with no types or primitives registered,
 * inheriting only the boolean singletons (t/f) from the calling base.
 * Used for custom tokenizer type registration on an isolated base.
 *
 * @param p_base  Base (execution context) (boolean singletons are inherited).
 * @param p_args  Unused.
 * @return New bare base object.
 * @see x_prim_make_base
 */
static x_obj_t *x_prim_make_token_base(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_new = x_eval_make(NULL, NULL);
	(void)p_args;

	/* Inherit boolean singletons from calling base. */
	x_eval_field_true(p_new) = x_firstobj(x_eval_field_true(p_base));
	x_eval_field_false(p_new) = x_firstobj(x_eval_field_false(p_base));

	return p_new;
}

/**
 * @brief Create a fully initialized sandboxed interpreter base.
 *
 * x-lang form: @code (make-base) @endcode
 *
 * Allocates a new base, registers all built-in types (prim, operative,
 * procedure, symbol, list, int, str, char, whitespace, comment), sets up
 * a read buffer, and registers all C primitives. The result is a complete
 * interpreter context that can be evaluated into via base-eval.
 *
 * @param p_base  Base (execution context) (unused beyond allocation).
 * @param p_args  Unused.
 * @return Fully bootstrapped base object.
 * @see x_prim_base_eval
 */
static x_obj_t *x_prim_make_base(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_new_base, *p_buffer;
	x_char_t *buffer;
	(void)p_args;

	buffer = (x_char_t *)x_sys_malloc(256);
	p_new_base = x_eval_make(NULL, NULL);

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

/**
 * @brief Evaluate an expression in a target base's environment.
 *
 * x-lang form: @code (base-eval base expr) @endcode
 *
 * Pushes a setjmp-based error handler onto the target base's error handler
 * stack, evaluates @p p_expr in the target, then pops the handler. If an
 * error occurs in the target, it is caught, the handler is popped, the
 * environment is restored, and the error is re-signaled to the calling
 * base's error handler (or printed if none exists).
 *
 * @param p_base  Calling execution context.
 * @param p_args  Unevaluated: (self target-base expr).
 * @return Result of evaluating @p expr in the target base, or NULL on error.
 * @note Uses setjmp/longjmp for error propagation across bases.
 */
static x_obj_t *x_prim_base_eval(x_obj_t *p_base, x_obj_t *p_args)
{
	jmp_buf jmp;
	x_obj_t *p_target, *p_expr;
	x_obj_t *p_handler, *p_result;
	x_obj_t *p_err, *p_parent;

	x_eargs(p_base, p_args, 3, NULL, &p_target, &p_expr);

	/* Build handler pair tree: (jmp-ptr saved-env error-value) */
	p_handler = x_mkspair(p_target, X_OBJ_FLAG_NONE,
		x_mkptr(p_target, &jmp),
		x_mkspair(p_target, X_OBJ_FLAG_NONE,
			x_firstobj(x_eval_field_env_alist(p_target)),
			x_mkspair(p_target, X_OBJ_FLAG_NONE, NULL, NULL)));

	/* Push handler onto error_handler_stack */
	x_eval_field_error_handler(p_target) = x_mkspair(p_target, X_OBJ_FLAG_NONE,
		p_handler, x_eval_field_error_handler(p_target));

	if (setjmp(jmp) == 0) {
		p_result = x_eval_arg(p_target, p_expr);
	} else {
		p_err = x_error_handler_error(p_handler);

		/* Error caught from target: pop handler, restore env, re-signal. */
		x_eval_field_error_handler(p_target)
			= x_restobj(x_eval_field_error_handler(p_target));
		x_firstobj(x_eval_field_env_alist(p_target))
			= x_error_handler_saved_env(p_handler);

		if ( ! x_obj_isnil(p_base, x_firstobj(x_eval_field_error_handler(p_base)))) {
			p_parent = x_firstobj(x_eval_field_error_handler(p_base));

			x_error_handler_error(p_parent) = p_err;
			x_error_handler_line(p_parent) = x_error_handler_line(p_handler);
			x_firstobj(x_eval_field_env_alist(p_base))
				= x_error_handler_saved_env(p_parent);
			longjmp(*(jmp_buf *)x_error_handler_jmp(p_parent), 1);
		}

		x_obj_error(p_base, "error", p_err);

		return NULL;
	}

	/* Pop error_handler_stack */
	x_eval_field_error_handler(p_target)
		= x_restobj(x_eval_field_error_handler(p_target));

	return p_result;
}

/**
 * @brief Bind a name-value pair in a target base's environment.
 *
 * x-lang form: @code (base-bind base name value) @endcode
 *
 * Creates a (name . value) pair and prepends it to the target base's
 * environment alist, making it visible to subsequent evaluations.
 *
 * @param p_base  Calling execution context.
 * @param p_args  Unevaluated: (self target-base name value).
 * @return The bound value.
 */
static x_obj_t *x_prim_base_bind(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_target, *p_name, *p_val;
	x_obj_t *p_pair;

	x_eargs(p_base, p_args, 4, NULL, &p_target, &p_name, &p_val);

	p_pair = x_mkspair(p_target, X_OBJ_FLAG_NONE, p_name, p_val);
	x_eval_env_alist_extend(p_target, p_pair);

	return p_val;
}

/**
 * @brief Extract the consumed portion of a buffer as a string.
 *
 * x-lang form: @code (buffer-token buffer) @endcode
 *
 * Reads the buffer's current length (bytes consumed by the tokenizer)
 * and duplicates that prefix into a new owned string.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self buffer).
 * @return New string containing the consumed buffer content.
 */
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

/**
 * Return the last character consumed by the tokenizer buffer.
 *
 * x-lang form: @code (buffer-last-char buffer) @endcode
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self buffer).
 * @return Integer character code, or NULL if buffer is empty.
 */
static x_obj_t *x_prim_buffer_last_char(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer;

	x_eargs(p_base, p_args, 2, NULL, &p_buffer);

	if (x_bufferlen(p_buffer) == 0) {
		return NULL;
	}

	return x_mkint(p_base, (x_int_t)x_bufferlastchar(p_buffer));
}

/**
 * @brief Tokenize a string using a token base's registered types.
 *
 * x-lang form: @code (token-read-string token-base string) @endcode
 *
 * Copies the input string into a read-only buffer, then repeatedly calls
 * x_token_read against the token base to produce a linked list of token
 * objects. If metadata tracking is active, the buffer is marked with
 * initial line number 1.
 *
 * @param p_base       Calling execution context (tokens allocated here).
 * @param p_args       Unevaluated: (self token-base string).
 * @return Linked list of token objects, or NULL for empty input.
 * @note The token base should have tokenizer types registered via
 *       make-token-base + base-make-type.
 */
static x_obj_t *x_prim_token_read_string(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_token_base, *p_str;
	x_char_t *str;
	x_int_t len;
	x_char_t *buf;
	x_obj_t *p_buffer, *p_token, *p_result, *p_tail, *p_node;
	x_spair_t read_args[1];

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

	read_args[0][X_OBJ_META_TYPE].p = NULL;
	read_args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)read_args) = p_buffer;
	x_restobj((x_obj_t *)read_args) = p_token_base;

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

	return p_result;
}

/**
 * @brief Allocate a typed object with n slots, all initialized to NULL.
 *
 * x-lang form: @code (make-obj type-handle n) @endcode
 *
 * Looks up the type by handle, allocates an object with @p n pointer-sized
 * slots, and zero-fills all slots. Used for vector-like custom types.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self type-handle n).
 * @return New object with @p n NULL slots, or NULL if type not found.
 * @see x_prim_make_type
 */
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
	p_type = x_eval_type_alist_assoc(p_base, (x_obj_t *)lookup_args);

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

/**
 * Read the next expression from a tokenizer buffer.
 *
 * Exposes x_token_read to x-lang so that custom reader hooks (defined
 * via make-type) can recursively read sub-expressions from the stream.
 *
 * x-lang form: @code (token-read buffer) @endcode
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self buffer).
 * @return Parsed expression, or NULL on EOF.
 */
static x_obj_t *x_prim_token_read(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer;
	x_spair_t read_args[1];

	x_eargs(p_base, p_args, 2, NULL, &p_buffer);

	read_args[0][X_OBJ_META_TYPE].p = NULL;
	read_args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)read_args) = p_buffer;
	x_restobj((x_obj_t *)read_args) = p_base;

	x_type_buffer_retain(p_base, (x_obj_t *)read_args);

	return x_token_read(p_base, (x_obj_t *)read_args);
}

/**
 * Evaluate one argument and pass it as a 1-element list to a type operation.
 *
 * The argument list is built on the stack (no heap allocation), so these
 * wrappers stay safe inside reader/tokenizer callbacks.  C primitives receive
 * unevaluated args, hence the explicit x_eargs before delegating.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self obj).
 * @param op      The type operation to drive with the evaluated object.
 * @return Whatever the operation returns.
 */
static x_obj_t *x_prim_op1(x_obj_t *p_base, x_obj_t *p_args,
	x_obj_t *(*op)(x_obj_t *, x_obj_t *))
{
	x_obj_t *p_obj;
	x_spair_t args[1];

	x_eargs(p_base, p_args, 2, NULL, &p_obj);

	args[0][X_OBJ_META_TYPE].p = NULL;
	args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)args) = p_obj;
	x_restobj((x_obj_t *)args) = NULL;

	return op(p_base, (x_obj_t *)args);
}

/** x-lang (make-iter step-fn value): build an iterator -- a boxed generator.
 *  Steps are PURE: an x-lang step-fn is (step state) -> (value . next-state)
 *  or () when exhausted; iter-next owns the box write-back -- value going
 *  nil marks exhaustion (iter-empty?). */
static x_obj_t *x_prim_make_iter(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_fn, *p_val;

	x_eargs(p_base, p_args, 3, NULL, &p_fn, &p_val);

	return x_make_iter(p_base, X_OBJ_FLAG_NONE, p_fn, p_val);
}

/** x-lang (iter-next iter): advance an iterator, returning its next element. */
static x_obj_t *x_prim_iter_next(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_prim_op1(p_base, p_args, x_type_iter_next);
}

/** x-lang (iter-step it): step an iterator FUNCTIONALLY -- returns
 *  (value . next-iterator) with the given iterator untouched, or () when
 *  exhausted.  The generator view of an iterator: Gen pipelines drive C
 *  steps through this door. */
static x_obj_t *x_prim_iter_step(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_prim_op1(p_base, p_args, x_type_iter_step);
}

/** x-lang (iter-empty? iter): #t when the iterator is exhausted, else #f.
 *  Wrapped (not x_type_iter_isempty directly) so it returns a real boolean
 *  rather than the p_base/p_args truthy convention. */
static x_obj_t *x_prim_iter_empty(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_iter;

	x_eargs(p_base, p_args, 2, NULL, &p_iter);

	return x_iterempty(p_base, p_iter)
		? x_firstobj(x_eval_field_true(p_base))
		: x_firstobj(x_eval_field_false(p_base));
}

/** x-lang (buffer-make str [flags]): a BUFFER viewing @p str's bytes.
 *  NON-OWNING (the wrap rule): the buffer's cursors walk the string's own
 *  storage, so the string must outlive the buffer and (str make) is the
 *  natural backing store.  Both cursors start at the base -- append then
 *  read, or fill the string first and walk the read cursor.  Optional
 *  flags (descriptor names, e.g. %obj-flag-ro): a read-only buffer
 *  returns EOF at exhaustion instead of extending from stdin. */
static x_obj_t *x_prim_buffer_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_str, *p_rest;
	x_obj_flag_t flags = 0;

	x_eargs(p_base, p_args, 2, NULL, &p_str);
	p_rest = x_11(p_args);
	if ( ! x_obj_isnil(p_base, p_rest))
		flags = (x_obj_flag_t)x_intval(
			x_eval_arg(p_base, x_firstobj(p_rest)));

	return x_make_buffer(p_base, flags, x_strval(p_str));
}

/** x-lang (buffer-reset buffer): empty the buffer (cursors back to base). */
static x_obj_t *x_prim_buffer_reset(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_prim_op1(p_base, p_args, x_type_buffer_reset);
}

/** x-lang (buffer-retain buffer): compact unread data to the front. */
static x_obj_t *x_prim_buffer_retain(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_prim_op1(p_base, p_args, x_type_buffer_retain);
}

/** x-lang (buffer-append buffer char): write one char at the write cursor. */
static x_obj_t *x_prim_buffer_append(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer, *p_char;
	x_spair_t args[2];

	x_eargs(p_base, p_args, 3, NULL, &p_buffer, &p_char);

	args[0][X_OBJ_META_TYPE].p = NULL;
	args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)args) = p_buffer;
	x_restobj((x_obj_t *)args) = (x_obj_t *)(args + 1);
	args[1][X_OBJ_META_TYPE].p = NULL;
	args[1][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)(args + 1)) = p_char;
	x_restobj((x_obj_t *)(args + 1)) = NULL;

	return x_type_buffer_append(p_base, (x_obj_t *)args);
}

/** x-lang (buffer-read buffer): read one char (extending from input), or nil. */
static x_obj_t *x_prim_buffer_read(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_prim_op1(p_base, p_args, x_type_buffer_read);
}

/** x-lang (buffer-read-text buffer): like buffer-read, NUL counts as EOF. */
static x_obj_t *x_prim_buffer_read_text(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_prim_op1(p_base, p_args, x_type_buffer_read_text);
}

/**
 * @brief Register all type-system and sandboxing primitives.
 *
 * Binds: make-type, base-make-type, make-instance, make-obj,
 * type?, type-of, buffer-token, make-token-base,
 * make-base, base-eval, base-bind, token-read, token-read-string.
 * ((iter new) is pure x-lang now: boot/reflect.x dispatches the type
 * tree's iter handler over the layout contracts.)
 * (obj-ref / obj-set! / type-name are pure x-lang now: boot/data.x +
 * boot/reflect.x implement them reflectively over the layout contracts.)
 *
 * @param p_base  Base (execution context) to bind primitives into.
 * @param p_args  Unused.
 * @return @p p_base.
 */
x_obj_t *x_prim_type_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_prim_entry_t entries[] = {
		{ "make-type",         x_prim_make_type,         "type",   "make"          },
		{ "base-make-type",    x_prim_base_make_type,    "base",   "make-type"     },
		{ "make-instance",     x_prim_make_instance,     "type",   "make-instance" },
		{ "make-obj",          x_prim_make_obj,          "obj",    "make"          },
		{ "type?",             x_prim_typep,             "type",   "?"             },
		{ "type-of",           x_prim_type_of,           "type",   "of"            },
		{ "buffer-token",      x_prim_buffer_token,      "buf", "tok"         },
		{ "buffer-last-char",  x_prim_buffer_last_char,  "buf", "last-char"     },
		{ "make-token-base",   x_prim_make_token_base,   "base",   "make-tok"      },
		{ "make-base",         x_prim_make_base,         "base",   "make"          },
		{ "base-eval",         x_prim_base_eval,         "base",   "eval"          },
		{ "base-bind",         x_prim_base_bind,         "base",   "bind"          },
		{ "token-read",        x_prim_token_read,        "tok",  "read"          },
		{ "token-read-string", x_prim_token_read_string, "tok",  "read-str"      },
		{ "make-iter",         x_prim_make_iter,         "iter",   "make"          },
		{ "iter-next",         x_prim_iter_next,         "iter",   "next"          },
		{ "iter-step",         x_prim_iter_step,         "iter",   "step"          },
		{ "iter-empty?",       x_prim_iter_empty,        "iter",   "empty?"        },
		{ "buffer-make",       x_prim_buffer_make,       "buf", "make"          },
		{ "buffer-reset",      x_prim_buffer_reset,      "buf", "reset"         },
		{ "buffer-retain",     x_prim_buffer_retain,     "buf", "retain"        },
		{ "buffer-append",     x_prim_buffer_append,     "buf", "append"        },
		{ "buffer-read",       x_prim_buffer_read,       "buf", "read"          },
		{ "buffer-read-text",  x_prim_buffer_read_text,  "buf", "read-text"     }
	};

	x_prims_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
