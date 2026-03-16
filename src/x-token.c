/*
 * # Computational Expressions in C
 *
 * ## x-token.c -- Implementation - Token
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
#include "x-base.h"
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

/*
 * # Tokenization Functions
 */

#define prim_arg_prim			x_0((x_obj_t *)prim_args)

x_obj_t *x_token_delimit(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_firstobj(p_args),
		*p_type = x_01(p_args),
		*p_types = x_base_field_type_alist(p_base);
	x_spair_t prim_args[1] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { p_args }),
		};

	/* Cycle through of all of the types. */
	while ( ! x_obj_isnil(p_base, p_types)) {
		prim_arg_prim = x_type_field_delimit(x_restobj(x_firstobj(p_types)));

		if (p_type != x_restobj(x_firstobj(p_types))
			&& ! x_obj_isnil(p_base, prim_arg_prim)
			&& x_type_prim_apply(p_base, (x_obj_t *)prim_args) == p_buffer
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

x_obj_t *x_type_alist_iter(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_type_list_iter(p_base, p_args);

	if (x_obj_isnil(p_base, p_obj)) {
		return p_obj;
	}

	return x_type_field_analyse(x_restobj(p_obj));
}

x_obj_t *x_token_analyse(x_obj_t *p_base, x_obj_t *p_args)
{
	x_int_t i_best, i_consumed;
	x_obj_t *p_buffer = x_firstobj(p_args), *p_winner, *p_entry, *p_analyse, *p_obj;
	x_satom_t chr = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .c = '\0' } ),
		arg_chr = x_obj_set(NULL, X_OBJ_FLAG_NONE, { .i = 0 });
	x_spair_t
		score = x_obj_set(NULL, X_OBJ_FLAG_NONE, {}),
		type_iter = x_obj_set(NULL, X_OBJ_FLAG_NONE, { x_type_list_iter_prim }, { x_base_field_type_alist(p_base) }),
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
		};
	x_obj_t *p_score = (x_obj_t *)score, *p_hint;
	x_char_t *p_bw, *p_hintstr;
	int skip;

	/* Retain: ensure bufferval == bufferread for correct x_bufferlen. */
	x_type_buffer_retain(p_base, (x_obj_t *)buffer_args);

	/* Cycle through all types, tracking the winning type entry. */
	p_winner = NULL;
	i_best = 0;

	while ( ! x_iterempty(p_base, (x_obj_t *)type_iter)) {
		p_entry = x_type_iter_next(p_base, (x_obj_t *)iter_args);

		if ( ! x_obj_isnil(p_base, p_entry)) {
		p_analyse = x_type_field_analyse(x_restobj(p_entry));

		if ( ! x_obj_isnil(p_base, p_analyse)) {

			/* First-char hint: skip x-lang closure if first char
			 * doesn't match the type's accepted characters. */
			skip = 0;
			if ( ! x_obj_type_issatom(p_analyse)
				&& ! x_buffereof(p_buffer)) {
				p_hint = x_type_field_data(x_restobj(p_entry));
				if ( ! x_obj_isnil(p_base, p_hint)) {
					p_hintstr = x_strval(p_hint);
					skip = 1;
					while (*p_hintstr) {
						if (*p_hintstr == *x_bufferread(p_buffer)) {
							skip = 0;
							break;
						}
						p_hintstr++;
					}
				}
			}

		if ( ! skip) {
			/* Clear score for this type. */
			x_firstint(p_score) = 0;

			while (1) {

				/* EOF for readonly buffers — don't reset,
				 * let auto-score compute from bufferlen. */
				if ((x_obj_flags(p_buffer) & X_OBJ_FLAG_RO)
					&& x_buffereof(p_buffer)) {
					break;
				}

				p_bw = x_bufferwrite(p_buffer);
				if (x_obj_isnil(p_base, x_type_buffer_read_text(p_base, (x_obj_t *)read_args))
					&& x_bufferwrite(p_buffer) == p_bw) {
					x_bufferread(p_buffer) = x_bufferval(p_buffer);
					break;
				}

				x_atomint(arg_chr) = (x_int_t)x_bufferlastchar(p_buffer);
				prim_arg_prim = p_analyse;
				p_obj = x_type_prim_apply(p_base, (x_obj_t *)prim_args);

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
			}

			/* EOF auto-score: if chars were consumed and a score
			 * was set (via set-first-int side effect on first
			 * match), use the sign from the partial score to
			 * compute final score from total consumed. */
			i_consumed = x_bufferlen(p_buffer);
			if (i_consumed > 0
				&& x_firstint(p_score) != 0) {
				x_int_t i_score = (x_firstint(p_score) < 0 ? -1 : 1)
					* i_consumed;

				if (i_score >= i_best
					|| (i_best < 1
						&& i_score <= i_best)) {
					i_best = i_score;
					p_winner = p_entry;
				}
			}
			x_bufferread(p_buffer) = x_bufferval(p_buffer);
		} /* if ( ! skip) */
		}
		}
	}

	x_bufferread(p_buffer) = x_bufferread(p_buffer) + x_lib_abs(i_best);

	return p_winner;
}

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
		if (x_obj_meta_extra > 0
				&& (x_obj_flags(p_buffer) & X_OBJ_FLAG_EXT)) {
			for (p_scan = x_bufferval(p_buffer);
					p_scan < x_bufferread(p_buffer); p_scan++) {
				if (*p_scan == '\n')
					x_obj_meta_slot(p_buffer, 0).i++;
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
		if (x_obj_meta_extra > 0
				&& (x_obj_flags(p_buffer) & X_OBJ_FLAG_EXT)) {
			line = x_obj_meta_slot(p_buffer, 0).i;
		}

		prim_arg_prim = p_read;
		p_obj = x_type_prim_apply(p_base, (x_obj_t *)prim_args);

		if (x_obj_isnil(p_base, p_obj)) {
			return NULL;
		}

		/* Stamp line number on created object. */
		if (x_obj_meta_extra > 0
				&& !x_obj_isnil(p_base, p_obj)
				&& (x_obj_flags(p_obj) & X_OBJ_FLAG_EXT)) {
			x_obj_meta_slot(p_obj, 0).i = line;
		}

		x_type_buffer_retain(p_base, (x_obj_t *)buffer_args);
		return p_obj;
	}
}

