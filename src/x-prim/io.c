/** @file io.c
 *  @brief I/O primitives: read, write, display, string conversion, heap/GC, system, REPL.
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
/*
 * # Includes
 */
#include "x-prim.h"
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

/** Write a string's bytes to the current output: the OUT port instruction.
 *  x-lang: (write-str s)
 *  @param p_base  Base (execution context).
 *  @param p_args  Unevaluated argument list (s).
 *  @return NULL.
 *  @note The byte door the pure-X printer (lib/x/boot/printer.x) bottoms
 *        out at.  Emits through x_base_write, so it respects the
 *        write-buffer capture stack exactly as the C renderers did.
 */
static x_obj_t *x_prim_write_str(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_s;
	x_satom_t data = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE,
		{ .s = NULL }),
		sz = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .i = 0 });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { data }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { sz }, { NULL })
	};

	x_eargs(p_base, p_args, 2, NULL, &p_s);
	x_atomstr(data) = x_strval(p_s);
	x_atomint(sz) = (x_int_t)x_lib_strlen(x_strval(p_s));
	x_base_write(p_base, (x_obj_t *)args);

	return NULL;
}

/** Read one s-expression from stdin, RAW: the EOF sentinel passes
 *  through, so loop callers can tell end of input from a nil VALUE
 *  (a top-level `()` reads as NULL by design).
 *  @param p_base  Base (execution context).
 *  @return Parsed s-expression, NULL for a nil value, or
 *          x_token_eof_prim at end of input.
 */
static x_obj_t *x_prim_read_expr_raw(x_obj_t *p_base)
{
	x_obj_t *p_buffer = x_firstobj(x_base_field_buffer(p_base));
	x_spair_t read_args[1];
	read_args[0][X_OBJ_META_TYPE].p = NULL;
	read_args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)read_args) = p_buffer;
	x_restobj((x_obj_t *)read_args) = p_base;

	return x_token_read(p_base, (x_obj_t *)read_args);
}

/** Read one s-expression from stdin.
 *  x-lang: (read)
 *  The EOF sentinel is mapped to nil at this boundary: `(Io read)`
 *  consumers loop on (null? ...) as the EOF test, and for them `()`
 *  and end-of-input may stay conflated as before.
 *  @param p_base  Base (execution context).
 *  @param p_args  Unused.
 *  @return Parsed s-expression, or NULL on EOF.
 *  @see x_prim_read_char
 */
static x_obj_t *x_prim_read_expr(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj;
	(void)p_args;

	p_obj = x_prim_read_expr_raw(p_base);

	return p_obj == (x_obj_t *)x_token_eof_prim ? NULL : p_obj;
}

/** Read one character from stdin.
 *  x-lang: (read-char)
 *  @param p_base  Base (execution context).
 *  @param p_args  Unused.
 *  @return Character object, or NULL on EOF.
 *  @see x_prim_read_expr
 */
static x_obj_t *x_prim_read_char(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_firstobj(x_base_field_buffer(p_base));
	x_spair_t buf_args[1];
	(void)p_args;
	buf_args[0][X_OBJ_META_TYPE].p = NULL;
	buf_args[0][X_OBJ_META_FLAGS].i = X_OBJ_FLAG_NONE;
	x_firstobj((x_obj_t *)buf_args) = p_buffer;
	x_restobj((x_obj_t *)buf_args) = p_base;

	p_buffer = x_type_buffer_read(p_base, (x_obj_t *)buf_args);

	if (x_obj_isnil(p_base, p_buffer)) {
		return NULL;
	}

	/* Byte-level read: a CHARACTER holding the raw byte (0-255). */
	return x_mkchar(p_base, (unsigned char)x_bufferlastchar(p_buffer));
}

#ifdef X_SYS_CLOCK
/** Return CPU microseconds since process start.
 *  x-lang: (clock)
 *  @param p_base  Base (execution context).
 *  @param p_args  Unused.
 *  @return Integer with microseconds elapsed.
 *  @note Only available when X_SYS_CLOCK is defined.
 */
static x_obj_t *x_prim_clock(x_obj_t *p_base, x_obj_t *p_args)
{
	(void)p_args;
	return x_mkint(p_base, x_sys_clock());
}
#endif /* X_SYS_CLOCK */

/** Minimal read-eval loop: reads and evaluates expressions until EOF.
 *  @param p_base  Base (execution context).
 *  @param p_args  Unused.
 *  @return NULL on EOF.
 *  @note No output, no prompt, no hooks. Used for C-level bootstrapping;
 *        the x-lang REPL operative in x-core.x provides the full experience.
 *  @note Clears shadows after each evaluation.
 */
