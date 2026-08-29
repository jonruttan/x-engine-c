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

- **`(base def-global)` — an operative can define for its caller, whatever the
  frame depth.** `def` decides global-versus-local by save-stack depth, "top-level
  iff the save-stack is empty", and that stays: include/import rely on it. The
  consequence was that an operative could not define for its caller, so every
  surface language on x worked around it the same way — put the eval in tail
  position and let TCO pop the operative's frame first. That is an accident of
  frame depth rather than a mechanism: one extra wrapper frame and the binding
  lands nowhere, not shadowed but gone, silently, and a definition in body
  position never worked at all.

  `def-global` takes the global path unconditionally — redefinition updates the
  BST entry in place, a fresh name is inserted. The env alist is extended only
  at top level, deliberately: inside a frame that spine unwinds when the frame
  pops, so extending it would leave the local boundary pointing into reclaimed
  structure.

  Found because x-r7rs could not load its own `guard`. R7RS `guard` and x's are
  different forms sharing a name, so providing one means shadowing the other,
  and shadowing interposes a frame. That bundle goes from **579 to 595 of 637**.
  Two more places were relying on the same accident and neither was visible
  until something added a frame: `define-record-type`'s generated defs, and
  `case` binding helpers with sequential defs inside an operative.

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

### Fixed

- **The isolated tokenizer base could not tokenize.** `(Base make-tok)`
  documents itself as the door for one thing — "for custom tokenizer type
  registration on an isolated base" — and died on the first character of any
  input. An empty string was fine, which is what made it look like a base
  problem rather than a buffer one.

  Two defects. `x-eval-layout.h` marks the true/false fields as **cells**, and
  `x_eval_make`'s parented path writes *through* them; `make_token_base`
  assigned to the field instead, replacing each cell with the singleton it
  should have contained. Nothing goes wrong until the collector traces that
  singleton's first slot as a cell — which is why it presented as radon-only:
  radon allocates more at boot, so a collect lands there without anyone asking.
  It also never created a read buffer, which `make_base` gives itself
  immediately after registering its types, for the same reason: the reader reads
  through the base's buffer. With no buffer, zero characters happen to work and
  the first one dereferences what was never made.

  This is the primitive that lets a surface language register its own token
  types without competing with the sexp reader's by score, and two langs were
  dead without it. **x-ash goes from 80 of 82 specs failing to 2** — exactly the
  number its README claims against a patched engine — and **x-python's
  tokenizer, which segfaulted on its first call, is 30 of 30**.

- **A dot delimits only when it stands alone, so `...` is a symbol.** Every
  token beginning with `.` reached x-lang as the pair-dot sentinel, so a
  syntax-rules pattern read as an improper list terminated by a raw C atom —
  `(lit (a ... b))` became `('a . #<ATOM>)`, and taking its rest segfaulted.
  Reachable from ordinary source text, and it made R5RS syntax-rules
  unimplementable: the ellipsis is the one token every macro pattern is written
  in.

  Two causes, and fixing either alone does nothing. The list analyser scored `.`
  immediately, on the grounds that list delimiters are always single-character
  tokens — `(` and `)` are, but `.` is the pair separator only when nothing
  follows it, so it now defers to a state that sees the next character. The
  lookahead has to be a state because `analyse` is called per character and the
  dot's fate is not knowable on the call that sees it. `.` was also in the
  DELIMITER set, which is what actually blocked the symbol analyser from ever
  accumulating `...` no matter what the analyser decided; delimiting is now `(`
  and `)` only.
