; tools/obj-layout.x — canonical layout of every object's header words.
;
; SINGLE SOURCE OF TRUTH for the object memory contract (the counterpart of
; tools/base-layout.x, which covers the base pair-tree).  The interpreter is
; fully reflective -- %obj->ptr + %ptr-ref-word reach every word of every
; object -- and reflective X code (the ISA-audit migrations) must read its
; offsets from THIS committed contract, not folklore constants.  Consumed:
;   1. by X at runtime: plain boot-level defs (only `def` and integers), so
;      any module or spec may (include "tools/obj-layout.x")
;   2. tests/x/specs/meta/obj-layout.spec.md -- probes the LIVE build's
;      objects word by word and fails if reality disagrees with these values
;   3. tools/obj-layout-scan.sh (make check-obj-layout) -- parses the same
;      values out of ext/x-expr/include/x-obj.h and diffs, so an x-expr bump
;      that moves the layout fails the build even before anything runs
;
; Units are x_obj_t WORDS; multiply by %word-size for byte offsets.
; VALUES DESCRIBE THE X_HEAP BUILD (every shipped personality): without
; -DX_HEAP the heap link vanishes (units-heap 0) and later slots shift down.
;
; FORMAT (rigid, one entry per line -- the awk parses the same bytes):
;   (def %obj-<name> <decimal integer>)
;
; An object is laid out as:
;   [meta -N ... meta -1][heap][type][flags][data 0][data 1 ...]
;                        ^ the object POINTER addresses the heap word
; Extended metadata (present iff %obj-flag-meta is set in the flags word)
; is PREPENDED: meta unit I lives at word -(I+1) relative to the object
; pointer (C: x_obj_meta_i).  Data begins at word %obj-meta-len.

; --- header slots (words, relative to the object pointer) ---
(def %obj-units-heap 1)   ; heap-chain link; 0 in non-X_HEAP builds
(def %obj-units-type 1)   ; pointer to the type object (nil for none)
(def %obj-units-flags 1)  ; the flags bitfield, held as an integer
(def %obj-slot-heap 0)
(def %obj-slot-type 1)
(def %obj-slot-flags 2)
(def %obj-meta-len 3)     ; header length = the word where data begins

; --- data shapes (words, relative to data start) ---
(def %obj-units-atom 1)   ; atom: the value word (int / str ptr / char)
(def %obj-units-pair 2)   ; pair: first at data 0, rest at data 1
(def %obj-slot-first 0)
(def %obj-slot-rest 1)

; --- flags word bits (decimal; hex noted in comments) ---
; Low nibble: general-purpose attribute bits.  Their MEANING is per-type:
; flag-1 is WRAP on procedures (wrapped applicative) and SHADOW on env
; pairs; flag-2 is COV.  The x-eval layer owns those aliases.
(def %obj-flag-attr-mask 15)    ; 0x0F
(def %obj-flag-1 1)             ; 0x01  WRAP / SHADOW
(def %obj-flag-2 2)             ; 0x02  COV
(def %obj-flag-3 4)             ; 0x04
(def %obj-flag-4 8)             ; 0x08
; Simple-type code (advisory tag for C consumers; NOT the type slot)
(def %obj-flag-simple-type 16)  ; 0x10  marker bit: a simple-type code follows
(def %obj-flag-prim 16)         ; 0x10
(def %obj-flag-fn 17)           ; 0x11
(def %obj-flag-int 18)          ; 0x12
(def %obj-flag-char 19)         ; 0x13
(def %obj-flag-str 20)          ; 0x14
(def %obj-flag-ptr 21)          ; 0x15
(def %obj-flag-type-mask 240)   ; 0xF0
; Independent attribute bits
(def %obj-flag-own 32)          ; 0x20  object owns its (string) storage
(def %obj-flag-ro 64)           ; 0x40  read-only
(def %obj-flag-meta 128)        ; 0x80  extended meta units prepended
; X_HEAP-only bits
(def %obj-flag-shared 256)      ; 0x100
(def %obj-flag-mark 512)        ; 0x200  the GC mark bit (was %obj-flag-heap pre-B1)
