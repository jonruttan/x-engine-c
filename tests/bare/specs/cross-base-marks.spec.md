# A parent's collect keeps what it handed a child

`base-bind` binds a value in a child base's root environment.  The value
was made on the parent's heap, so the parent's sweep decides its fate, and
the only thing that refers to it is the child's environment, on the
child's own chain.  The parent's mark walk reaches it through the child:
the walk enters the child base object, descends its tree and marks the
value.  That holds on every collect, not only the first.

### a parent-made value bound into a child survives the parent's second collect

The walk's mark bits on the child's cells outlive the parent's sweep, which
clears the parent's chain only.  A stale bit on the child base object stops
the next walk at its door, and the value goes unmarked.  Allocation between
the collects recycles what the sweep freed, so a lost value reads back as
something else.

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
