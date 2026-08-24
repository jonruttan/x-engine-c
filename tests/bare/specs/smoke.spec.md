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

### a prim raises on a dotted argument list instead of walking off it

A prim reads its arguments by walking the spine, and a proper list ends at
nil while an improper one ends at an ATOM -- which the walk used to read as
a pair, dereferencing the atom's value word (#487).  No `guard` could catch
it, because a prim call never enters the applicative walk that #69 guarded.
The shape is reachable from ordinary text: with no float module loaded,
`1.5` reads as `(1 . 5)`, so `(= 1.5 1.5)` is exactly this call.

```scheme
(guard (e (error "raised")) (= 1 . 5))
```
---
    *** ERROR: raised

### the same guarantee reaches a prim that walks its own spine

`match` walks its clause list itself rather than through the shared
argument helpers, and the body walkers navigate a body the same way --
each terminated on nil alone, so each had the same hole.  The clauses here
are well-formed and only the TAIL is dotted, which is what isolates the
spine walk (a malformed clause is a different contract: the C layer
navigates first/rest unchecked by design).

```scheme
(guard (e (error "raised")) (match ((eq? 1 2) 3) . 5))
```
---
    *** ERROR: raised

### a dotted body raises the same way

```scheme
(guard (e (error "raised")) ((fn (_ x) x . 5) 2))
```
---
    *** ERROR: raised

### the walk stops where the answer is found

A clause that matches before the dotted tail still answers: the guard fires
only where the walk actually goes.

```scheme
(match ((eq? (match ((eq? 1 1) 3) . 5) 3) (error "answered")) (#t (error "no")))
```
---
    *** ERROR: answered

### a satisfied arity still ignores the junk tail

The guard fires only where the walk actually goes: a prim that never reads
past the proper prefix is unaffected, which is what it did before.

```scheme
(match ((eq? (= 1 2 . 5) #f) (error "ignored")) (#t (error "no")))
```
---
    *** ERROR: ignored
