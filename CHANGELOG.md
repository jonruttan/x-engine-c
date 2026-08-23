# Changelog

All notable changes to the C engine are recorded here.

This repository was split out of [x-lang][x-lang] and carries the full history
of the C sources, headers, C spec suite and contract manifests — 503 commits,
reaching back to the first evaluator. Everything before the split is also in
x-lang's own [CHANGELOG][x-changelog], where the engine's changes are recorded
alongside the library changes they landed with.

[x-lang]: https://github.com/jonruttan/x-lang
[x-changelog]: https://github.com/jonruttan/x-lang/blob/main/CHANGELOG.md

## Unreleased

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
