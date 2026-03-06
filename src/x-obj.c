/*
 * # Computational Expressions in C
 *
 * ## x-obj.c -- Implementation - Objects
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
#include "x-type.h"


x_satom_t x_type_atom_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, {.s = (x_char_t *)X_TYPE_ATOM_NAME}),
	x_type_pair_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, {.s = (x_char_t *)X_TYPE_PAIR_NAME}),
	x_type_units_atom_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, {.i = X_OBJ_UNITS_ATOM}),
	x_type_units_pair_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, {.i = X_OBJ_UNITS_PAIR}),
	x_type_length_atom_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, {.i = X_OBJ_LENGTH_ATOM}),
	x_type_length_pair_obj = x_obj_set(NULL, X_OBJ_FLAG_NONE, {.i = X_OBJ_LENGTH_PAIR});

/*
 * # Object Functions
 */
int x_obj_isnil(x_obj_t *p_base, x_obj_t *p_obj)
{
	return p_obj == p_base || p_obj == NULL;
}

x_obj_t *x_obj_alloc(x_obj_t *p_base, x_obj_t *p_type, x_obj_flag_t flags, size_t units)
{
	x_obj_t *p_obj;

	p_obj = (x_obj_t *)x_sys_malloc(sizeof(x_obj_t) * (X_OBJ_META_LEN + units));

	if (p_obj == NULL) {
		return p_base;
	}

	x_obj_type(p_obj) = p_type;
	x_obj_flags(p_obj) = flags;

#ifdef X_GC
	if (p_base) {
		x_obj_gc(p_obj) = x_obj_gc(p_base);
		x_obj_gc(p_base) = p_obj;
	} else {
		x_obj_gc(p_obj) = NULL;
	}
#endif /* X_GC */

	return p_obj;
}

x_obj_t *x_obj_make_va(x_obj_t *p_base, x_obj_t *p_type, x_obj_flag_t flags, size_t units, va_list ap)
{
	x_obj_t *p_obj = x_obj_alloc(p_base, p_type, flags, units);
	x_obj_t **p = &x_firstobj(p_obj);

	for (; units--; *p++ = va_arg(ap, x_obj_t *));

	return p_obj;
}

x_obj_t *x_obj_make(x_obj_t *p_base, x_obj_t *p_type, x_obj_flag_t flags, size_t units, ...)
{
	x_obj_t *p_obj;
	va_list ap;

	va_start(ap, units);
	p_obj = x_obj_make_va(p_base, p_type, flags, units, ap);
	va_end(ap);

	return p_obj;
}

void x_obj_free(x_obj_t *p_obj)
{
	if (x_obj_flags(p_obj) & X_OBJ_FLAG_OWN) {
		x_sys_free(x_firstobj(p_obj));
	}

	x_sys_free(p_obj);
}

x_obj_t *x_obj_prim_type_name(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_name, *p_obj;

	if (x_obj_isnil(p_base, p_args) || x_obj_isnil(p_base, (p_obj = x_firstobj(p_args)))) {
		return p_base;
	}

	if (x_obj_type_issatom(p_obj)
			|| x_obj_type_isspair(p_obj)
			|| x_obj_isnil(p_base, x_obj_type(p_obj))) {
		return x_obj_type(p_obj);
	}

	p_name = x_type_field_name(x_obj_type(p_obj));

	if (x_obj_isnil(p_base, p_name)) {
		return p_base;
	}

	return p_name;
}

x_char_t *x_obj_type_name(x_obj_t *p_base, x_obj_t *p_obj)
{
	x_obj_t *p_name;
	x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_obj }, { p_base });

	p_name = x_obj_prim_type_name(p_base, (x_obj_t *)args);

	if (x_obj_isnil(p_base, p_name)) {
		return X_TYPE_NIL_NAME;
	}

	return x_atomstr(p_name);
}

x_obj_t *x_atom_prim_units(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_type_units_atom_obj;
}

x_obj_t *x_pair_prim_units(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_type_units_pair_obj;
}

x_obj_t *x_obj_prim_units(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_units, *p_obj;

	if (x_obj_isnil(p_base, p_args) || x_obj_isnil(p_base, (p_obj = x_firstobj(p_args)))) {
		return p_base;
	}

	if (x_obj_type_isspair(p_obj)) {
		return x_pair_prim_units(p_base, p_args);
	}

	if (x_obj_type_issatom(p_obj) || x_obj_isnil(p_base, x_obj_type(p_obj))) {
		return x_atom_prim_units(p_base, p_args);
	}

	p_units = x_type_field_units(x_obj_type(p_obj));

	if (x_obj_isnil(p_base, p_units) || x_obj_isnil(p_base, x_atomobj(p_units))) {
		return p_base;
	}

	return (*x_atomfn(p_units))(p_base, p_args);
}

