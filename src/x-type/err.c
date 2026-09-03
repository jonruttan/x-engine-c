/**
 * @file err.c
 * @brief The raised-error type: a structured error value the language owns.
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
 * WHAT THIS TYPE IS FOR.
 *
 * A raise used to reach x-lang as a bare, TYPE-LESS atom holding one
 * pre-flattened English string: the engine took "Unbound SYMBOL" and the
 * symbol it could not find, concatenated them into a static buffer, and
 * threw the symbol away.  Nothing downstream could do better than
 * pattern-match the English, because by the time x-lang saw it the
 * structure was gone -- and a type-less atom has no dispatch stacks, so
 * there was nowhere to hang a replacement even if you had one.
 *
 * ERR is that value given a TYPE.  Two slots, (code . obj): the raise
 * site's message literal, and the object it was complaining about.  The
 * IO stacks BOOT EMPTY, deliberately -- exactly as CHARACTER's do (see
 * x-type/char.c and lib/x/type/char-io.x in x-lang) -- so the wording
 * belongs to the language, not to this file.  x-lang's err-io.x pushes
 * the default English onto the write/display stacks; a lang pushes its
 * own over that and pops it again.  The engine names no kinds and
 * spells no prose here.
 *
 * ZERO ALLOCATION ON THE RAISE PATH.  x_eval_error formats in place so
 * that a raise is safe even when the failure IS out of memory.  This
 * type does not weaken that: the base holds ONE ERR instance, allocated
 * at base construction, and a raise only stores two pointers into its
 * slots -- the message literal (static storage, so it survives the
 * longjmp) and the offending object.  Nothing is allocated, nothing is
 * copied, and no x-lang code runs.  Rendering happens later, at display
 * time, where allocation is safe again.
 */
/*
 * # Includes
 */
#include "x-obj.h"
#include "x-type.h"
#include "x-eval-layout.h"
#include "x-type/err.h"

x_satom_t x_type_err_name =
		x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_ERR_NAME }),
	x_type_err_struct_prim =
		x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_err_struct });

/**
 * Build the ERR type descriptor.
 *
 * Name only.  No make handler: an ERR is not constructed from x-lang --
 * the base owns the single instance and the raise path fills it.  No
 * write or display handler either: those stacks are the language's
 * extension point and boot empty on purpose, so an ERR renders as the
 * bounded #<obj:ERR> form until x-lang's err-io.x pushes the prose.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_obj   x_obj_t* -- Unused
 * @return x_obj_t* -- Type descriptor pair list
 */
x_obj_t *x_type_err_struct(x_obj_t *p_base, x_obj_t *p_obj)
{
	struct x_type_t type = { 0 };

	type.p_name = x_type_err_name;
	/* Two slots, so the GC marks BOTH -- the code atom and the offending
	 * object.  Leaving this unset costs the obj slot: the collector sizes
	 * the instance at zero units, never marks what it points at, and the
	 * error renders as reclaimed garbage on the second collection. */
	type.p_units = (x_obj_t *)&x_type_units_pair_obj;

	return x_type_struct_make(p_base, type);
}

/**
 * Register or retrieve the ERR type on the base context.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Unused
 * @return x_obj_t* -- Registered type object
 */
x_obj_t *x_type_err_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type;
	x_obj_t *p_err;
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_err_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_err_struct_prim }, { NULL })
	};

	p_type = x_type_struct_get(p_base, (x_obj_t *)args);

	/* Build the base's one ERR here rather than in x_eval_make.  The
	 * evaluator must not depend on the type registry -- it is constructed
	 * before one exists -- so the instance is made at the first moment it
	 * can be: registration.  Idempotent, because registration is. */
	if ( ! x_obj_isnil(p_base, p_type)
		&& x_obj_isnil(p_base, x_firstobj(x_eval_field_err(p_base)))) {
		p_err = x_obj_make(p_base, p_type, 0, X_OBJ_LENGTH_PAIR, NULL, NULL);
		/* Two permanent atoms whose STRING POINTERS each raise repoints.
		 * Allocated once, here, so a raise allocates nothing. */
		x_err_code(p_err) = x_mksatom(p_base, X_OBJ_FLAG_NONE, NULL);
		x_err_subject(p_err) = x_mksatom(p_base, X_OBJ_FLAG_NONE, NULL);
		x_firstobj(x_eval_field_err(p_base)) = p_err;
	}

	return p_type;
}
