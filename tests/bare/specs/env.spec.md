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

### a name is found by identity, not by spelling

Symbols intern per base. A name the host binds in a child under its own
symbol is not found by the child's symbol of the same spelling: the
lookup compares objects, and the same-spelled symbol is another name.
The conformance suite in x-lang states this law; the engine's own smoke
keeps a copy, because the root's bindings are a tree that steers by
spelling, and a spelling hit would satisfy every other test.

```scheme
(include "tests/bare/prim-ref.x")
(def %make  (%prim-ref (lit base) (lit make)))
(def %bind  (%prim-ref (lit base) (lit bind)))
(def %eval  (%prim-ref (lit base) (lit eval)))
(def %->sym (%prim-ref (lit str) (lit ->sym)))
(def b (%make))
(%bind b (lit answer) 42)
(%bind b (lit mk) %->sym)
(match ((eq? (%all-found? (pair %make (pair %bind (pair %eval (pair %->sym ()))))) #f)
        (error "prims missing"))
       ((guard (e #f) (%eval b (lit (eval! (mk "answer"))))) (error "found by spelling"))
       (#t (error "ok")))
```
---
    *** ERROR: ok

### a form the host read evaluates in a child: its symbols stand for the child's own

The host's `+` is not the child's `+` object, and the child's binding is
keyed by the child's. A foreign symbol has no identity in the child, so it
stands for the child's own symbol of its spelling, and the form runs.

```scheme
(include "tests/bare/prim-ref.x")
(def b ((%prim-ref (lit base) (lit make))))
(match ((eq? ((%prim-ref (lit base) (lit eval)) b (lit (+ 2 3))) 5) (error "ok")) (#t (error "no")))
```
---
    *** ERROR: ok

## an environment with many names

An environment under the root that comes to hold many bindings, as a
module's does, keeps a cache of the cells its lookups find, so a lookup
from inside it costs what one from the root does. What the environment
finds is unchanged: its own names, the root's names through it, a root name
it defines as its own, and the alist of its bindings, one cell a name.

### each name is found, and the root's names through it

```scheme
(def root ((op () e e)))
(def env (pair () root))
(def fill (fn (self l k) (match ((eq? l ()) k) (#t (self (rest l) (+ (eval (pair (lit def) (pair (first l) (pair k ()))) env) 1))))))
(fill (lit (n0 n1 n2 n3 n4 n5 n6 n7 n8 n9 n10 n11 n12 n13 n14 n15 n16 n17 n18 n19)) 0)
(match ((= (eval (lit n0) env) 0) (match ((= (eval (lit n19) env) 19) (match ((eq? (eval (lit (first (pair 1 2))) env) 1) (error "found")) (#t (error "root name")))) (#t (error "n19")))) (#t (error "n0")))
```
---
    *** ERROR: found

### a root name it defines, after a lookup reached the root, is its own

```scheme
(def root ((op () e e)))
(def env (pair () root))
(def fill (fn (self l k) (match ((eq? l ()) k) (#t (self (rest l) (+ (eval (pair (lit def) (pair (first l) (pair k ()))) env) 1))))))
(fill (lit (n0 n1 n2 n3 n4 n5 n6 n7 n8 n9 n10 n11 n12 n13 n14 n15 n16 n17 n18 n19)) 0)
(def g 1)
(eval (lit g) env)
(eval (lit (def g 2)) env)
(match ((= (eval (lit g) env) 2) (match ((= g 1) (error "own")) (#t (error "root changed")))) (#t (error "not shadowed")))
```
---
    *** ERROR: own

### set! of a root name through it changes the root's

```scheme
(def root ((op () e e)))
(def env (pair () root))
(def fill (fn (self l k) (match ((eq? l ()) k) (#t (self (rest l) (+ (eval (pair (lit def) (pair (first l) (pair k ()))) env) 1))))))
(fill (lit (n0 n1 n2 n3 n4 n5 n6 n7 n8 n9 n10 n11 n12 n13 n14 n15 n16 n17 n18 n19)) 0)
(def h 1)
(eval (lit h) env)
(eval (lit (set! h 3)) env)
(match ((= h 3) (match ((= (eval (lit h) env) 3) (error "root")) (#t (error "stale")))) (#t (error "no")))
```
---
    *** ERROR: root

### a root name defined after its lookups missed is found through it

```scheme
(def root ((op () e e)))
(def env (pair () root))
(def fill (fn (self l k) (match ((eq? l ()) k) (#t (self (rest l) (+ (eval (pair (lit def) (pair (first l) (pair k ()))) env) 1))))))
(fill (lit (n0 n1 n2 n3 n4 n5 n6 n7 n8 n9 n10 n11 n12 n13 n14 n15 n16 n17 n18 n19)) 0)
(guard (x ()) (eval (lit late) env))
(def late 8)
(match ((= (eval (lit late) env) 8) (error "found")) (#t (error "no")))
```
---
    *** ERROR: found

### its alist still holds each binding, one cell a name

```scheme
(def root ((op () e e)))
(def env (pair () root))
(def fill (fn (self l k) (match ((eq? l ()) k) (#t (self (rest l) (+ (eval (pair (lit def) (pair (first l) (pair k ()))) env) 1))))))
(fill (lit (n0 n1 n2 n3 n4 n5 n6 n7 n8 n9 n10 n11 n12 n13 n14 n15 n16 n17 n18 n19)) 0)
(eval (lit (def n7 70)) env)
(def cells (fn (self l n) (match ((eq? l ()) n) (#t (self (rest l) (match ((eq? (first (first l)) (lit n7)) (+ n 1)) (#t n)))))))
(def cell (fn (self l) (match ((eq? l ()) ()) ((eq? (first (first l)) (lit n7)) (first l)) (#t (self (rest l))))))
(match ((= (cells (first env) 0) 1) (match ((= (rest (cell (first env))) 70) (error "one")) (#t (error "stale cell")))) (#t (error "no")))
```
---
    *** ERROR: one
