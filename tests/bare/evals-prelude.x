; evals-prelude.x -- what every case of tests/bare/profile/ needs.
;
; An X_PROFILE engine counts, in each object's flags word, how many times
; evaluation reached it.  The cases read that count the way reflective x-lang
; code must: the flags word at the offset tools/contract/obj-layout.x declares,
; shifted and masked by the %obj-evals-* rows beside it.  The raw word prims
; have no bare names, so they come from the catalogue.
(include "tools/contract/obj-layout.x")
(include "tests/bare/prim-ref.x")
(def obj->ptr     (%prim-ref (lit obj) (lit ->ptr)))
(def ptr-ref-word (%prim-ref (lit ptr) (lit ref-word)))
(def ptr->int     (%prim-ref (lit ptr) (lit ->int)))
(def int->ptr     (%prim-ref (lit int) (lit ->ptr)))
; #t when every lookup above found its primitive; a case checks it first.
(def %evals-prims-found (%all-found?
  (pair obj->ptr (pair ptr-ref-word (pair ptr->int (pair int->ptr ()))))))
; The word size, as image-prelude.x finds it.
(def %word-size (match ((< 0 (ptr->int (int->ptr 4294967296))) 8) (#t 4)))
; The eval count of o.
(def %evals (fn (_ o)
  (& (>> (ptr-ref-word (obj->ptr o) (* %obj-slot-flags %word-size))
         %obj-evals-shift)
     (- (<< 1 %obj-evals-bits) 1))))
; A procedure's body: the second cell of its state, (params body . env).
(def %body-of (fn (_ f) (first (rest (rest f)))))
