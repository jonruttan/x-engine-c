/**
 * @file list.c
 * @brief S-expression analyser, reader, writer, and display for list literals.
 * @author Jon Ruttan (jonruttan@gmail.com)
 * @copyright 2023 Jon Ruttan
 * @license MIT No Attribution (MIT-0)
 */
/*
 *     ., .,
 *     {O,O}
 *     (   )
 *      " "
 */

#include "x-type/list.h"
#include "x-type/buffer.h"
#include "x-type/symbol.h"
#include "x-interp.h"
#include "x-token.h"
#include "x-token.h"
#include "x-token/sexp/list.h"
#include "x-token/sexp/whitespace.h"

/**
 * Check if a list matches (name X) and write shorthand prefix + X.
 *
 * Tests whether @p p_obj is a two-element list whose first element is
 * a symbol matching one of quasi/unquote/unquote-splicing, and if so
 * outputs the corresponding reader shorthand (` , ,@) followed by the
 * second element.
 *
 * @param p_base    Execution context.
 * @param p_obj     The list to check.
 * @param dispatch  Output function (x_token_write or x_token_display).
 * @return 1 if shorthand was emitted, 0 otherwise.
 */
static int x_sexp_list_write_quasi_shorthand(x_obj_t *p_base,
	x_obj_t *p_obj,
	x_obj_t *(*dispatch)(x_obj_t *, x_obj_t *))
{
	x_obj_t *p_head, *p_tail;
	x_char_t *name;
	x_char_t *prefix = NULL;
	x_satom_t str = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = NULL });
	x_spair_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { str }, { NULL }),
		elem_wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL }, { NULL });

	/* Must be a proper two-element list: (sym . (val . nil)) */
	if (x_obj_isnil(p_base, p_obj) || ! x_obj_type_islist(p_base, p_obj))
		return 0;

	p_head = x_firstobj(p_obj);
	p_tail = x_restobj(p_obj);

	if (x_obj_isnil(p_base, p_head)
		|| ! x_obj_type_issymbol(p_base, p_head)
		|| x_obj_isnil(p_base, p_tail)
		|| ! x_obj_type_islist(p_base, p_tail)
		|| ! x_obj_isnil(p_base, x_restobj(p_tail)))
		return 0;

	name = x_symbolval(p_head);

	if (x_lib_strcmp(name, "quasi") == 0)
		prefix = "`";
	else if (x_lib_strcmp(name, "unquote") == 0)
		prefix = ",";
	else if (x_lib_strcmp(name, "unquote-splicing") == 0)
		prefix = ",@";

	if (prefix == NULL)
		return 0;

	/* Emit shorthand prefix and the second element. */
	x_atomstr(str) = prefix;
	x_interp_write_str(p_base, (x_obj_t *)&wrap);

	x_firstobj((x_obj_t *)elem_wrap) = x_firstobj(p_tail);
	dispatch(p_base, (x_obj_t *)&elem_wrap);

	return 1;
}

x_satom_t x_sexp_list_analyse_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_list_analyse }),
	x_sexp_list_delimit_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_list_delimit }),
	x_sexp_list_read_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_list_read }),
	x_sexp_list_write_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_list_write }),
	x_sexp_list_display_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_list_display });

/**
 * Analyse: score on any list delimiter character.
 *
 * Matches @c (, @c ), and @c . characters.  Scores the buffer length
 * immediately since list delimiters are always single-character tokens.
 *
 * @param p_base  Execution context.
 * @param p_args  Read-args containing the token buffer and score.
 * @return Score on match, or NULL.
 */
x_obj_t *x_sexp_list_analyse(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_score = x_token_read_arg_score(p_args);

	if (x_lib_strchr(X_SEXP_LIST_CHARS_STR, x_bufferlastchar(p_buffer))) {
		x_firstint(p_score) = x_bufferlen(p_buffer);
		return p_score;
	}

	return NULL;
}

/**
 * Delimiter callback for list characters.
 *
 * Backs up the read pointer when a list delimiter is encountered so
 * the current token is terminated before the delimiter.
 *
 * @param p_base  Execution context.
 * @param p_args  Read-args containing the token buffer.
 * @return The buffer on delimiter match, or NULL.
 */
x_obj_t *x_sexp_list_delimit(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	if (x_lib_strchr(X_SEXP_LIST_CHARS_STR, x_bufferlastchar(p_buffer))) {
		x_bufferread(p_buffer)--;
		return p_buffer;
	}

	return NULL;
}

/**
 * Read a list (or dotted pair) from the token stream.
 *
 * Handles three cases based on the delimiter character:
 * - @c ) -- returns the read_prim sentinel (end of list).
 * - @c . -- returns the delimit_prim sentinel (dotted pair).
 * - @c ( -- recursively reads elements via @c x_token_read until
 *   a close-paren or dot sentinel is encountered.
 *
 * @param p_base  Execution context.
 * @param p_args  Read-args containing the token buffer.
 * @return The constructed list, or a sentinel primitive.
 */
