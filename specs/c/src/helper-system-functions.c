#ifndef HELPER_SYSTEM_FUNCTIONS
#define HELPER_SYSTEM_FUNCTIONS

#include "test-helper-mem.h"
#include "test-helper-file.h"


/*
 * ## System Functions
 */
void *x_sys_malloc(size_t size)
{
	return helper_malloc(size);
}

void x_sys_free(void *ptr)
{
	helper_free(ptr);
}

ssize_t x_sys_read(int fd, void *p_buf, size_t size)
{
	if (file_buffer_ptr[fd][TEST_HELPER_FILE_READ] == NULL) {
		return read(fd, p_buf, size);
	}

	return helper_file_read(fd, p_buf, size);
}

ssize_t x_sys_write(int fd, const void *p_buf, size_t size)
{
	if (file_buffer_ptr[fd][TEST_HELPER_FILE_WRITE] == NULL) {
		return write(fd, p_buf, size);
	}

	return helper_file_write(fd, p_buf, size);
}

int x_sys_exit_status = X_SYS_EXIT_SUCCESS;

void x_sys_exit(int status)
{
	x_sys_exit_status = status;
	/*exit(status);*/
}

#endif /* HELPER_SYSTEM_FUNCTIONS */
