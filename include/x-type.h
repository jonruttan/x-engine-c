#ifndef X_TYPE_H
#define X_TYPE_H

/**
 * @file x-type.h
 * @brief Type system field accessors and type struct definition.
 *
 * Defines the @c x_type_t struct that mirrors the pair-tree layout of a
 * type descriptor, along with macros to navigate each field group (name,
 * data, heap, proc, cvt, io, iter).  Each field has a @c _stack variant
 * returning the full @c (current . saved) cell, and a bare variant
 * returning the current value.
 *
 * Type descriptor pair-tree layout (all leaf fields are stack-wrapped):
 * @code
 * '(
 *    name          [S] type name symbol
 *    data          [S] arbitrary type-specific data
 *    (mark         [S] GC mark callback
 *     make         [S] constructor
 *     free         [S] destructor
 *     clone        [S] clone callback
 *     units        [S] element count (for GC traversal)
 *     length)      [S] length callback
 *    (call         [S] call handler (for callable types)
 *     eval)        [S] eval handler (for self-evaluating types)
 *    (from         [S] inbound conversion alist
 *     to)          [S] outbound conversion alist
 *    (analyse      [S] tokenizer scoring callback
 *     delimit      [S] delimiter predicate
 *     read         [S] reader (token -> object)
 *     write        [S] writer (s-expression output)
 *     display)     [S] display (human-readable output)
 *    (iter)        [S] iterator constructor
 *    (ops)         [S] generic-operator alist ((op-sym . handler) ...)
 * )
 * @endcode
 *
 * Every field is stack-wrapped: stored as @c (current . saved).
 * The @c _stack macros return the cell, the bare macros return
 * @c x_firstobj(cell) (the current value).  Stack-wrapping enables
 * @c type-push-write / @c type-pop-write to temporarily override
 * handlers (e.g. for write-to-str capture).
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

#include "x-eval.h"

/** The op-arbitration refusal (#584); x_eval_error appends the type name. */
#define X_TYPE_NO_CVT_TEXT            "no declared promotion; declare the cvt relation for"

/** @name Type Construction
 * @{ */
/** Allocate a new type object with the given payload. */
#define x_mktype(B,T,X)               x_obj_make((B), (T), 0, 1, (X))

/** Get the type alist from a base object. */
#define x_type_types(B)               (x_firstobj((B)))

/** Set the type alist on a base object. */
#define x_type_settypes(B,X)          (x_type_types((B)) = (X))
/** @} */

/**
 * @defgroup type_fields Type Field Accessor Macros
 * @brief Navigate the pair-tree structure of a type descriptor.
 *
 * Macros named @c x_type_field_*_stack return the stack cell
 * @c (current . saved).  The bare @c x_type_field_* variants return
 * the current value via @c x_firstobj.
 * @{
 */

/** @name Name and Data Fields
 * @{ */
#define x_type_field_name_stack(X)    x_firstobj(X)                          /**< Name stack cell. */
#define x_type_field_name(X)          x_firstobj(x_type_field_name_stack((X))) /**< Current name. */

#define x_type_field_data_stack(X)    x_firstobj(x_restobj(X))               /**< Data stack cell. */
#define x_type_field_data(X)          x_firstobj(x_type_field_data_stack((X))) /**< Current data. */
/** @} */

/** @name Heap Group -- Memory Management Handlers
 *  @c (mark make free clone units length)
 * @{ */
#define x_type_field_heap(X)          x_firstobj(x_restobj(x_restobj(X)))    /**< Heap handler group. */
#define x_type_field_mark_stack(X)    x_firstobj(x_type_field_heap((X)))     /**< GC mark stack cell. */
#define x_type_field_mark(X)          x_firstobj(x_type_field_mark_stack((X))) /**< Current GC mark handler. */
#define x_type_field_make_stack(X)    x_firstobj(x_restobj(x_type_field_heap((X)))) /**< Constructor stack cell. */
#define x_type_field_make(X)          x_firstobj(x_type_field_make_stack((X))) /**< Current constructor. */
#define x_type_field_free_stack(X)    x_firstobj(x_restobj(x_restobj(x_type_field_heap((X))))) /**< Destructor stack cell. */
#define x_type_field_free(X)          x_firstobj(x_type_field_free_stack((X))) /**< Current destructor. */
#define x_type_field_clone_stack(X)   x_firstobj(x_restobj(x_restobj(x_restobj(x_type_field_heap((X)))))) /**< Clone stack cell. */
#define x_type_field_clone(X)         x_firstobj(x_type_field_clone_stack((X))) /**< Current clone handler. */
#define x_type_field_units_stack(X)   x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_type_field_heap((X))))))) /**< Units stack cell. */
#define x_type_field_units(X)         x_firstobj(x_type_field_units_stack((X))) /**< Current units handler. */
#define x_type_field_length_stack(X)  x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_type_field_heap((X)))))))) /**< Length stack cell. */
#define x_type_field_length(X)        x_firstobj(x_type_field_length_stack((X))) /**< Current length handler. */
/** @} */