x_obj_t *x_token_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args);

	if (x_obj_isnil(p_base, p_obj)) {
		x_satom_t nil_str = x_obj_set(x_type_atom_obj,
			X_OBJ_FLAG_NONE, { .s = (x_char_t *)"()" });
		x_spair_t nil_args[1] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { nil_str }, { NULL })
		};
		x_base_write_str(p_base, (x_obj_t *)nil_args);
		return NULL;
	}

	if (p_obj == x_base_field_true(p_base)
			|| p_obj == x_base_field_false(p_base)) {
		x_satom_t bool_str = x_obj_set(x_type_atom_obj,
			X_OBJ_FLAG_NONE, { .s = x_atomstr(p_obj) });
		x_spair_t bool_args[1] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { bool_str }, { NULL })
		};
		x_base_write_str(p_base, (x_obj_t *)bool_args);
		return NULL;
	}

	if (x_obj_type_issatom(p_obj)) {
		return x_sexp_atom_write(p_base, p_args);
	}

	if (x_obj_type_isspair(p_obj)) {
		return x_sexp_pair_write(p_base, p_args);
	}

	if ( ! x_obj_isnil(p_base, x_obj_type(p_obj))) {
		return x_type_write(p_base, p_args);
	}

	return NULL;
}

x_obj_t *x_token_display(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args);

	if (x_obj_isnil(p_base, p_obj)) {
		return NULL;
	}

	if (p_obj == x_base_field_true(p_base)
			|| p_obj == x_base_field_false(p_base)) {
		x_satom_t bool_str = x_obj_set(x_type_atom_obj,
			X_OBJ_FLAG_NONE, { .s = x_atomstr(p_obj) });
		x_spair_t bool_args[1] = {
			x_obj_set(NULL, X_OBJ_FLAG_NONE, { bool_str }, { NULL })
		};
		x_base_write_str(p_base, (x_obj_t *)bool_args);
		return NULL;
	}

	if (x_obj_type_issatom(p_obj)) {
		return x_sexp_atom_write(p_base, p_args);
	}

	if (x_obj_type_isspair(p_obj)) {
		return x_sexp_pair_write(p_base, p_args);
	}

	if ( ! x_obj_isnil(p_base, x_obj_type(p_obj))) {
		return x_type_display(p_base, p_args);
	}

	return NULL;
}
