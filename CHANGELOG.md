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

  The three contracts whose source half is a scan of the C — `check-isa`,
  `check-obj-layout`, `check-base-paths` — came along and now gate this repo on
  its own. Their runtime halves stay in x-lang under `tests/x/specs/meta/`,
  because only a booted engine can answer them, as does `check-prim-coverage`,
  whose answer is spread across both spec suites.

  `X_RELEASE` now defaults to *this* repo's `git describe` for a standalone
  build; x-lang passes its own tag down when it builds the submodule, so the
  engine keeps reporting the language release it was built for and the pin
  guard's pairing check is unchanged.
