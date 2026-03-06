/*
 * # Computational Expressions in C
 *
 * ## x.c -- Implementation
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

/*
 * Output an error message to *stderr*, then **exit**.
 *
 * @function x_error
 * @param {x_char_t *} message A C string error message to output.
 * @param {x_char_t *} m A C string with additional symbolic information or _NULL_.
 */
void x_error(int fd, x_char_t *message, x_char_t *symbol)
{
	x_sys_write(fd, (x_char_t *)"*** ERROR: ", 11);
	x_sys_write(fd, message, x_lib_strlen(message));

	if (symbol) {
		x_sys_write(fd, (x_char_t *)" '", 2);
		x_sys_write(fd, symbol, x_lib_strlen(symbol));
	}

	x_sys_exit(X_SYS_EXIT_FAILURE);
}


#ifdef DEBUG

void _x_debug_va(char *file, long unsigned line, int fd, char *fmt, va_list ap)
{
	x_char_t buffer[X_DEBUG_BUFFER_SIZE], *s;
	int n;

	s = (x_char_t *)"\n*** DEBUG(%s:%lu): ";
	sprintf((char *)buffer, s, file, line);
	x_sys_write(fd, buffer, x_lib_strlen(buffer));

	n = vsprintf((char *)buffer, fmt, ap);

	x_sys_write(fd, buffer, n);

	s = (x_char_t *)" ***\n";
	x_sys_write(fd, s, x_lib_strlen(s));
}

void _x_debug(char *file, long unsigned line, int fd, char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_x_debug_va(file, line, fd, fmt, ap);
	va_end(ap);
}

#else /* DEBUG */

#endif /* DEBUG */
