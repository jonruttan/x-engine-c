#ifndef X_SYS_H
#define X_SYS_H

/*
 * # Computational Expressions in C
 *
 * ## x.h -- Header - System Functions
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
#include "x.h"

#include <stdio.h>	/* For *EOF* */
#include <unistd.h> /* For *STD*_FILENO* */

#ifndef X_SYS_EOF
#define X_SYS_EOF EOF
#endif /* X_SYS_EOF */

#ifndef X_SYS_EXIT_SUCCESS
#define X_SYS_EXIT_SUCCESS 0
#endif /* X_SYS_EXIT_SUCCESS */

#ifndef X_SYS_EXIT_FAILURE
#define X_SYS_EXIT_FAILURE 1
#endif /* X_SYS_EXIT_FAILURE */

/*
 * # System Functions
 */
void *x_sys_malloc(size_t size);
void x_sys_free(void *ptr);

ssize_t x_sys_read(int fd, void *p_buf, size_t size);
ssize_t x_sys_write(int fd, const void *p_buf, size_t size);

void x_sys_exit(int status);

x_char_t x_sys_read_char(int fd);

#endif /* X_SYS_H */
