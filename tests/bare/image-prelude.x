; image-prelude.x -- what every case of tests/bare/specs/image.spec.md needs.
;
; The bare engine binds a handful of names; the rest of its surface is the
; prims catalog on the base, reached by the committed base paths, so the
; primitives the cases call are looked up there and bound under their bare
; names.  Word access is by the offsets tools/contract/obj-layout.x declares.
(include "tools/contract/obj-layout.x")
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
(def ptr-alloc      (%prim (lit ptr) (lit alloc)))
(def ptr-ref-word   (%prim (lit ptr) (lit ref-word)))
(def ptr-set-word!  (%prim (lit ptr) (lit set-word!)))
(def ptr-set!       (%prim (lit ptr) (lit set!)))
(def ptr-strlen     (%prim (lit ptr) (lit strlen)))
(def ptr->obj       (%prim (lit ptr) (lit ->obj)))
(def int->ptr       (%prim (lit int) (lit ->ptr)))
(def ptr->int       (%prim (lit ptr) (lit ->int)))
(def obj->ptr       (%prim (lit obj) (lit ->ptr)))
(def make-type      (%prim (lit type) (lit make)))
(def image-save!    (%prim (lit image) (lit save!)))
(def image-rebuild! (%prim (lit image) (lit rebuild!)))
(def image-write!   (%prim (lit image) (lit write!)))
;  The word size, as lib/img.x finds it: 2^32 survives a pointer round trip
; only where a pointer is wider than 32 bits.
(def %word-size (match ((< 0 (ptr->int (int->ptr 4294967296))) 8) (#t 4)))
; Word i of the memory at p; set it.
(def w (fn (_ p i) (ptr-ref-word p (* i %word-size))))
(def s (fn (_ p i v) (ptr-set-word! p (* i %word-size) v)))
; The struct the registry filed most recently: the type just made.
(def %newest-struct (fn (_) (rest (first (first (%cell (lit type-alist)))))))
