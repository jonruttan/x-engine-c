/*
 * # Computational Expressions in C
 *
 * ## x-prim/io.c -- Implementation - Primitives - I/O
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
#include "x-prim.h"
#include "x-base.h"
#include "x-eval.h"
#include "x-heap.h"
#include "x-token.h"
#include "x-type.h"
#include "x-type/buffer.h"
#include "x-type/char.h"
#include "x-type/int.h"
#include "x-type/prim.h"
#include "x-type/procedure.h"
#include "x-type/str.h"
#include "x-type/symbol.h"
#include "x-obj/prim.h"

/* write: (write obj) -> output s-expression to stdout */
static x_obj_t *x_prim_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_val;
	p_args = x_restobj(p_args);
	p_val = x_prim_eval_arg(p_base, x_firstobj(p_args));
	x_spair_t write_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_val }, { NULL })
	};

	x_token_write(p_base, (x_obj_t *)write_args);

	return NULL;
}

/* display: (display obj) -> output human-readable via type system */
static x_obj_t *x_prim_display(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_val;
	p_args = x_restobj(p_args);
	p_val = x_prim_eval_arg(p_base, x_firstobj(p_args));
	x_spair_t display_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_val }, { NULL })
	};

	x_token_display(p_base, (x_obj_t *)display_args);

	return NULL;
}

/* read: (read) -> read one s-expression from stdin */
static x_obj_t *x_prim_read_expr(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer;
	x_spair_t read_args[1];
	p_args = x_restobj(p_args);
	p_buffer = x_base_field_buffer(p_base);
	read_args[0][X_OBJ_META_TYPE].p = NULL;
	read_args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)read_args) = p_buffer;
	x_restobj((x_obj_t *)read_args) = p_base;

	return x_token_read(p_base, (x_obj_t *)read_args);
}

/* read-char: (read-char) -> read one character from stdin */
static x_obj_t *x_prim_read_char(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer;
	x_spair_t buf_args[1];
	p_args = x_restobj(p_args);
	p_buffer = x_base_field_buffer(p_base);
	buf_args[0][X_OBJ_META_TYPE].p = NULL;
	buf_args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)buf_args) = p_buffer;
	x_restobj((x_obj_t *)buf_args) = p_base;

	p_buffer = x_type_buffer_read(p_base, (x_obj_t *)buf_args);

	if (x_obj_isnil(p_base, p_buffer)) {
		return NULL;
	}

	return x_mkchar(p_base, x_bufferlastchar(p_buffer));
}

/* to-string helper: capture output of dispatch function into a string */
static x_obj_t *x_prim_to_string(x_obj_t *p_base, x_obj_t *p_args,
	x_obj_t *(*dispatch)(x_obj_t *, x_obj_t *))
{
	x_obj_t *p_val, *p_result;
	x_char_t *buf;
	x_satom_t buf_pos = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = 0 });
	x_spair_t buf_obj[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE,
			{ .v = NULL }, { (x_obj_t *)&buf_pos })
	};
	x_spair_t dispatch_args[1] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL })
	};

	p_val = x_prim_eval_arg(p_base, x_firstobj(p_args));

	if (x_obj_isnil(p_base, p_val)) {
		return x_mkstrown(p_base, x_lib_strndup((x_char_t *)"()", 2));
	}

	buf = (x_char_t *)x_sys_malloc(65536);
	if (buf == NULL) return NULL;
	x_first((x_obj_t *)buf_obj).v = buf;

	/* Push write-buffer onto write_buf_stack */
	x_base_field_write_buf_stack(p_base) = x_mkspair(p_base,
		(x_obj_t *)buf_obj, x_base_field_write_buf_stack(p_base));

	x_firstobj((x_obj_t *)dispatch_args) = p_val;
	dispatch(p_base, (x_obj_t *)dispatch_args);

	/* Pop write_buf_stack */
	x_base_field_write_buf_stack(p_base)
		= x_restobj(x_base_field_write_buf_stack(p_base));
	buf[x_atomint(buf_pos)] = '\0';

	p_result = x_mkstrown(p_base,
		x_lib_strndup(buf, x_atomint(buf_pos)));
	x_sys_free(buf);
	return p_result;
}

