/*
 * # Unit Tests: *x-lib*
 */

#include "test-runner.h"

#define X_OPT_MEMZERO

#include "x.h"

#ifndef X_TEST_LIB
#undef X_USE_STDLIB
#endif /* X_TEST_LIB */

#include "src/x-sys.c"
#include "src/x-lib.c"
#include "src/x.c"

#include "helper-system-functions.c"


/*
 * ## Test Overhead
 */
static void setup(void)
{
	helper_set_alloc(MEM_GUARANTEED);
}

static void teardown(void)
{
}


/*
 * ## Test Runners
 */
static char *test_lib_inttostr(void)
{
	x_char_t dst[16], *p_dst;

	_it_should("return null when destination buffer is null", x_lib_inttostr(0, NULL, 0) == NULL);
	_it_should("return null when base is less than 2", x_lib_inttostr(0, dst, 0) == NULL && x_lib_inttostr(0, dst, 1) == NULL);
	_it_should("return null when base is greater than 36", x_lib_inttostr(0, dst, 37) == NULL);

	p_dst = x_lib_inttostr(0, dst, 2);
	_it_should("return \"0\" when number is 0, base 2",
		p_dst && 0 == strcmp((char *)p_dst, "0")
	);

	p_dst = x_lib_inttostr(1, dst, 2);
	_it_should("return \"1\" when number is 1, base 2",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "1")
	);

	p_dst = x_lib_inttostr(-1, dst, 2);
	_it_should("return \"-1\" when number is -1, base 2",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "-1")
	);

	p_dst = x_lib_inttostr(2, dst, 2);
	_it_should("return \"10\" when number is 2, base 2",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "10")
	);

	p_dst = x_lib_inttostr(-2, dst, 2);
	_it_should("return \"-10\" when number is -2, base 2",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "-10")
	);


	p_dst = x_lib_inttostr(0, dst, 3);
	_it_should("return \"0\" when number is 0, base 3",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "0")
	);

	p_dst = x_lib_inttostr(1, dst, 3);
	_it_should("return \"1\" when number is 1, base 3",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "1")
	);

	p_dst = x_lib_inttostr(-1, dst,3);
	_it_should("return \"-1\" when number is -1, base 3",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "-1")
	);

	p_dst = x_lib_inttostr(3, dst, 3);
	_it_should("return \"10\" when number is 3, base 3",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "10")
	);

	p_dst = x_lib_inttostr(-3, dst, 3);
	_it_should("return \"-10\" when number is -3, base 3",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "-10")
	);


	p_dst = x_lib_inttostr(0, dst, 10);
	_it_should("return \"0\" when number is 0, base 10",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "0")
	);

	p_dst = x_lib_inttostr(1, dst, 10);
	_it_should("return \"1\" when number is 1, base 10",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "1")
	);

	p_dst = x_lib_inttostr(-1, dst,10);
	_it_should("return \"-1\" when number is -1, base 10",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "-1")
	);

	p_dst = x_lib_inttostr(10, dst, 10);
	_it_should("return \"10\" when number is 10, base 10",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "10")
	);

	p_dst = x_lib_inttostr(-10, dst, 10);
	_it_should("return \"-10\" when number is -10, base 10",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "-10")
	);


	p_dst = x_lib_inttostr(0, dst, 16);
	_it_should("return \"0\" when number is 0, base 16",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "0")
	);

	p_dst = x_lib_inttostr(1, dst, 16);
	_it_should("return \"1\" when number is 1, base 16",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "1")
	);

	p_dst = x_lib_inttostr(-1, dst,16);
	_it_should("return \"-1\" when number is -1, base 16",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "-1")
	);

	p_dst = x_lib_inttostr(10, dst, 16);
	_it_should("return \"a\" when number is 10, base 16",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "a")
	);

	p_dst = x_lib_inttostr(-10, dst, 16);
	_it_should("return \"-a\" when number is -10, base 16",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "-a")
	);

	p_dst = x_lib_inttostr(15, dst, 16);
	_it_should("return \"f\" when number is 15, base 16",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "f")
	);

	p_dst = x_lib_inttostr(-15, dst, 16);
	_it_should("return \"-f\" when number is -15, base 16",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "-f")
	);

	p_dst = x_lib_inttostr(16, dst, 16);
	_it_should("return \"10\" when number is 16, base 16",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "10")
	);

	p_dst = x_lib_inttostr(-16, dst, 16);
	_it_should("return \"-10\" when number is -16, base 16",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "-10")
	);


	p_dst = x_lib_inttostr(0, dst, 36);
	_it_should("return \"0\" when number is 0, base 36",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "0")
	);

	p_dst = x_lib_inttostr(1, dst, 36);
	_it_should("return \"1\" when number is 1, base 36",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "1")
	);

	p_dst = x_lib_inttostr(-1, dst,36);
	_it_should("return \"-1\" when number is -1, base 36",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "-1")
	);

	p_dst = x_lib_inttostr(10, dst, 36);
	_it_should("return \"a\" when number is 10, base 36",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "a")
	);

	p_dst = x_lib_inttostr(-10, dst, 36);
	_it_should("return \"-a\" when number is -10, base 36",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "-a")
	);

	p_dst = x_lib_inttostr(35, dst, 36);
	_it_should("return \"z\" when number is 35, base 36",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "z")
	);

	p_dst = x_lib_inttostr(-35, dst, 36);
	_it_should("return \"-z\" when number is -35, base 36",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "-z")
	);

	p_dst = x_lib_inttostr(36, dst, 36);
	_it_should("return \"10\" when number is 36, base 36",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "10")
	);

	p_dst = x_lib_inttostr(-36, dst, 36);
	_it_should("return \"-10\" when number is -36, base 36",
		p_dst != NULL && 0 == strcmp((char *)p_dst, "-10")
	);


	return NULL;
}

