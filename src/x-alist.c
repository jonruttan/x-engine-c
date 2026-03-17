/*
 * # Computational Expressions in C
 *
 * ## x-obj.c -- Implementation - Objects
 *
 * @description Computational Expressions in C
 * @author [Jon Ruttan](jonruttan@gmail.com)
 * @copyright 2021 Jon Ruttan
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
#include "x-alist.h"
#include "x-base.h"

x_obj_t *x_alist_extend(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_assoc = x_firstobj(p_args), *p_alist = x_restobj(p_args);

	return x_mkspair(p_base, p_assoc, p_alist);
}

x_obj_t *x_alist_assoc(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args),
		*p_alist = x_firstobj(x_restobj(p_args)),
		*p_sym = x_firstobj(p_obj);
	int depth = 0;
	int is_env_lookup = 0;

#ifdef X_PROFILE
	if (x_base_isset(p_base))
		x_atomint(x_base_field_profile_assoc_calls(p_base))++;
#endif

	/* Check if this is an env alist lookup (not type alist) */
	if (x_base_isset(p_base)
		&& p_alist == x_base_field_env_alist(p_base)) {
		is_env_lookup = 1;
	}

	while ( ! x_obj_isnil(p_base, p_alist)) {
#ifdef X_PROFILE
		if (x_base_isset(p_base))
			x_atomint(x_base_field_profile_assoc_steps(p_base))++;
#endif

		if (x_firstobj(x_firstobj(x_firstobj(p_alist))) == p_sym) {
			/* Promote deep env hits to front of alist */
			if (depth > 64 && is_env_lookup) {
				x_base_field_env_alist(p_base) = x_mkspair(p_base,
					x_firstobj(p_alist),
					x_base_field_env_alist(p_base));
#ifdef X_PROFILE
				x_atomint(x_base_field_profile_assoc_promotes(p_base))++;
#endif
			}

			return x_firstobj(p_alist);
		}

		p_alist = x_restobj(p_alist);
		depth++;
	}

	return NULL;
}
