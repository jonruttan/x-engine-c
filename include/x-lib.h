#ifndef X_LIB_H
#define X_LIB_H

/*
 * # Computational Expressions in C
 *
 * ## x-lib.h -- Header - Library Functions
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
#include "x-sys.h"

/*
 * # Library Functions
 */
int x_lib_abs(int i);
x_char_t *x_lib_inttostr(x_int_t num, x_char_t *p_str, unsigned short base);
void *x_lib_memcpy(void *p_dest, const void *p_src, size_t n);
void *x_lib_memdup(const void *p_src, size_t size);
void *x_lib_memset(void *p_dest, int byte, size_t size);
x_char_t *x_lib_strchr(const x_char_t *p_str, int c);
int x_lib_strcmp(const x_char_t *p_str1, const x_char_t *p_str2);
size_t x_lib_strlen(const x_char_t *p_str);
int x_lib_strncmp(const x_char_t *p_str1, const x_char_t *p_str2, size_t n);
x_char_t *x_lib_strndup(const x_char_t *p_str, size_t size);
x_int_t x_lib_strtoint(const x_char_t *p_str, x_char_t **pp_end, unsigned short base);

#endif /* X_LIB_H */
