; image-prelude.x -- what every case of tests/bare/specs/image.spec.md needs.
;
; The bare engine binds a handful of names; the rest of its surface is the
; prims catalog on the base, reached by the committed base paths, so the
; primitives the cases call are looked up there and bound under their bare
; names.  Word access is by the offsets tools/contract/obj-layout.x declares.
(include "tools/contract/obj-layout.x")
(include "tests/bare/prim-ref.x")
(def ptr-alloc      (%prim-ref (lit ptr) (lit alloc)))
(def ptr-ref-word   (%prim-ref (lit ptr) (lit ref-word)))
(def ptr-set-word!  (%prim-ref (lit ptr) (lit set-word!)))
(def ptr-set!       (%prim-ref (lit ptr) (lit set!)))
(def ptr-strlen     (%prim-ref (lit ptr) (lit strlen)))
(def ptr->obj       (%prim-ref (lit ptr) (lit ->obj)))
(def int->ptr       (%prim-ref (lit int) (lit ->ptr)))
(def ptr->int       (%prim-ref (lit ptr) (lit ->int)))
(def obj->ptr       (%prim-ref (lit obj) (lit ->ptr)))
(def make-type      (%prim-ref (lit type) (lit make)))
(def image-save!    (%prim-ref (lit image) (lit save!)))
(def image-rebuild! (%prim-ref (lit image) (lit rebuild!)))
(def image-write!   (%prim-ref (lit image) (lit write!)))
; #t when every lookup above found its primitive.  A case whose verdict is
; a guard's checks it first: a missing primitive would otherwise raise into
; the guard -- directly, or through a name its setup never bound.
(def %image-prims-found (%all-found?
  (pair ptr-alloc (pair ptr-ref-word (pair ptr-set-word! (pair ptr-set!
  (pair ptr-strlen (pair ptr->obj (pair int->ptr (pair ptr->int
  (pair obj->ptr (pair make-type (pair image-save! (pair image-rebuild!
  (pair image-write! ()))))))))))))))))
;  The word size, as lib/img.x finds it: 2^32 survives a pointer round trip
; only where a pointer is wider than 32 bits.
(def %word-size (match ((< 0 (ptr->int (int->ptr 4294967296))) 8) (#t 4)))
; Word i of the memory at p; set it.
(def w (fn (_ p i) (ptr-ref-word p (* i %word-size))))
(def s (fn (_ p i v) (ptr-set-word! p (* i %word-size) v)))
; The struct the registry filed most recently: the type just made.
(def %newest-struct (fn (_) (rest (first (first (%base-cell (lit type-alist)))))))
