; Loaded by smoke.spec.md's "a collect inside a load leaves the includer's
; frame alive".  Collect, then churn: a swept includer frame has to be
; RECYCLED before the includer resumes for the loss to show -- glibc's
; immediate reuse does that on its own, macOS's allocator mostly does not.
;
; A bare engine binds no name for the collector (the heap namespace lives in
; the catalog only), so reach it the way the base-paths smoke case does: walk
; the committed path to the prims cell and look the coordinate up by hand.
(include "tools/contract/base-paths.x")
(def %lc-assoc (fn (self k l)
  (match ((eq? l ()) ())
         ((eq? (first (first l)) k) (first l))
         (#t (self k (rest l))))))
(def %lc-walk (fn (self steps o)
  (match ((eq? steps ()) o)
         ((eq? (first steps) (lit f)) (self (rest steps) (first o)))
         (#t (self (rest steps) (rest o))))))
; The path reaches the prims CELL; the catalog is its first.
(def %lc-catalog (first (%lc-walk (rest (rest (%lc-assoc (lit prims) %base-paths))) (%base))))
(def %lc-collect (rest (%lc-assoc (lit collect) (rest (%lc-assoc (lit heap) %lc-catalog)))))
(%lc-collect)
(def %lc-churn (fn (self n) (match ((= n 0) 0) (#t (self (- n 1))))))
(%lc-churn 2000)
