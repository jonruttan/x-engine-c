; tools/contract/claims.x -- what this engine ASSERTS about itself.
;
; x-engine.xon is generated (by x-lang's tools/contract/gen-engine-xon.sh), and
; most of it is DERIVED: the capability groups fall out of tools/contract/isa.x,
; the profiles fall out of those, the digests fall out of the files.  Two kinds of
; row cannot be derived, and they live here.
;
; 1. GUARANTEES -- behaviours, which are mostly things this engine does NOT do.
;    No manifest can show them: there is no primitive called "never collects
;    during allocation".  They are stated here, copied into x-engine.xon verbatim,
;    and are worth exactly as much as this file's author until the compliance
;    test falsifies them.  x-lang's library depends on every one; six sites hold
;    raw pointers across allocating expressions on the collection promise alone.
;
; 2. NON-ISA CAPABILITIES -- ones behind a build flag or in the wrapper protocol,
;    which the ISA manifest does not describe because it describes the instruction
;    surface, not the switches or the calling convention.
;
; SCOPE: THE DEFAULT BUILD.  `make` here produces x-bin with -DX_HEAP -DX_TYPE
; -DX_SYS_CLOCK -DX_SYSCALL -DX_INCLUDE -DX_SIGNAL.  Coverage and profiling are
; VARIANT builds (x-bin-cov, x-bin-profile), so instr/cov and instr/profile are
; deliberately NOT claimed: the default engine genuinely does not have them, and
; claiming them would be the over-declaration that compliance exists to catch.
;
; Vocabulary: every atom below must exist in x-lang's tools/contract/features.x.
; A typo here is caught there, not tolerated here.

(def %claims (lit (
  ; --- non-ISA capabilities the default build has ---
  ; The `include` primitive, -DX_INCLUDE.  Repo-mode boot cannot start without
  ; it: the wrapper cats an entry whose first act is to include the boot closure.
  (provides io/include)
  ; The layout descriptors ship in this repo (tools/contract/obj-layout.x,
  ; base-paths.x, base-layout.x) and x-lang's boot includes them before data.x.
  ; This is decision L1's runtime shape: the engine supplies its own layout.
  (provides reflect/layout-data)
  ; int<->ptr round-trips faithfully enough for x-lang to size a word at boot.
  (provides reflect/word-probe)
  ; The wrapper protocol (x.sh): library concatenated on stdin, --batch
  ; suppressing the entry's launcher, argv reaching the `args` value, the REPL
  ; reclaiming terminal stdin from fd 3, and errors printed to stdout as
  ; `Error: <value>`.
  (provides invoke/pipe-stdin)
  (provides invoke/batch-flag)
  (provides invoke/argv)
  (provides io/fd3-stdin)
  (provides err/stdout-prefix)

  ; --- guarantees ---
  ; Collection happens only when asked.  Allocation never triggers it, so a raw
  ; pointer held as an integer stays valid across an allocating expression.
  (guarantee gc/explicit-only)
  ; Mark-sweep frees in place; a live object never moves, so an address taken
  ; once stays an address.
  (guarantee gc/non-moving)
  ; Proper tail calls, unbounded.  Not a speed claim: x-lang's library recurses
  ; in tail position throughout and binds tail `def`s globally because of it.
  (guarantee eval/tco)
  ; Tokenizer callbacks run inside the reader's inner loop without allocating.
  (guarantee tok/callback-no-alloc)
  ; A str value IS a C string: bytes past the NUL are unobservable.
  (guarantee str/nul-terminated)
  ; ext/x-expr/include/x.h asserts sizeof(x_int_t) == sizeof(void *) at COMPILE
  ; time, so the fixnum and the pointer cannot diverge in width.  This is the
  ; strongest row here -- it is the only one the C compiler already enforces.
  (guarantee int/ptr-same-width)
)))
