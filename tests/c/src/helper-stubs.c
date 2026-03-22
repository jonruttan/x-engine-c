/*
 * # Common Stubs for Unit Tests
 *
 * Provides dummy implementations for symbols not under test.
 * Each test defines STUB_* macros before including this file
 * to select which stubs are needed.
 *
 * Usage:
 *   #define STUB_X_PRIM
 *   #define STUB_X_BASE_ERROR
 *   #include "helper-stubs.c"
 */
#ifndef HELPER_STUBS_C
#define HELPER_STUBS_C

#ifdef STUB_X_PRIM
x_obj_t *x_prim_eval_arg(x_obj_t *p_base, x_obj_t *p_arg) { return p_arg; }
x_obj_t *x_prim_evlis(x_obj_t *p_base, x_obj_t *p_args) { return p_args; }
x_obj_t *x_prim_multiple_extend(x_obj_t *p_base, x_obj_t *p_env,
	x_obj_t *p_params, x_obj_t *p_vals) { return p_env; }
x_obj_t *x_prim_body_eval(x_obj_t *p_base, x_obj_t *p_body) { return NULL; }
x_obj_t *x_prim_body_eval_tco(x_obj_t *p_base, x_obj_t *p_body) { return NULL; }
x_obj_t *x_prim_body_eval_tco_simple(x_obj_t *p_base, x_obj_t *p_body) { return NULL; }
x_obj_t *x_prim_tco_trampoline(x_obj_t *p_base, x_obj_t *p_result) { return p_result; }
void x_prim_bind(x_obj_t *p_base, x_char_t *name, x_prim_fn fn) {}
#endif

#ifdef STUB_X_PROCEDURE
x_obj_t *x_type_procedure_call(x_obj_t *p_base, x_obj_t *p_args) { return NULL; }
#endif

#ifdef STUB_X_OPERATIVE
x_obj_t *x_type_operative_call(x_obj_t *p_base, x_obj_t *p_args) { return NULL; }
#endif

#ifdef STUB_X_EVAL
x_obj_t *x_eval(x_obj_t *p_base, x_obj_t *p_obj) { return NULL; }
#endif

#ifdef STUB_X_BASE_ERROR
void x_base_error(x_obj_t *p_base, x_char_t *message, x_obj_t *p_obj) {}
#endif

#ifdef STUB_X_TOKEN
x_obj_t *x_token_read(x_obj_t *p_base, x_obj_t *p_args) { return NULL; }
x_obj_t *x_token_write(x_obj_t *p_base, x_obj_t *p_args) { return NULL; }
#endif

#ifdef STUB_X_HEAP
#include "x-heap.h"
x_obj_t *x_heap_mark(x_obj_t *p_base, x_obj_t *p_obj, x_obj_flag_t flags,
	x_heap_mark_fn_t p_mark_fn) { return NULL; }
#endif

#ifdef STUB_X_OBJ_OBJ
x_satom_t x_type_base_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE,
	{.s = (x_char_t *)"BASE"});
#endif

/* x_obj_meta_extra needed by ext/x-expr/src/x-obj.c alloc/free;
   skip if src/x-obj/obj.c already provides it */
#ifndef X_OBJ_META_EXTRA_DEFINED
#define X_OBJ_META_EXTRA_DEFINED
size_t x_obj_meta_extra = 0;
#endif

#ifdef STUB_X_STR
x_obj_t *x_make_str(x_obj_t *p_base, x_obj_flag_t flags,
	x_char_t *str) { return NULL; }
#endif

#ifdef STUB_X_INT
x_obj_t *x_make_int(x_obj_t *p_base, x_obj_flag_t flags,
	x_int_t i) { return NULL; }
#endif

#ifdef STUB_X_SYMBOL
x_obj_t *x_make_symbol(x_obj_t *p_base, x_obj_flag_t flags,
	x_char_t *s) { return NULL; }
#endif

#ifdef STUB_X_PRIM_REGISTER
x_obj_t *x_prim_core_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_arith_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_pred_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_string_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_io_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_type_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_ffi_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_prim_callcc_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_binding_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_closure_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_control_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
x_obj_t *x_syntax_quote_register(x_obj_t *p_base, x_obj_t *p_args) { return p_base; }
#endif

#ifdef STUB_X_PRIM_FLAG1
void x_prim_clear_flag1_to(x_obj_t *p_base, x_obj_t *p_to) {}
#endif

#ifdef STUB_X_TYPE_PRIM
x_obj_t *x_type_prim_type_name(x_obj_t *p_base, x_obj_t *p_args) { return NULL; }
x_obj_t *x_type_prim_units(x_obj_t *p_base, x_obj_t *p_args) { return NULL; }
x_obj_t *x_type_prim_length(x_obj_t *p_base, x_obj_t *p_args) { return NULL; }
#endif

#ifdef STUB_X_SEXP_PAIR_WRITE
x_satom_t x_sexp_pair_write_prim = x_obj_set(NULL, X_OBJ_FLAG_NONE,
	{.fn = NULL});
#endif

#endif /* HELPER_STUBS_C */
