# Environments are values

An environment is one pair, bindings and parent. A procedure call makes a
child of the closure's environment; an operative body runs in a child of
its own and receives the caller's environment as a value; `def` binds in
the current environment; `eval` with an environment makes it current, and
a `def` inside stays bound, because the environment is an object and the
binding is in it. A `define` built on that is the whole of what a lang
needs:

    (def define (op (n v) e (eval (pair (lit def) (pair n (pair (pair (lit lit) (pair (eval v e) ())) ()))) e)))

### a bare def in a closure body binds, as it always has

```scheme
((fn (_) (def a 1) (match ((= a 1) (error "bound")) (#t (error "no")))))
```
---
    *** ERROR: bound

### an operative's def, evaluated in the caller's env, binds in the caller's frame

```scheme
(def wrap (op (f) e (eval f e)))
((fn (_) (wrap (def b 2)) (match ((= b 2) (error "bound")) (#t (error "no")))))
```
---
    *** ERROR: bound

### a definition in body position is visible to the next one

```scheme
(def define (op (n v) e (eval (pair (lit def) (pair n (pair (pair (lit lit) (pair (eval v e) ())) ()))) e)))
((fn (_) (define a 1) (define b (+ a 1)) (match ((= b 2) (error "ok")) (#t (error "no")))))
```
---
    *** ERROR: ok

### the definition stays private to the frame

```scheme
(def define (op (n v) e (eval (pair (lit def) (pair n (pair (pair (lit lit) (pair (eval v e) ())) ()))) e)))
((fn (_) (define p 9) ()))
(match ((guard (x #f) p) (error "leaked")) (#t (error "unbound outside")))
```
---
    *** ERROR: unbound outside

### one more operative frame between does not lose it

```scheme
(def define (op (n v) e (eval (pair (lit def) (pair n (pair (pair (lit lit) (pair (eval v e) ())) ()))) e)))
(def framed (op (f . rest) e (eval (pair f rest) e)))
((fn (_) (framed define d 4) (match ((= d 4) (error "bound")) (#t (error "no")))))
```
---
    *** ERROR: bound

### a redefinition in the same frame updates in place

```scheme
(def define (op (n v) e (eval (pair (lit def) (pair n (pair (pair (lit lit) (pair (eval v e) ())) ()))) e)))
((fn (_) (define r 1) (define r 2) (match ((= r 2) (error "updated")) (#t (error "no")))))
```
---
    *** ERROR: updated

### a closure captured before the definition sees it

```scheme
(def define (op (n v) e (eval (pair (lit def) (pair n (pair (pair (lit lit) (pair (eval v e) ())) ()))) e)))
((fn (_) (define f (fn (_) (g))) (define g (fn (_) 7)) (match ((= (f) 7) (error "sees")) (#t (error "no")))))
```
---
    *** ERROR: sees

### at top level the environment is the root, and the binding is global

```scheme
(def define (op (n v) e (eval (pair (lit def) (pair n (pair (pair (lit lit) (pair (eval v e) ())) ()))) e)))
(define t 5)
((fn (_) (match ((= t 5) (error "global")) (#t (error "no")))))
```
---
    *** ERROR: global

### a parameterless body has an environment of its own

```scheme
((fn (_) (def a 1) ((fn () (def a 2))) (match ((= a 1) (error "kept")) (#t (error "no")))))
```
---
    *** ERROR: kept

### a parameter with a global's name shadows it for that body only

```scheme
(def g (fn (_) (pair 1 (pair 2 ()))))
((fn (_ list) (match ((eq? (first (g)) 1) (error "callee sees the global")) (#t (error "no")))) 1)
```
---
    *** ERROR: callee sees the global

### a top-level name shared with an env parameter does not hijack it

```scheme
(def e 42)
(def probe (op () e (eval (lit ok) e)))
(def ok 1)
(match ((= (probe) 1) (error "ok")) (#t (error "no")))
```
---
    *** ERROR: ok

### set! through two frames mutates the local, not the global

```scheme
(def z 1)
(def r ((fn (_ z) ((fn (_ y) (set! z 5) z) 9)) 2))
(match ((= r 5) (match ((= z 1) (error "local")) (#t (error "global changed")))) (#t (error "no")))
```
---
    *** ERROR: local

### an error handler runs in a child of the guard's environment

```scheme
((fn (_) (def a 1) (guard (x (def a 2) ()) (error "boom")) (match ((= a 1) (error "kept")) (#t (error "handler leaked")))))
```
---
    *** ERROR: kept
