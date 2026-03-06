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
static char *test_rl_prim_length(void)
{
	x_obj_t *p_obj;

	p_obj = obj_alloc(1, 2, 3);
	_it_should("make a new object", p_obj != NULL);
	_it_should("set the new object's type to the value given", x_obj_type(p_obj) == 1);
	_it_should("set the new object's flags to the value given", x_obj_flags(p_obj) == 2);
	_it_should("set the new object's gc pointer to NULL", x_obj_gc(p_obj) == NULL);
	sys_free(p_obj);

	return NULL;
}

static char *run_tests() {
	_run_test(test_rl_prim_length);

	return NULL;
}
