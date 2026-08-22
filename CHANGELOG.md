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