#define TEST_BYTE 0x7f
#define TEST_SIZE 17
static char *test_lib_memcpy(void)
{
	x_char_t dest[TEST_SIZE];
	const x_char_t *p_src = (x_char_t *)"@ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	x_char_t *p_ret;
	size_t n;

	memset(dest, TEST_BYTE, TEST_SIZE);

	n = (size_t)rand() * (TEST_SIZE - 1) / RAND_MAX;
	p_ret = x_lib_memcpy(dest, p_src, n);
	_it_should("duplicate a vector", 0 == memcmp(dest, p_src, n));
	_it_should("not duplicate beyond the first n bytes of a vector", dest[n + 1] == TEST_BYTE);
	_it_should("return p_dest", p_ret == dest);

	return NULL;
}

static char *test_lib_memdup(void)
{
	x_char_t *p_dst;
	const x_char_t *p_src = (x_char_t *)"Hello, World!";

	helper_set_alloc(MEM_GUARANTEED);
	helper_alloc_reset();
	p_dst = x_lib_memdup(p_src, strlen((char *)p_src));
	_it_should("have called alloc", helper_alloc_count() == 1);
	_it_should("duplicate a memory vector when p_src is not NULL",
		0 == memcmp(p_dst, p_src, strlen((char *)p_src))
	);
	x_sys_free(p_dst);

	helper_set_alloc(MEM_ERROR);
	helper_alloc_reset();
	p_dst = x_lib_memdup(NULL, 16);
	_it_should("have called alloc", 1 == helper_alloc_count());
	_it_should("return NULL on alloc failure", p_dst == NULL);
	x_sys_free(p_dst);

	return NULL;
}

static char *test_lib_memset(void)
{
	x_char_t buffer[TEST_SIZE],
		memcmp_buffer[TEST_SIZE];
	x_char_t *p_ret;
	int byte;
	size_t size;

	/* Fill with zeros */
	memset(buffer, TEST_BYTE, TEST_SIZE);

	/* Create some random values */
	byte = (int)rand() * 255 / RAND_MAX;
	size = (size_t)rand() * (TEST_SIZE - 1) / RAND_MAX;

	/* Fill a buffer with values, use memcmp to compare during tests */
	memset(memcmp_buffer, byte, size);

	p_ret = x_lib_memset(buffer, byte, size);

	_it_should("fill a vector with a constant", memcmp(buffer, memcmp_buffer, size) == 0);
	_it_should("not set beyond the first n bytes of a vector", buffer[size + 1] == TEST_BYTE);
	_it_should("return p_dest", p_ret == buffer);

	return NULL;
}

