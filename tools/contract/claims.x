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
; SCOPE: THE IMPLEMENTATION, NOT ONE BUILD OF IT.  x-engine.xon is generated from
; SOURCE facts -- the ISA manifest, the layout descriptors, their digests -- so its
; subject is what this engine CAN do, not what one compilation of it happened to
; switch on.  Coverage and profiling are claimed here because this repo builds them
; (the cov and profile variant targets); the fact that a given binary was compiled
; without -DX_COV is a property of that BINARY, and per-build facts are stamped
; beside it at install time, which is where the param rows go for exactly the same
; reason.  Scoping this file to the default build while deriving everything else
; from source would put two different subjects in one manifest.
;
; Vocabulary: every atom below must exist in x-lang's tools/contract/features.x.
; A typo here is caught there, not tolerated here.

(def %claims (lit (
  ; --- who this engine IS ---
  ; The generator used to take the name from the DIRECTORY it found the engine
  ; in.  That is an accident of delivery, not an identity: a checkout sits in
  ; `x-engine-c`, an unpacked release sits in `x-engine-c-<release>-<os>-<arch>`,
  ; and the same engine would answer to two different names depending on how it
  ; arrived -- refusing a project whose pin.xon says (engine "x-engine-c") for no
  ; reason but the shape of a path.  So the name is asserted here, beside the
  ; binary's, for the same reason the binary's is: no one else is entitled to
  ; decide it.
  (name "x-engine-c")
  ; --- non-ISA capabilities this implementation has ---
  ; The `include` primitive, -DX_INCLUDE.  Repo-mode boot cannot start without
  ; it: the wrapper cats an entry whose first act is to include the boot closure.
  (provides io/include)
  ; The layout descriptors ship in this repo (tools/contract/obj-layout.x,
  ; base-paths.x, base-layout.x) and x-lang's boot includes them before data.x.
  ; This is decision L1's runtime shape: the engine supplies its own layout.
  (provides reflect/layout-data)
  ; int<->ptr round-trips faithfully enough for x-lang to size a word at boot.
  (provides reflect/word-probe)
  ; What this engine actually owes the wrapper: it reads its program from stdin,
  ; binds every argv element as the `args` list, and writes diagnostics to STDERR
  ; prefixed `*** ERROR: `.  It parses no flags -- --batch and --quiet are read by
  ; lib/x/repl/banner.x, and the fd-3 stdin reclaim is lib/x/repl/loop.x calling
  ; (Sys dup2 3 0).  Those are wrapper/library conventions, not engine ones, and
  ; claiming them here would have been claiming credit for someone else's work.
  (provides invoke/pipe-stdin)
  (provides invoke/argv)
  (provides err/stderr-prefix)
  ; Coverage marking and eval counters.  Claimed at the IMPLEMENTATION level: the
  ; repo builds both variants.  A binary compiled without them is a build fact,
  ; recorded beside that binary, not a limit of this engine.
  (provides instr/cov)
  (provides instr/profile)
  ; The in-process assembler lane: this engine EXPORTS its jit_* runtime helpers
  ; from the running binary, so `dlopen` of self resolves jit_buffer_len and its
  ; siblings, and it permits executing the pages the assembler writes.  Consumers
  ; are x-lang's x/tool/asm-compile.x and the ten spec files carrying
  ; `# @requires native/jit`.
  ;
  ; Claimed at the IMPLEMENTATION level, for the reason instr/cov above is: the
  ; repo builds it, and what a particular binary ended up with is a build fact
  ; recorded beside that binary.  Two such facts are worth naming because both
  ; have bitten.  On macOS the execute half needs the code signature the Makefile
  ; applies from entitlements.plist (allow-jit, allow-unsigned-executable-memory);
  ; a build that skips the codesign step has the symbols and cannot run the pages.
  ; And a bare `strip` drops the exported symbol table that `strip -x` keeps, so
  ; an installed engine had no jit_* symbols at all while the repo build was fine
  ; (x-lang#201) -- which asm-compile.x refuses on, by probing every helper before
  ; it emits anything rather than compiling a `blr` to address 0.
  (provides native/jit)

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
  ; A str value IS a C string: bytes past the NUL are unobservable.
  (guarantee str/nul-terminated)
  ; ext/x-expr/include/x.h asserts sizeof(x_int_t) == sizeof(void *) at COMPILE
  ; time, so the fixnum and the pointer cannot diverge in width.  This is the
  ; strongest row here -- it is the only one the C compiler already enforces.
  (guarantee int/ptr-same-width)
)))