/* write-to-string: (write-to-string obj) -> string representation */
x_obj_t *x_prim_write_to_string(x_obj_t *p_base, x_obj_t *p_args)
{
	p_args = x_restobj(p_args);
	return x_prim_to_string(p_base, p_args, x_token_write);
}

/* display-to-string: (display-to-string obj) -> display representation */
static x_obj_t *x_prim_display_to_string(x_obj_t *p_base, x_obj_t *p_args)
{
	p_args = x_restobj(p_args);
	return x_prim_to_string(p_base, p_args, x_token_display);
}

#ifdef X_CLOCK
/* clock: (clock) -> CPU microseconds since process start */
static x_obj_t *x_prim_clock(x_obj_t *p_base, x_obj_t *p_args)
{
	p_args = x_restobj(p_args);
	return x_mkint(p_base, x_sys_clock());
}
#endif /* X_CLOCK */

/* heap-sweep: (heap-sweep) -> sweep unmarked objects from heap */
static x_obj_t *x_prim_heap_sweep(x_obj_t *p_base, x_obj_t *p_args)
{
	p_args = x_restobj(p_args);
#ifdef X_PROFILE
	if (x_base_isset(p_base))
		x_atomint(x_base_field_profile_gc_runs(p_base))++;
#endif

	/* Call free hooks before sweep */
	if (x_base_isset(p_base)) {
		x_obj_t *p_hooks = x_base_field_heap_free_hooks(p_base);
		x_spair_t hook_args[1];

		hook_args[0][X_OBJ_META_TYPE].p = NULL;
		hook_args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;

		while ( ! x_obj_isnil(p_base, p_hooks)) {
			x_firstobj((x_obj_t *)hook_args) = x_firstobj(p_hooks);
			x_restobj((x_obj_t *)hook_args) = NULL;
			x_obj_prim_call(p_base, (x_obj_t *)hook_args);
			p_hooks = x_restobj(p_hooks);
		}
	}

	x_heap_sweep(p_base, x_obj_heap(p_base), X_OBJ_FLAG_HEAP,
		x_type_heap_free);

	return NULL;
}

/* heap-count: (heap-count) -> count objects on heap */
static x_obj_t *x_prim_heap_count(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p = x_obj_heap(p_base);
	p_args = x_restobj(p_args);
	long count = 0;

	while (p) {
		count++;
		p = x_obj_heap(p);
	}

	return x_mkint(p_base, count);
}

/* heap-mark: (heap-mark) -> mark reachable objects on heap */
static x_obj_t *x_prim_heap_mark(x_obj_t *p_base, x_obj_t *p_args)
{
	p_args = x_restobj(p_args);
	/* Normal mark: trace from base data tree */
	x_heap_mark(p_base, x_atomobj(p_base), X_OBJ_FLAG_HEAP,
		x_type_heap_mark);

	/* Conservative stack scan: mark objects referenced from C stack */
	x_heap_mark_stack(p_base, X_OBJ_FLAG_HEAP, x_type_heap_mark);

	/* Mark all registered GC roots */
	if (x_base_isset(p_base)) {
		x_obj_t *p_roots = x_base_field_heap_mark_roots(p_base);

		while ( ! x_obj_isnil(p_base, p_roots)) {
			x_heap_mark(p_base, x_firstobj(p_roots),
				X_OBJ_FLAG_HEAP, x_type_heap_mark);
			p_roots = x_restobj(p_roots);
		}
	}

	/* Call mark hooks (each is a callable) */
	if (x_base_isset(p_base)) {
		x_obj_t *p_hooks = x_base_field_heap_mark_hooks(p_base);
		x_spair_t hook_args[1];

		hook_args[0][X_OBJ_META_TYPE].p = NULL;
		hook_args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;

		while ( ! x_obj_isnil(p_base, p_hooks)) {
			x_firstobj((x_obj_t *)hook_args) = x_firstobj(p_hooks);
			x_restobj((x_obj_t *)hook_args) = NULL;
			x_obj_prim_call(p_base, (x_obj_t *)hook_args);
			p_hooks = x_restobj(p_hooks);
		}
	}

	return NULL;
}

