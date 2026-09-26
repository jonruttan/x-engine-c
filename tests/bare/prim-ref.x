; Included by bare specs that must reach a primitive the engine binds under
; no name.  The catalog protocol (prims / prim-ref / use) is pure x-lang, so
; a bare engine has no prim-ref: reach the catalog the way the base-paths
; smoke case does -- walk the committed path to the prims cell and look the
; (namespace method) coordinate up by hand.
;
; (%prim-ref ns m) answers the primitive, or () when the catalog has no such
; entry.  A case calls it OUTSIDE any guard and asserts the answer non-nil
; first, so a missing primitive fails the case instead of raising into a
; guard that would count the miss as the error under test.
(include "tools/contract/base-paths.x")
(def %pr-assoc (fn (self k l)
  (match ((eq? l ()) ())
         ((eq? (first (first l)) k) (first l))
         (#t (self k (rest l))))))
(def %pr-walk (fn (self steps o)
  (match ((eq? steps ()) o)
         ((eq? (first steps) (lit f)) (self (rest steps) (first o)))
         (#t (self (rest steps) (rest o))))))
; The path reaches the prims CELL; the catalog is its first.
(def %pr-catalog (first (%pr-walk (rest (rest (%pr-assoc (lit prims) %base-paths))) (%base))))
(def %prim-ref (fn (_ ns m)
  ((fn (_ n)
     (match ((eq? n ()) ())
            (#t ((fn (_ e) (match ((eq? e ()) ()) (#t (rest e))))
                 (%pr-assoc m (rest n))))))
   (%pr-assoc ns %pr-catalog))))