x_int_t x_obj_units(x_obj_t *p_base, x_obj_t *p_obj)
{
	x_obj_t *p_units;
	x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_obj }, { p_base });

	p_units = x_obj_prim_units(p_base, (x_obj_t *)args);

	if (x_obj_isnil(p_base, p_units)) {
		return X_OBJ_UNITS_ATOM;
	}

	return x_atomint(p_units);
}

x_obj_t *x_atom_prim_length(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_type_length_atom_obj;
}

x_obj_t *x_pair_prim_length(x_obj_t *p_base, x_obj_t *p_args)
{
	return x_type_length_pair_obj;
}

x_obj_t *x_obj_prim_length(x_obj_t *p_base, x_obj_t *p_args)
{
	x_obj_t *p_length, *p_obj;

	if (x_obj_isnil(p_base, p_args) || x_obj_isnil(p_base, (p_obj = x_firstobj(p_args)))) {
		return p_base;
	}

	if (x_obj_type_isspair(p_obj)) {
		return x_pair_prim_length(p_base, p_args);
	}

	if (x_obj_type_issatom(p_obj) || x_obj_isnil(p_base, x_obj_type(p_obj))) {
		return x_atom_prim_length(p_base, p_args);
	}

	p_length = x_type_field_length(x_obj_type(p_obj));

	if (x_obj_isnil(p_base, p_length) || x_obj_isnil(p_base, x_atomobj(p_length))) {
		return p_base;
	}

	return (*x_atomfn(p_length))(p_base, p_args);
}

x_int_t x_obj_length(x_obj_t *p_base, x_obj_t *p_obj)
{
	x_obj_t *p_length;
	x_spair_t args = x_obj_set(NULL, X_OBJ_FLAG_NONE, { p_obj }, { p_base });

	p_length = x_obj_prim_length(p_base, (x_obj_t *)args);

	if (x_obj_isnil(p_base, p_length)) {
		return 0;
	}

	return x_atomint(p_length);
}

/*
 * Output an error message to *stderr*, then **exit**.
 *
 * @function x_obj_error
 * @param {x_char_t *} message A C string error message to output.
 * @param {x_char_t *} symbol A C string with additional symbolic information or _NULL_.
 */
void x_obj_error(x_obj_t *p_base, x_char_t *message, x_char_t *symbol)
{
	int fd = x_base_isset(p_base) ? x_atomint(x_base_field_fileerr(p_base)) : STDERR_FILENO;

	x_error(fd, message, symbol);
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

/*
 * Output an Object's properties to *stderr*.
 *
 * **Note:** Only functions when *`DEBUG`* is defined.
 *
 * @function x_obj_dump
 * @param   {x_obj_t *} p_base A pointer to the p_base object.
 * @param   {x_obj_t *} p_obj A pointer to the object to dump.
 * @param   {x_char_t *} msg A message to prepend to the message.
 *
 * @example
 *   x_obj_t *p_obj = x_obj_alloc(X_RESOURCE, X_OBJ_FLAG_OWN|X_OBJ_FLAG_GC, 0);
 *   x_obj_debug(NULL, p_obj, ":");
 */
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

		/*if (x_obj_type_issatom(p_obj) || x_obj_type_issatom(x_obj_type(p_obj))) {
			s += sprintf(s, "0x%"X_INT_STR_PRINTF_CONV"x", x_atomint(p_obj));
		}*/

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
#ifdef X_GC
		"GC[%p]"
#endif /* X_GC */
		"%s"
		, msg ? (char *)msg : ""
		, type
		, flags
		, p_obj
#ifdef X_GC
		, p_obj ? x_obj_gc(p_obj) : NULL
#endif /* X_GC */
		, s
	);
}

#else /* DEBUG */

/*void x_obj_debug_va(x_obj_t *p_base, char *fmt, va_list ap) {}
void x_obj_debug(x_obj_t *p_base, char *fmt, ...) {}
void x_obj_dump(x_obj_t *p_base, x_obj_t *p_obj, char *msg) {}
*/
#endif /* DEBUG */
