; tools/isa.x — the C instruction set: every C function reachable from X.
;
; SINGLE SOURCE OF TRUTH for the C surface, the "ISA" of the interpreter.
; The C layer is a CPU: unchecked, minimal, fixed.  Checks, dispatch, and
; policy live in X.  Consumed two ways, so the surface cannot drift silently:
;   1. tests/x/specs/meta/isa.spec.md -- walks the LIVE catalog at runtime
;      and fails on any C prim not listed here (and any stale entry)
;   2. tools/isa-scan.sh (make check-isa) -- extracts every binding site from
;      the C SOURCE and diffs it against this file, catching bare bindings
;      the runtime walk cannot see
; Growing the C layer therefore requires editing this manifest in the same
; commit -- a deliberate, reviewable act.  Shrinking it is always welcome.
;
; FORMAT (rigid, one entry per line -- the awk parses the same bytes):
;   %isa-catalog: (ns method tag)   filed in the prims catalog by C
;   %isa-bare:    (name tag)        bound bare by C, no catalog entry
;   %isa-values:  (name [tag])      non-prim VALUES bound by C
;
; Tags justify why the entry must be C.  An entry that cannot honestly take
; one of these tags does not belong in C -- it is a migration candidate:
;   spine    the evaluator/binder itself (eval, apply, fn/op, def, call/cc)
;   alloc    constructs heap objects (pair, atoms, instances)
;   gc       heap management (collect, hooks, limits)
;   raw-mem  unchecked memory/byte access (first, obj-ref, str byte-*)
;   raw-op   machine ALU/compare/cast ops (int +, eq?, char->int)
;   tok      tokenizer inner loop (buffers, token read)
;   io       the process I/O boundary (read, write, display)
;   ffi      the foreign-function/syscall door (dlopen, ptr calls)
;   sys      OS facilities (clock, signals)
;   types    the C type-object registry protocol (type-of, iter)
;   registry the prims catalog protocol itself (prim-ref, use)
;   hot      DERIVED (X-expressible via reflection) but kept in C on an
;            explicit exception: used inside reader lambdas (X calls allocate
;            arg spines; tokenizer callbacks must not allocate) or measured
;            per-element heat.  Re-audit whenever those constraints move.
; Audited 2026-07-13 under the REFLECTIVE test: the interpreter is fully
; reflective (%base + first/rest + %obj->ptr/%ptr-ref-word + the committed
; layouts), so the bar is not "touches interpreter internals" but "requires a
; capability X lacks".  Capabilities X genuinely lacks: allocation (incl.
; structural spairs -- X pair makes list-pairs only), GC, the eval trampoline,
; machine ops, syscall/FFI, signals, byte I/O, the zero-alloc tokenizer loop.
; Anything that just reads/writes state at a known layout offset is `review`
; (or `hot` when reader/heat-pinned).  Object-header layout (heap link word 0,
; type word 1, flags word 2, data at word 3) is informal knowledge today
; (boot/data.x %data-offset); migrations below want it formalized first as a
; committed descriptor (tools/obj-layout.x, gen-checked like base-layout).
;
; Flag-gated entries (present in the default build): (sys clock) needs
; X_SYS_CLOCK; sigint-install/sigint-restore/%sigint-flag need X_SIGNAL.
; #t/#f are bound from interned singletons, not name literals -- the scanner
; special-cases them.

