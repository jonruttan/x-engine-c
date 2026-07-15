/** @file x-token.c
 *  @brief Tokenizer: analysis, reading, writing, and display of expressions.
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
/*
 * # Includes
 */
#include "x-eval.h"
#include "x-heap.h"
#include "x-obj.h"
#include "x-token.h"
#include "x-type.h"
#include "x-type/buffer.h"
#include "x-type/char.h"
#include "x-type/int.h"
#include "x-type/iter.h"
#include "x-type/prim.h"
#include "x-type/str.h"
#include "x-type/list.h"
#include "x-token/sexp/atom.h"
#include "x-token/sexp/pair.h"

#define prim_arg_prim			x_0((x_obj_t *)prim_args)

/**
 * Check whether any type's delimiter matches at the current buffer position.
 *
 * Iterates all registered types (except the given type itself) and
 * calls each type's delimit hook. Returns the buffer if a delimiter
 * matched, NULL otherwise.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- (buffer . type) where type is excluded from checks
 * @return x_obj_t* -- The buffer if a delimiter matched, or NULL
 */
x_obj_t *x_token_delimit(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_firstobj(p_args),
		*p_type = x_01(p_args),
		*p_types = x_firstobj(x_eval_field_type_alist(p_base));
	x_spair_t prim_args[1] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { p_args }),
		};

	/* Cycle through of all of the types. */
	while ( ! x_obj_isnil(p_base, p_types)) {
		prim_arg_prim = x_type_field_delimit(x_restobj(x_firstobj(p_types)));

		if (p_type != x_restobj(x_firstobj(p_types))
			&& ! x_obj_isnil(p_base, prim_arg_prim)
			&& x_callable_apply(p_base, (x_obj_t *)prim_args) == p_buffer
		) {
			return p_buffer;
		}

		p_types = x_restobj(p_types);
	}

	return NULL;
}

extern x_satom_t x_type_list_iter_prim;

x_obj_t *x_type_alist_iter(x_obj_t *p_base, x_obj_t *p_args);
x_satom_t x_type_alist_iter_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_type_alist_iter });

/**
 * Iterator that yields analyse hooks from type alist entries.
 *
 * Wraps x_type_list_iter to advance through the type alist, then
 * extracts the analyse field from each entry's type struct.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- Iterator state pair
 * @return x_obj_t* -- Analyse hook from the next type entry, or NULL at end
 */
x_obj_t *x_type_alist_iter(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_type_list_iter(p_base, p_args);

	if (x_obj_isnil(p_base, p_obj)) {
		return p_obj;
	}

	return x_type_field_analyse(x_restobj(p_obj));
}

/**
 * Determine which type best matches the next token in the buffer.
 *
 * Iterates all registered types, calling each type's analyse hook
 * character by character. Tracks the winning type by score (number
 * of buffer characters consumed). Uses first-char hint strings on
 * x-lang closures to skip non-matching types early. The >= comparison
 * means later types (C built-ins) win ties against earlier (custom)
 * types.
 *
 * After finding the winner, advances the buffer read pointer by the
 * winning score.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- (buffer . base) pair
 * @return x_obj_t* -- Winning type alist entry (name . type-struct),
 *                      or NULL if no type matched
 *
 * @note Negative scores indicate inverse-priority matches (e.g.
 *       whitespace). The absolute value determines advancement.
 */
