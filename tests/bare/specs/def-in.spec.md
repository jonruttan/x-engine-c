# def-in: a define directed at a given environment

An operative cannot define for its caller with a plain `def`: inside a
frame the binding lands in the operative's own frame and is dropped with
it. `(base def-in)` binds in the environment the caller names, so a
definer written as an operative works inside a frame. Each case below
reaches the primitive through the catalog, the way the library does, and
builds a `define` on it:

    (def define (op (n v) e (def-in e n (eval v e))))

### a bare def in a closure body binds, as it always has

```scheme
((fn (_) (def a 1) (match ((= a 1) (error "bound")) (#t (error "no")))))
```
---
    *** ERROR: bound

### an operative's def, evaluated in the caller's env, is lost inside a frame

```scheme
(def wrap (op (f) e (eval f e)))
((fn (_) (wrap (def b 2)) (guard (x (error "lost")) b) (error "bound")))
```
---
    *** ERROR: lost

### def-in from an operative binds in the caller's frame

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
(def %cat (first (%walk (rest (rest (%assoc (lit prims) %base-paths))) (%base))))
(def def-in (rest (%assoc (lit def-in) (rest (%assoc (lit base) %cat)))))
(def define (op (n v) e (def-in e n (eval v e))))
((fn (_) (define c 3) (match ((= c 3) (error "bound")) (#t (error "no")))))
```
---
    *** ERROR: bound

### a definition in body position is visible to the next one

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
(def %cat (first (%walk (rest (rest (%assoc (lit prims) %base-paths))) (%base))))
(def def-in (rest (%assoc (lit def-in) (rest (%assoc (lit base) %cat)))))
(def define (op (n v) e (def-in e n (eval v e))))
((fn (_) (define a 1) (define b (+ a 1)) (match ((= b 2) (error "ok")) (#t (error "no")))))
```
---
    *** ERROR: ok

### the definition stays private to the frame

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
(def %cat (first (%walk (rest (rest (%assoc (lit prims) %base-paths))) (%base))))
(def def-in (rest (%assoc (lit def-in) (rest (%assoc (lit base) %cat)))))
(def define (op (n v) e (def-in e n (eval v e))))
((fn (_) (define p 9) ()))
(match ((guard (x #f) p) (error "leaked")) (#t (error "unbound outside")))
```
---
    *** ERROR: unbound outside

### one more operative frame between does not lose it

The case that broke every lang's `define`: a wrapper operative, such as a
lang's own `guard`, between the definer and the frame it defines for.

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
(def %cat (first (%walk (rest (rest (%assoc (lit prims) %base-paths))) (%base))))
(def def-in (rest (%assoc (lit def-in) (rest (%assoc (lit base) %cat)))))
(def define (op (n v) e (def-in e n (eval v e))))
(def framed (op (f . rest) e (eval (pair f rest) e)))
((fn (_) (framed define d 4) (match ((= d 4) (error "bound")) (#t (error "no")))))
```
---
    *** ERROR: bound

### a redefinition in the same frame updates in place

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
(def %cat (first (%walk (rest (rest (%assoc (lit prims) %base-paths))) (%base))))
(def def-in (rest (%assoc (lit def-in) (rest (%assoc (lit base) %cat)))))
(def define (op (n v) e (def-in e n (eval v e))))
((fn (_) (define r 1) (define r 2) (match ((= r 2) (error "updated")) (#t (error "no")))))
```
---
    *** ERROR: updated

### a closure captured before the definition sees it

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
(def %cat (first (%walk (rest (rest (%assoc (lit prims) %base-paths))) (%base))))
(def def-in (rest (%assoc (lit def-in) (rest (%assoc (lit base) %cat)))))
(def define (op (n v) e (def-in e n (eval v e))))
((fn (_) (define f (fn (_) (g))) (define g (fn (_) 7)) (match ((= (f) 7) (error "sees")) (#t (error "no")))))
```
---
    *** ERROR: sees

### at top level the environment is the global chain, and the binding is global

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
(def %cat (first (%walk (rest (rest (%assoc (lit prims) %base-paths))) (%base))))
(def def-in (rest (%assoc (lit def-in) (rest (%assoc (lit base) %cat)))))
(def define (op (n v) e (def-in e n (eval v e))))
(define t 5)
((fn (_) (match ((= t 5) (error "global")) (#t (error "no")))))
```
---
    *** ERROR: global
