/*
 * # Computational Expressions in C
 *
 * ## x-lib.c -- Implementation - Library Functions
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
#include "x-lib.h"

#ifdef X_USE_STDLIB

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#else /* X_USE_STDLIB */

#include <ctype.h>

#endif /* X_USE_STDLIB */

/*
 * # Library Functions
 */
/*
 * Compute the absolute value of the integer i.
 *
 * @function x_lib_abs
 * @param {int} i The integer to compute the value on.
 * @returns {i} The absolute value of i.
 */
int x_lib_abs(int i)
{
#ifdef X_USE_STDLIB
	return abs(i);
#else
	return i >= 0 ? i : -i;
#endif /* X_USE_STDLIB */
}

/*
 * Convert an integer to a string.
 *
 * Converts the integer value from val into an ASCII representation that will
 * be stored under _str_.
 *
 * Conversion is done using a numeric _base_, which may be a number between 2
 * (binary conversion) and up to 36. If base is greater than 10, the next
 * digit after '9' will be the letter 'a'.
 *
 * If val is negative, a minus sign will be prepended.
 *
 * **NOTE:** The caller is responsible for providing sufficient storage in
 *           _str_. If the buffer is too small, you risk a buffer overflow.
 *
 * **NOTE:** The minimal size of the buffer _str_ depends on the choice of
 *           _base_. For example, if the base is 2 (binary), you need to supply
 *           a buffer with a minimal length of `8 * sizeof (int_t) + 1`
 *           characters, _i.e. one character for each bit plus one for the
 *           string terminator_. Using a larger base will require a smaller
 *           minimal buffer size.
 *
 * @function x_lib_inttostr
 * @param {int_t} num The integer to be converted.
 * @param {x_char_t *} str A C string to store the converted representation.
 * @param {unsigned short} base The number base to convert to.
 * @returns The pointer passed as _str_.
 */
x_char_t *x_lib_inttostr(x_int_t num, x_char_t *p_str, unsigned short base)
{
#ifdef X_USE_STDLIB_NONSTD
	return ltoa(num, (char *)p_str, base);
#else
	x_char_t *p1 = p_str, *p2;
	short offset;

	if (p_str == NULL || base < 2 || base > 36) {
		return NULL;
	}

	if (num < 0) {
		*p1++ = '-';
		num = -num;
	}

	p2 = p1;
	do {
		offset = num % base;
		*p2++ = offset + (offset >= 10 ? 'a' - 10 : '0');
	} while ((num /= base));

	/* Add a trailing '0' and reverse the digits in the string */
	for (*p2-- = 0; p2 > p1; ++p1, --p2)
	{
		/* Swap the value at p1 with that at p2 */
		*p1 ^= *p2;
		*p2 ^= *p1;
		*p1 ^= *p2;
	}

	return p_str;
#endif /* X_USE_STDLIB_NONSTD */
}

/*
 * Copy bytes from a source memory vector to a destination.
 *
 * @function x_lib_memcpy
 * @param {void *} p_dest A pointer to the destination memory vector.
 * @param {const void *} p_src A pointer to the source memory vector.
 * @param {size_t} size The number of bytes to copy.
 * @returns {void *} The pointer to the destination memory vector.
 */
void *x_lib_memcpy(void *p_dest, const void *p_src, size_t size)
{
#ifdef X_USE_STDLIB
	return memcpy(p_dest, p_src, size);
#else
	x_char_t *pd = (x_char_t *)p_dest;
	const x_char_t *ps = (const x_char_t *)p_src;

	while (size--) {
		*pd++ = *ps++;
	}

	return p_dest;
#endif /* X_USE_STDLIB */
}

/*
 * Duplicate a memory vector.
 *
 * @function x_lib_memdup
 * @param {const void *} p_src A pointer to the source memory vector.
 * @param {size_t} size The size of the memory vector in bytes.
 * @returns {void *} A pointer to the duplicate memory vector, or _NULL_ on
 *                   error.
 */
void *x_lib_memdup(const void *p_src, size_t size)
{
	void *p_dst;

	if ((p_dst = x_sys_malloc(size)) == NULL) {
		return NULL;
	}

	if (p_src) {
		x_lib_memcpy(p_dst, p_src, size);
	}
#ifdef X_OPT_MEMZERO
	else {
		x_lib_memset(p_dst, 0, size);
	}
#endif /* X_OPT_MEMZERO */

	return p_dst;
}

/*
 * Fill a memory vector with a constant byte.
 *
 * @function x_lib_memset
 * @param {void *} p_vector A pointer to the destination memory vector.
 * @param {int} byte The constant byte value.
 * @param {size_t} size The number of bytes to set.
 * @returns {void *} The pointer to the destination memory vector.
 */
void *x_lib_memset(void *p_dest, int byte, size_t size)
{
#ifdef X_USE_STDLIB
	return memset(p_dest, byte, size);
#else
	x_char_t *pd = (x_char_t *)p_dest;

	while (size--) {
		*pd++ = byte;
	}

	return p_dest;
#endif /* X_USE_STDLIB */
}

