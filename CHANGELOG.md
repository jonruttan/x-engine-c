# Changelog

All notable changes to the C engine are recorded here.

This repository was split out of [x-lang][x-lang] and carries the full history
of the C sources, headers, C spec suite and contract manifests — 503 commits,
reaching back to the first evaluator. Everything before the split is also in
x-lang's own [CHANGELOG][x-changelog], where the engine's changes are recorded
alongside the library changes they landed with.

[x-lang]: https://github.com/jonruttan/x-lang
[x-changelog]: https://github.com/jonruttan/x-lang/blob/main/CHANGELOG.md

## 0.2.0 — 2026-09-03

**A raise carries its facts instead of a sentence** ([#25]). `x_eval_error`
used to flatten the message literal and the offending symbol into one English
string in a static buffer and hand a guard a bare, NIL-TYPED atom. Nothing
above could do better than pattern-match that English: the structure was gone,
and a type-less value has no dispatch stacks to hang a replacement on. The
handler now receives a typed **ERR** — a two-slot `(code . subject)` value
whose type is `x-type/err.c` — so the wording belongs to the language instead
of to C.

The raise path stays **allocation-free**, which is the property the old
in-place formatting existed to protect: the base holds one ERR, built at
type registration, and a raise stores two pointers into it — the message
literal (static storage, so it survives the `longjmp`) and the subject
string. Nothing is copied, nothing is allocated, no truncation, and no
x-lang code runs; rendering happens later, at display time, where
allocation is safe again. `X_ERROR_BUF_SIZE`'s 64KB scratch buffer and the
copy loop that filled it are both gone.

The subject is the interned name **as a string**, not the object: raise
sites build theirs on the C stack (`x-type/symbol.c` fills a local array and
passes its address), and the `longjmp` destroys that frame, so retaining the
pointer would hand out a dangling one. The string it points at is what the
engine already trusted and all it ever kept.

ERR's write/display stacks **boot empty**, as CHARACTER's do — x-lang's
`x/type/err-io.x` pushes the default wording, byte-for-byte what C emitted
before, and a lang pushes its own over that. The uncaught path is unchanged
and still words itself in C: it runs before any library exists, and nothing
on a fatal path calls into x-lang.

The shape is a declared guarantee, `err/typed-raise`, claimed in `claims.x`
and so copied into `x-engine.xon` by the generator: a raise delivers a value
of a registered type carrying `(code . subject)`, and the base's `err` row
holds a value of that same type. Identity is deliberately not claimed — this
engine reuses one instance so a raise allocates nothing, but an engine that
allocates per raise satisfies it equally. x-lang's
`tools/contract/compliance/guarantee-err-typed-raise.spec.md` is the
executable form.

`x_eval_make` does **not** build the ERR — it runs before the type registry
exists, and x-eval must not depend on x-type (`tests/c/src/2.x-base.spec.c`
pins that layering). `x_type_err_register` builds it at the first moment it
can. A base still in that window raises through an ERR-SHAPED static
fallback, so every C consumer reads `x_err_code`/`x_err_subject` without
asking which window it came from.

### Added

- **`jit_buffer_last_char`** ([#24]) — the last-read character as a raw long,
  exposed to compiled code.

  The JIT lane already publishes the engine's buffer and score macros as real
  callable functions (`jit_score_set`, `jit_buffer_unread`, `jit_buffer_len`).
  This is the peer of `jit_buffer_len` — a different buffer macro
  (`x_bufferlastchar` vs `x_bufferlen`), the same three-line shape, not a
  duplicate to factor — and it lets the tokenizer's per-character delimiter
  handler JIT-compile through the same lane the tower's numeric analysers
  already use, instead of running interpreted on every character of every
  symbol.

  Three lines plus one export; the stripped binary is unchanged in size. The
  library side compiles the delimiter through this symbol, and an engine
  without it is unaffected: the compile is guarded and falls back to the
  interpreted handler.

### Fixed

- **Op arbitration read instance payload words as integers on an undeclared
  pair** ([#22]).

  `x_type_op_try` returned 0 when *both* operand types registered the op and
  *neither* side's cvt from-alist declared the other, and every caller's raw
  fallback then read instance payload words as integers. The cross-engine
  differential fuzzer caught the result as address garbage that moves with
  ASLR (x-lang#584).

  Both sides declared interest in the operator, so the raw integer path is
  certainly wrong for them. The arbitration raises instead, through
  `x_eval_error`'s own append mechanism — one call, no local composition:

      no declared promotion; declare the cvt relation for 'RATIONAL'

  One raise covers every operator door: `+ - * / %` via the arith binop, `-`
  via diff, `=` `<` via pred. Single-handler, same-type and declared-pair
  dispatch are untouched, as is the no-handler fallthrough — int/int stays
  pure C.

- **The refusal over-reached on `=`** ([#23]). An undeclared typed pair *is*
  answerable under equality — unrelated values are not equal — and raising
  broke exactly the bundles that relied on that answer. x-python's
  tuple-versus-list came up first: `(1, 2) == [1, 2]` must be `False`, and
  the old raw fallback got it right only by address accident.

  Arbitration now answers `#f` for `=` and keeps the teaching raise for every
  op that has no answer without a declared relation.

[#22]: https://github.com/jonruttan/x-engine-c/pull/22
[#23]: https://github.com/jonruttan/x-engine-c/pull/23
[#24]: https://github.com/jonruttan/x-engine-c/pull/24
[#25]: https://github.com/jonruttan/x-engine-c/pull/25

## 0.1.6 — 2026-08-30

The engine declares the ISA it actually ships.

### Fixed

- **`x-engine.xon` declared the previous manifest's digest** ([#20]). 0.1.5
  added `(base def-global)` to `tools/contract/isa.x` and did not regenerate
  the declaration, so that release's

      (isa "sha256:b1a4ab9c…")

  is the digest of the ISA *before* the primitive was added. The engine
  claimed one surface and carried another. One line of one file; nothing in
  the built engine differs, and `make test` was 12/0 on either side of it.

  x-lang caught it, on the first pin bump that pulled 0.1.5 in — its
  `check-engine-contract` regenerates the declaration and compares:

      STALE: engine/x-engine.xon is not what the generator produces
      FAIL: the vocabulary and the engine ISA disagree.

  **Nothing here could have caught it**, and that is the part worth
  recording. `x-engine.xon` is generated by x-lang's
  `tools/contract/gen-engine-xon.sh`, deliberately: generating the
  declaration needs the *vocabulary*, and an engine that generated its own
  would be choosing the terms it is judged by. The consequence is that this
  repository cannot verify its own declaration — CI here is the bare-engine
  spec suite and knows nothing about the digest — so a release can go out
  inconsistent and only the downstream discovers it. Which is what happened,
  one release later.

  Regenerating is one command, run from an x-lang checkout:

      sh tools/contract/gen-engine-xon.sh <engine-dir>

  Worth running from CI here against a cloned x-lang, the way the bundles
  clone the lang kit — a check that lives elsewhere is still a check this
  repository can run. Not done in this release.

  0.1.5 stays published and stays inconsistent: anything pinning it fails
  `check-engine-contract`, so this is the release to pin.

[#20]: https://github.com/jonruttan/x-engine-c/pull/20

## 0.1.5 — 2026-08-30

An operative can define for its caller.

### Added

- **`(base def-global name value)`** ([#19]) — bind in the base's global
  environment whatever the frame depth.

  `x_prim_define` decides global-versus-local by save-stack depth ("top-level
  iff the save-stack is empty"), which is settled semantics `include`/`import`
  and define-sugar rely on and which nothing here changes. The consequence is
  that an **operative cannot define for its caller**: Scheme's `define` and
  Kernel's `$define!` are operatives, so `def` inside one sees a non-empty
  save-stack, binds locally, and the binding is discarded when the frame pops.
  Not shadowed — gone.

  Every surface language on x worked around it the same two ways, and both are
  unsound. Putting the `eval` in *tail* position lets TCO pop the operative's
  frame first, which works and is an accident of frame depth — one extra
  wrapper frame anywhere up the chain and every definition silently vanishes.
  `eval!` evaluates with no env save/restore so the binding persists in the
  current env, which is correct at the prompt and breaks the moment an
  operative frame is interposed.

  Measured: x-r7rs goes from 43 failures to **27** with no change to that
  bundle — all of `error`, `error objects` and `guard`. `guard` is the live
  case, because R7RS `guard` and x's `guard` are different forms sharing a
  name, so providing one means shadowing the other, and shadowing interposes
  exactly the frame that breaks `eval!`.

  It extends the env alist **always** and advances the local boundary only at
  top level. Skipping the extension inside a frame left the binding in the BST
  but not on the spine, and anything walking the alist rather than resolving
  through the BST could not see it — `syntax-rules`' hygiene lookup is one such
  walker, and a macro expanding to a lambda bound its parameter to a stale
  entry. A half-present binding is worse than either alternative.

  **Two things it is not.** It is a special case standing in for a general
  capability: x already hands an operative its caller's environment as `e` and
  lets you `eval` in it, and what remains impossible is *binding into* an
  environment you were given. First-class bindable environments would make this
  redundant. And it duplicates `def`'s global-bind rule — BST update-in-place
  on redefinition, insert on a fresh name, boundary advance — across two sites
  with nothing keeping them in sync. A shared helper is worth doing before they
  drift.

[#19]: https://github.com/jonruttan/x-engine-c/pull/19

## 0.1.4 — 2026-08-30

The reader stops claiming a character it never meant to own, and stops handing
an internal marker back as a value.

### Fixed

- **A dot separates a pair only when it is one** ([#18]). `(lit (a ... b))`
  read as an improper list whose tail was the reader's own separator satom,
  and `(first (rest …))` on it segfaulted. Ordinary source text, not
  malformed input.

  The dot was a token *kind*: it sat in `X_SEXP_LIST_CHARS_STR` beside the
  brackets, so the analyser scored it on sight. That is correct for `(` and
  `)`, which really are always single-character tokens, and false for `.`,
  which is a separator only when nothing follows it. A token merely
  *beginning* with a dot was taken whole as the separator, and
  `x_sexp_list_read` returned `x_sexp_list_delimit_prim` for it — consumed
  inside a list, and returned to the caller at the head of one, where a raw C
  satom is not a value any x program can survive touching.

  It is an ordinary character now. Nothing claims it, the symbol analyser
  accumulates it like any other, and the list *reader* recognises the
  one-character symbol `.` as the separator, at the point where the structural
  decision is already being made. The separator has no sentinel of its own, so
  there is nothing left to leak.

  This takes no view on `...`, which is Scheme's ellipsis and none of the
  engine's business — it is simply a symbol the reader does not recognise and
  passes through, exactly like `.foo`. Nor does it rule any dot sequence an
  error, because the engine cannot know one: a base may register a type that
  claims `.` and mean something by it.

  One reading changes with it: `(a.b)` is the symbol `a.b` rather than an
  improper list, the dot no longer terminating an adjacent token. That reading
  was an accident of the delimiter set rather than deliberate syntax.

- **A changed header rebuilds the objects that include it** ([#14]). The
  compile rule emitted no dependency files, so `make` compared each object
  against its `.c` and never learned which headers that `.c` included. Editing
  a constant, a struct layout or a macro left every object reading it stale,
  and the binary silently mixed old and new definitions — it did not fail, it
  reported something that was not in the source in front of you.

  Found while preparing this release, by an hour spent chasing a test failure
  that did not exist: bisected to an innocent PR, then defended through four
  reverts that each changed nothing, and unmasked only by reverting *every*
  file changed since 0.1.2 and still reproducing it. Source byte-identical to
  a passing tree, still failing, is not a code problem.

  `-MMD -MP` on the compile rule, `.d` files following `OBJ_EXT` so the
  variant builds keep separate sets, `-include` so a clean tree is not an
  error, and `clean` removing them. CI never saw the defect, because CI always
  builds from clean: this one is paid by local work alone.

  Recorded under 0.1.3 before it landed: the entry was written while that
  release was being prepared and the commit merged after the tag, so v0.1.3's
  published notes describe a fix v0.1.3 does not contain. It ships here.

[#14]: https://github.com/jonruttan/x-engine-c/pull/14
[#18]: https://github.com/jonruttan/x-engine-c/pull/18

## 0.1.3 — 2026-08-29

Two things that could not do the one job they existed for: an error reporter
that dropped the detail naming what went wrong, and an isolated tokenizer base
that could not tokenize. Both had been that way since they were written, and
both were found by someone trying to use them.


### Fixed

- **The error buffer no longer truncates away the part worth reading**
  ([#12]). `x_eval_error` copies the message into the buffer and *then*
  appends `" '<symbol>'"` — so the name saying which symbol was unbound, which
  file could not be opened, which type was wrong, is appended last and is the
  first thing dropped when the message fills the buffer. The loop simply stops
  at the cap, silently. An error long enough to hit 256 bytes reported
  everything except the one detail worth having.

  `X_ERROR_BUF_SIZE` is 65536 now. The buffer is engine-level — `err_buf` is a
  single file-scope static, one per process rather than one per base — so the
  size is paid once and there is nothing to economise on. A diagnostic that
  cannot fit in 64K is not being truncated, it is being generated wrong.

- **`(Base make-tok)` can tokenize** ([#11]). It is documented for custom
  tokenizer type registration on an isolated base, and it segfaulted on the
  first character of any input. Two defects, and they were the same defect:
  `make-base` and `make-token-base` were one constructor written twice, and
  the copy drifted.

  `true`/`false`/`sigint` are cells, and the parented path assigns
  `x_firstobj(field)` for that reason. The parentless copy assigned the
  *field*, replacing each cell with the singleton it should have contained —
  so every later `x_firstobj()` on it read the singleton's first slot as a
  cell, and the tokenizer segfaulted the moment it consulted a truth value.
  `sigint` was not inherited at all. And a tokenizer base needs a read buffer,
  because the reader reads *through* it; `make-base` set one up and
  `make-token-base` never did, which is why empty input happened to work and
  the first character did not.

  The two buffer sizes in this release arrive at 64K for unrelated reasons —
  that one is per-base and sized for input, the error buffer is per-process
  and sized for a message. They keep separate names so a later change to one
  cannot silently move the other.


### Changed

[#11]: https://github.com/jonruttan/x-engine-c/pull/11
[#12]: https://github.com/jonruttan/x-engine-c/pull/12

## 0.1.2 — 2026-08-25

Another uncatchable crash made catchable, found the same way the last one
was: by a test suite meeting a platform for the first time.


### Fixed

- **The FFI raises on a nil function pointer or operand instead of calling
  it** ([x-lang#171][i171] class). A dlsym miss answers nil, and every call
  convention handed that nil straight to the machine — `x_ptrval(nil)` as a
  call target, `x_intval(nil)` as a memcpy source. x-lang's v0.5.0 release
  run died on exactly this: the first conformance run Linux ever saw
  resolved `sqrt` against an engine that links no libm, got nil, and called
  it. One door per harm: `x_ffi_fptr` guards the function-pointer
  conventions and `ptr-call`; the nil-operand check lives in
  `x_ffi_to_double`, which every double convention shares. The arithmetic
  and comparison conventions never touch the fptr and are untouched. Three
  bare specs pin the behaviour.

[i171]: https://github.com/jonruttan/x-lang/issues/171

## 0.1.1 — 2026-08-23

A crash fix. `(= 1.5 1.5)` killed the process — uncatchably, from ordinary
source text.


### Fixed

- **A prim raises on a dotted argument list instead of walking off it**
  ([x-lang#487][i487]). A prim reads its arguments by walking the spine, and
  the walk tested only for the PROPER ending: a proper list bottoms out at
  nil, an improper one at an ATOM, which was then read as a pair — the tail
  integer's value word dereferenced as a pointer. No `guard` could catch it,
  because a prim call never enters the applicative walk that #69 guarded.

  Ordinary text reaches it because the reader is honest: with no float module
  loaded `1.5` reads as `(1 . 5)`, so `(= 1.5 1.5)` is exactly that call.
  `=`, `eq?` and `same?` crashed while `+` and `<` raised cleanly — the split
  being that the library shadows the latter with tower generics, so only the
  unshadowed keep-list entries reached C directly.

  The fix hoists #69's own structural cell test into `x_eval_spine_guard` and
  points every C consumer of a spine at that one implementation: the prim
  argument helpers, the three body walkers, `match`'s clause walk, and the
  variadic walks in `atomic`/`syscall`/`ffi`. Ops still receive their spines
  raw and bind dotted tails legitimately. Three bare specs pin the behaviour.

  Unchanged: a satisfied arity still ignores a junk tail (`(= 1 2 . 5)` is
  `#f`), and a non-pair or non-callable ARGUMENT is still the unchecked
  contract it always was — `(first 1)` and `(match 1 2)` are a separate
  question, deliberately not folded into this fix.

[i487]: https://github.com/jonruttan/x-lang/issues/487

## 0.1.0 — 2026-08-23

The first release of this engine under its own version number. Everything
before it shipped inside an x-lang release, which is what these three entries
are about: an engine that can be packaged, named, and told apart from the
language it runs.

The version line starts here. The ten tags this repository inherited when the
C was carved out of x-lang (`v0.3.1-rc*`, `v0.4.0`) were x-lang releases; no
engine was ever cut or tested as one of them, and none had been pushed, so
they were deleted rather than published.


### Added

- **`make dist` — this engine as a consumable directory.** A tarball holding
  the binary, its build params, its declaration, the contract manifests and
  both include trees: everything a consumer resolves, at the paths a checkout
  uses. x-lang points its `engine` link at an unpacked release and treats it
  exactly as it treats a checkout. Sources, tests and build tooling stay out —
  a half-source tarball that looks buildable and is not would be worse than an
  honest binary one.

- **A release pipeline, and a version line of its own.** A version tag builds,
  gates and publishes one tarball per platform (`macos-14`, `ubuntu-24.04`),
  each with a sha256 sidecar, and proves the *tarball* — unpacked elsewhere —
  rather than the staging directory it came from.

  **This line starts at v0.1.0.** The ten tags the carve inherited
  (`v0.3.1-rc*`, `v0.4.0`) were x-lang releases; no engine was ever cut or
  tested as one, so they have been removed rather than published. v0.x also
  says the honest thing about the distribution format: it is new, and it may
  change before it is worth a 1.0.

  The engine still reports whatever `X_RELEASE` it is built with, so an engine
  built inside x-lang reports x-lang's tag and a released one reports its own.
  Those are two different questions, and x-lang's `x -V` now labels both
  answers; teaching the pin lock to record the engine's is the next step there.

- **The engine asserts its own name** (`(name "x-engine-c")` in `claims.x`).
  x-lang's generator used to take it from the directory it found the engine
  in, which is where an engine happens to sit rather than who it is — a
  checkout and an unpacked release answered to different names.

### Changed

- **The C engine is its own repository.** `src/`, `include/`, `opt/`,
  `tests/c/`, the `ext/x-expr` submodule, the Doxygen C reference and the
  contract manifests for the C surface moved here from x-lang, keeping their
  repo-relative paths and their history. x-lang consumes this repo as a
  submodule and builds `x-bin` from it.

  The contract manifests came too, and with them the three ratchets that
  check the C against them, so this repo gates itself with no x-lang checkout
  in the loop. The manifests are the engine's published self-description:
  x-lang's boot loads `base-paths.x` and `obj-layout.x` to learn this engine's
  field offsets, and its `pin.x` reads `isa.x`. Shipping them with the engine
  is what stops a library running on an engine whose layout disagrees with the
  copy the library holds.

  `check-prim-coverage` stays in x-lang: it asks whether every primitive is
  *exercised*, and most are reachable only through the library, so the answer
  needs both spec suites at once.

  `X_RELEASE` now defaults to *this* repo's `git describe` for a standalone
  build; x-lang passes its own tag down when it builds the submodule, so the
  engine keeps reporting the language release it was built for and the pin
  guard's pairing check is unchanged.
