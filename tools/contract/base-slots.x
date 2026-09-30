; tools/contract/base-slots.x — the positions in the base's slot vector.
;
; SINGLE SOURCE OF TRUTH for the slot vector: the base's `slots` field holds
; a vector whose elements are function pointers, one per slot, and a routine
; the engine calls through a slot is replaced by storing another function in
; it.  This is the fourth layout contract, after tools/contract/base-layout.x,
; tools/contract/obj-layout.x and tools/contract/base-paths.x.  Consumed by
; tools/check/base-slots.sh (make check-base-slots), which reads the positions
; from include/x-eval-slots.h and diffs, so a slot that moves fails the build
; before anything runs.  A language that replaces a routine reads its position
; from here, never from a literal.
;
; The slot vector is a vector: its first data unit holds its length, and slot
; I is its element I, in data unit I + 1.
;
; A routine in a slot has the engine's one signature, (base args), and args
; is an argument run: a run of datum words, one per argument, in the order
; the third element of a row gives them.  The fourth element gives the type
; of the word each argument travels in: object, integer or string.
;
; FORMAT (rigid, one entry per line -- the awk parses the same bytes):
;   (position name (argument...) (type...))
; Regenerate with: sh tools/check/base-slots.sh --gen

(def %base-slots (lit (
  (0 eval (expression) (object))
  (1 eval-list (args) (object))
  (2 eval-body (body) (object))
  (3 eval-body-tco (body) (object))
  (4 eval-tco-trampoline (result) (object))
  (5 eval-op-body (body caller) (object object))
  (6 callable-call (args) (object))
  (7 callable-apply (args) (object))
  (8 obj-prim-call (args) (object))
  (9 env-lookup (env symbol) (object object))
  (10 env-bind (env symbol value) (object object object))
  (11 env-extend (parent params values) (object object object))
  (12 alist-bst-lookup (tree symbol) (object object))
  (13 token-read (args) (object))
  (14 token-analyse (args label) (object integer))
  (15 token-delimit (args) (object))
)))
