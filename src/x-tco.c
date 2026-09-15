/** @file x-tco.c
 *  @brief The environment save and restore around a tail call.
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2026 Jon Ruttan
 *  @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */
#include "x-tco.h"
#include "x-eval.h"

/* Part of the evaluator engine (see x-eval.c): unit tests that exercise only
 * the base layer omit it by defining STUB_X_EVAL or X_EVAL_OWN before
 * #including this file. */
#if !defined(STUB_X_EVAL) && !defined(X_EVAL_OWN)
/**
 * Push the current environment onto the save-stack and return it.
 *
 * A procedure call and eval-with-env snapshot the environment this way
 * before making another one current; the trampoline, or x_eval_body_tco's
 * early exits, put it back with x_tco_restore().  The environment is a
 * value, so the snapshot is the pointer and nothing else -- there is no
 * boundary, tree or shadow list to carry beside it.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @return x_obj_t* -- The environment pushed
 * @see x_tco_restore
 */
x_obj_t *x_tco_env_save(x_obj_t *p_base)
{
	x_obj_t *p_env = x_eval_field_env(p_base);

	x_eval_field_save_stack(p_base) = x_mkspair(p_base, X_OBJ_FLAG_NONE,
		p_env, x_eval_field_save_stack(p_base));

	return p_env;
}

/**
 * Make @p p_env the current environment.
 *
 * Does NOT touch the save-stack -- a caller that took the environment from
 * the save-stack top pops it separately.  This is the single restore used
 * by both trampoline exit points (x_eval, x_eval_tco_trampoline),
 * x_eval_body_tco's early-exit paths, eval-with-env, and the operative
 * return.
 *
 * @param p_base  x_obj_t* -- Base (execution context)
 * @param p_env   x_obj_t* -- The environment to make current
 * @see x_tco_env_save
 */
void x_tco_restore(x_obj_t *p_base, x_obj_t *p_env)
{
	x_eval_field_env(p_base) = p_env;
}

#endif /* !STUB_X_EVAL && !X_EVAL_OWN -- evaluator engine */
