# The profile build's eval count

An X_PROFILE engine counts, in every object's flags word, how many times
evaluation reached it: each expression it evaluates and each body cell it
steps onto.  These cases run only against `x-bin-profile`
(`make test-bare-profile`) -- a build without X_PROFILE never writes the
count, so here every one of them would read zero.  They read the count the
way reflective x-lang code has to, through the committed contract
(`tools/contract/obj-layout.x`); `tests/bare/evals-prelude.x` has the reader.

### a procedure's first body cell counts its calls

```scheme
(include "tests/bare/evals-prelude.x")
(def %f (fn (_) (+ 1 2)))
(%f) (%f) (%f)
(match ((eq? %evals-prims-found #f) (error "prims missing"))
       ((= (%evals (%body-of %f)) 3) (error "three calls"))
       (#t (error "no")))
```
---
    *** ERROR: three calls

### a branch counts the times it was taken, and one never taken reads zero

One bit said only whether a branch ran.  The count says how often, and a
branch that never ran still reads zero, so coverage falls out of it.

```scheme
(include "tests/bare/evals-prelude.x")
(def %g (fn (_ x) (match ((= x 0) (+ 1 1)) (#t (+ 2 2)))))
(%g 1) (%g 1)
(def %clauses (rest (first (%body-of %g))))
(def %taken (first (rest (first (rest %clauses)))))
(def %not-taken (first (rest (first %clauses))))
(match ((eq? %evals-prims-found #f) (error "prims missing"))
       ((= (%evals %taken) 2)
         (match ((= (%evals %not-taken) 0) (error "two and none"))
                (#t (error "the branch not taken counted"))))
       (#t (error "no")))
```
---
    *** ERROR: two and none

### a tail call counts every time the trampoline runs it

The tail is not evaluated in place: the body hands it to the trampoline,
which evaluates it on the next pass.  It still counts, once a pass, so a
loop written as a tail call reads its iterations -- five tails -- and its
calls: the five they made and the one that started them.

```scheme
(include "tests/bare/evals-prelude.x")
(def %loop (fn (self n) (match ((= n 0) ()) (#t (self (- n 1))))))
(%loop 5)
(def %tail (first (rest (first (rest (rest (first (%body-of %loop))))))))
(match ((eq? %evals-prims-found #f) (error "prims missing"))
       ((= (%evals (%body-of %loop)) 6)
         (match ((= (%evals %tail) 5) (error "six calls, five tails"))
                (#t (error "the tail miscounted"))))
       (#t (error "the calls miscounted")))
```
---
    *** ERROR: six calls, five tails
