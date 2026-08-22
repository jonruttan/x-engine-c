# x-engine-c API Reference

The C engine: the evaluator, the primitive surface, and the object, type and
heap machinery underneath them. Generated from the sources by Doxygen; the
version above the title is the release this reference was built from.

## The other half

This documents the engine written in C, which is all this repository holds.
The library written *in x-lang* lives in the
[x-lang](https://github.com/jonruttan/x-lang) repository and has its own
reference, generated from the `(doc ...)` forms in its sources:

- [x-lang API Reference](https://jonruttan.github.io/x-lang/docs/ref/x/index.html)
  — every module, class, method and member
- [Documentation index](https://jonruttan.github.io/x-lang/docs/index.html)
  — the hand-written guides, the specification, and the syntax rulings

## Where to start here

- **Files** — every translation unit and header, each with the symbols it
  defines. `src/x-prim/` is the primitive surface; `src/x-eval.c` is the
  evaluator; `ext/x-expr/` is the expression layer the engine embeds.
- **Data Structures** — the object, base and type structs the whole system
  is expressed in.
- **Topics** — grouped surfaces, where a group has been declared.

The primitive surface is also specified as data, independently of this
reference, in `tools/contract/isa.x`: a committed descriptor that a gate
checks the C against, so the two cannot drift apart silently.