/* system!: (system! obj) -> recursively mark object and all reachable
 * objects as SYSTEM (immune to GC sweep). Uses the same traversal
 * pattern as x_heap_mark. */
static x_obj_t *x_prim_system_mark(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj;
	p_args = x_restobj(p_args);
	p_obj = x_prim_eval_arg(p_base, x_firstobj(p_args));

	/* Reuse the mark traversal with SYSTEM flag */
	x_heap_mark(p_base, p_obj, X_OBJ_FLAG_SYSTEM, x_type_heap_mark);

	return p_obj;
}

/* atomic: (atomic f1 f2 ...) -> call each zero-arg fn with no allocations between.
 * Registered as a wrapped combiner so args are pre-evaluated. */
static x_obj_t *x_prim_atomic(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_result = NULL;
	x_spair_t call_args[1];
	p_args = x_restobj(p_args);

	call_args[0][X_OBJ_META_TYPE].p = NULL;
	call_args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;

	/* Root p_args so mark+sweep inside the loop doesn't free them */
	x_base_field_eval_list_stack(p_base) = x_mkspair(p_base,
		p_args, x_base_field_eval_list_stack(p_base));

	while ( ! x_obj_isnil(p_base, p_args)) {
		x_firstobj((x_obj_t *)call_args) = x_firstobj(p_args);
		x_restobj((x_obj_t *)call_args) = NULL;
		p_result = x_obj_prim_call(p_base, (x_obj_t *)call_args);
		p_args = x_restobj(p_args);
	}

	x_base_field_eval_list_stack(p_base)
		= x_restobj(x_base_field_eval_list_stack(p_base));

	return p_result;
}

/* repl: minimal read-eval loop until EOF (no output, no hooks) */
x_obj_t *x_prim_repl(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_exp;
	x_spair_t read_state[1];
	read_state[0][X_OBJ_META_TYPE].p = NULL;
	read_state[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)read_state) = NULL;
	x_restobj((x_obj_t *)read_state) = NULL;

	for (;;) {
		p_exp = x_prim_read_expr(p_base, (x_obj_t *)read_state);
		if (x_obj_isnil(p_base, p_exp))
			break;
		x_prim_eval_arg(p_base, p_exp);
		x_prim_clear_flag1(p_base);
	}

	return NULL;
}

x_obj_t *x_prim_io_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_prim_entry_t entries[] = {
		{ "write", x_prim_write },
		{ "display", x_prim_display },
		{ "read", x_prim_read_expr },
		{ "read-char", x_prim_read_char },
		{ "write-to-string", x_prim_write_to_string },
		{ "display-to-string", x_prim_display_to_string },
		{ "heap-mark", x_prim_heap_mark },
		{ "heap-sweep", x_prim_heap_sweep },
		{ "heap-count", x_prim_heap_count },
		{ "gc-pin!", x_prim_system_mark }
	};
#ifdef X_CLOCK
	static const x_prim_entry_t clock_entry[] = {
		{ "clock", x_prim_clock }
	};
#endif /* X_CLOCK */

	x_prim_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));
#ifdef X_CLOCK
	x_prim_bind_table(p_base, clock_entry,
		sizeof(clock_entry) / sizeof(clock_entry[0]));
#endif /* X_CLOCK */

	/* applicative: wrapped so args are pre-evaluated */
	{
		x_obj_t *p_sym = x_make_symbol(p_base, X_OBJ_FLAG_NONE,
			"applicative"),
			*p_prim = x_mkprim(p_base, x_prim_atomic),
			*p_wrapped = x_mkwrap(p_base, p_prim),
			*p_pair = x_mkspair(p_base, p_sym, p_wrapped);
		x_base_env_alist_extend(p_base, p_pair);
	}

	return p_base;
}
