# A parent's collect keeps what it handed a child

`base-bind` binds a value in a child base's root environment.  The value
was made on the parent's heap and only the child's environment refers to
it.  A child base allocates on its parent's chain, so the parent's mark
reaches the value through the child and the parent's sweep clears every
bit it set, on every collect.

### a parent-made value bound into a child survives the parent's second collect

Allocation between the collects recycles what a sweep freed, so a lost
value reads back as something else.

```scheme
(include "tests/bare/prim-ref.x")
(def %make    (%prim-ref (lit base) (lit make)))
(def %bind    (%prim-ref (lit base) (lit bind)))
(def %eval    (%prim-ref (lit base) (lit eval)))
(def %collect (%prim-ref (lit heap) (lit collect)))
(def churn (fn (self n acc) (match ((= n 0) acc) (#t (self (- n 1) (pair n acc))))))
(def b (%make))
(%bind b (lit xs) (pair 1 (pair 2 (pair 3 ()))))
(%collect)
(churn 3000 ())
(%collect)
(churn 3000 ())
(error (match ((eq? (%all-found? (pair %make (pair %bind (pair %eval (pair %collect ()))))) #f) "prims missing")
              ((= (first (rest (rest (%eval b (lit xs))))) 3) "kept")
              (#t "lost")))
```
---
    *** ERROR: kept

### a collect run inside a child leaves the parent's heap alone

A child's cells sit on its parent's chain, so the child does not collect;
the parent's collect does it for both.

```scheme
(include "tests/bare/prim-ref.x")
(def %make    (%prim-ref (lit base) (lit make)))
(def %bind    (%prim-ref (lit base) (lit bind)))
(def %eval    (%prim-ref (lit base) (lit eval)))
(def %collect (%prim-ref (lit heap) (lit collect)))
(def churn (fn (self n acc) (match ((= n 0) acc) (#t (self (- n 1) (pair n acc))))))
(def keep (pair 1 (pair 2 (pair 3 ()))))
(def b (%make))
(%bind b (lit c) %collect)
(%eval b (lit (c)))
(churn 3000 ())
(%collect)
(error (match ((= (first (rest (rest keep))) 3) "kept")
              (#t "lost")))
```
---
    *** ERROR: kept
