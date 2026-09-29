; tools/contract/base-slots.x — the positions in the base's slot vector.
;
; SINGLE SOURCE OF TRUTH for the slot vector: the base's first data unit holds
; an object whose data units are function pointers, one per slot, and a routine
; the engine calls through a slot is replaced by storing another function in
; it.  This is the fourth layout contract, after tools/contract/base-layout.x,
; tools/contract/obj-layout.x and tools/contract/base-paths.x.  Consumed:
;   1. by X at runtime: a position is read from here, never from a literal
;   2. tools/check/base-slots.sh (make check-base-slots) -- reads the positions
;      from the two headers and diffs, so a slot that moves fails the build
;      before anything runs
;
; The slot vector is a vector: its first data unit holds its length, and slot
; I is its element I, in data unit I + 1.
;
; A slot function has the engine's one signature, (base args), and args is an
; argument vector: a vector whose elements are the routine's arguments, in the
; order the third element of a row gives them.  Each argument is an object; an
; integer or a string travels in an atom.
;
; FORMAT (rigid, one entry per line -- the awk parses the same bytes):
;   (position name (argument...))
; Positions 0 to 10 are x-expr's (ext/x-expr/include/x-slots.h); the rest are
; the engine's (include/x-eval-slots.h).
; Regenerate with: sh tools/check/base-slots.sh --gen

(def %base-slots (lit (
  (0 type-name (object))
  (1 units (object))
  (2 length (object))
  (3 error (message object))
  (4 heap-mark (object flags))
  (5 heap-free (object))
  (6 obj-alloc (type flags units))
  (7 obj-free (object))
  (8 heap-tree-mark (object flags))
  (9 heap-sweep (object flags))
  (10 heap-root-chain-mark (flags))
  (11 eval (args))
  (12 eval-arg (arg))
  (13 eval-list (args))
  (14 eval-body (body))
  (15 eval-body-tco (body))
  (16 eval-tco-trampoline (result))
  (17 eval-op-body (body caller))
  (18 callable-call (args))
  (19 callable-apply (args))
  (20 obj-prim-call (args))
  (21 env-lookup (env symbol))
  (22 env-bind (env symbol value))
  (23 env-extend (parent params values))
  (24 alist-bst-lookup (tree symbol))
  (25 token-read (args))
  (26 token-analyse (args label))
  (27 token-delimit (args))
)))
