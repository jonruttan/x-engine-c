/**
 * @file x-type/comment.c
 * @brief Comment token type implementation for the tokenizer.
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2023 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
#include "x-eval.h"
#include "x-type/comment.h"
#include "x-token/sexp/comment.h"

x_satom_t x_type_comment_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_COMMENT_NAME }),
	x_type_comment_struct_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_comment_struct });

/**
 * Build the COMMENT type struct descriptor.
 *
 * Registers the s-expression comment analyse and delimit callbacks.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_obj   x_obj_t* -- Unused
 * @return Type struct pair list
 */
x_obj_t *x_type_comment_struct(x_obj_t *p_base, x_obj_t *p_obj)
{
	struct x_type_t type = {
		.p_name = x_type_comment_name,
		.p_analyse = x_sexp_comment_analyse1_prim,
		.p_delimit = x_sexp_comment_delimit_prim
	};

	return x_type_struct_make(p_base, type);
}

/**
 * Register (or retrieve) the COMMENT type struct on p_base.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_args  x_obj_t* -- Unused
 * @return The registered type struct object
 */
x_obj_t *x_type_comment_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_comment_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_comment_struct_prim }, { NULL })
	};

	return x_type_struct_get(p_base, (x_obj_t *)args);
}
