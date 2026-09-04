; tools/contract/isa.x — the C instruction set: every C function reachable from x-lang.
;
; SINGLE SOURCE OF TRUTH for the C surface, the "ISA" of the interpreter.
; The C layer is a CPU: unchecked, minimal, fixed.  Checks, dispatch, and
; policy live in X.  Consumed two ways, so the surface cannot drift silently:
;   1. tests/x/specs/meta/isa.spec.md -- walks the LIVE catalog at runtime
;      and fails on any C prim not listed here (and any stale entry)
;   2. tools/check/isa.sh (make check-isa) -- extracts every binding site from
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
; committed descriptor (tools/contract/obj-layout.x, gen-checked like base-layout).
;
; Flag-gated entries (present in the default build): (sys clock) needs
; X_SYS_CLOCK; sigint-install/sigint-restore/%sigint-flag need X_SIGNAL.
; #t/#f are bound from interned singletons, not name literals -- the scanner
; special-cases them.

(def %isa-catalog (lit (
  (alloc limit! gc)           ; the runaway guard: the spec harness arms it BEFORE any lib loads, so it
                              ;   must exist in a bare env -- C by necessity (derived otherwise)
  (base bind spine)           ; SURVIVES the reflective test: allocates a STRUCTURAL spair for the env spine, which X pair cannot make
  (base def-global spine)    ; binds in the GLOBAL env whatever the frame depth: `def` decides that by
  (base eval spine)
  (base make spine)
  (base make-tok spine)
  (base make-type spine)
  (buf append tok)
  (buf make alloc)            ; a BUFFER viewing a string's bytes (non-owning, the wrap rule);
                              ;   revives buffer construction from x. Added 2026-07-15 (user-approved)
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
  (heap count gc)             ; derived (heap-chain walk from header word 0) but O(heap): an interpreted
                              ;   walk of millions of objects takes minutes vs ms -- perf-pinned in C
  (heap free-hook! gc)
  (heap mark gc)
  (heap mark-hook! gc)
  (heap mark-root! gc)
  (heap pin! gc)
  (heap tree-mark! gc)        ; the collector's own tree traversal, with the flag to set as an
                              ;   argument.  The one way to ask what is reachable from an object
                              ;   and get an answer that counts the base sentinel, the custom mark
                              ;   handlers, the hooks and the root chain -- an X walk sees none of
                              ;   them, and a structural pair in a base spine can hold a raw C
                              ;   function pointer that following would dereference.  WHICH flag is
                              ;   X's problem: the collector owns SHARED and MARK, so a caller picks
                              ;   a bit above them and names it in its own source.
  (heap chain-clear! gc)      ; clear flags across the allocation chain, freeing nothing.  Sweeping
                              ;   clears too, but sweeping frees; and a tree clear would miss
                              ;   whatever became garbage since the mark.
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
  (int abs raw-op)            ; x_lib_abs
  (int << raw-op)
  (int = raw-op)
  (int >> raw-op)
  (int ^ raw-op)
  (int | raw-op)
  (int ~ raw-op)
  (image rebuild! alloc)      ; allocate and patch every object of a state image.  The only
                              ;   per-object half of loading one; everything else -- header,
                              ;   foreign names, base paths -- is per-entry work X does in
                              ;   milliseconds.  In X the same two passes take ~30s and
                              ;   exhaust memory, because collection is manual and a
                              ;   per-object interpreted loop has nowhere safe to collect.
  (io read io)
  (io read-char io)
  (io repl-read io)
  (io write-str io)           ; the OUT port instruction: raw bytes of a string to the current output;
                              ;   the pure-X printer bottoms out here. UNCHECKED like first/rest --
                              ;   callers pass a STR. Added 2026-07-14 (printer batch)
  (iter empty? hot)           ; derived dispatch, but per-ELEMENT hot (cached as %-vars in hot paths)
  (iter make types)
  (iter next hot)             ; derived dispatch, but per-ELEMENT hot
  (iter step hot)             ; functional step: (value . next-iter) | (), no mutation -- the
                              ;   generator view of an iterator; Gen runs on C steps through this.
                              ;   Added 2026-07-17 (iter recontract: pure steps + one driver)
  (mem alloc alloc)           ; raw UNMANAGED region as a ptr, zeroed; caller must (mem free) it.
                              ;   Prefer (str make) (GC-owned). UNCHECKED like first/rest.
                              ;   Added 2026-07-15 (user-approved: the missing malloc door)
  (mem cmp raw-mem)           ; block compare (TRUE memcmp: NULs don't terminate) -> 0/-1/1.
                              ;   The machine's rep-cmps; str=? bottoms out here. UNCHECKED.
                              ;   Added 2026-07-15 (user-approved: block-op round)
  (mem copy raw-mem)          ; block copy (memcpy, non-overlapping); the machine's rep-movs.
                              ;   UNCHECKED. Added 2026-07-15 (user-approved: block-op round)
  (mem free alloc)            ; release a (mem alloc) region; double/foreign free is UB as in C
  (mem set raw-mem)           ; block fill (memset); the machine's rep-stos. UNCHECKED.
                              ;   Added 2026-07-15 (user-approved: block-op round)
  (obj ->ptr ffi)
  (obj eq? raw-op)
  (obj make alloc)
  (obj make-callable alloc)
  (obj same? raw-op)
  (ptr ->int ffi)
  (ptr ->obj ffi)             ; the materialization instruction (inverse of obj ->ptr): read a word AS an
                              ;   object. UNCHECKED like first/rest. Added 2026-07-14 (user-approved) --
                              ;   the ONE instruction that lets reflective X accessors return objects
  (ptr ->str ffi)
  (ptr call ffi)
  (ptr ref raw-mem)
  (ptr ref-word raw-mem)
  (ptr set! raw-mem)
  (ptr set-word! raw-mem)
  (ptr alloc raw-mem)         ; x_sys_malloc, the engine's own allocator -- the one every
                              ;   other part of the engine uses.  Unreachable from X, so X
                              ;   code wanting a scratch buffer reached libc through dlsym.
                              ;   (obj make) is not the answer for a buffer: it allocates
                              ;   through the COLLECTOR, so an image writer's workspace
                              ;   would land inside the image it writes.
  (ptr free! raw-mem)         ; x_sys_free; the caller owns what (ptr alloc) returns.
  (ptr copy! raw-mem)
  (ptr strlen raw-mem)        ; x_lib_strlen, and the four below: the engine's own C
  (ptr strcmp raw-mem)        ;   library, on RAW memory rather than x-lang strings, which
  (ptr strncmp raw-mem)       ;   is why they are filed here and not under `str`.
  (ptr strchr raw-mem)
  (ptr strndup raw-mem)       ; the caller owns the result; (ptr free!) releases it.         ; the engine's own memcpy (x_lib_memcpy).  Without it the
                              ;   only ways to move bytes were libc through dlsym -- a
                              ;   system library a runtime cannot assume is present -- or
                              ;   a per-byte loop in X, which costs hundreds of evals a
                              ;   byte.  A hack or a pathology, for a routine the engine
                              ;   already had.
  (ptr fill! raw-mem)         ; x_lib_memset; the zeroing half, so a buffer that wants
                              ;   zeroing need not come from libc's calloc.
  (str ->ptr ffi)
  (str ->sym alloc)
  (str append alloc)
  (str byte-len hot)          ; derived (atom header read) but string inner-loop hot (utf8 codec, reader lambdas)
  (str byte-ref hot)          ; derived (byte read) but same inner-loop heat
  (str byte-sub raw-mem)
  (str make alloc)            ; a fresh OWNED n-byte string region (space-filled, NUL-terminated so
                              ;   byte-len sees n); the GC frees it -- no free door. Fill via
                              ;   (str ->ptr) + raw-mem stores, File read, or FFI. UNCHECKED (n trusted).
                              ;   Added 2026-07-15 (user-approved: make-str for File/FFI/buffers)
  (sym ->str alloc)
  (sys clock sys)
  (sys read sys)              ; x_sys_read/write over a POINTER and a length.  (io ...) and
  (sys write sys)             ;   the library's Sys class work in strings, which cannot
                              ;   carry binary -- an image is NULs and all -- so this was
                              ;   the door that sent X code to libc through dlsym.
  (sys open sys)
  (sys close sys)
  (sys read-char sys)
  (sys exit sys)
  (tok read tok)
  (tok read-str tok)
  (type ? hot)                ; derived (tag compare) but HOT: runs per `do` form (dotted-body validator)
                              ;   and per predicate call (pair?, str?, ...) -- the hottest sites in the system
  (type make types)
  (type make-instance alloc)
  (type of hot)               ; derived (header word 1 + name walk) but HOT: operatives.x's do-body
                              ;   validator (%boot-cell?) and the predicate layer call it per invocation
  (type set-shape! types)     ; installs the PAIR form of a type's units slot -- (count . mask), a kind
                              ;   per unit (X_TYPE_UNIT_REF..FOREIGN).  C because the shape must be a
                              ;   STRUCTURAL pair: X makes list-pairs only, and the collector
                              ;   discriminates the two forms of the slot by x_obj_type_isspair() on a
                              ;   path it walks per object.  The count half stays (type set-units!)'s,
                              ;   which is X and unchanged.
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
  (rest raw-mem)
  (set! spine)
  (sigint-install sys)
  (sigint-restore sys)
  (syscall ffi)
  (tail-eval spine)
  (unwrap spine)
  (wrap spine)
)))

; The keep-list (x_prims_name_kept): the approved permanent global
; vocabulary -- names that bind BARE even when their catalog namespace is
; de-registered.  Tracked here so growing the C array requires a manifest
; edit (isa-scan.sh extracts the array; the runtime env walk enforces that
; every live PRIMITIVE-typed global is catalog-filed or manifested).
(def %isa-keep (lit (
  (% raw-op)
  (& raw-op)
  (* raw-op)
  (+ raw-op)
  (- raw-op)
  (/ raw-op)
  (< raw-op)
  (<< raw-op)
  (= raw-op)
  (>> raw-op)
  (^ raw-op)
  (call/cc spine)
  (eq? raw-op)
  (same? raw-op)
  (| raw-op)
  (~ raw-op)
)))

; X-level value aliases of BARE prims: an x module captures the raw C prim
; under a new global before shadowing the bare name with an X wrapper
; (module.x: (def %raw-include include), then set!).  Not C binding sites --
; the scan ignores this section -- but the runtime env walk must recognise
; them, since the raw prim value is no longer reachable through its bare
; name.  Each entry: (alias-name bare-name).
(def %isa-aliases (lit (
  (%raw-include include)
)))

(def %isa-values (lit (
  (#t)
  (#f)
  (%sigint-flag)
  (%token-eof)
  (args)
  (x-machine)
  (x-release)
  (x-version)
)))
