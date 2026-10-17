# Continuations and the collector

A continuation is captured with the C stack it was called from and taken
up again by copying that stack back.  While it waits, its root chain is
in the copy: the objects a frame holds as roots -- an argument already
evaluated while the call's next argument runs -- are held by nothing
else, and the collector reads them from there.

The two continuations below hand control back and forth the way a
generator and its caller do.  The body suspends in the middle of building
a list, with the list's first element evaluated and held only by the
frame building it; the caller collects, and takes the body up again.

    (def %put (fn (_ name v) (eval (pair (lit def) (pair name (pair (pair (lit lit) (pair v ())) ()))) %top)))

### an object a suspended frame holds survives a collect

```scheme
(include "tests/bare/prim-ref.x")
(def %str-append (%prim-ref (lit str) (lit append)))
(def %collect (%prim-ref (lit heap) (lit collect)))
(def %top ((op () e e)))
(def %put (fn (_ name v) (eval (pair (lit def) (pair name (pair (pair (lit lit) (pair v ())) ()))) %top)))
(def %kgen ())
(def %kcall ())
(def %out ())
(def %yield (fn (_ v) (call/cc (fn (_ k) (%seq (%put (lit %kgen) k) (%kcall v))))))
(def %body (fn (_) (%seq (%put (lit %out) (pair (%str-append "x" "y") (pair (%yield 1) (pair (%str-append "p" "q") (pair (%yield 2) ()))))) (%kcall (lit done)))))
(def %resume (fn (_ v) (call/cc (fn (_ k) (%seq (%put (lit %kcall) k) (match ((eq? %kgen ()) (%body)) (#t (%kgen v))))))))
(def %fill (fn (self i) (match ((= i 0) ()) (#t (%seq (%str-append "garbage-" "fill") (self (- i 1)))))))
(def %r1 (%resume 0))
(%collect)
(%fill 5000)
(def %r2 (%resume 10))
(%collect)
(%fill 5000)
(def %r3 (%resume 20))
(match ((= %r1 1)
        (match ((= %r2 2)
                (match ((eq? %r3 (lit done))
                        (match ((= (first (rest %out)) 10)
                                (error (%str-append (first %out) (first (rest (rest %out))))))
                               (#t (error "sent"))))
                       (#t (error "done"))))
               (#t (error "second"))))
       (#t (error "first")))
```
---
    *** ERROR: xypq

### the same with no collect

```scheme
(include "tests/bare/prim-ref.x")
(def %str-append (%prim-ref (lit str) (lit append)))
(def %collect (%prim-ref (lit heap) (lit collect)))
(def %top ((op () e e)))
(def %put (fn (_ name v) (eval (pair (lit def) (pair name (pair (pair (lit lit) (pair v ())) ()))) %top)))
(def %kgen ())
(def %kcall ())
(def %out ())
(def %yield (fn (_ v) (call/cc (fn (_ k) (%seq (%put (lit %kgen) k) (%kcall v))))))
(def %body (fn (_) (%seq (%put (lit %out) (pair (%str-append "x" "y") (pair (%yield 1) (pair (%str-append "p" "q") (pair (%yield 2) ()))))) (%kcall (lit done)))))
(def %resume (fn (_ v) (call/cc (fn (_ k) (%seq (%put (lit %kcall) k) (match ((eq? %kgen ()) (%body)) (#t (%kgen v))))))))
(def %r1 (%resume 0))
(def %r2 (%resume 10))
(def %r3 (%resume 20))
(error (%str-append (first %out) (first (rest (rest %out)))))
```
---
    *** ERROR: xypq
