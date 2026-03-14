/*
 * # Computational Expressions in C
 *
 * ## x-type.c -- Implementation - Type - String
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
#include "x-token/sexp/str.h"
#include "x-base.h"
#include "x-token.h"
#include "x-type/str.h"
#include "x-type/buffer.h"

/* Forward declaration for escape state */

x_satom_t x_sexp_str_analyse1_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_str_analyse1 }),
	x_sexp_str_analyse2_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_str_analyse2 }),
	x_sexp_str_analyse3_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_str_analyse3 }),
	x_sexp_str_read_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_str_read }),
	x_sexp_str_write_prim = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .fn = x_sexp_str_write });

/* analyse3 prim: escape state — consumes one char after backslash */

/* hex_digit: convert hex character to integer value, or -1 */
static int hex_digit(x_char_t c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

x_obj_t *x_sexp_str_analyse1(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);

	if (0 == x_lib_strncmp(X_SEXP_STR_PRE_STR, x_bufferval(p_buffer), X_SEXP_STR_PRE_STR_LEN)) {
		return x_sexp_str_analyse2_prim;
	}

	return NULL;
}

x_obj_t *x_sexp_str_analyse2(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args),
		*p_score = x_token_read_arg_score(p_args);

	/* Backslash: enter escape state — next char consumed unconditionally */
	if (*(x_bufferread(p_buffer) - 1) == '\\') {
		return x_sexp_str_analyse3_prim;
	}

	/* Closing quote: match */
	if (0 == x_lib_strncmp(X_SEXP_STR_POST_STR,
			x_bufferread(p_buffer) - X_SEXP_STR_PRE_STR_LEN,
			X_SEXP_STR_POST_STR_LEN)) {
		x_firstint(p_score) = x_bufferlen(p_buffer);
		x_restobj(p_score) = x_sexp_str_read_prim;
		return p_score;
	}

	return x_sexp_str_analyse2_prim;
}

/* analyse3: escape state — consume one character, return to normal reading */
x_obj_t *x_sexp_str_analyse3(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_sexp_str_analyse2_prim;
}

x_obj_t *x_sexp_str_read(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_buffer = x_token_read_arg_buffer(p_args);
	x_char_t *raw = x_bufferval(p_buffer) + X_SEXP_STR_PRE_STR_LEN;
	size_t len = x_bufferlen(p_buffer) - X_SEXP_STR_PRE_STR_LEN
		- X_SEXP_STR_POST_STR_LEN;
	x_char_t *s = x_lib_strndup(raw, len);
	size_t i, j;

	/* Unescape in-place (result is always <= source length) */
	for (i = 0, j = 0; i < len; i++, j++) {
		if (s[i] == '\\' && i + 1 < len) {
			i++;
			switch (s[i]) {
			case '"':  s[j] = '"';  break;
			case '\\': s[j] = '\\'; break;
			case 'n':  s[j] = '\n'; break;
			case 't':  s[j] = '\t'; break;
			case 'r':  s[j] = '\r'; break;
			case '0':  s[j] = '\0'; break;
			case 'x':
				if (i + 2 < len
					&& hex_digit(s[i + 1]) >= 0
					&& hex_digit(s[i + 2]) >= 0) {
					s[j] = (x_char_t)(
						hex_digit(s[i + 1]) * 16
						+ hex_digit(s[i + 2]));
					i += 2;
				} else {
					s[j++] = '\\';
					s[j] = 'x';
				}
				break;
			default:
				s[j++] = '\\';
				s[j] = s[i];
				break;
			}
		} else {
			s[j] = s[i];
		}
	}
	s[j] = '\0';

	return x_mkstrown(p_base, s);
}

x_obj_t *x_sexp_str_write(x_obj_t *p_base, x_obj_t *p_args)
{
	x_char_t *s = x_firststr(x_firstobj(p_args));
	size_t len = x_lib_strlen(s);
	size_t i = 0;
	x_char_t hex[5];
	const x_char_t *esc;
	size_t run_start;
	x_satom_t data = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .s = NULL }),
		sz = x_obj_set(x_type_atom_obj, X_OBJ_FLAG_NONE, { .i = 0 });
	x_spair_t args[2] = {
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { data }, { (x_obj_t *)(args + 1) }),
		x_obj_set(NULL, X_OBJ_FLAG_NONE, { sz }, { NULL })
	};

	x_atomstr(data) = X_SEXP_STR_PRE_STR;
	x_atomint(sz) = X_SEXP_STR_PRE_STR_LEN;
	x_base_write(p_base, (x_obj_t *)args);

	while (i < len) {
		/* Find next character that needs escaping */
		run_start = i;
		while (i < len) {
			x_char_t c = s[i];
			if (c == '"' || c == '\\' || c == '\n' || c == '\t'
				|| c == '\r' || c == '\0'
				|| ((unsigned char)c < 0x20
					&& c != '\n' && c != '\t' && c != '\r')) {
				break;
			}
			i++;
		}

		/* Write run of safe characters */
		if (i > run_start) {
			x_atomstr(data) = s + run_start;
			x_atomint(sz) = i - run_start;
			x_base_write(p_base, (x_obj_t *)args);
		}

		/* Write escape sequence */
		if (i < len) {
			esc = NULL;
			switch (s[i]) {
			case '"':  esc = "\\\""; break;
			case '\\': esc = "\\\\"; break;
			case '\n': esc = "\\n";  break;
			case '\t': esc = "\\t";  break;
			case '\r': esc = "\\r";  break;
			default:
				/* Non-printable: \xHH */
				hex[0] = '\\';
				hex[1] = 'x';
				hex[2] = "0123456789abcdef"[((unsigned char)s[i] >> 4) & 0xf];
				hex[3] = "0123456789abcdef"[(unsigned char)s[i] & 0xf];
				hex[4] = '\0';
				esc = hex;
				break;
			}
			x_atomstr(data) = (x_char_t *)esc;
			x_atomint(sz) = x_lib_strlen(esc);
			x_base_write(p_base, (x_obj_t *)args);
			i++;
		}
	}

	x_atomstr(data) = X_SEXP_STR_POST_STR;
	x_atomint(sz) = X_SEXP_STR_POST_STR_LEN;
	x_base_write(p_base, (x_obj_t *)args);

	return x_firstobj(p_args);
}