static char *test_lib_strchr(void)
{
	x_char_t *p, *s = (x_char_t *)"aba";

	p = x_lib_strchr(s, 'b');
	_it_should("return first occurrence of the character", p == s + 1);

	p = x_lib_strchr(s, 'b');
	_it_should("find characters at the start of the string", p == s + 1);

	p = x_lib_strchr(s + 1, 'a');
	_it_should("find characters at the end of the string", p == s + 2);

	p = x_lib_strchr(s, 'c');
	_it_should("return NULL if the character is not found", p == NULL);

	return NULL;
}

static char *test_lib_strcmp(void)
{
	int n;

	n = x_lib_strcmp((x_char_t *)"abc", (x_char_t *)"abc");
	_it_should("return 0 when strings are equal", n == 0);

	n = x_lib_strcmp((x_char_t *)"abe", (x_char_t *)"abc");
	_it_should("return > 0 when first string is greater", n > 0);
	/*_it_should("return 2 when first string is greater", n == 2);*/

	n = x_lib_strcmp((x_char_t *)"abc", (x_char_t *)"abe");
	_it_should("return < 0 when second string is greater", n < 0);
	/*_it_should("return -2 when second string is greater", n == -2);*/

	n = x_lib_strcmp((x_char_t *)"ab", (x_char_t *)"abc");
	_it_should("return < 0 when first string is shorter", n < 0);

	n = x_lib_strcmp((x_char_t *)"abc", (x_char_t *)"ab");
	_it_should("return > 0 when second string is shorter", n > 0);

	return NULL;
}

static char *test_lib_strlen(void)
{
	const x_char_t *p_str = (x_char_t *)"Hello, World!";

	_it_should("calculate string length when p_str is a valid C string",
		x_lib_strlen(p_str) == strlen((char *)p_str)
	);
	_it_should("calculate string length 0 when p_str is an empty string",
		0 == x_lib_strlen((x_char_t *)"")
	);

	return NULL;
}

static char *test_lib_strncmp(void)
{
	int n;

	n = x_lib_strncmp((x_char_t *)"abc1", (x_char_t *)"abc2", 3);
	_it_should("return 0 when strings are equal", n == 0);
	n = x_lib_strncmp((x_char_t *)"abc1", (x_char_t *)"abc2", 2);
	_it_should("return 0 when strings are equal", n == 0);
	n = x_lib_strncmp((x_char_t *)"abc1", (x_char_t *)"abc2", 1);
	_it_should("return 0 when strings are equal", n == 0);

	n = x_lib_strncmp((x_char_t *)"abe1", (x_char_t *)"abc2", 3);
	_it_should("return > 0 when first string is greater", n > 0);
	/*_it_should("return 2 when first string is greater", n == 2);*/

	n = x_lib_strncmp((x_char_t *)"abc1", (x_char_t *)"abe2", 3);
	_it_should("return < 0 when second string is greater", n < 0);
	/*_it_should("return -2 when second string is greater", n == -2);*/

	n = x_lib_strncmp((x_char_t *)"ab", (x_char_t *)"abc", 3);
	_it_should("return < 0 when first string is shorter", n < 0);

	n = x_lib_strncmp((x_char_t *)"abc", (x_char_t *)"ab", 3);
	_it_should("return > 0 when second string is shorter", n > 0);

	return NULL;
}