(def %isa-catalog (lit (
  (alloc limit! review)       ; writes two base-tree cells (alloc-limit int, alloc-error str) -- reflective cell writes
  (base bind spine)           ; SURVIVES the reflective test: allocates a STRUCTURAL spair for the env spine, which X pair cannot make
  (base eval spine)
  (base make spine)
  (base make-tok spine)
  (base make-type spine)
  (buf append tok)
  (buf last-char tok)
  (buf read tok)
  (buf read-text tok)
  (buf reset tok)
  (buf retain tok)
  (buf tok tok)
  (bytes ->str alloc)
  (char ->int raw-op)
  (ctrl call/cc spine)
  (ffi call ffi)
  (ffi dlopen ffi)
  (ffi dlsym ffi)
  (heap collect gc)
  (heap count review)         ; cold diagnostic: walks the heap chain via header word 0, X-walkable from %base
  (heap free-hook! gc)
  (heap mark gc)
  (heap mark-hook! gc)
  (heap mark-root! gc)
  (heap pin! gc)
  (heap sweep gc)
  (int % raw-op)
  (int & raw-op)
  (int * raw-op)
  (int + raw-op)
  (int - raw-op)
  (int ->char raw-op)
  (int ->ptr ffi)
  (int / raw-op)
  (int < raw-op)
  (int << raw-op)
  (int = raw-op)
  (int >> raw-op)
  (int ^ raw-op)
  (int | raw-op)
  (int ~ raw-op)
  (io display review)        ; the structure-walking printer is POLICY; X owns repr already -- home over a raw byte-out door
  (io display-to-str review)  ; printer capture; becomes X (Stream) once the printer homes
  (io error-line review)      ; reads io-state (cell line) -- a reflective cell read
  (io read io)
  (io read-char io)
  (io repl-read io)
  (io write review)           ; same as display
  (io write-to-str review)    ; same as display-to-str
  (iter empty? hot)           ; derived dispatch, but per-ELEMENT hot (cached as %-vars in hot paths)
  (iter make types)
  (iter new review)           ; = fetch type iter-cell + apply, cold (runs once per iteration START); next/empty? stay C (hot)
  (iter next hot)             ; derived dispatch, but per-ELEMENT hot
  (obj ->ptr ffi)
  (obj eq? raw-op)
  (obj make alloc)
  (obj make-callable alloc)
  (obj meta-count review)     ; header-word sugar: all four meta ops are %ptr-ref/set-word at fixed offsets
  (obj meta-count! review)
  (obj meta-ref review)
  (obj meta-set! review)
  (obj ref review)            ; C body is literally (&firstobj(obj))[i] -- an indexed word read
  (obj same? raw-op)
  (obj set! review)           ; indexed word write; data.x set-first!/set-rest! already front it in X
  (ptr ->int ffi)
  (ptr ->str ffi)
  (ptr call ffi)
  (ptr ref raw-mem)
  (ptr ref-word raw-mem)
  (ptr set! raw-mem)
  (ptr set-word! raw-mem)
  (str ->ptr ffi)
  (str ->sym alloc)
  (str append alloc)
  (str byte-len hot)          ; derived (atom header read) but string inner-loop hot (utf8 codec, reader lambdas)
  (str byte-ref hot)          ; derived (byte read) but same inner-loop heat
  (str byte-sub raw-mem)
  (sym ->str alloc)
  (sys clock sys)
  (tok read tok)
  (tok read-str tok)
  (type ? review)             ; = eq? of the type name field against a handle, expressible over (type of)
  (type make types)
  (type make-instance alloc)
  (type name review)          ; type-object field walk; callers are boot-time cold -- audit reader lambdas on migration
  (type of review)            ; header word 1 read + name walk; lib uses are boot-time registration (cold)
)))

(def %isa-bare (lit (
  (%base spine)
  (%cc-invoke spine)
  (%seq spine)
  (apply spine)
  (atomic spine)
  (def spine)
  (error spine)
  (eval spine)
  (eval! spine)
  (first raw-mem)
  (fn spine)
  (guard spine)
  (include io)
  (lit spine)
  (match spine)
  (op spine)
  (pair alloc)
  (prim-domain review)        ; ZERO callers (doc prose only); a two-line X assoc over (prims)
  (prim-ref review)           ; assoc walk over the prims cell; callers fetch at BOOT (cold), caching the result
  (prim-reg! review)          ; setcar splice into the prims cell -- X has set-first! (data.x, via %obj-set!)
  (prims review)              ; reads (cell prims) from the base spine (base-layout.x state group)
  (rest raw-mem)
  (set! spine)
  (sigint-install sys)
  (sigint-restore sys)
  (syscall ffi)
  (tail-eval spine)
  (unwrap spine)
  (use review)                ; ZERO callers (doc prose only); X def-loop over (prims) + eval!
  (wrap spine)
)))

(def %isa-values (lit (
  (#t)
  (#f)
  (%sigint-flag)
  (args)
  (x-machine)
  (x-version)
)))