x_char_t *x_lib_strchr(const x_char_t *p_str, int c)
{
#ifdef X_USE_STDLIB
	return strchr(p_str, c);
#else
	const x_char_t *ps = (const x_char_t *)p_str;

	for (;*ps && *ps != c; ps++) ;

	return *ps ? (x_char_t *)ps : NULL;
#endif /* X_USE_STDLIB */
}

/*
 * Compare two strings.
 *
 * @function x_lib_strcmp
 * @param {const x_char_t *} p_str1 A pointer to the first string to compare.
 * @param {const x_char_t *} p_str2 A pointer to the second string to compare.
 * @returns {int} The difference between the two strings.
 */
int x_lib_strcmp(const x_char_t *p_str1, const x_char_t *p_str2)
{
#ifdef X_USE_STDLIB
	return strcmp((char *)p_str1, (char *)p_str2);
#else
	const x_char_t *ps1 = (const x_char_t *)p_str1;
	const x_char_t *ps2 = (const x_char_t *)p_str2;

	for (;*ps1 && *ps2 && *ps2 == *ps1; ps1++, ps2++) ;

	return *ps1 - *ps2;
#endif /* X_USE_STDLIB */
}

/*
 * Calculate the length of a C string.
 *
 * **NOTE:** This function doesn't handle wide characters.
 *
 * @function x_lib_strlen
 * @param {const x_char_t *} p_str A pointer to the C string memory vector.
 * @returns {size_t} The size of the string in bytes.
 */
size_t x_lib_strlen(const x_char_t *p_str)
{
#ifdef X_USE_STDLIB
	return strlen((char *)p_str);
#else
	size_t size;

	for (size=0; *p_str++; size++) ;

	return size;
#endif /* X_USE_STDLIB */
}

/*
 * Compare two strings, to a maximum of `n` characters.
 *
 * @function x_lib_strcmp
 * @param {const x_char_t *} p_str1 A pointer to the first string to compare.
 * @param {const x_char_t *} p_str2 A pointer to the second string to compare.
 * @param {size_t} n The maximum number of characters to compare.
 * @returns {int} The difference between the two strings.
 */
int x_lib_strncmp(const x_char_t *p_str1, const x_char_t *p_str2, size_t n)
{
#ifdef X_USE_STDLIB
	return strncmp((char *)p_str1, (char *)p_str2, n);
#else
	const x_char_t *ps1 = (const x_char_t *)p_str1;
	const x_char_t *ps2 = (const x_char_t *)p_str2;

	for (;--n && *ps1 && *ps2 && *ps2 == *ps1; ps1++, ps2++) ;

	return *ps1 - *ps2;
#endif /* X_USE_STDLIB */
}

/*
 * Duplicate a C string memory vector.
 *
 * @function x_lib_strdup
 * @param {const x_char_t *} p_str A pointer to a C string memory vector.
 * @param {size_t} size The size of the C string memory vector in bytes.
 * @returns {x_char_t *} A pointer to the duplicate C string memory vector, or
 *                   _NULL_ on error.
 */
x_char_t *x_lib_strndup(const x_char_t *p_str, size_t size)
{
	x_char_t *p_clone;

	if ( ! (p_clone = (x_char_t *)x_lib_memdup((void *)p_str, size + 1))) {
		return NULL;
	}

	p_clone[size] = 0;

	return p_clone;
}

x_int_t x_lib_strtoint(const x_char_t *p_str, x_char_t **pp_end, unsigned short base)
{
#ifdef X_USE_STDLIB
	return strtol((char *)p_str, (char **)pp_end, base);
#else
	x_int_t i = 0, n, sign = 1;

	while (isspace(*p_str)) {
		p_str++;
	}

	if (*p_str == '+') {
		p_str++;
	} else if (*p_str == '-') {
		p_str++;
		sign = -1;
	}

	if (base == 0) {
		if (*p_str == '0') {
			if (toupper(*++p_str) == 'X') {
				base = 16;
				p_str++;
			} else {
				base = 8;
			}
		} else {
			base = 10;
		}
	}

	for (;*p_str; p_str++) {
		if (isdigit(*p_str)) {
			n = *p_str - '0';
#define BREAK_ON_BASE
#ifdef BREAK_ON_BASE
			if (n >= base) {
				break;
			}
			i *= base;
			i += n;
#else
			if (n < base) {
				i *= base;
				i += n;
			}
#endif
		}
		else if (toupper(*p_str) >= 'A' && toupper(*p_str) <= 'Z') {
			n = toupper(*p_str) - 'A' + 10;
#ifdef BREAK_ON_BASE
			if (n >= base) {
				break;
			}
			i *= base;
			i += n;
#else
			if (n < base) {
				i *= base;
				i += n;
			}
#endif
		} else {
			break;
		}
	}

	if (pp_end) {
		*pp_end = (x_char_t *)p_str;
	}

	return i * sign;
#endif /* X_USE_STDLIB */
}