static char *test_lib_strndup(void)
{
	x_char_t *p_dst;
	x_char_t *p_src = (x_char_t *)"test_lib_strndup";

	helper_set_alloc(MEM_GUARANTEED);
	helper_alloc_reset();
	p_dst = x_lib_strndup(p_src, strlen((char *)p_src));
	_it_should("have called alloc", 1 == helper_alloc_count());
	_it_should("duplicate an entire string",
		0 == strncmp((char *)p_dst, (char *)p_src, strlen((char *)p_src))
	);
	x_sys_free(p_dst);

	helper_set_alloc(MEM_ERROR);
	helper_alloc_reset();
	p_dst = x_lib_strndup(p_src, strlen((char *)p_src));
	_it_should("have called alloc", 1 == helper_alloc_count());
	_it_should("return NULL on alloc failure", p_dst == NULL);
	x_sys_free(p_dst);

	helper_set_alloc(MEM_GUARANTEED);
	helper_alloc_reset();
	p_dst = x_lib_strndup(p_src, strlen((char *)p_src) / 2);
	_it_should("only duplicate the first n characters of a string",
		0 == strncmp((char *)p_dst, (char *)p_src, strlen((char *)p_src) / 2)
	);
	x_sys_free(p_dst);

	return NULL;
}

static char *test_lib_strtoint(void)
{
	x_char_t *s, *ps;
	x_int_t i;

	s = (x_char_t *)"";
	i = x_lib_strtoint(s, &ps, 0);
	_it_should("handle the empty string", 0 == i);

	i = x_lib_strtoint(s, NULL, 0);
	_it_should("handle the empty string with a NUL end pointer", 0 == i);


	s = (x_char_t *)"1";
	i = x_lib_strtoint(s, NULL, 16);
	_it_should("handle a single numeric digit", 1 == i);


	s = (x_char_t *)"1";
	i = x_lib_strtoint(s, NULL, 16);
	_it_should("handle a single alpha digit", 1 == i);


	s = (x_char_t *)"11";
	i = x_lib_strtoint(s, NULL, 16);
	_it_should("handle two numeric digits", 17 == i);


	s = (x_char_t *)"ff";
	i = x_lib_strtoint(s, NULL, 16);
	_it_should("handle lower case integers", 255 == i);


	s = (x_char_t *)"FF";
	i = x_lib_strtoint(s, NULL, 16);
	_it_should("handle upper case integers", 255 == i);


	s = (x_char_t *)"+ff";
	i = x_lib_strtoint(s, NULL, 16);
	_it_should("handle positive integers", 255 == i);


	s = (x_char_t *)"-ff";
	i = x_lib_strtoint(s, NULL, 16);
	_it_should("handle negative integers", -255 == i);



	s = (x_char_t *)"0xff";
	i = x_lib_strtoint(s, NULL, 0);
	_it_should("handle signless prefixed integers", 255 == i);


	s = (x_char_t *)"+0xff";
	i = x_lib_strtoint(s, NULL, 0);
	_it_should("handle positive prefixed integers", 255 == i);


	s = (x_char_t *)"-0xff";
	i = x_lib_strtoint(s, NULL, 0);
	_it_should("handle negative prefixed integers", -255 == i);



	s = (x_char_t *)" 00 ";
	i = x_lib_strtoint(s, &ps, 0);
	_it_should("convert 00 in base 0", 0 == i);
	_it_should("update end pointer", s + 3 == ps);


	s = (x_char_t *)" 0x0 ";
	i = x_lib_strtoint(s, &ps, 0);
	_it_should("convert 0x0 in base 0", 0 == i);
	_it_should("update end pointer", s + 4 == ps);


	s = (x_char_t *)" 0 ";
	i = x_lib_strtoint(s, &ps, 0);
	_it_should("convert 0 in base 0", 0 == i);
	_it_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 1);
	_it_should("convert 0 in base 1", 0 == i);
	/* NOTE: The result differs between GNU and BSD. */
	_xit_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 2);
	_it_should("convert 0 in base 2", 0 == i);
	_it_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 8);
	_it_should("convert 0 in base 8", 0 == i);
	_it_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 16);
	_it_should("convert 0 in base 16", 0 == i);
	_it_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 36);
	_it_should("convert 0 in base 36", 0 == i);
	_it_should("update end pointer", s + 2 == ps);



	s = (x_char_t *)" 01 ";
	i = x_lib_strtoint(s, &ps, 0);
	_it_should("convert 01 in base 0", 1 == i);
	_it_should("update end pointer", s + 3 == ps);


	s = (x_char_t *)" 0x1 ";
	i = x_lib_strtoint(s, &ps, 0);
	_it_should("convert 0x1 in base 0", 1 == i);
	_it_should("update end pointer", s + 4 == ps);


	s = (x_char_t *)" 1 ";
	i = x_lib_strtoint(s, &ps, 0);
	_it_should("convert 1 in base 0", 1 == i);
	_it_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 1);
	_it_should("convert 1 in base 1", 0 == i);
	_xit_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 2);
	_it_should("convert 1 in base 2", 1 == i);
	_it_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 8);
	_it_should("convert 1 in base 8", 1 == i);
	_it_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 16);
	_it_should("convert 1 in base 16", 1 == i);
	_it_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 36);
	_it_should("convert 1 in base 36", 1 == i);
	_it_should("update end pointer", s + 2 == ps);



	s = (x_char_t *)" 02 ";
	i = x_lib_strtoint(s, &ps, 0);
	_it_should("convert 02 in base 0", 2 == i);
	_it_should("update end pointer", s + 3 == ps);


	s = (x_char_t *)" 0x2 ";
	i = x_lib_strtoint(s, &ps, 0);
	_it_should("convert 0x2 in base 0", 2 == i);
	_it_should("update end pointer", s + 4 == ps);


	s = (x_char_t *)" 2 ";
	i = x_lib_strtoint(s, &ps, 0);
	_it_should("convert 2 in base 0", 2 == i);
	_it_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 1);
	_it_should("convert 2 in base 1", 0 == i);
	_xit_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 2);
	_it_should("convert 2 in base 2", 0 == i);
	_xit_should("update end pointer", s == ps);

	i = x_lib_strtoint(s, &ps, 8);
	_it_should("convert 2 in base 8", 2 == i);
	_it_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 16);
	_it_should("convert 2 in base 16", 2 == i);
	_it_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 36);
	_it_should("convert 2 in base 36", 2 == i);
	_it_should("update end pointer", s + 2 == ps);



	s = (x_char_t *)" 07 ";
	i = x_lib_strtoint(s, &ps, 0);
	_it_should("convert 07 in base 0", 7 == i);
	_it_should("update end pointer", s + 3 == ps);


	s = (x_char_t *)" 0x7 ";
	i = x_lib_strtoint(s, &ps, 0);
	_it_should("convert 0x7 in base 0", 7 == i);
	_it_should("update end pointer", s + 4 == ps);


	s = (x_char_t *)" 7 ";
	i = x_lib_strtoint(s, &ps, 0);
	_it_should("convert 7 in base 0", 7 == i);
	_it_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 1);
	_it_should("convert 7 in base 1", 0 == i);
	_xit_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 2);
	_it_should("convert 7 in base 2", 0 == i);
	_xit_should("update end pointer", s == ps);

	i = x_lib_strtoint(s, &ps, 8);
	_it_should("convert 7 in base 8", 7 == i);
	_it_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 16);
	_it_should("convert 7 in base 16", 7 == i);
	_it_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 36);
	_it_should("convert 7 in base 36", 7 == i);
	_it_should("update end pointer", s + 2 == ps);



	s = (x_char_t *)" 08 ";
	i = x_lib_strtoint(s, &ps, 0);
	_it_should("convert 08 in base 0", 0 == i);
	_it_should("update end pointer", s + 2 == ps);


	s = (x_char_t *)" 0x8 ";
	i = x_lib_strtoint(s, &ps, 0);
	_it_should("convert 0x8 in base 0", 8 == i);
	_it_should("update end pointer", s + 4 == ps);


	s = (x_char_t *)" 8 ";
	i = x_lib_strtoint(s, &ps, 0);
	_it_should("convert 8 in base 0", 8 == i);
	_it_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 1);
	_it_should("convert 8 in base 1", 0 == i);
	_xit_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 2);
	_it_should("convert 8 in base 2", 0 == i);
	_xit_should("update end pointer", s == ps);

	i = x_lib_strtoint(s, &ps, 8);
	_it_should("convert 8 in base 8", 0 == i);
	_xit_should("update end pointer", s == ps);

	i = x_lib_strtoint(s, &ps, 16);
	_it_should("convert 8 in base 16", 8 == i);
	_it_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 36);
	_it_should("convert 8 in base 36", 8 == i);
	_it_should("update end pointer", s + 2 == ps);



	s = (x_char_t *)" 0xf ";
	i = x_lib_strtoint(s, &ps, 0);
	_it_should("convert 0xf in base 0", 15 == i);
	_it_should("update end pointer", s + 4 == ps);


	s = (x_char_t *)" f ";
	i = x_lib_strtoint(s, &ps, 0);
	_it_should("convert f in base 0", 0 == i);
	_xit_should("update end pointer", s == ps);

	i = x_lib_strtoint(s, &ps, 1);
	_it_should("convert f in base 1", 0 == i);
	_xit_should("update end pointer", s == ps);

	i = x_lib_strtoint(s, &ps, 2);
	_it_should("convert f in base 2", 0 == i);
	_xit_should("update end pointer", s == ps);

	i = x_lib_strtoint(s, &ps, 8);
	_it_should("convert f in base 8", 0 == i);
	_xit_should("update end pointer", s == ps);

	i = x_lib_strtoint(s, &ps, 16);
	_it_should("convert f in base 16", 15 == i);
	_it_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 36);
	_it_should("convert f in base 36", 15 == i);
	_it_should("update end pointer", s + 2 == ps);



	s = (x_char_t *)" 0x10 ";
	i = x_lib_strtoint(s, &ps, 0);
	_it_should("convert 0x10 in base 0", 16 == i);
	_it_should("update end pointer", s + 5 == ps);


	s = (x_char_t *)" 10 ";
	i = x_lib_strtoint(s, &ps, 0);
	_it_should("convert 10 in base 0", 10 == i);
	_it_should("update end pointer", s + 3 == ps);

	i = x_lib_strtoint(s, &ps, 1);
	_it_should("convert 10 in base 1", 0 == i);
	_xit_should("update end pointer", s + 3 == ps);

	i = x_lib_strtoint(s, &ps, 2);
	_it_should("convert 10 in base 2", 2 == i);
	_it_should("update end pointer", s + 3 == ps);

	i = x_lib_strtoint(s, &ps, 8);
	_it_should("convert 10 in base 8", 8 == i);
	_it_should("update end pointer", s + 3 == ps);

	i = x_lib_strtoint(s, &ps, 16);
	_it_should("convert 10 in base 16", 16 == i);
	_it_should("update end pointer", s + 3 == ps);

	i = x_lib_strtoint(s, &ps, 36);
	_it_should("convert 10 in base 36", 36 == i);
	_it_should("update end pointer", s + 3 == ps);



	s = (x_char_t *)" 0x1f ";
	i = x_lib_strtoint(s, &ps, 0);
	_it_should("convert 0x1f in base 0", 31 == i);
	_it_should("update end pointer", s + 5 == ps);


	s = (x_char_t *)" 1f ";
	i = x_lib_strtoint(s, &ps, 0);
	_it_should("convert 1f in base 0", 1 == i);
	_it_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 1);
	_it_should("convert 1f in base 1", 0 == i);
	_xit_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 2);
	_it_should("convert 1f in base 2", 1 == i);
	_it_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 8);
	_it_should("convert 1f in base 8", 1 == i);
	_it_should("update end pointer", s + 2 == ps);

	i = x_lib_strtoint(s, &ps, 16);
	_it_should("convert 1f in base 16", 31 == i);
	_it_should("update end pointer", s + 3 == ps);

	i = x_lib_strtoint(s, &ps, 36);
	_it_should("convert 1f in base 36", 51 == i);
	_it_should("update end pointer", s + 3 == ps);


	return NULL;
}

static char *run_tests() {
	_run_test(test_lib_inttostr);
	_run_test(test_lib_memcpy);
	_run_test(test_lib_memdup);
	_run_test(test_lib_memset);
	_run_test(test_lib_strchr);
	_run_test(test_lib_strcmp);
	_run_test(test_lib_strlen);
	_run_test(test_lib_strncmp);
	_run_test(test_lib_strndup);
	_run_test(test_lib_strtoint);

	return NULL;
}
