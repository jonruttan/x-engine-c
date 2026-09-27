; Loaded by smoke.spec.md's "a collect inside a load leaves the includer's
; frame alive".  Collect, then churn: a swept includer frame has to be
; RECYCLED before the includer resumes for the loss to show -- glibc's
; immediate reuse does that on its own, macOS's allocator mostly does not.
;
; A bare engine binds no name for the collector (the heap namespace lives in
; the catalogue only), so reach it through tests/bare/prim-ref.x.
(include "tests/bare/prim-ref.x")
(def %lc-collect (%prim-ref (lit heap) (lit collect)))
(%lc-collect)
(def %lc-churn (fn (self n) (match ((= n 0) 0) (#t (self (- n 1))))))
(%lc-churn 2000)
