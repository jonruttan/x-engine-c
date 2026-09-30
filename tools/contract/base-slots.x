; tools/contract/base-slots.x — the positions in the base's slot vector.
;
; SINGLE SOURCE OF TRUTH for the slot vector: the base's first data unit holds
; an object whose data units are function pointers, one per slot, and a routine
; the engine calls through a slot is replaced by storing another function in
; it.  This is the fourth layout contract, after tools/contract/base-layout.x,
; tools/contract/obj-layout.x and tools/contract/base-paths.x.  Consumed by
; tools/check/base-slots.sh (make check-base-slots), which reads the positions
; from the two headers and diffs, so a slot that moves fails the build before
; anything runs.  A language that replaces a routine reads its position from
; here, never from a literal.
;
; The slot vector is a vector: its first data unit holds its length, and slot
; I is its element I, in data unit I + 1.
;
; A routine in a slot has the engine's one signature, (base args), and args
; is an argument run: a run of datum words, one per argument, in the order
; the third element of a row gives them.  The fourth element gives the label
; of the word each argument travels in: object, integer or string.
;
; FORMAT (rigid, one entry per line -- the awk parses the same bytes):
;   (position name (argument...) (label...))
; Positions 0 to 10 are x-expr's (ext/x-expr/include/x-slots.h); the rest are
; the engine's (include/x-eval-slots.h).
; Regenerate with: sh tools/check/base-slots.sh --gen

(def %base-slots (lit (
  (0 type-name (object) (object))
  (1 units (object) (object))
  (2 length (object) (object))
  (3 error (message object) (string object))
  (4 heap-mark (object flags) (object integer))
  (5 heap-free (object) (object))
  (6 obj-alloc (type flags units) (object integer integer))
  (7 obj-free (object) (object))
  (8 heap-tree-mark (object flags) (object integer))
  (9 heap-sweep (object flags) (object integer))
  (10 heap-root-chain-mark (flags) (integer))
  (11 eval (expression) (object))
  (12 eval-list (args) (object))
  (13 eval-body (body) (object))
  (14 eval-body-tco (body) (object))
  (15 eval-tco-trampoline (result) (object))
  (16 eval-op-body (body caller) (object object))
  (17 callable-call (args) (object))
  (18 callable-apply (args) (object))
  (19 obj-prim-call (args) (object))
  (20 env-lookup (env symbol) (object object))
  (21 env-bind (env symbol value) (object object object))
  (22 env-extend (parent params values) (object object object))
  (23 alist-bst-lookup (tree symbol) (object object))
  (24 token-read (args) (object))
  (25 token-analyse (args label) (object integer))
  (26 token-delimit (args) (object))
)))