x_obj_t *x_prim_repl(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_exp;
	x_obj_t **p_cell = x_heap_root_slot(p_base);
	x_spair_t root = x_obj_set((x_obj_t *)x_type_pair_obj, X_OBJ_FLAG_NONE,
		{ NULL }, { NULL });

	x_heap_root_push(p_cell, root);

	for (;;) {
		p_exp = x_prim_read_expr_raw(p_base);
		if (p_exp == (x_obj_t *)x_token_eof_prim)
			break;
		/* nil is a VALUE (a top-level `()`), not end of input --
		 * breaking on it used to end the boot/batch stream there. */
		if (x_obj_isnil(p_base, p_exp))
			continue;
		/* The freshly read form is this frame's only reference. */
		x_firstobj((x_obj_t *)root) = p_exp;
		x_eval_arg(p_base, p_exp);
		x_prim_clear_shadows(p_base);
	}

	x_heap_root_pop(p_cell);

	return NULL;
}

/**
 * Read one expression, after resetting the source-line counter to 0.
 *
 * The REPL uses this instead of plain read so that forms typed at the prompt
 * are tagged with lines relative to THIS input rather than the cumulative
 * boot+session stream.  The reset must happen here, inside the read primitive:
 * eval updates the line counter from each form's source-line metadata
 * (x-eval.c) when it evaluates the (repl-read) call, so an earlier reset would
 * be clobbered before the read runs.  (include pushes its own counter, so file
 * lines are unaffected.)
 *
 * x-lang form: @code (repl-read) @endcode
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unused.
 * @return The expression read, nil for a nil value, or the EOF
 *         sentinel (%token-eof) at clean end of input.
 */
static x_obj_t *x_prim_repl_read(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_firstobj(x_base_field_buffer(p_base));

	/* The tokenizer tags each form's source line from the buffer's line
	 * metadata (x-token.c), which eval then copies into the counter that
	 * error-line reports.  Reset it to 0 so forms typed here are numbered
	 * relative to this input, not the cumulative boot+session stream. */
	if (p_buffer != NULL && (x_obj_flags(p_buffer) & X_OBJ_FLAG_META)) {
		x_obj_meta_i(p_buffer, 0).i = 0;
	}

	/* RAW read: the EOF sentinel reaches x-lang (bound as %token-eof),
	 * giving the REPL loop its three outcomes -- value, clean EOF, and
	 * (via the raised Unterminated-input error) truncation. */
	return x_prim_read_expr_raw(p_base);
}

/** Register I/O primitives into the environment.
 *
 *  Binds: write-str, read, read-char, heap-mark, heap-sweep, heap-count,
 *  gc-pin!, repl-read.  Conditionally binds clock (when X_SYS_CLOCK defined).
 *  (The printers -- write, display, write-to-str, display-to-str -- and
 *  error-line are pure x-lang now: boot/printer.x renders over the
 *  (io write-str) OUT door; boot/reflect.x walks the error handler.)
 *
 *  @param p_base  Base (execution context).
 *  @param p_args  Unused.
 *  @return The base object.
 */
x_obj_t *x_prim_io_register(x_obj_t *p_base, x_obj_t *p_args)
{
	static const x_prim_entry_t entries[] = {
		{ "write-str",       x_prim_write_str,         "io",   "write-str"      },
		{ "read",            x_prim_read_expr,         "io",   "read"           },
		{ "read-char",       x_prim_read_char,         "io",   "read-char"      },
		{ "repl-read",       x_prim_repl_read,         "io",   "repl-read"      }
	};
#ifdef X_SYS_CLOCK
	static const x_prim_entry_t clock_entry[] = {
		{ "clock", x_prim_clock, "sys", "clock" }
	};
#endif /* X_SYS_CLOCK */

	x_prims_bind_table(p_base, entries,
		sizeof(entries) / sizeof(entries[0]));
#ifdef X_SYS_CLOCK
	x_prims_bind_table(p_base, clock_entry,
		sizeof(clock_entry) / sizeof(clock_entry[0]));
#endif /* X_SYS_CLOCK */

	/* The clean-EOF sentinel, bound for x-lang: the REPL loop compares
	 * repl-read's result against it with (obj same?) -- identity, never
	 * eq? (which compares value words). */
	x_value_bind(p_base, "%token-eof", (x_obj_t *)x_token_eof_prim);

	return p_base;
}
