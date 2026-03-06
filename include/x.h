#ifndef X_H
#define X_H

/*
 * # Computational Expressions in C
 *
 * ## x.h -- Header
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
#include <stdarg.h>		/* For *va_list* */
/*
#include <sys/types.h>
*/

/*#define X_USE_STDLIB*/
/*#define X_USE_STDLIB_NONSTD*/

/*
 * # Constants
 */
/*
 * The current version of the Computational Expressions. Version numbering
 * conforms to the [Symantic Versioning] spec.
 *
 * [Symantic Versioning]: http://semver.org/
 *
 * @constant X_VERSION
 */
#define X_VERSION "0.1.0"

#define X_DEBUG_BUFFER_SIZE	65536

/*
 * Whether to include Garbage Collection structures.
 *
 * @constant X_GC
 */
#ifndef X_GC
/*#define X_GC*/
#endif /* X_GC */

/*
 * The C compiler's target machine, for example, `i686-pc-linux-gnu`.
 *
 * To override the default value of `undefined`, a value can be passed to the
 * C compiler (preprocessor) via the command line. When supported by the
 * compiler, this value will be the output generated when calling the compiler
 * with the `-dumpmachine` switch.
 *
 * @constant X_MACHINE
 */
#ifndef X_MACHINE
#define X_MACHINE "undefined"
#endif /* X_MACHINE */

/*
 * The C compiler's target CPU architecture.
 *
 * Values are determined by probing the C compiler's definitions for matches to
 * known architecture values, or set to the value `undefined` when no match is
 * found.
 *
 * Adapted from <https://sourceforge.net/p/predef/wiki/Architectures/>
 *
 * @constant X_ARCH
 */
#if __alpha__ || __alpha || _M_ALPHA
#define X_ARCH "alpha"
#elif __amd64__ || __amd64 || __x86_64__ || __x86_64 || _M_X64 || _M_AMD64
#define X_ARCH "amd64"
#elif __arm__ || __thumb__ || __TARGET_ARCH_ARM || __TARGET_ARCH_THUMB || _ARM || _M_ARM || _M_ARMT || __arm
#define X_ARCH "arm"
#elif __aarch64__
#define X_ARCH "arm64"
#elif __bfin || __BFIN__
#define X_ARCH "blackfin"
#elif __convex__
#define X_ARCH "convex"
#elif __epiphany__
#define X_ARCH "epiphany"
#elif __hppa__ || __HPPA__ || __hppa
#define X_ARCH "pa_risc"
#elif i386 || __i386 || __i386__ || __i386 || __i386 || __IA32__ || _M_I86 || _M_IX86 || __X86__ || _X86_ || __THW_INTEL__ || __I86__ || __INTEL__ || __386
#define X_ARCH "x86"
#elif __ia64__ || _IA64 || __IA64__ || __ia64 || _M_IA64 || _M_IA64 || __itanium__
#define X_ARCH "ia64"
#elif __m68k__ || M68000 || __MC68K__
#define X_ARCH "m68k"
#elif __mips__ || mips || __mips || __MIPS__
#define X_ARCH "mips"
#elif  __powerpc || __powerpc__ || __powerpc64__ || __POWERPC__ || __ppc__ || __ppc64__ || __PPC__ || __PPC64__ || _ARCH_PPC || _ARCH_PPC64 || _M_PPC || _ARCH_PPC || _ARCH_PPC64 || __ppc
#define X_ARCH "powerpc"
#elif pyr
#define X_ARCH "pyramid9810"
#elif __THW_RS6000 || _IBMR2 || _POWER || _ARCH_PWR || _ARCH_PWR2 || _ARCH_PWR3 || _ARCH_PWR4
#define X_ARCH "rs6000"
#elif __sparc__ || __sparc
#define X_ARCH "sparc"
#elif __sh__
#define X_ARCH "superh"
#elif __370__ || __THW_370__ || __s390__ || __s390x__ || __zarch__ || __SYSC_ZARCH__
#define X_ARCH "systemz"
#elif _TMS320C2XX || __TMS320C2000__ || _TMS320C5X || __TMS320C55X__ || TMS320C6X || __TMS320C6X__
#define X_ARCH "tms320"
#elif __TMS470__
#define X_ARCH "tms470"
#else
#define X_ARCH "undefined"
#endif

/*
 * # Basic Types
 *
 * ## Integers
 */
/*
 * The *integer* type.
 *
 * @constant x_int_t
 */
#ifdef __STDC__

typedef long x_int_t;
#define X_INT_STR_PRINTF_CONV	"l"

#else /* __STDC__v */

typedef long long x_int_t;
#define X_INT_STR_PRINTF_CONV	"ll"

#endif /* __STDC__ */

/*
 * ## Characters
 */
/*
 * The *character* type.
 *
 * @constant x_char_t
 */
typedef char x_char_t;
#define X_INT_CHAR_PRINTF_CONV	"c"


/*
 * # Definitions
 */
#ifdef DEBUG

void _x_debug_va(char *file, long unsigned line, int fd, char *fmt, va_list ap);
#define x_debug_va(fd, fmt, ap)\
	_x_debug_va(__FILE__, __LINE__, fd, fmt, ap)

void _x_debug(char *file, long unsigned line, int fd, char *fmt, ...);
#define x_debug(fd, fmt, ...)\
	_x_debug(__FILE__, __LINE__, fd, fmt, __VA_ARGS__)

#else /* DEBUG */

#define x_debug_va(fd, fmt, ap)		{}
#define x_debug(fd, fmt, ...)		{}

#endif /* DEBUG */

void x_error(int fd, x_char_t *message, x_char_t *symbol);

#endif /* X_H */
