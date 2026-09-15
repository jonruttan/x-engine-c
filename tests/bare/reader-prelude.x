; reader-prelude.x -- what every case of tests/bare/specs/reader-eof.spec.md needs.
;
; The bare engine binds a handful of names; the rest of its surface is the
; prims catalog on the base, reached by the committed base paths, so the
; reader is looked up there and bound under its bare name.
(include "tools/contract/base-paths.x")
(def %assoc (fn (self k l)
  (match ((eq? l ()) ())
         ((eq? (first (first l)) k) (first l))
         (#t (self k (rest l))))))
(def %walk (fn (self steps o)
  (match ((eq? steps ()) o)
         ((eq? (first steps) (lit f)) (self (rest steps) (first o)))
         (#t (self (rest steps) (rest o))))))
(def %cell (fn (_ row) (%walk (rest (rest (%assoc row %base-paths))) (%base))))
(def %cat (first (%cell (lit prims))))
(def %prim (fn (_ ns nm) (rest (%assoc nm (rest (%assoc ns %cat))))))
(def read-str (%prim (lit tok) (lit read-str)))
