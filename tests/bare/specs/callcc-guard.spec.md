# A continuation captured inside a guard is marked after the raise

call/cc keeps the installed error handler in the continuation's state list,
and the collector tree-marks that list while the continuation is reachable.
Every word of the handler tree is an object: the raise line is kept in the
base's err-line cell, never as a raw word in the handler's error cell.

### the collect in a handler clause marks a continuation captured in the body

The guard's handler is popped before its clause runs, so the continuation's
state list is the only path to it; the collect walks the record, error value
and all.

```scheme
(include "tests/bare/prim-ref.x")
(def %collect (%prim-ref (lit heap) (lit collect)))
(def %callcc (%prim-ref (lit ctrl) (lit call/cc)))
(guard (e (%collect) (%collect))
  (def %kept (%callcc (fn (_ k) k)))
  (error "boom"))
(error (match ((eq? %kept ()) "lost") (#t "kept")))
```
---
    *** ERROR: kept
