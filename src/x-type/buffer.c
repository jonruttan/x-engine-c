/** @file x-type/buffer.c
 *  @brief Byte buffer type -- construction, cursor management, and character I/O.
 *  @author Jon Ruttan (jonruttan@gmail.com)
 *  @copyright 2021 Jon Ruttan
 *  @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */

#include "x-type/buffer.h"
#include "x-type/char.h"
#include "x-eval.h"
#include "x-heap.h"

x_satom_t x_type_buffer_name = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = (x_char_t *)X_TYPE_BUFFER_NAME }),
	x_type_buffer_mark_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_buffer_mark }),
	x_type_buffer_make_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_buffer_make }),
	x_type_buffer_struct_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_buffer_struct });

/**
 * Allocate a BUFFER wrapping the character array at @p p.
 *
 * Both read and write cursors are initialized to point at @p p.
 *
 * @param p_base  Base (execution context).
 * @param flags   Object flags (e.g. @c X_OBJ_FLAG_OWN, @c X_OBJ_FLAG_RO).
 * @param p       Pointer to the underlying character array.
 * @return Newly allocated BUFFER object.
 */
x_obj_t *x_make_buffer(x_obj_t *p_base, x_obj_flag_t flags, void *p)
{
	x_satom_t buffer = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .v = p }),
		flags_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = flags });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { buffer }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { flags_obj }, { NULL })
	};

	return x_type_buffer_make(p_base, (x_obj_t *)args);
}

/**
 * GC mark handler for BUFFER objects.
 *
 * Flags the inner bookkeeping pair directly without traversing its
 * slots, since those contain raw @c char* pointers rather than objects.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Argument list: (buffer-object flags).
 * @return NULL (mark handlers have no meaningful return).
 */
x_obj_t *x_type_buffer_mark(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args);
	x_obj_flag_t flags = x_firstint(x_restobj(p_args));

	/* Buffer data slots contain raw char pointers, not objects.
	 * Flag the inner bookkeeping object directly — don't traverse
	 * its slots since they're raw char pointers, not objects. */
	if ( ! x_obj_isnil(p_base, x_restobj(p_obj))) {
		x_obj_flags(x_restobj(p_obj)) |= flags;
	}

	return NULL;
}

/**
 * Build the BUFFER type struct descriptor.
 *
 * Populates name, mark, and make hooks.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unused.
 * @return Type struct pair-tree for BUFFER.
 */
/* Image save: the outer is (bytes . inner); the inner is (read . write),
 * pointers into those bytes, saved as OFFSETS -- an inner is told apart
 * from an outer by its rest slot pointing back into the same region. */
x_obj_t *x_type_buffer_save(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args);
	x_obj_t *p_buf = x_firstobj(x_restobj(p_args));
	x_obj_t *p_rest;
	x_int_t *buf;
	x_char_t *val;

	buf = (x_int_t *)x_firstptr(p_buf);
	p_rest = x_restobj(p_obj);
	if (p_rest != NULL && x_obj_type(p_rest) == x_obj_type(p_obj)) {
		/* outer: (val . inner) */
		buf[0] = 2;
		buf[1] = X_TYPE_UNIT_BYTES; buf[2] = (x_int_t)x_firststr(p_obj);
		buf[3] = X_TYPE_UNIT_REF;   buf[4] = (x_int_t)p_rest;
	} else {
		/* inner: (read . write) as offsets from the outer's val, which is
		 * the smaller of the two pointers' region start -- read is never
		 * below val, so val is recovered as read minus the unread-so-far
		 * count the outer's load re-adds.  Offsets are what survive. */
		val = x_firststr(p_obj);
		buf[0] = 2;
		buf[1] = X_TYPE_UNIT_WORD;  buf[2] = 0;
		buf[3] = X_TYPE_UNIT_WORD;  buf[4] = (x_int_t)(x_reststr(p_obj) - val);
	}

	return p_obj;
}
x_satom_t x_type_buffer_save_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_buffer_save });

/* The inner object saved its read/write pointers as OFFSETS from val (its
 * own save cannot know val is being re-based); the outer re-bases them. */
x_obj_t *x_type_buffer_load(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args);
	x_obj_t *p_inner;
	x_char_t *val;

	p_inner = x_restobj(p_obj);
	val = x_bufferval(p_obj);
	if (p_inner != NULL && val != NULL) {
		x_firststr(p_inner) = val + x_firstint(p_inner);
		x_reststr(p_inner) = val + x_restint(p_inner);
	}

	return p_obj;
}
x_satom_t x_type_buffer_load_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { (x_obj_t *)&x_type_buffer_load });

