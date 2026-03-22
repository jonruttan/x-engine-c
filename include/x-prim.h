#ifndef X_PRIM_H
#define X_PRIM_H

/*
 * # Computational Expressions in C
 *
 * ## x-prim.h -- Header - Primitives
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
#include <stdarg.h>

/*
 * # Data Structures
 */
x_obj_t *x_prim_eval_arg(x_obj_t *p_base, x_obj_t *p_arg);

/*
 * x_args: unpack N elements from an args list into output pointers.
 * NULL pointers skip that position (like _ in pattern matching).
 *   x_args(p_args, 3, NULL, &a, &b)  -- skip self, extract 2
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

/*
 * x_eargs: unpack + eval N elements from an args list.
 * NULL pointers skip that position without evaluating.
 *   x_eargs(p_base, p_args, 3, NULL, &a, &b)  -- skip self, eval+extract 2
 */
static void __attribute__((unused)) x_eargs(x_obj_t *p_base, x_obj_t *p_args, int count, ...)
{
	va_list ap;
	int i;

	va_start(ap, count);
	for (i = 0; i < count; i++) {
		x_obj_t **slot = va_arg(ap, x_obj_t **);
		if (p_args == NULL) { if (slot) *slot = NULL; continue; }
		if (slot != NULL)
			*slot = x_prim_eval_arg(p_base, x_firstobj(p_args));
		p_args = x_restobj(p_args);
	}
	va_end(ap);
}
x_obj_t *x_prim_evlis(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_prim_multiple_extend(x_obj_t *p_base, x_obj_t *p_env,
	x_obj_t *p_params, x_obj_t *p_vals);

x_obj_t *x_prim_body_eval(x_obj_t *p_base, x_obj_t *p_body);
x_obj_t *x_prim_body_eval_tco(x_obj_t *p_base, x_obj_t *p_body);
x_obj_t *x_prim_body_eval_tco_simple(x_obj_t *p_base, x_obj_t *p_body);
x_obj_t *x_prim_tco_trampoline(x_obj_t *p_base, x_obj_t *p_result);

typedef struct {
	x_char_t *name;
	x_prim_fn fn;
} x_prim_entry_t;

void x_prim_bind(x_obj_t *p_base, x_char_t *name, x_prim_fn fn);
void x_prim_bind_table(x_obj_t *p_base, const x_prim_entry_t *table, int count);

x_obj_t *x_prim_core_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_prim_arith_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_prim_pred_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_prim_string_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_prim_io_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_prim_repl(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_prim_write_to_string(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_prim_type_register(x_obj_t *p_base, x_obj_t *p_args);
x_obj_t *x_prim_ffi_register(x_obj_t *p_base, x_obj_t *p_args);

x_obj_t *x_prim_callcc_register(x_obj_t *p_base, x_obj_t *p_args);
void x_callcc_init(void);

void x_prim_clear_flag1(x_obj_t *p_base);
void x_prim_clear_flag1_to(x_obj_t *p_base, x_obj_t *p_old);

x_obj_t *x_prim_register(x_obj_t *p_base, x_obj_t *p_args);

#endif /* X_PRIM_H */