x_obj_t *x_token_analyse(x_obj_t *p_base, x_obj_t *p_args)
{
	x_int_t i_best, i_consumed;
	x_obj_t *p_buffer = x_firstobj(p_args), *p_winner, *p_entry, *p_analyse, *p_analyse_slot, *p_obj;
	x_satom_t chr = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .c = '\0' } ),
		arg_chr = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = 0 });
	x_spair_t
		score = x_obj_set(NULL, X_OBJ_FLAG_NONE, {}),
		type_iter = x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_list_iter_prim }, { x_firstobj(x_eval_field_type_alist(p_base)) }),
		iter_args[2] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { type_iter }, { (x_obj_t *)(iter_args + 1) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL }),
		},
		buffer_args[3] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_buffer }, { (x_obj_t *)(buffer_args + 1) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { score }, { (x_obj_t *)(buffer_args + 2) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { arg_chr }, { NULL }),
		},
		prim_args[1] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { (x_obj_t *)(buffer_args) }),
		},
		read_args[2] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_buffer }, { (x_obj_t *)(read_args + 1) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { chr }, { NULL }),
		},
		/* an_iter walks the analyse slot -- the slot's list itself, or
		 * an_one (a one-element list wrapping a lone handler) so the
		 * per-handler loop stays uniform.  Its state is set per type. */
		an_one = x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL }),
		an_iter = x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_list_iter_prim }, { NULL }),
		an_args[2] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { an_iter }, { (x_obj_t *)(an_args + 1) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL }),
		},
		analyse_root = x_obj_set((x_obj_t *)x_type_pair_obj, X_OBJ_FLAG_NONE,
			{ NULL }, { NULL });
	x_obj_t *p_score = (x_obj_t *)score;
	x_obj_t **p_cell = x_heap_root_cell(p_base);
	x_char_t *p_bw;

	/* Root the active analyse handler: the replace-analyser protocol
	 * below can hand this frame a freshly allocated handler whose only
	 * reference is p_analyse, held across further handler applies.  The
	 * cell mirrors p_analyse at both assignment sites. */
	x_heap_root_push(p_cell, analyse_root);

	/* Retain: ensure bufferval == bufferread for correct x_bufferlen. */
	x_type_buffer_retain(p_base, (x_obj_t *)buffer_args);

	/* Cycle through all types, tracking the winning type entry. */
	p_winner = NULL;
	i_best = 0;

	while ( ! x_iterempty(p_base, (x_obj_t *)type_iter)) {
		p_entry = x_type_iter_next(p_base, (x_obj_t *)iter_args);

		if (x_obj_isnil(p_base, p_entry)) {
			continue;
		}

		p_analyse_slot = x_type_field_analyse(x_restobj(p_entry));

		if (x_obj_isnil(p_base, p_analyse_slot)) {
			continue;
		}

		/* Score every handler in the analyse slot.  A list is walked
		 * directly; a lone handler is wrapped in an_one so the walk
		 * stays uniform -- this lets the quote/quasi/unquote readers
		 * live on the symbol type next to the symbol reader. */
		x_firstobj((x_obj_t *)an_one) = p_analyse_slot;
		x_restobj((x_obj_t *)an_iter) = x_obj_type_islist(p_base, p_analyse_slot)
			? p_analyse_slot : (x_obj_t *)an_one;

		while ( ! x_iterempty(p_base, (x_obj_t *)an_iter)) {
			p_analyse = x_type_iter_next(p_base, (x_obj_t *)an_args);
			x_firstobj((x_obj_t *)analyse_root) = p_analyse;

			/* Clear score for this handler. */
			x_firstint(p_score) = 0;

			for (;;) {

				/* EOF for readonly buffers — don't reset,
				 * let auto-score compute from bufferlen. */
				if ((x_obj_flags(p_buffer) & X_OBJ_FLAG_RO) && x_buffereof(p_buffer)) {
					break;
				}

				p_bw = x_bufferwrite(p_buffer);

				if (x_obj_isnil(p_base, x_type_buffer_read_text(p_base, (x_obj_t *)read_args))
					&& x_bufferwrite(p_buffer) == p_bw)
				{
					x_bufferread(p_buffer) = x_bufferval(p_buffer);

					break;
				}

				x_atomint(arg_chr) = (x_int_t)x_bufferlastchar(p_buffer);
				prim_arg_prim = p_analyse;
				p_obj = x_callable_apply(p_base, (x_obj_t *)prim_args);

				/* Not recognized. */
				if (x_obj_isnil(p_base, p_obj)) {
					x_bufferread(p_buffer) = x_bufferval(p_buffer);

					break;
				}

				if (p_obj == (x_obj_t *)buffer_args) {
					continue;
				}

				if (p_obj == p_score) {
					/* If this is the longest match, mark this type as best. */
					if (x_firstint(p_score) >= i_best || (i_best < 1 && x_firstint(p_score) <= i_best)) {
						i_best = x_firstint(p_score);
						p_winner = p_entry;
					}

					x_bufferread(p_buffer) = x_bufferval(p_buffer);

					break;
				}

				/* Replace Analyser. */
				p_analyse = p_obj;
				x_firstobj((x_obj_t *)analyse_root) = p_analyse;
			}

			/* EOF auto-score: if chars were consumed and a score
			 * was set (via set-first-int side effect on first
			 * match), use the sign from the partial score to
			 * compute final score from total consumed. */
			i_consumed = x_bufferlen(p_buffer);

			if (i_consumed > 0 && x_firstint(p_score) != 0) {
				x_int_t i_score = (x_firstint(p_score) < 0 ? -1 : 1) * i_consumed;

				if (i_score >= i_best || (i_best < 1 && i_score <= i_best)) {
					i_best = i_score;
					p_winner = p_entry;
				}
			}

			x_bufferread(p_buffer) = x_bufferval(p_buffer);
		}
	}

	x_bufferread(p_buffer) = x_bufferread(p_buffer) + x_lib_abs(i_best);

	x_heap_root_pop(p_cell);

	return p_winner;
}