x_obj_t *x_sexp_list_read(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);
	x_char_t c = x_bufferlastchar(p_buffer);

	if (c == *X_SEXP_LIST_POST_STR) {
		return x_sexp_list_read_prim;
	}

	if (c == *X_SEXP_LIST_DOT_STR) {
		return x_sexp_list_delimit_prim;
	}

	/* '(' — read list contents. */
	{
		x_obj_t *head = NULL, *tail = NULL, *elem, *pair;
		x_spair_t read_args = x_obj_set(NULL, X_OBJ_FLAG_NONE,
			{ p_buffer }, { p_base });

		x_type_buffer_retain(p_base, (x_obj_t *)read_args);

		for (;;) {
			elem = x_token_read(p_base, (x_obj_t *)read_args);

			if (elem == (x_obj_t *)x_sexp_list_read_prim) {
				break;
			}

			if (elem == (x_obj_t *)x_sexp_list_delimit_prim) {
				x_restobj(tail) = x_token_read(p_base, (x_obj_t *)read_args);
				x_token_read(p_base, (x_obj_t *)read_args);
				break;
			}

			pair = x_mklist(p_base, elem, NULL);

			if (x_obj_isnil(p_base, head)) {
				head = pair;
				/* Root head so GC can reach the list under construction */
				x_obj_push_field(p_base, &x_interp_field_eval_list(p_base), head, X_OBJ_FLAG_NONE);
			} else {
				x_restobj(tail) = pair;
			}
			tail = pair;
		}

		/* Unroot if we rooted */
		if ( ! x_obj_isnil(p_base, head)) {
			x_obj_pop_field(p_base, &x_interp_field_eval_list(p_base));
		}

		return head;
	}
}
/**
 * Write the external representation of a list.
 *
 * Outputs parenthesised elements separated by spaces.  Improper lists
 * are rendered with dot notation (e.g. @c (a\ .\ b)).  Nil elements
 * within the list are written as @c ().
 *
 * @param p_base  Execution context.
 * @param p_args  Pair whose first element is the list to write.
 * @return The tail of the list after writing.
 */
x_obj_t *x_sexp_list_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args);
	x_satom_t str = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = NULL }),
		write_wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL });
	x_spair_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { str }, { NULL });

	if (x_sexp_list_write_quasi_shorthand(p_base, p_obj, x_token_write))
		return p_obj;

	x_atomstr(str) = X_SEXP_LIST_PRE_STR;
	x_interp_write_str(p_base, (x_obj_t *)&wrap);

	for (;;) {
		if (x_obj_isnil(p_base, x_firstobj(p_obj))) {
			x_atomstr(str) = "()";
			x_interp_write_str(p_base, (x_obj_t *)&wrap);
		} else {
			x_firstobj((x_obj_t *)write_wrap) = x_firstobj(p_obj);
			x_token_write(p_base, (x_obj_t *)write_wrap);
		}

		p_obj = x_restobj(p_obj);

		if (x_obj_isnil(p_base, p_obj)) {
			x_atomstr(str) = X_SEXP_LIST_POST_STR;
			x_interp_write_str(p_base, (x_obj_t *)&wrap);

			break;
		}

		if ( ! x_obj_type_islist(p_base, p_obj)) {
			x_atomstr(str) = X_SEXP_WHITESPACE_SPACE_STR X_SEXP_LIST_DOT_STR X_SEXP_WHITESPACE_SPACE_STR;
			x_interp_write_str(p_base, (x_obj_t *)&wrap);

			x_firstobj((x_obj_t *)write_wrap) = p_obj;
			x_token_write(p_base, (x_obj_t *)write_wrap);

			x_atomstr(str) = X_SEXP_LIST_POST_STR;
			x_interp_write_str(p_base, (x_obj_t *)&wrap);

			break;
		}

		x_atomstr(str) = X_SEXP_WHITESPACE_SPACE_STR;
		x_interp_write_str(p_base, (x_obj_t *)&wrap);
	}

	return p_obj;
}

/**
 * Display a list in human-readable form.
 *
 * Same structure as write but dispatches through @c x_token_display
 * for each element so that strings appear unquoted, etc.
 *
 * @param p_base  Execution context.
 * @param p_args  Pair whose first element is the list to display.
 * @return The tail of the list after displaying.
 */
x_obj_t *x_sexp_list_display(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_obj = x_firstobj(p_args);
	x_satom_t str = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = NULL }),
		disp_wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { NULL });
	x_spair_t wrap = x_obj_set(NULL, X_OBJ_FLAG_NONE, { str }, { NULL });

	if (x_sexp_list_write_quasi_shorthand(p_base, p_obj, x_token_display))
		return p_obj;

	x_atomstr(str) = X_SEXP_LIST_PRE_STR;
	x_interp_write_str(p_base, (x_obj_t *)&wrap);

	for (;;) {
		if (x_obj_isnil(p_base, x_firstobj(p_obj))) {
			x_atomstr(str) = "()";
			x_interp_write_str(p_base, (x_obj_t *)&wrap);
		} else {
			x_firstobj((x_obj_t *)disp_wrap) = x_firstobj(p_obj);
			x_token_display(p_base, (x_obj_t *)disp_wrap);
		}

		p_obj = x_restobj(p_obj);

		if (x_obj_isnil(p_base, p_obj)) {
			x_atomstr(str) = X_SEXP_LIST_POST_STR;
			x_interp_write_str(p_base, (x_obj_t *)&wrap);

			break;
		}

		if ( ! x_obj_type_islist(p_base, p_obj)) {
			x_atomstr(str) = X_SEXP_WHITESPACE_SPACE_STR X_SEXP_LIST_DOT_STR X_SEXP_WHITESPACE_SPACE_STR;
			x_interp_write_str(p_base, (x_obj_t *)&wrap);

			x_firstobj((x_obj_t *)disp_wrap) = p_obj;
			x_token_display(p_base, (x_obj_t *)disp_wrap);

			x_atomstr(str) = X_SEXP_LIST_POST_STR;
			x_interp_write_str(p_base, (x_obj_t *)&wrap);

			break;
		}

		x_atomstr(str) = X_SEXP_WHITESPACE_SPACE_STR;
		x_interp_write_str(p_base, (x_obj_t *)&wrap);
	}

	return p_obj;
}
