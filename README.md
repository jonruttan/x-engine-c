# x-eval-c

The C engine for [x-lang][x-lang]: the evaluator, the primitive surface, and
the tokenizer, built on the [x-expr][x-expr] object, type and heap machinery.

This repository builds one artifact — `x-bin` — and `x-bin` cannot do anything
on its own. It has no printer, no REPL, no standard library; those are written
*in x-lang* and live in the [x-lang][x-lang] repository. The engine reads a
program from stdin and evaluates it, and the wrapper there is what pipes the
library in front of your source:

    { cat lib/x-core.x; cat program.x; } | ./x-bin

That is the whole coupling. The engine embeds no x-lang text and generates
none; it binds exactly one string, `x-release`, so a booted image can say which
release it was built for.

## Build

    git clone --recursive https://github.com/jonruttan/x-eval-c.git
    cd x-eval-c
    make

`--recursive` matters: [x-expr][x-expr] is a submodule and the build needs its
sources. C89 (`-ansi`), no dependencies beyond libc and `-ldl` (the FFI and JIT
layers use `dlopen`/`dlsym`).

    make            # build + strip
    make test       # the C spec suite
    make test-c     # the same, spelled explicitly
    make test-asan  # the C suite under AddressSanitizer
    make help       # every target

Variant builds — `x-bin-debug`, `x-bin-profile`, `x-bin-asan`, `x-bin-cov` —
each compile to their own object suffix, so no two configurations share an
object path and variants rebuild incrementally.

## Layout

| path | what |
|---|---|
| `src/x-eval.c` | the evaluator |
| `src/x-prim/` | the primitive surface (arith, string, io, ffi, heap, …) |
| `src/x-syntax/` | the special forms (binding, closure, control, quote) |
| `src/x-token/` | the s-expression reader |
| `src/x-type/` | the type implementations |
| `src/x-obj/` | object and JIT support |
| `src/x-cli.c` | the command-line entry point |
| `opt/x-prim/signal.c` | optional SIGINT handling (`make X_SIGNAL=` omits it) |
| `include/` | headers, including the generated `x-eval-layout.h` |
| `ext/x-expr/` | the foundation library, as a submodule |
| `tests/c/` | the C spec suite |
| `tools/contract/base-layout.x` | the base-object descriptor `gen-layout` reads |

## The contracts

Four ratchets pin this C surface so it cannot grow or shift silently — and
none of them live here. Each checks the C against a manifest under x-lang's
`tools/contract/`, and those manifests are not merely descriptions of the
engine: `lib/x-core.x` *includes* `base-paths.x` and `obj-layout.x` as the
first things it loads, and `pin.x` reads `isa.x` at runtime. They are boot
data. So they live with the library that boots on them, and the gates live
there too, scanning this submodule's sources across the boundary.

| contract | what it pins | run from |
|---|---|---|
| `check-isa` | every binding site in the C | x-lang |
| `check-obj-layout` | the object header layout | x-lang |
| `check-base-paths` | the base-field accessor chains | x-lang |
| `check-prim-coverage` | every primitive is exercised by a spec | x-lang |

The one descriptor that *is* ours is `tools/contract/base-layout.x`, a pure
build input: `make gen-layout` turns it into `include/x-eval-layout.h`. That
header is committed, so a plain checkout builds without awk. Edit the
descriptor, regenerate, then `make clean && make` — header changes do not
trigger object rebuilds on their own here.

## Adding a primitive

Growing the C surface takes a deliberate manifest edit in the same commit —
that is the point of the ratchet, and the edit lands in **x-lang**, not here.
Register the primitive, then in an x-lang checkout run `make check-isa` and add
the line it reports to `tools/contract/isa.x`. Give it a spec too: in `tests/c/`
here if it is reachable from C, or in x-lang's suite if it is only reachable
through the library — `make check-prim-coverage` there will fail until one of
those exists, or until the primitive carries a written reason for being
deliberately unspecced.

This repo's CI cannot catch a missing manifest edit; x-lang's will, on the
commit that bumps the submodule pointer.

## Documentation

`make doc-c` generates the Doxygen C reference into `docs/ref/c/` (needs
Doxygen). `make install-man` installs its section-3 man pages.

## Licence

MIT No Attribution (MIT-0). See [LICENSE](LICENSE).

[x-lang]: https://github.com/jonruttan/x-lang
[x-expr]: https://github.com/jonruttan/x-expr
