/*
 * # Unit Tests: *rl*
 */

#include "test-runner.h"
#include "test-helper-mem.h"

#include "../../src/rl.c"

/*
 * ## System Functions
 */
void *sys_malloc(size_t size)
{
	return helper_malloc(size);
}

void sys_free(void *ptr)
{
	helper_free(ptr);
}

ssize_t sys_read(int fd, void *p_buf, size_t size)
{
	return read(fd, p_buf, size);
}

ssize_t sys_write(int fd, const void *p_buf, size_t size)
{
	return write(fd, p_buf, size);
}

void sys_error(char *message, char *symbol)
{
	fprintf(stderr, "*** ERROR: %s", message);
	if(symbol)
		fprintf(stderr, " '%s", symbol);
	fprintf(stderr, "\n");
}

void sys_debug(char *fmt, ...) {};
void sys_obj_dump(x_obj_t *p_obj, char *msg) {};


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

static char *run_tests() {

	return NULL;
}
