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
 * @copyright 2024 Jon Ruttan
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
#include <stdarg.h>

/** Evaluate a single argument expression. */
x_obj_t *x_eval_arg(x_obj_t *p_base, x_obj_t *p_arg);

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
	x_obj_t **p_cell = x_heap_root_cell(p_base);
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

/** @name Evaluation Entry Points
 * @{ */

/** Evaluate an argument list, returning a list of results. */
x_obj_t *x_eval_list(x_obj_t *p_base, x_obj_t *p_args);

/** Extend an environment by binding params to vals. */
x_obj_t *x_env_extend(x_obj_t *p_base, x_obj_t *p_env,
	x_obj_t *p_params, x_obj_t *p_vals);

/** Evaluate a body (sequence of expressions), returning the last result. */
x_obj_t *x_eval_body(x_obj_t *p_base, x_obj_t *p_body);

/** Evaluate a body with TCO, setting up a trampoline for the tail call. */
x_obj_t *x_eval_body_tco(x_obj_t *p_base, x_obj_t *p_body);

/** Simplified TCO body evaluation for non-wrapping contexts. */
x_obj_t *x_eval_body_tco_simple(x_obj_t *p_base, x_obj_t *p_body);

/** Execute the TCO trampoline loop until a non-TCO result is produced. */
x_obj_t *x_eval_tco_trampoline(x_obj_t *p_base, x_obj_t *p_result);

/** Push the current env state as a TCO restore compound onto the save-stack.
 *  The compound shape is @c ((env-alist . local-boundary) . (global-bst .
 *  shadow-head)); returns the pushed compound.  Used by procedure calls and
 *  eval-with-env to snapshot the environment before extending it. */
x_obj_t *x_tco_compound_save(x_obj_t *p_base);

/** Restore env-alist, local-boundary, global-bst, and shadow list from a TCO
 *  compound previously built by x_tco_compound_save().  Does NOT pop the
 *  save-stack (callers that took the compound from the save-stack pop
 *  separately). */
void x_tco_restore(x_obj_t *p_base, x_obj_t *p_compound);

/** Discriminator atom whose address tags a tco_env value as an operative
 *  restore record (vs a procedure env compound).  See x_eval_op_body. */
extern x_satom_t x_tco_op_tag;

/** Restore env-alist, local-boundary, and shadow from an operative record
 *  @c (TAG . ((caller . op_head) . (boundary . shadow))).  Env restore is
 *  conditional on op_head reachability; never touches the BST. */
void x_op_restore(x_obj_t *p_base, x_obj_t *p_record, int force_caller);

/** Defer an operative body's tail to the outer trampoline: evaluate non-tail
 *  forms, then set tco_expr (tail) and a tagged restore record in tco_env. */
x_obj_t *x_eval_op_body(x_obj_t *p_base, x_obj_t *p_body,
	x_obj_t *p_caller, x_obj_t *p_op_head,
	x_obj_t *p_boundary, x_obj_t *p_shadow);

/** @} */

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

/** The primitives catalog (the prims-slot value); nil before registration. */
x_obj_t *x_prims(x_obj_t *p_base);

/** Find a namespace's method alist in the catalog, or NULL if absent. */
x_obj_t *x_prims_domain(x_obj_t *p_base, x_obj_t *p_ns);

/** Find a method's prim within the catalog, or NULL if absent. */
x_obj_t *x_prims_ref(x_obj_t *p_base, x_obj_t *p_ns, x_obj_t *p_method);

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
/** Register core primitives (eval, if, do, let, fn, op, apply, guard, etc.). */
x_obj_t *x_prim_core_register(x_obj_t *p_base, x_obj_t *p_args);

/** Register arithmetic primitives (+, -, *, /, modulo, etc.). */
x_obj_t *x_prim_arith_register(x_obj_t *p_base, x_obj_t *p_args);

/** Register predicate primitives (eq?, pair?, atom?, null?, etc.). */
x_obj_t *x_prim_pred_register(x_obj_t *p_base, x_obj_t *p_args);

/** Register string primitives (string-length, substring, etc.). */
x_obj_t *x_prim_string_register(x_obj_t *p_base, x_obj_t *p_args);

/** Register I/O primitives (read, write, display, load, etc.). */
x_obj_t *x_prim_io_register(x_obj_t *p_base, x_obj_t *p_args);

/** Minimal C read-eval loop (no output, no hooks). */
x_obj_t *x_prim_repl(x_obj_t *p_base, x_obj_t *p_args);

/** Write an object to a string representation. */
x_obj_t *x_prim_write_to_string(x_obj_t *p_base, x_obj_t *p_args);

/** Register custom type primitives (make-type, type accessors, etc.). */
x_obj_t *x_prim_type_register(x_obj_t *p_base, x_obj_t *p_args);

/** Register FFI primitives (ffi-call, ffi-lib, etc.). */
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
