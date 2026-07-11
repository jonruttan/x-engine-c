Title:       X-Expressions Development Notes  
Description: Development notes for X-Expressions.  
Keywords:    [#X, #X-Exp, #Computational, #Expressions, #Dev, #Notes]  
Author:      "[Jon Ruttan](jonruttan@gmail.com)"  
Date:        2021-10-05  
Revision:    6 (2026-07-10)  

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


## Testing

### Testing a Single Spec

```sh
TESTS=tests/c/src/1.x-alist.spec.c make test-c
```

### Test Helpers

The C specs link against the [test-runner](tests/c/test-runner/) harness,
which provides helper systems for capturing and supplying file data.

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
