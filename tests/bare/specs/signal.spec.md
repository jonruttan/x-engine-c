# Bare engine signals

`signal catch` records a signal's arrival and does nothing else; `signal
take` reports whether it arrived and clears the record.  SIGWINCH is 28 on
Darwin and Linux alike, and its default action is to ignore it, so a case
that raises it changes nothing when the catch did not take.  The signal is
raised through libc's `raise`, reached by the CLI's dlopen and dlsym.

### a number that names no signal is refused

```scheme
(include "tests/bare/prim-ref.x")
(def %catch (%prim-ref (lit signal) (lit catch)))
(def %take (%prim-ref (lit signal) (lit take)))
(match ((eq? %catch ()) (error "no catch"))
       ((eq? %take ()) (error "no take"))
       ((= (%catch 0) -1)
        (match ((= (%catch 100000) -1) (error "refused"))
               (#t (error "a large number caught"))))
       (#t (error "zero caught")))
```
---
    *** ERROR: refused

### a signal that cannot be caught is refused

```scheme
(include "tests/bare/prim-ref.x")
(def %catch (%prim-ref (lit signal) (lit catch)))
(match ((eq? %catch ()) (error "no catch"))
       ((= (%catch 9) -1) (error "refused"))
       (#t (error "SIGKILL caught")))
```
---
    *** ERROR: refused

### a caught signal is taken once

```scheme
(include "tests/bare/prim-ref.x")
(def %catch (%prim-ref (lit signal) (lit catch)))
(def %take (%prim-ref (lit signal) (lit take)))
(def %call (%prim-ref (lit ptr) (lit call)))
(def %raise ((%prim-ref (lit ffi) (lit dlsym)) ((%prim-ref (lit ffi) (lit dlopen)) () 1) "raise"))
(match ((eq? %raise ()) (error "no raise"))
       ((= (%catch 28) 0)
        (match ((= (%take 28) 1) (error "arrived before it was raised"))
               (#t ((fn (_) (%call %raise 28) (%call %raise 28)
                       (match ((= (%take 28) 1)
                               (match ((= (%take 28) 0) (error "taken once"))
                                      (#t (error "taken twice"))))
                              (#t (error "not recorded"))))))))
       (#t (error "not caught")))
```
---
    *** ERROR: taken once

### a number that names no signal has not arrived

```scheme
(include "tests/bare/prim-ref.x")
(def %take (%prim-ref (lit signal) (lit take)))
(match ((eq? %take ()) (error "no take"))
       ((= (%take 0) 0) (match ((= (%take -5) 0) (error "none")) (#t (error "negative arrived"))))
       (#t (error "zero arrived")))
```
---
    *** ERROR: none
