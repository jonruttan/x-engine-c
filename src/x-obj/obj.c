/*
 * # Computational Expressions in C
 *
 * ## x-obj/obj.c -- Implementation - Objects - Base Overrides
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
#include "x-obj.h"
#include "x-base.h"


x_satom_t x_type_base_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, {.s = (x_char_t *)"BASE"});

/*
 * # Object Functions
 */
int x_obj_isnil(x_obj_t *p_base, x_obj_t *p_obj)
{
	return p_obj == NULL;
}

x_obj_t *x_obj_alloc(x_obj_t *p_base, x_obj_t *p_type, x_obj_flag_t flags, size_t units)
{
	x_obj_t *p_obj;

	p_obj = (x_obj_t *)x_sys_malloc(sizeof(x_obj_t) * (X_OBJ_META_LEN + units));

	if (p_obj == NULL) {
		return NULL;
	}

	x_obj_type(p_obj) = p_type;
	x_obj_flags(p_obj) = flags;

#ifdef X_HEAP
	if (p_base) {
		x_obj_heap(p_obj) = x_obj_heap(p_base);
		x_obj_heap(p_base) = p_obj;
		if (x_base_isset(p_base)
			&& x_restobj(x_restobj(x_restobj(x_firstobj(p_base)))) != NULL
			&& x_base_field_profile(p_base) != NULL)
			x_atomint(x_base_field_profile_allocs(p_base))++;
	} else {
		x_obj_heap(p_obj) = NULL;
	}
#endif /* X_HEAP */

	return p_obj;
}


#ifdef DEBUG

void _x_obj_debug_va(char *file, long unsigned line, x_obj_t *p_base, char *fmt, va_list ap)
{
	int fd = x_base_isset(p_base) ? x_atomint(x_base_field_fileerr(p_base)) : STDERR_FILENO;

	_x_debug_va(file, line, fd, fmt, ap);
}

void _x_obj_debug(char *file, long unsigned line, x_obj_t *p_base, char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_x_obj_debug_va(file, line, p_base, fmt, ap);
	va_end(ap);
}

void _x_obj_dump(char *file, long unsigned line, x_obj_t *p_base, x_obj_t *p_obj, char *msg)
{
	x_char_t *type = x_obj_type_name(p_base, p_obj);
	char data_buffer[X_OBJ_DUMP_BUFFER_SIZE], *s = "",
		flag_buffer[(sizeof(x_obj_flag_t) << 3) + 1], *flags = "-";

	if (p_obj != NULL) {
		/* Convert object flags to a string and skip leading zeros */
		flags = x_lib_strchr(x_lib_inttostr(x_obj_flags(p_obj), flag_buffer, 2), '1');

		if (flags == NULL) {
			flags = flag_buffer + x_lib_strlen(flag_buffer) - 1;
		}
	}


	if ( ! (x_obj_isnil(p_base, p_obj) || x_obj_isnil(p_base, x_obj_type(p_obj)))) {
		s = data_buffer;
		s += sprintf(s, ":[");

		if (x_obj_type_isspair(p_obj)) {
			if (x_obj_isnil(p_base, x_firstobj(p_obj))) {
				s += sprintf(s, X_TYPE_NIL_NAME);
			} else {
				s += sprintf(s, "0x%"X_INT_STR_PRINTF_CONV"x", x_atomint(x_firstobj(p_obj)));
			}

			if (x_obj_isnil(p_base, x_restobj(p_obj))) {
				s += sprintf(s, ", "X_TYPE_NIL_NAME);
			} else {
				s += sprintf(s, ", 0x%"X_INT_STR_PRINTF_CONV"x", x_atomint(x_restobj(p_obj)));
			}
		} else {
			s += sprintf(s, "0x%"X_INT_STR_PRINTF_CONV"x", x_atomint(p_obj));
		}

		s += sprintf(s, "]");
		s = data_buffer;
	}

	_x_obj_debug(file, line, p_base, "%s[%s:%s][%p] "
#ifdef X_HEAP
		"HEAP[%p]"
#endif /* X_HEAP */
		"%s"
		, msg ? (char *)msg : ""
		, type
		, flags
		, p_obj
#ifdef X_HEAP
		, p_obj ? x_obj_heap(p_obj) : NULL
#endif /* X_HEAP */
		, s
	);
}

#else /* DEBUG */
#endif /* DEBUG */
