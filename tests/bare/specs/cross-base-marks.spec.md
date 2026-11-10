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

### a collect run inside a child keeps what the parent holds

A child's cells sit on its parent's chain, so a collect the child asks for
runs in the parent, with the parent's marks.

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

### a collect run inside a child reclaims the child's garbage

The child's garbage is on the parent's chain, so the parent's count drops
when the child collects.

```scheme
(include "tests/bare/prim-ref.x")
(def %make    (%prim-ref (lit base) (lit make)))
(def %bind    (%prim-ref (lit base) (lit bind)))
(def %eval    (%prim-ref (lit base) (lit eval)))
(def %collect (%prim-ref (lit heap) (lit collect)))
(def %count   (%prim-ref (lit heap) (lit count)))
(def churn (fn (self n acc) (match ((= n 0) acc) (#t (self (- n 1) (pair n acc))))))
(def b (%make))
(%bind b (lit c) %collect)
(%bind b (lit churn) churn)
(%collect)
(def before (%count))
(%eval b (lit (churn 20000 ())))
(%eval b (lit (c)))
(error (match ((< (%count) (+ before 2000)) "reclaimed")
              (#t "kept the garbage")))
```
---
    *** ERROR: reclaimed

### a collect run inside a child keeps what the child's evaluation holds

The list is an argument already evaluated, held by the child's call in
progress and by nothing else; the churn after the collect recycles what it
freed.

```scheme
(include "tests/bare/prim-ref.x")
(def %make    (%prim-ref (lit base) (lit make)))
(def %bind    (%prim-ref (lit base) (lit bind)))
(def %eval    (%prim-ref (lit base) (lit eval)))
(def %collect (%prim-ref (lit heap) (lit collect)))
(def churn (fn (self n acc) (match ((= n 0) acc) (#t (self (- n 1) (pair n acc))))))
(def b (%make))
(%bind b (lit c) %collect)
(%bind b (lit churn) churn)
(%bind b (lit third) (fn (_ xs done more) (first (rest (rest xs)))))
(error (match ((= (%eval b (lit (third (churn 100 ()) (c) (churn 3000 ())))) 3) "kept")
              (#t "lost")))
```
---
    *** ERROR: kept
