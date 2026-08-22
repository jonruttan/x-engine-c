# x-bin-c

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

    git clone --recursive https://github.com/jonruttan/x-bin-c.git
    cd x-bin-c
    make

`--recursive` matters: [x-expr][x-expr] is a submodule and the build needs its
sources. C89 (`-ansi`), no dependencies beyond libc and `-ldl` (the FFI and JIT
layers use `dlopen`/`dlsym`).

    make            # build + strip
    make test       # contract gates + the C spec suite
    make test-c     # the C spec suite alone
    make gates      # the three contract gates
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
| `tools/contract/` | the committed contract manifests |

## The contracts

Four ratchets pin the C surface so it cannot grow or shift silently. Each has a
**source half** — a scan of the C, checked here — and a **runtime half**, a
spec that probes a live engine and therefore lives in [x-lang][x-lang].

| contract | source half (here) | runtime half (x-lang) |
|---|---|---|
| binding surface | `make check-isa` vs `tools/contract/isa.x` | `tests/x/specs/meta/isa.spec.md` |
| object layout | `make check-obj-layout` vs `tools/contract/obj-layout.x` | `tests/x/specs/meta/obj-layout.spec.md` |
| base-field paths | `make check-base-paths` vs `tools/contract/base-paths.x` | `tests/x/specs/meta/base-paths.spec.md` |
| primitive coverage | — | `make check-prim-coverage` |

Primitive coverage has no source half: it asks whether every primitive the C
registers is *exercised* by a spec, and the answer is spread across both suites,
so only the repository that has both trees can ask it.

`include/x-eval-layout.h` is generated from `tools/contract/base-layout.x` by
`make gen-layout` and committed, so a plain checkout builds without awk. Edit
the descriptor, regenerate, then `make clean && make` — header changes do not
trigger object rebuilds on their own here.

## Adding a primitive

Growing the C surface takes a deliberate manifest edit in the same commit —
that is the point of the ratchet. Register the primitive, run `make check-isa`,
and add the line it reports to `tools/contract/isa.x`. Then give it a spec: in
`tests/c/` if it is reachable from C, or in x-lang's suite if it is only
reachable through the library. `make check-prim-coverage` over there will fail
until one of those exists, or until the primitive carries a written reason for
being deliberately unspecced.

## Documentation

`make doc-c` generates the Doxygen C reference into `docs/ref/c/` (needs
Doxygen). `make install-man` installs its section-3 man pages.

## Licence

MIT No Attribution (MIT-0). See [LICENSE](LICENSE).

[x-lang]: https://github.com/jonruttan/x-lang
[x-expr]: https://github.com/jonruttan/x-expr
