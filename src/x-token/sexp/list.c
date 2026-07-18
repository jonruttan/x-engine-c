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
#include "x-eval.h"
#include "x-heap.h"
#include "x-token.h"
#include "x-token.h"
#include "x-token/sexp/list.h"
#include "x-token/sexp/whitespace.h"

/** Analyser / delimiter / reader primitive satoms for the list type. */
x_satom_t x_sexp_list_analyse_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_list_analyse }),
	x_sexp_list_delimit_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_list_delimit }),
	x_sexp_list_read_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_list_read });

/**
 * Analyse: score on any list delimiter character.
 *
 * Matches @c (, @c ), and @c . characters.  Scores the buffer length
 * immediately since list delimiters are always single-character tokens.
 *
 * @param p_base  Base (execution context).
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
 * @param p_base  Base (execution context).
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
 * @param p_base  Base (execution context).
 * @param p_args  Read-args containing the token buffer.
 * @return The constructed list, or a sentinel primitive.
 */
x_obj_t *x_sexp_list_read(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);
	x_char_t c = x_bufferlastchar(p_buffer);
	x_obj_t *head = NULL, *tail = NULL, *elem, *pair;
	x_obj_t **p_cell = x_heap_root_cell(p_base);
	x_spair_t read_args = x_obj_set(NULL, X_OBJ_FLAG_NONE,
		{ p_buffer }, { p_base }),
		root = x_obj_set((x_obj_t *)x_type_pair_obj, X_OBJ_FLAG_NONE,
			{ NULL }, { NULL });

	if (c == *X_SEXP_LIST_POST_STR) {
		return x_sexp_list_read_prim;
	}

	if (c == *X_SEXP_LIST_DOT_STR) {
		return x_sexp_list_delimit_prim;
	}

	/* '(' — read list contents.  Root the list under construction: each
	 * nested x_token_read runs reader code that can collect. */
	x_heap_root_push(p_cell, root);
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
			x_firstobj((x_obj_t *)root) = head;
		} else {
			x_restobj(tail) = pair;
		}
		tail = pair;
	}

	x_heap_root_pop(p_cell);

	return head;
}
