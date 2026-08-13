#ifndef X_PRIM_H
#define X_PRIM_H

/**
 * @file x-prim.h
 * @brief Primitive function infrastructure and registration.
 *
 * Declares the callable binding mechanism used to register C primitives
 * into the x-lang environment, argument unpacking helpers, body/TCO
 * evaluation entry points, shadow-list management, and the per-module
 * register functions.
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

#include "x-obj.h"
#include "x-heap.h"
#include "x-eval.h"	/* x_eval_arg: the x_eargs helper below evaluates args */
#include <stdarg.h>


/**
 * @defgroup arg_helpers Argument Unpacking Helpers
 * @brief Variadic helpers for extracting arguments from x-lang arg lists.
 * @{
 */

/**
 * Unpack @p count elements from an args list into output pointers.
 *
 * NULL pointers skip that position (like @c _ in pattern matching).
 * @code
 *   x_args(p_args, 3, NULL, &a, &b);  // skip self, extract 2
 * @endcode
 *
 * @param p_args  Argument list (pair chain).
 * @param count   Number of positions to unpack.
 * @param ...     Pointers to @c x_obj_t* slots (or NULL to skip).
 */
static void __attribute__((unused)) x_args(x_obj_t *p_args, int count, ...)
{
	va_list ap;
	int i;

	va_start(ap, count);
	for (i = 0; i < count; i++) {
		x_obj_t **slot = va_arg(ap, x_obj_t **);
		if (slot != NULL)
			*slot = x_firstobj(p_args);
		p_args = x_restobj(p_args);
	}
	va_end(ap);
}

/**
 * Unpack and evaluate @p count elements from an args list.
 *
 * NULL pointers skip that position without evaluating.
 * @code
 *   x_eargs(p_base, p_args, 3, NULL, &a, &b);  // skip self, eval+extract 2
 * @endcode
 *
 * @param p_base  Base/execution context.
 * @param p_args  Argument list (pair chain).
 * @param count   Number of positions to unpack.
 * @param ...     Pointers to @c x_obj_t* slots (or NULL to skip).
 */
static void __attribute__((unused)) x_eargs(x_obj_t *p_base, x_obj_t *p_args, int count, ...)
{
	va_list ap;
	int i;
	int held = 0;
	x_obj_t **p_cell = x_heap_root_slot(p_base);
	/* Earlier results may be fresh objects whose only other homes are the
	 * caller's out-slots -- bare C stack the collector does not scan under
	 * precise rooting -- so each result is parked in a registered slot
	 * while the later arguments evaluate.  Two pair cells give four slots:
	 * enough for the deepest x_eargs caller (count 5 = four results, of
	 * which the last needs no protection here). */
	x_spair_t roots[2] = {
		x_obj_set((x_obj_t *)x_type_pair_obj, X_OBJ_FLAG_NONE,
			{ NULL }, { NULL }),
		x_obj_set((x_obj_t *)x_type_pair_obj, X_OBJ_FLAG_NONE,
			{ NULL }, { NULL })
	};

	x_heap_root_push(p_cell, roots[0]);
	x_heap_root_push(p_cell, roots[1]);

	va_start(ap, count);
	for (i = 0; i < count; i++) {
		x_obj_t **slot = va_arg(ap, x_obj_t **);
		if (p_args == NULL) { if (slot) *slot = NULL; continue; }
		if (slot != NULL) {
			*slot = x_eval_arg(p_base, x_firstobj(p_args));
			if (held < 4) {
				x_obj_data_i((x_obj_t *)roots[held >> 1], held & 1).p = *slot;
				held++;
			}
		}
		p_args = x_restobj(p_args);
	}
	va_end(ap);

	x_heap_root_pop(p_cell);
	x_heap_root_pop(p_cell);
}

/** @} */ /* end arg_helpers */


/**
 * @defgroup callable_bind Callable Binding
 * @brief Bind C functions into the x-lang environment.
 * @{
 */

/** Name/function pair for bulk registration via x_callable_bind_table(). */
typedef struct {
	x_char_t *name;                        /**< Symbol name to bind. */
	x_fn_t fn;                             /**< C primitive function pointer. */
} x_callable_entry_t;

/** Bind a named symbol to an arbitrary value in the global environment. */
void x_value_bind(x_obj_t *p_base, x_char_t *name, x_obj_t *p_val);

/** Bind a single C function as a named callable in the environment. */
void x_callable_bind(x_obj_t *p_base, x_char_t *name, x_fn_t fn);

