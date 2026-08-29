# Changelog

All notable changes to the C engine are recorded here.

This repository was split out of [x-lang][x-lang] and carries the full history
of the C sources, headers, C spec suite and contract manifests — 503 commits,
reaching back to the first evaluator. Everything before the split is also in
x-lang's own [CHANGELOG][x-changelog], where the engine's changes are recorded
alongside the library changes they landed with.

[x-lang]: https://github.com/jonruttan/x-lang
[x-changelog]: https://github.com/jonruttan/x-lang/blob/main/CHANGELOG.md

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
