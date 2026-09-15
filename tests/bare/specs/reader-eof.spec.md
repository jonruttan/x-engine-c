# The reader at end of input

A token the input ends on is read.  Each built-in leaf analyser keeps its
score current while it consumes, so when the source ends under it the span
so far is its claim; a literal that needs its closer is claimed the same way
and its reader raises, so a truncated string or character prefix is an error
rather than silence.  A custom analyser that scores only on a terminator is
untouched: it claims nothing at end of input, as `core/reader.spec.md` in
x-lang's conformance suite asserts.

Each case includes `tests/bare/reader-prelude.x`, which reaches `tok
read-str` through the prims catalog, and reads on the engine's own base.  A
case that expects a value ends in `(error ...)` whose text is the verdict, as
the smoke spec does; a case that expects a raise lets the raise print, since
a guarded condition writes as the bare word `error` here, and the `(error
"read")` after it is what prints if nothing was raised.

### an integer the input ends on is read

```scheme
(include "tests/bare/reader-prelude.x")
(def r (read-str (%base) "12 34"))
(error (match ((eq? (rest (rest r)) ()) (match ((= (first (rest r)) 34) "two") (#t "second"))) (#t "count")))
```
---
    *** ERROR: two

### a symbol the input ends on is read

```scheme
(include "tests/bare/reader-prelude.x")
(def r (read-str (%base) "ab cd"))
(error (match ((eq? (first (rest r)) (lit cd)) "cd") (#t "no")))
```
---
    *** ERROR: cd

### a comment the input ends on is discarded whole

```scheme
(include "tests/bare/reader-prelude.x")
(def r (read-str (%base) "1 ; note"))
(error (match ((eq? (rest r) ()) (match ((= (first r) 1) "one") (#t "first"))) (#t "count")))
```
---
    *** ERROR: one

### a character the input ends on is read

```scheme
(include "tests/bare/reader-prelude.x")
(def r (read-str (%base) "#\\a"))
(error (match ((eq? (first r) #\a) "char") (#t "no")))
```
---
    *** ERROR: char

### a string the input ends inside raises

```scheme
(include "tests/bare/reader-prelude.x")
(read-str (%base) "\"abc")
(error "read")
```
---
    *** ERROR: Unterminated input

### a character prefix the input ends on raises

```scheme
(include "tests/bare/reader-prelude.x")
(read-str (%base) "#\\")
(error "read")
```
---
    *** ERROR: Unterminated input

### a list the input ends inside still raises

```scheme
(include "tests/bare/reader-prelude.x")
(read-str (%base) "(1 2")
(error "read")
```
---
    *** ERROR: Unterminated input
