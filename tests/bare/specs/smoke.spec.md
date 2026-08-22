# Bare engine smoke

### an assertion needs no library and no syscall

```scheme
(match ((= (+ 1 2) 3) (error "ok")) (#t (error "no")))
```
---
    *** ERROR: ok

### the committed base paths reach the prims catalog

```scheme
(include "tools/contract/base-paths.x")
(def %assoc (fn (self k l)
  (match ((eq? l ()) ())
         ((eq? (first (first l)) k) (first l))
         (#t (self k (rest l))))))
(def %walk (fn (self steps o)
  (match ((eq? steps ()) o)
         ((eq? (first steps) (lit f)) (self (rest steps) (first o)))
         (#t (self (rest steps) (rest o))))))
(def %entry (%assoc (lit prims) %base-paths))
(def %cat (%walk (rest (rest %entry)) (%base)))
(match ((eq? %cat ()) (error "nil")) (#t (error "catalog")))
```
---
    *** ERROR: catalog