/** @name Proc Group -- Call and Eval Handlers
 *  @c (call eval)
 * @{ */
#define x_type_field_proc(X)          x_firstobj(x_restobj(x_restobj(x_restobj(X)))) /**< Proc handler group. */
#define x_type_field_call_stack(X)    x_firstobj(x_type_field_proc((X)))     /**< Call stack cell. */
#define x_type_field_call(X)          x_firstobj(x_type_field_call_stack((X))) /**< Current call handler. */
#define x_type_field_eval_stack(X)    x_firstobj(x_restobj(x_type_field_proc((X)))) /**< Eval stack cell. */
#define x_type_field_eval(X)          x_firstobj(x_type_field_eval_stack((X))) /**< Current eval handler. */
/** @} */

/** @name Cvt Group -- Conversion Handlers
 *  @c (from to)
 * @{ */
#define x_type_field_cvt(X)           x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(X))))) /**< Conversion handler group. */
#define x_type_field_from_stack(X)    x_firstobj(x_type_field_cvt((X)))      /**< From-conversion stack cell. */
#define x_type_field_from(X)          x_firstobj(x_type_field_from_stack((X))) /**< Current from-conversion handler. */
#define x_type_field_to_stack(X)      x_firstobj(x_restobj(x_type_field_cvt((X)))) /**< To-conversion stack cell. */
#define x_type_field_to(X)            x_firstobj(x_type_field_to_stack((X))) /**< Current to-conversion handler. */
/** @} */

/** @name I/O Group -- Read, Write, and Display Handlers
 *  @c (analyse delimit read write display)
 * @{ */
#define x_type_field_io(X)            x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(X)))))) /**< I/O handler group. */
#define x_type_field_analyse_stack(X) x_firstobj(x_type_field_io((X)))       /**< Tokenizer analyse stack cell. */
#define x_type_field_analyse(X)       x_firstobj(x_type_field_analyse_stack((X))) /**< Current analyse handler. */
#define x_type_field_delimit_stack(X) x_firstobj(x_restobj(x_type_field_io((X)))) /**< Delimiter stack cell. */
#define x_type_field_delimit(X)       x_firstobj(x_type_field_delimit_stack((X))) /**< Current delimiter handler. */
#define x_type_field_read_stack(X)    x_firstobj(x_restobj(x_restobj(x_type_field_io((X))))) /**< Reader stack cell. */
#define x_type_field_read(X)          x_firstobj(x_type_field_read_stack((X))) /**< Current reader handler. */
#define x_type_field_write_stack(X)   x_firstobj(x_restobj(x_restobj(x_restobj(x_type_field_io((X)))))) /**< Writer stack cell. */
#define x_type_field_write(X)         x_firstobj(x_type_field_write_stack((X))) /**< Current write handler. */
#define x_type_field_display_stack(X) x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_type_field_io((X))))))) /**< Display stack cell. */
#define x_type_field_display(X)       x_firstobj(x_type_field_display_stack((X))) /**< Current display handler. */
/** @} */

/** @name Iter Group -- Iterator Handler
 *  @c (iter)
 * @{ */
#define x_type_field_iter_group(X)    x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(X))))))) /**< Iterator handler group. */
#define x_type_field_iter_stack(X)    x_firstobj(x_type_field_iter_group((X))) /**< Iterator stack cell. */
#define x_type_field_iter(X)          x_firstobj(x_type_field_iter_stack((X))) /**< Current iterator handler. */
/** @} */

/** @name Ops Group -- Generic-Operator Dispatch
 *  @c (ops) -- the per-type generic-operator alist.  A typed operand
 *  dispatches @c + - * / % = < to its type's registered handler; a type
 *  with a nil ops alist never dispatches (ints keep the pure-C fast path).
 * @{ */
#define x_type_field_ops_group(X)     x_firstobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(x_restobj(X)))))))) /**< Ops handler group. */
#define x_type_field_ops_stack(X)     x_firstobj(x_type_field_ops_group((X))) /**< Ops alist stack cell. */
#define x_type_field_ops(X)           x_firstobj(x_type_field_ops_stack((X))) /**< Current ops alist. */
/** @} */

/** Extract the type object from a type-dispatch argument list. */
#define x_type_arg_type(X)            x_firstobj((X))

/** @} */ /* end type_fields */

/**
 * @brief C-side mirror of a type descriptor's fields.
 *
 * Passed to x_type_struct_make() to build a type pair tree from
 * named C fields.  Any NULL field is stored as nil in the tree.
 */
