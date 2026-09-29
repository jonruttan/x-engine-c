/*
 * # Calls for Unit Tests
 *
 * A routine that takes an argument vector is called from a spec through
 * one of these, which builds the vector from plain arguments.  A spec's
 * calls sit inside the expressions it asserts, where a vector cannot be
 * declared.
 */
#ifndef HELPER_CALLS_C
#define HELPER_CALLS_C

static x_obj_t * __attribute__((unused)) test_env_lookup(x_obj_t *p_base,
	x_obj_t *p_env, x_obj_t *p_sym)
{
	x_obj_t args[x_slot_args_units(2)] =
		x_slot_args({ .p = p_env }, { .p = p_sym });

	return x_env_lookup(p_base, args);
}

static x_obj_t * __attribute__((unused)) test_alist_bst_lookup(x_obj_t *p_base,
	x_obj_t *p_tree, x_obj_t *p_sym)
{
	x_obj_t args[x_slot_args_units(2)] =
		x_slot_args({ .p = p_tree }, { .p = p_sym });

	return x_alist_bst_lookup(p_base, args);
}

#endif /* HELPER_CALLS_C */