x_obj_t *x_type_buffer_struct(x_obj_t *p_base, x_obj_t *p_args)
{
	struct x_type_t type = {
		.p_save = (x_obj_t *)x_type_buffer_save_prim,
		.p_load = (x_obj_t *)x_type_buffer_load_prim,
		.p_name = x_type_buffer_name,
		.p_mark = x_type_buffer_mark_prim,
		.p_make = x_type_buffer_make_prim
	};

	return x_type_struct_make(p_base, type);
}

/**
 * Register (or retrieve) the BUFFER type in the type alist.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Unused.
 * @return The registered BUFFER type object.
 */
x_obj_t *x_type_buffer_register(x_obj_t *p_base, x_obj_t *p_args)
{
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_buffer_name }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_buffer_struct_prim }, { NULL })
	};

	return x_type_struct_get(p_base, (x_obj_t *)args);
}

/**
 * Type-system make handler for BUFFER objects.
 *
 * Creates a two-level structure: the outer pair holds the base pointer,
 * and the inner pair holds the read and write cursors (both initialized
 * to the base pointer).  The @c X_OBJ_FLAG_OWN flag is set only on the
 * outer object.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Argument list: (buffer-atom [flags-atom]).
 * @return Newly allocated BUFFER object.
 */
x_obj_t *x_type_buffer_make(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_type = x_type_buffer_register(p_base, p_base),
		*p_buffer = x_0(p_args);
	x_obj_flag_t flags = x_obj_isnil(p_base, x_1(p_args))
		? 0 : x_firstint(x_01(p_args));

	return x_obj_make(p_base, p_type, flags, X_OBJ_LENGTH_PAIR,
			x_bufferval(p_buffer),
			x_obj_make(p_base, p_type,
				flags & ~X_OBJ_FLAG_OWN, X_OBJ_LENGTH_PAIR,
				x_bufferval(p_buffer), x_bufferval(p_buffer)));
}

/**
 * Reset both read and write cursors to the base pointer.
 *
 * Effectively empties the buffer without deallocating the array.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Argument list whose first element is the buffer.
 * @return The buffer object.
 */
x_obj_t *x_type_buffer_reset(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_firstobj(p_args);

	x_bufferwrite(p_buffer) = x_bufferread(p_buffer) = x_bufferval(p_buffer);

	return p_buffer;
}

/**
 * Compact unread data to the front of the underlying array.
 *
 * Copies the remaining unread bytes to position 0 and adjusts both
 * cursors accordingly, freeing space at the end of the buffer.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Argument list whose first element is the buffer.
 * @return The buffer object.
 */
x_obj_t *x_type_buffer_retain(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_firstobj(p_args);
	x_int_t n = x_bufferwrite(p_buffer) - x_bufferread(p_buffer);

	/* A read-only buffer never refills, so compaction serves nothing
	 * but the val==read mark the tokenizer's consumption math needs
	 * (x_bufferlen = read - val).  Advance the mark as a pointer bump
	 * instead of memcpying the whole unread remainder: the tokenizer
	 * retains PER TOKEN, which made (tok read-str) -- whose buffer is
	 * the entire input -- O(input^2) in total bytes copied (#354; a
	 * 47KB source file cost hundreds of MB of memcpy per read).  The
	 * bytes do not move, so lastchar (read[-1]) stays valid; unread
	 * (write - read) is untouched; and no RO tokenizer buffer is OWN
	 * (token-read-string frees its copy itself), so the free path
	 * never sees the bumped val. */
	if (x_obj_flags(p_buffer) & X_OBJ_FLAG_RO) {
		x_bufferval(p_buffer) = x_bufferread(p_buffer);
		return p_buffer;
	}

	x_lib_memcpy(x_bufferval(p_buffer), x_bufferread(p_buffer), n);
	x_bufferread(p_buffer) = x_bufferval(p_buffer);
	x_bufferwrite(p_buffer) = x_bufferread(p_buffer) + n;

	return p_buffer;
}

/**
 * Append a single character at the write cursor.
 *
 * Writes the character from the second argument and advances the
 * write cursor by one.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Argument list: (buffer char-object).
 * @return The buffer object.
 */
x_obj_t *x_type_buffer_append(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_firstobj(p_args);

	*x_bufferwrite(p_buffer) = x_charval(x_firstobj(x_restobj(p_args)));
	x_bufferwrite(p_buffer) += 1;

	return p_buffer;
}

/**
 * Read one character from the buffer, extending from stdin if needed.
 *
 * If the buffer is exhausted and not read-only, reads one character
 * from the base input channel and appends it.  Advances the read
 * cursor by one and tracks newlines for error reporting.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Argument list whose first element is the buffer.
 * @return The buffer object after advancing, or NULL on EOF.
 *
 * @note Read-only buffers return NULL when exhausted instead of
 *       reading from stdin.
 */
