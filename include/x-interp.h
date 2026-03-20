#ifndef X_INTERP_H
#define X_INTERP_H

/*
 * # Computational Expressions in C
 *
 * ## x-interp.h -- Header - Interpreter Core
 *
 * Evaluation engine: argument evaluation, environment extension,
 * body evaluation, and TCO trampoline.
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
#include "x-obj.h"
#include "x-base.h"

/*
 * # BST Shadow Tracking
 *
 * FLAG_1 marks symbols that shadow a BST global binding.
 * The bst_shadow_list tracks flagged symbols so flags can be
 * cleared when restoring env state (scope exit / TCO unwind).
 */
#define X_BST_SHADOW_FLAG			X_OBJ_FLAG_1
#define x_bst_shadow_list(X)		x_base_field_flag1_list(X)

/*
 * # Environment Save / Restore
 *
 * The env state is saved as a compound pair:
 *   ((env . boundary) . (bst . shadow-head))
 */
#define x_env_restore(p_base, p_saved) do { \
	x_base_field_env_alist(p_base) \
		= x_firstobj(x_firstobj(p_saved)); \
	x_base_field_env_local_boundary(p_base) \
		= x_restobj(x_firstobj(p_saved)); \
	x_base_field_env_global_tree(p_base) \
		= x_firstobj(x_restobj(p_saved)); \
	x_clear_bst_shadows_to(p_base, \
		x_restobj(x_restobj(p_saved))); \
} while (0)

#define x_env_restore_pop(p_base) do { \
	x_obj_t *p__saved \
		= x_firstobj(x_base_field_save_stack(p_base)); \
	x_env_restore(p_base, p__saved); \
	x_base_field_save_stack(p_base) \
		= x_restobj(x_base_field_save_stack(p_base)); \
} while (0)

#define x_env_save_push(p_base) \
	x_base_field_save_stack(p_base) = x_mkspair(p_base, \
		x_mkspair(p_base, \
			x_mkspair(p_base, \
				x_base_field_env_alist(p_base), \
				x_base_field_env_local_boundary(p_base)), \
			x_mkspair(p_base, \
				x_base_field_env_global_tree(p_base), \
				x_bst_shadow_list(p_base))), \
		x_base_field_save_stack(p_base))

/*
 * # BST Shadow Functions
 */
void x_clear_bst_shadows(x_obj_t *p_base);
void x_clear_bst_shadows_to(x_obj_t *p_base, x_obj_t *p_old);

/*
 * # Interpreter Core Functions
 */
x_obj_t *x_interp_eval_arg(x_obj_t *p_base, x_obj_t *p_arg);
x_obj_t *x_interp_evlis(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_interp_extend_env(x_obj_t *p_base, x_obj_t *p_env,
	x_obj_t *p_params, x_obj_t *p_vals);

x_obj_t *x_interp_body_eval(x_obj_t *p_base, x_obj_t *p_body);
x_obj_t *x_interp_body_eval_tco(x_obj_t *p_base, x_obj_t *p_body);
x_obj_t *x_interp_body_eval_tco_simple(x_obj_t *p_base, x_obj_t *p_body);
x_obj_t *x_interp_tco_trampoline(x_obj_t *p_base, x_obj_t *p_result);

#endif /* X_INTERP_H */
