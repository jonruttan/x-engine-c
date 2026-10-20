# A type's mark handler may be a closure

`type make` takes a `mark` handler, and the collector calls it with the
object being marked and the mark flags.  A C primitive there gets the
argument run; a closure is applied to the same two values, inside the mark
phase, and what it marks it marks through `heap tree-mark!`.

### a closure mark handler runs at collect, with the object and the flags

The handler runs inside the mark phase, so it records into a raw buffer: a
binding made there is born after the marking passes it and is swept.

```scheme
(include "tests/bare/prim-ref.x")
(def %make-type (%prim-ref (lit type) (lit make)))
(def %make-instance (%prim-ref (lit type) (lit make-instance)))
(def %collect (%prim-ref (lit heap) (lit collect)))
(def %alloc (%prim-ref (lit ptr) (lit alloc)))
(def %ref-word (%prim-ref (lit ptr) (lit ref-word)))
(def %set-word! (%prim-ref (lit ptr) (lit set-word!)))
(def %ptr->int (%prim-ref (lit ptr) (lit ->int)))
(def %obj->ptr (%prim-ref (lit obj) (lit ->ptr)))
(def %seen (%alloc 64))
(def h (%make-type "MARKED" (pair (pair (lit mark) (fn (_ o flags) (%set-word! %seen 0 (%ptr->int (%obj->ptr o))) (%set-word! %seen 8 flags))) ())))
(def m (%make-instance h (pair 7 ())))
(%collect)
(match ((= (%ref-word %seen 0) 0) (error "never ran"))
       ((= (%ref-word %seen 0) (%ptr->int (%obj->ptr m)))
        (match ((= (%ref-word %seen 8) 512) (error "object and flags")) (#t (error "flags"))))
       (#t (error "object")))
```
---
    *** ERROR: object and flags

### what the handler marks survives the sweep

```scheme
(include "tests/bare/prim-ref.x")
(def %make-type (%prim-ref (lit type) (lit make)))
(def %make-instance (%prim-ref (lit type) (lit make-instance)))
(def %collect (%prim-ref (lit heap) (lit collect)))
(def %tree-mark! (%prim-ref (lit heap) (lit tree-mark!)))
(def %alloc (%prim-ref (lit ptr) (lit alloc)))
(def %ref-word (%prim-ref (lit ptr) (lit ref-word)))
(def %set-word! (%prim-ref (lit ptr) (lit set-word!)))
(def %ptr->obj (%prim-ref (lit ptr) (lit ->obj)))
(def %ptr->int (%prim-ref (lit ptr) (lit ->int)))
(def %int->ptr (%prim-ref (lit int) (lit ->ptr)))
(def %obj->ptr (%prim-ref (lit obj) (lit ->ptr)))
(def %kept (%alloc 64))
(def %held (fn (_) (%ptr->obj (%int->ptr (%ref-word %kept 0)))))
(def h (%make-type "KEEPER" (pair (pair (lit mark) (fn (_ o flags) (%tree-mark! (%held) flags))) ())))
(def m (%make-instance h ()))
(%set-word! %kept 0 (%ptr->int (%obj->ptr (pair 1 (pair 2 (pair 3 ()))))))
(%collect)
(%collect)
(error (match ((= (first (rest (rest (%held)))) 3) "kept") (#t "lost")))
```
---
    *** ERROR: kept