/**
 * Read the next token from the buffer and return its parsed object.
 *
 * Calls x_token_analyse to identify the token type, counts newlines
 * in the consumed region for line tracking, then invokes the winning
 * type's read hook. Types with no read hook (e.g. whitespace) are
 * discarded and the loop retries. Stamps source line numbers on
 * created objects when meta tracking is enabled.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- (buffer . base) pair
 * @return x_obj_t* -- Parsed object, or NULL on EOF / read failure
 */
x_obj_t *x_token_read(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_firstobj(p_args), *p_entry, *p_read, *p_obj;
	x_char_t *p_scan;
	x_int_t line;
	x_spair_t buffer_args[3] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_buffer }, { (x_obj_t *)(buffer_args + 1) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL }),
		},
		prim_args[1] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { (x_obj_t *)(buffer_args) }),
		},
		/* read_iter walks the read slot -- the slot's list, or read_one
		 * (wrapping a lone reader) so the loop is uniform.  State is set
		 * per token below. */
		read_one = x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL }),
		read_iter = x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_list_iter_prim }, { NULL }),
		read_args[2] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { read_iter }, { (x_obj_t *)(read_args + 1) }),
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL }),
		};

	for (;;) {
		p_entry = x_token_analyse(p_base, p_args);

		if (x_obj_isnil(p_base, p_entry)) {
			return NULL;
		}

		/* Count newlines in consumed region for line tracking.
		 * After x_token_analyse, bufferval..bufferread is the
		 * consumed token text. Track independently of the base
		 * line counter (which gets inflated by analysis rescans). */
		if (x_atomint(x_firstobj(x_base_field_obj_meta_extra(p_base))) > 0
				&& (x_obj_flags(p_buffer) & X_OBJ_FLAG_META)) {
			for (p_scan = x_bufferval(p_buffer);
					p_scan < x_bufferread(p_buffer); p_scan++) {
				if (*p_scan == '\n')
					x_obj_meta_i(p_buffer, 0).i++;
			}
		}

		p_read = x_type_field_read(x_restobj(p_entry));

		if (x_obj_isnil(p_base, p_read)) {
			/* Discard (null reader): retain buffer, fetch another. */
			x_type_buffer_retain(p_base, (x_obj_t *)buffer_args);
			continue;
		}

		/* Save line before read (read may advance buffer via
		 * recursive x_token_read calls for nested lists). */
		line = 0;
		if (x_atomint(x_firstobj(x_base_field_obj_meta_extra(p_base))) > 0
				&& (x_obj_flags(p_buffer) & X_OBJ_FLAG_META)) {
			line = x_obj_meta_i(p_buffer, 0).i;
		}

		/* Walk the read slot's reader(s) and take the first non-nil
		 * result.  A list is walked directly; a lone reader is wrapped in
		 * read_one so the walk is uniform.  A reader declines a token it
		 * does not handle by returning nil WITHOUT consuming, so the next
		 * reader sees the same buffer. */
		x_firstobj((x_obj_t *)read_one) = p_read;
		x_restobj((x_obj_t *)read_iter) = x_obj_type_islist(p_base, p_read)
			? p_read : (x_obj_t *)read_one;

		p_obj = NULL;
		while ( ! x_iterempty(p_base, (x_obj_t *)read_iter)) {
			prim_arg_prim = x_type_iter_next(p_base, (x_obj_t *)read_args);
			p_obj = x_callable_apply(p_base, (x_obj_t *)prim_args);

			if ( ! x_obj_isnil(p_base, p_obj)) {
				break;
			}
		}

		if (x_obj_isnil(p_base, p_obj)) {
			return NULL;
		}

		/* Stamp line number on created object. */
		if (x_atomint(x_firstobj(x_base_field_obj_meta_extra(p_base))) > 0
				&& !x_obj_isnil(p_base, p_obj)
				&& (x_obj_flags(p_obj) & X_OBJ_FLAG_META)) {
			x_obj_meta_i(p_obj, 0).i = line;
		}

		x_type_buffer_retain(p_base, (x_obj_t *)buffer_args);

		return p_obj;
	}
}

/**
 * Write an object in its external (machine-readable) representation.
 *
 * Dispatches based on object type: nil prints "()", booleans print
 * their atom string (#t/#f), stack atoms and pairs use sexp writers,
 * and heap-typed objects delegate to their type's write hook.
 *
 * @param p_base  x_obj_t* -- Execution context
 * @param p_args  x_obj_t* -- (object)
 * @return x_obj_t* -- Write result, or NULL
 */
