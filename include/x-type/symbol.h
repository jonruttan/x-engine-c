#ifndef X_TYPE_SYMBOL_H
#define X_TYPE_SYMBOL_H

/**
 * @file x-type/symbol.h
 * @brief Interned symbol type with BST-accelerated lookup.
 *
 * Symbols are atom objects keyed by their string name.  All symbols with
 * the same name share the same object (interning), which enables fast
 * pointer-identity comparison during evaluation.  An internal BST index
 * provides O(log n) lookup by name.
 *
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2021 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 */

/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */

#include "x-type.h"

#define X_TYPE_SYMBOL_NAME	"SYMBOL"  /**< Canonical type name. */

/** @name Predicates
 * @{ */
/** Test whether @p X is a SYMBOL object. */
#define x_obj_type_issymbol(B,X)	x_obj_is_type((B), (X), X_TYPE_SYMBOL_NAME)
/** @} */

/** @name Accessors
 * @{ */
/** Get the symbol's string value. */
#define x_symbolval(X)				x_firststr((X))
/** Alias for x_symbolval -- get the symbol's name. */
#define x_symbolname(X)				x_firststr((X))
/** @} */

/** @name Type Data Accessors
 * @{ */
/** Access the type-level data field for the SYMBOL type. */
#define x_symbol_data				x_type_field_data
/** Get the interned symbol list from the type data. */
#define x_symbol_data_list(X)		x_0(x_symbol_data((X)))
/** @} */

/** @name Constructors
 * @{ */
/** Intern or create a symbol with default flags. */
#define x_mksymbol(B, S)			x_make_symbol((B), X_OBJ_FLAG_NONE, (S))
/** Intern or create a symbol with explicit flags. */
#define x_mkfsymbol(B, F, S)		x_make_symbol((B), (F), (S))
/** Intern or create an owning symbol. */
#define x_mksymbolown(B, S)			x_make_symbol((B), X_OBJ_FLAG_OWN, (S))
/** Intern or create an owning symbol with extra flags. */
#define x_mkfsymbolown(B, F, S)		x_make_symbol((B), X_OBJ_FLAG_OWN | (F), (S))
/** @} */

/** Allocate (or retrieve interned) SYMBOL for string @p s. */
x_obj_t *x_make_symbol(x_obj_t *p_base, x_obj_flag_t flags, x_char_t *s);

/** Build the SYMBOL type struct descriptor. */
x_obj_t *x_type_symbol_struct(x_obj_t *p_base, x_obj_t *p_args);
/** Register (or retrieve) the SYMBOL type and init its data field. */
x_obj_t *x_type_symbol_register(x_obj_t *p_base, x_obj_t *p_args);
/** Initialize the SYMBOL type's intern table. */
x_obj_t *x_type_symbol_init(x_obj_t *p_base, x_obj_t *p_args);
/** Type-system make handler -- intern or allocate a new SYMBOL. */
x_obj_t *x_type_symbol_make(x_obj_t *p_base, x_obj_t *p_args);
/** Look up an interned symbol by name (BST-accelerated). */
x_obj_t *x_type_symbol_find(x_obj_t *p_base, x_obj_t *p_args);
/** Type-system eval handler -- 3-step environment lookup. */
x_obj_t *x_type_symbol_eval(x_obj_t *p_base, x_obj_t *p_args);
/** Type-system write handler -- delegates to sexp symbol writer. */
x_obj_t *x_type_symbol_write(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_TYPE_SYMBOL_H */
