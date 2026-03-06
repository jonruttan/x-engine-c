Title:       X-Expressions Development Notes  
Description: Development notes for X-Expressions.  
Keywords:    [#X, #X-Exp, #Computational, #Expressions, #Dev, #Notes]  
Author:      "[Jon Ruttan](jonruttan@gmail.com)"  
Date:        2021-10-05  
Revision:    5 (2021-10-06)  

# Notes

## Filenames

***Preferred:***

```regex
[0-9a-zA-Z][-.0-9a-zA-Z]*\.md
```

***Expanded:***

```regex
[0-9a-zA-Z_][-.0-9a-zA-Z_]*\.md
```

***Minimal:***

```regex
[0-9A-Z][-.0-9A-Z]*\.MD
```


### Alternates

***QR Alphanumeric***

Full set:

```regex
[- $%*+./:0-9A-Z]*
```

Filenames:

```regex
[0-9A-Z][-.0-9A-Z]*
```

---



## X-Objects

### Freeing Using GC links

```c
  /* Free everything by following the GC links, p_base must be last. */
  x_obj_t *p_base, *p_gc, *p_tmp;

  p_gc = p_base->gc;
  while (p_gc) {
    p_tmp = p_gc->gc;
    x_sys_free(p_gc);
    p_gc = p_tmp;
  }
  x_sys_free(p_base);
```

---


## Testing

### Testing a Single Spec

```sh
TESTS=specs/c/1.0.x-lib.spec.c make test
```

### Test Helper File System

```c
#include "test-runner.h"
#include "test-helper-file.h"

static char *test_type_init(void)
{
  x_obj_t *p_top, *p_obj;

  char buffer[4096], *s;
  memset(buffer, 0, sizeof(buffer));

  helper_file_buffer_ptr[TEST_HELPER_FILE_STDOUT] = buffer;

  p_top = x_atom(NULL, NULL);

  helper_file_reset();
  p_obj = x_type_init(p_top, NULL);
  x_sexp_write(p_top, x_type_types(p_obj));
  s = helper_file_str(TEST_HELPER_FILE_STDOUT);
  x_debug(p_top, "TYPES:'%s'", s);

  return NULL;
}
```

#### Capturing File Data

```c
#include <stdio.h>
#include "test-helper-file.h"

/* Create a buffer for the captured data. */
char buffer[4096];

/* Attach the buffer to STDOUT. */
helper_file_buffer_ptr[TEST_HELPER_FILE_STDOUT] = buffer;

/* Reset the Helper File system. */
helper_file_reset();

/* Write to stdout. */
fputs("hello, world", stdout);

/* Get the captured data as a string, and print it. */
printf("STDOUT:'%s'", helper_file_str(TEST_HELPER_FILE_STDOUT));

```

#### Supplying File Data

```c
#include <stdio.h>
#include "test-helper-file.h"

/* Create a buffer to read into, and the supplied data. */
char buffer[4096], *s = "hello, world";

/* Attach the buffer to STDIN. */
helper_file_buffer_ptr[TEST_HELPER_FILE_STDIN] = s;

/* Reset the Helper File system. */
helper_file_reset();

/* Read from stdin, and print it. */
printf("STDIN:'%s'", fgets(buffer, 4096, stdin));

```