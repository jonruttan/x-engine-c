# A child base raises again after a collect

### an error caught from a child survives a collect before the next one

The first raise is an unbound symbol inside the child, caught in the parent.
A collect and some allocation run before a second raise in the same child,
also caught in the parent.

```scheme
(include "tests/bare/prim-ref.x")
(def %make    (%prim-ref (lit base) (lit make)))
(def %eval    (%prim-ref (lit base) (lit eval)))
(def %collect (%prim-ref (lit heap) (lit collect)))
(def churn (fn (self n acc) (match ((= n 0) acc) (#t (self (- n 1) (pair n acc))))))
(def b (%make))
(def first-raise (guard (e (lit caught)) (%eval b (lit nope))))
(%collect)
(churn 3000 ())
(%collect)
(churn 3000 ())
(def second-raise (guard (e (lit caught)) (%eval b (lit (error "boom")))))
(error (match ((eq? second-raise (lit caught)) "kept") (#t "lost")))
```
---
    *** ERROR: kept
