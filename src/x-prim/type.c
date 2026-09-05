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
 * implement it reflectively over tools/contract/obj-layout.x.
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
x_obj_t *x_prim_type_build_struct(x_obj_t *p_base,
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
		{ "ops",     offsetof(struct x_type_t, p_ops) },
		{ "save",    offsetof(struct x_type_t, p_save) },
		{ "load",    offsetof(struct x_type_t, p_load) }
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
 * @return #t if the type matches, #f otherwise.
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
 * come from live probes at boot, the name walk from tools/contract/base-paths.x's
 * type-rooted entries). */

/**
 * @brief Declare a type's unit shape.
 *
 * x-lang form: @code (type set-shape! type count mask) @endcode
 *
 * Installs the pair form of the @c p_units slot: @p count keeps both of the
 * bare form's meanings (fixed, or negative for dynamic-size), and @p mask
 * gives each unit its kind -- see #X_TYPE_UNIT_REF.
 *
 * The shape must be a STRUCTURAL pair, which is why this is a primitive at
 * all: x builds list-pairs only, and the readers discriminate the two forms
 * of the slot by x_obj_type_isspair() -- one pointer comparison on a path
 * the collector walks per object. A list-pair here would be read as a bare
 * count, and the count would be the pair's first data word.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unevaluated: (self type count mask).
 * @return The type struct, or nil if it was nil.
 */
static x_obj_t *x_prim_type_set_shape(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type, *p_count, *p_mask;

	x_eargs(p_base, p_args, 4, NULL, &p_type, &p_count, &p_mask);

	if (x_obj_isnil(p_base, p_type)) {
		return NULL;
	}

	x_firstobj(x_type_field_units_stack(p_type))
		= x_mkspair(p_base, X_OBJ_FLAG_NONE, p_count, p_mask);

	return p_type;
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
x_obj_t *x_prim_op1(x_obj_t *p_base, x_obj_t *p_args,
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
 * ((obj retag!) is pure x-lang too: boot/reflect.x writes the type header
 * slot over the layout contract -- retired from C by the #101 ruling.)
 *
 * @param p_base  Base (execution context) to bind primitives into.
 * @param p_args  Unused.
 * @return @p p_base.
 */
x_obj_t *x_prim_type_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_prim_entry_t entries[] = {
		{ "make-type",         x_prim_make_type,         "type",   "make"          },
		{ "make-instance",     x_prim_make_instance,     "type",   "make-instance" },
		{ "make-obj",          x_prim_make_obj,          "obj",    "make"          },
		{ "type-set-shape!",   x_prim_type_set_shape,    "type",   "set-shape!"    },
		{ "type?",             x_prim_typep,             "type",   "?"             },
		{ "type-of",           x_prim_type_of,           "type",   "of"            },
	};

	x_prims_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));

	return p_base;
}
