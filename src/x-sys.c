/*
 * # Computational Expressions in C
 *
 * ## x-sys.c -- Implementation - System Functions
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
#include <stdio.h>			/* For *vsprintf */
#include <stdlib.h>
#include <unistd.h>
#include "x-sys.h"
#include "x-lib.h"

#ifndef TESTS

/*
 * # System Functions
 */
/*
 * Allocate a memory vector from the heap.
 *
 * @function x_sys_malloc
 * @param {size_t} size The size in bytes of the memory vector to allocate.
 * @returns {void *} A pointer to the allocated memory.
 */
void *x_sys_malloc(size_t size)
{
	return malloc(size);
}

/*
 * Free a memory vector to the heap.
 *
 * @function x_sys_free
 * @param {void *} ptr A pointer to the memory vector to free.
 * @returns {void}
 */
void x_sys_free(void *ptr)
{
	free(ptr);
}

/*
 * Read from a file descriptor.
 *
 * @function x_sys_read
 * @param {int} fd A file descriptor.
 * @param {void *} p_buf A pointer to the memory to read to.
 * @param {size_t} size The size of the buffer to read to.
 * @returns {ssize_t} The number of bytes read (may be less than size.)
 */
ssize_t x_sys_read(int fd, void *p_buf, size_t size)
{
	return read(fd, p_buf, size);
}

/*
 * Write to a file descriptor.
 *
 * @function x_sys_write
 * @param {int} fd A file descriptor.
 * @param {void *} p_buf A pointer to the memory to write from.
 * @param {size_t} size The size of the buffer to write from.
 * @returns {ssize_t} The number of bytes written (may be less than size.)
 */
ssize_t x_sys_write(int fd, const void *p_buf, size_t size)
{
	return write(fd, p_buf, size);
}

/*
 * Cause normal process termination.
 *
 * @param {int} status An integer returned to the parent.
 * @returns The exit function does not return.
 */
void x_sys_exit(int status)
{
	exit(status);
}

#endif /* TESTS */

x_char_t x_sys_read_char(int fd)
{
	x_char_t c;

	if (x_sys_read(fd, &c, sizeof(c)) <= 0) {
		return X_SYS_EOF;
	}

	return c;
}