struct x_type_t
{
	x_obj_t *p_name;       /**< Type name symbol. */
	x_obj_t *p_data;       /**< Arbitrary type-specific data. */
	x_obj_t *p_mark;       /**< GC mark handler. */
	x_obj_t *p_make;       /**< Constructor handler. */
	x_obj_t *p_free;       /**< Destructor handler. */
	x_obj_t *p_clone;      /**< Clone handler. */
	x_obj_t *p_units;      /**< Unit shape: read (never called) by the GC,
	                            the spine guard, and x_type_prim_units.
	                            Two forms, see x_type_units_count():
	                            an INT atom is a bare count (negative =
	                            dynamic-size sentinel, instance slot 0 holds
	                            the payload count); a pair (count . mask)
	                            adds a kind per unit -- see
	                            #X_TYPE_UNIT_REF. */
	x_obj_t *p_length;     /**< Length handler. */
	x_obj_t *p_call;       /**< Call handler. */
	x_obj_t *p_eval;       /**< Eval handler. */
	x_obj_t *p_from;       /**< From-conversion handler. */
	x_obj_t *p_to;         /**< To-conversion handler. */
	x_obj_t *p_analyse;    /**< Tokenizer analyse handler. */
	x_obj_t *p_delimit;    /**< Delimiter handler. */
	x_obj_t *p_read;       /**< Reader handler. */
	x_obj_t *p_write;      /**< Writer handler. */
	x_obj_t *p_display;    /**< Display handler. */
	x_obj_t *p_iter;       /**< Iterator handler. */
	x_obj_t *p_ops;        /**< Generic-operator alist ((op-sym . handler) ...). */
};

/** @name Type Functions
 * @{ */

/** Build a type pair tree from a C x_type_t struct. */
x_obj_t *x_type_struct_make(x_obj_t *p_base, struct x_type_t type);

/** Try generic-operator dispatch for a binary op; 1 if dispatched. */
int x_type_op_try(x_obj_t *p_base, x_char_t *op, x_obj_t *p_a, x_obj_t *p_b,
	x_obj_t **pp_result);

/** Look up a type struct from a type-dispatch argument list. */
x_obj_t *x_type_struct_get(x_obj_t *p_base, x_obj_t *p_args);

/** Primitive: return the name of an object's type. */
x_obj_t *x_type_prim_type_name(x_obj_t *p_base, x_obj_t *p_args);

/** Primitive: return the units (element size) of an object. */
x_obj_t *x_type_prim_units(x_obj_t *p_base, x_obj_t *p_args);

/** @name Unit Kinds
 *
 * A type's @c p_units slot says how many units an instance has and, in its
 * pair form, what each unit @e is. The kind decides who may touch the unit:
 * only #X_TYPE_UNIT_REF holds a heap object pointer, so only #X_TYPE_UNIT_REF
 * may be handed to x_heap_tree_mark() -- which sets a mark bit through the
 * pointer before it can establish that the pointer is a heap object, so a
 * traced #X_TYPE_UNIT_BYTES would corrupt the bytes it names.
 *
 * The mask packs #X_TYPE_UNIT_BITS bits per unit, unit 0 lowest. Units past
 * the described prefix take the kind of the last described unit, which is
 * what gives a dynamic-size type its payload kind: a count of @c -1 with mask
 * @c (word, ref) is a length word followed by slot-0-many references.
 *
 * #X_TYPE_UNIT_REF is 0, so a zero mask means "every unit a reference" -- the
 * bare-count form's meaning, unchanged.
 * @{ */
#define X_TYPE_UNIT_REF		0  /**< A heap object pointer; the GC traces it. */
#define X_TYPE_UNIT_WORD	1  /**< An immediate; nothing dereferences it. */
#define X_TYPE_UNIT_BYTES	2  /**< A pointer to bytes the type can measure. */
#define X_TYPE_UNIT_FOREIGN	3  /**< An address C owns. */
#define X_TYPE_UNIT_BITS	2  /**< Mask bits per unit. */
#define X_TYPE_UNIT_KIND_MASK	3  /**< Low-bit mask of one kind field. */
/** Units a mask can describe before the repeat rule takes over. */
#define X_TYPE_UNIT_DESCRIBED_MAX \
	((x_int_t)(sizeof(x_int_t) * 8 / X_TYPE_UNIT_BITS))
/** @} */

/** The declared unit count from either form of a @c p_units slot. */
x_int_t x_type_units_count(x_obj_t *p_units);

/** The kind mask from either form of a @c p_units slot (0 = all references). */
x_int_t x_type_units_mask(x_obj_t *p_units);

/** How many leading units a @c p_units slot's mask describes. */
x_int_t x_type_units_described(x_obj_t *p_units);

/** The kind of unit @p i, applying the repeat rule past @p described. */
int x_type_unit_kind(x_int_t mask, x_int_t i, x_int_t described);

/** Primitive: return the length of an object. */
x_obj_t *x_type_prim_length(x_obj_t *p_base, x_obj_t *p_args);

/** @} */

/** @name Type Heap Callbacks
 * @{ */

/** GC mark callback -- mark a typed object and its contents. */
x_obj_t *x_type_heap_mark(x_obj_t *p_base, x_obj_t *p_obj, x_obj_flag_t flags);

/** GC free callback -- free a typed object's resources. */
void x_type_heap_free(x_obj_t *p_base, x_obj_t *p_obj);

/** @} */

#endif /* X_TYPE_H */