/** Bind an array of name/function entries into the environment. */
void x_callable_bind_table(x_obj_t *p_base, const x_callable_entry_t *table, int count);

/** @} */

/**
 * @defgroup prims_catalog Primitives Catalog
 * @brief The type-keyed primitive registry stored in the base's prims slot.
 *
 * The catalog is an alist-of-alists @c ((type . ((method . prim) ...)) ...)
 * keyed by type/section namespace, with bare method names.  It is the
 * transitional home for primitives ahead of mapping them onto the type
 * objects as static methods.  Namespace and method names are interned, so
 * the find accessors compare by pointer identity.
 * @{
 */

/** The primitives catalog (the prims-slot value); nil before registration.
 *  Catalog LOOKUP is pure x-lang (boot/registry.x over tools/contract/base-paths.x);
 *  C only files entries, via x_prims_bind_table below. */
x_obj_t *x_prims(x_obj_t *p_base);

/** A primitive's env name + catalog coordinates, for x_prims_bind_table().
 *  @c ns NULL => bound into the env only (not cataloged).  A module adopts the
 *  catalog by switching its table to this type; unconverted modules keep using
 *  x_callable_entry_t untouched. */
typedef struct {
	x_char_t *name;                        /**< Env symbol name (transitional). */
	x_fn_t fn;                             /**< C primitive function pointer. */
	x_char_t *ns;                          /**< Catalog namespace (type/section), or NULL. */
	x_char_t *method;                      /**< Catalog bare method name. */
} x_prim_entry_t;

/** Bind a table into the env AND file its cataloged entries (those with ns).
 *  The env binding is transitional -- de-registration drops it, leaving the
 *  catalog as the single source. */
void x_prims_bind_table(x_obj_t *p_base, const x_prim_entry_t *table, int count);

/** @} */

/** @name Module Registration Functions
 * @{ */
/** Register core primitives (pair, first, rest, apply, eval, eval!,
 *  tail-eval, wrap, unwrap, atomic, %base). */
x_obj_t *x_prim_core_register(x_obj_t *p_base, x_obj_t *p_args);

/** Register arithmetic primitives (+, -, *, /, %, ~, &, |, ^, <<, >>). */
x_obj_t *x_prim_arith_register(x_obj_t *p_base, x_obj_t *p_args);

/** Register predicate primitives (same?, eq?, =, <, char->integer,
 *  integer->char). */
x_obj_t *x_prim_pred_register(x_obj_t *p_base, x_obj_t *p_args);

/** Register string primitives (str-append, make-str, str->symbol,
 *  symbol->str, bytes->str, str-byte-len/-ref/-sub). */
x_obj_t *x_prim_string_register(x_obj_t *p_base, x_obj_t *p_args);

/** Register I/O and heap primitives (write-str, read, read-char, clock,
 *  repl-read, heap-collect/-mark/-sweep/-count, alloc-limit!,
 *  heap-*-hook!, gc-pin!). */
x_obj_t *x_prim_io_register(x_obj_t *p_base, x_obj_t *p_args);

/** Minimal C read-eval loop (no output, no hooks). */
x_obj_t *x_prim_repl(x_obj_t *p_base, x_obj_t *p_args);

/** Register the type/base/buffer/token/iter primitives (make-type,
 *  make-instance, type?, type-of, make-base, base-eval, token-read,
 *  buffer-*, make-iter, iter-*). */
x_obj_t *x_prim_type_register(x_obj_t *p_base, x_obj_t *p_args);

/** Register FFI primitives (dlopen, dlsym, ffi-call, ptr-call,
 *  int->ptr/ptr->int, mem-*, ptr-*). */
x_obj_t *x_prim_ffi_register(x_obj_t *p_base, x_obj_t *p_args);

/** Register call/cc continuation primitives. */
x_obj_t *x_prim_callcc_register(x_obj_t *p_base, x_obj_t *p_args);

#ifdef X_SIGNAL
/** Register signal handling primitives and %sigint-flag. */
x_obj_t *x_prim_signal_register(x_obj_t *p_base, x_obj_t *p_args);
#endif

/** Initialize the call/cc subsystem. */
void x_callcc_init(void);
/** @} */

/** @name Shadow List Management
 * @{ */
/** Clear all shadow-list entries from the environment. */
void x_prim_clear_shadows(x_obj_t *p_base);

/** Clear shadow-list entries back to a saved checkpoint. */
void x_prim_clear_shadows_to(x_obj_t *p_base, x_obj_t *p_old);
/** @} */

/** Register all primitive modules into the base environment. */
x_obj_t *x_prim_register(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_PRIM_H */