x_obj_t *x_type_buffer_read(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_firstobj(p_args), *p_char, *p_sigint;
	x_satom_t char_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .c = '\0' }),
		int_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = 1 });
	x_spair_t read_args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { char_obj }, { (x_obj_t *)(read_args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { int_obj }, { NULL })
	};
	x_spair_t append_args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_buffer }, { (x_obj_t *)(append_args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { char_obj }, { NULL })
	};

	if (x_bufferwrite(p_buffer) <= x_bufferread(p_buffer)) {
		/* Readonly buffers don't extend from stdin. */
		if (x_obj_flags(p_buffer) & X_OBJ_FLAG_RO) {
			return NULL;
		}

		/* Latched EOF (#90): the current stream already ended, so
		 * report EOF without another syscall.  See the latch below. */
		if (x_base_isset(p_base)
				&& x_atomint(x_firstobj(x_base_field_filein(p_base))) < 0) {
			return NULL;
		}

		p_char = x_base_read(p_base, (x_obj_t *)read_args);

		if (x_obj_isnil(p_base, p_char)) {
			/* EOF latch (#90).  Terminal EOF is ONE-SHOT: ctrl-d
			 * yields a single zero-length read and the next read
			 * blocks -- but the tokenizer scores every type x
			 * analyser, each re-observing EOF, so it needs EOF to
			 * be STICKY (as it is on pipes and files).  Latch the
			 * CURRENT filein cell to the fd's bitwise complement
			 * (negative for every valid fd; stdin's ~0 is the old
			 * -1): every later read fails instantly through the
			 * existing < 0 check, no new state.  The complement,
			 * not a flat -1, because the fd must stay RECOVERABLE:
			 * when the #170 read-error raise below fires with the
			 * latch already set, the guard unwind (#278) closes the
			 * cut-away include's fd out of this cell -- a destroyed
			 * value leaked the descriptor.  Scoping is the filein
			 * chain itself -- include pushes a cell per file and
			 * pops it, so an included file's latch dies with its
			 * cell and the outer stream's cell is never touched.
			 * This is the sole x_base_read call site in the tree. */
			if (x_base_isset(p_base)) {
				x_atomint(x_firstobj(x_base_field_filein(p_base)))
					= ~x_atomint(x_firstobj(x_base_field_filein(p_base)));
			}

			/* Read ERROR vs end-of-stream (#170): x_base_read wrote
			 * the raw read result into the count atom; negative
			 * means the stream did NOT end -- the read failed.
			 * Treating that as EOF silently truncated batch input
			 * at top-level form boundaries.  ONE negative case is
			 * legitimate: ctrl-c aborting a blocked read (EINTR
			 * with the sigint flag set) IS the cancel protocol --
			 * the REPL loop consumes the resulting latched EOF
			 * (lib/x/repl/loop.x), so it stays silent.  Any other
			 * failure (EAGAIN, EIO, ...) raises so it is loud.
			 * The latch above stays deliberate in both cases: a
			 * PERSISTENT error (e.g. tty EIO after hangup) must
			 * not re-raise from every retry under a guard loop --
			 * after the raise, later reads report plain EOF and
			 * the stream ends. */
			if (x_atomint((x_obj_t *)int_obj) < 0) {
				p_sigint = x_base_isset(p_base)
					? x_firstobj(x_eval_field_sigint(p_base)) : NULL;
				if (p_sigint == NULL
						|| *(volatile x_int_t *)&x_atomint(p_sigint) == 0) {
					x_eval_error(p_base, (x_char_t *)"Input read failed", NULL);
				}
			}

			return NULL;
		}

		x_type_buffer_append(p_base, (x_obj_t *)append_args);
	}

	x_bufferread(p_buffer) += 1;

	/* Track line numbers for error reporting. */
	if (x_bufferlastchar(p_buffer) == '\n' && x_base_isset(p_base)) {
		x_atomint(x_firstobj(x_eval_field_line(p_base))) += 1;
	}

	return p_buffer;
}

/**
 * Read one text character, treating NUL as EOF.
 *
 * Delegates to x_type_buffer_read() and returns NULL if the result
 * is NULL or the last character read was @c '\\0'.
 *
 * @param p_base  Base (execution context).
 * @param p_args  Argument list whose first element is the buffer.
 * @return The buffer object after advancing, or NULL on EOF/NUL.
 *
 * @see x_type_buffer_read
 */
x_obj_t *x_type_buffer_read_text(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer;

	p_buffer = x_type_buffer_read(p_base, p_args);

	if (x_obj_isnil(p_base, p_buffer) || '\0' == x_bufferlastchar(p_buffer)) {
		return NULL;
	}

	return p_buffer;
}
