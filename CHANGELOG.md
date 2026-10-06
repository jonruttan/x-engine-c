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

## 0.2.21 — 2026-10-06

**A NUL is an ordinary byte in the tokenizer** ([#88]). The buffer's write
cursor is the only end of input, so a 0x00 reaches the analysers like any other
byte. The sexp list, comment and integer analysers refuse it, and whitespace
takes it, so a NUL in x source still ends a symbol. `buf tok` copies every byte
of the token, and `tok read-str` takes an optional `start len` span, NULs
included.

[#88]: https://github.com/jonruttan/x-engine-c/pull/88

## 0.2.20 — 2026-10-03

**`ptr call` passes up to eight arguments** ([#86]). The eighth fills arm64's
last integer argument register, so a C function such as zlib's
`deflateInit2_`, which takes eight, can be called. More than eight are still
ignored.

[#86]: https://github.com/jonruttan/x-engine-c/pull/86

## 0.2.19 — 2026-10-01

**Any signal can be caught and its arrival read back** ([#84]). `(signal
catch N)` installs a handler for signal N that records the arrival in a
static flag and does nothing else; `(signal take N)` answers 1 if N arrived
since the last take, clearing the record, and 0 otherwise. What an arrival
means is the caller's to decide, so the handler stays C and the policy
stays in x. As SIGINT's handler does, it does not restart an interrupted
read or poll, so a program waiting for input wakes up to look. A number
that names no signal, or a signal that cannot be caught, answers -1 from
catch. Both are in the default build, beside SIGINT's, under `X_SIGNAL`.

[#84]: https://github.com/jonruttan/x-engine-c/pull/84

**A call through a slot costs less** ([#83]). The evaluator, symbol lookup,
argument evaluation and the call path call through the slot without the
test for a base and a vector, which they always have; the test stays where
a call can come before a base has its vector. A call form whose head is a
primitive goes to the callable-call slot directly, not through the
PRIMITIVE type's call handler, which only handed it back; a type whose call
handler was replaced takes the full path. Measured in retired instructions
on x-lang's helium boot and on a loop of 300,000 calls, darwin/arm64, the
cost of the slot vector over 0.2.17 falls from 2.7% and 3.3% to 1.6% and
1.6%.

[#83]: https://github.com/jonruttan/x-engine-c/pull/83

## 0.2.18 — 2026-09-30

**The base holds a slot vector, and the engine's routines are called
through it** ([#76], [#81], [#82]). The engine's base object has two data
units: the first holds the tree, as x-expr's one-unit base does; the second
holds the slot vector, a vector of function pointers, one per slot. A call
locates the vector in one load from the base. `x_eval_make` makes the
object: x-expr builds the tree, and the engine's object takes it, with the
chain of what was allocated building it. A slot holds the routine itself: each of the nineteen
routines has the engine's one signature,
`(x_obj_t *p_base, x_obj_t *p_args)`, and `p_args` is an argument run, a
run of datum words, one per argument, with no header and no length. An
integer or a string travels as a word. A caller writes the run in stack
storage and calls through the slot, so a routine is replaced while the
engine runs by storing another function in its slot, and each base has a
vector of its own, filled from the engine's table when it is made.

- The positions are a contract, `tools/contract/base-slots.x`, which
  `make check-base-slots` diffs against `include/x-eval-slots.h`. A row
  gives the slot's arguments and the type of the word each travels in. It
  is one of the gates.
- The routines are the evaluator's `x_eval`, `x_eval_list`, `x_eval_body`,
  `x_eval_body_tco`, `x_eval_tco_trampoline` and `x_eval_op_body`; the
  calling routines `x_callable_call`, `x_callable_apply` and
  `x_obj_prim_call`; the environment's `x_env_lookup`, `x_env_bind`,
  `x_env_extend` and `x_alist_bst_lookup`; the reader's `x_token_read`,
  `x_token_analyse` and `x_token_delimit`; and the collector's
  `x_heap_mark_phase` and `x_heap_sweep_phase`, with `x_eval_alloc`, which
  allocates an object. The calls are `x_eval_call` and `x_eval_call_or`,
  the slot `x_eval_slot`, and `x_argrun` writes a run as an expression.
- The collector is the engine's. The mark and sweep phases and the
  allocation of an object are routines in the vector, written over
  x-expr's chain, traversal, sweep and hooks, which the engine calls by
  name and which are never in a slot. x-expr reaches the engine through the
  hooks `x_base_make` takes, as before.
- `x_eval` takes the expression. `x_eval_arg` is removed: it wrapped an
  expression for `x_eval`, which now wraps it itself.
- A primitive and a type's handler take a pair list, as before.
  `x_callable_prim_call` is the primitive in front of `x_callable_call`, and
  `jit_eval_arg` keeps its `(base, expr)` signature for compiled code.
- A call with no base reaches the routine by name (`x_eval_call_or`): the
  types are registered on a nil base before one exists.
- The vector layout is `include/x-vector.h` and `src/x-vector.c`:
  `x_vector_make`, `x_mksvector`, the static lengths, and `x_slots_make`.
  The engine has a VECTOR type, `x-type/vector`, registered with the
  others, and the registration gives the base's slot vector the type.
  `x_mkvector` makes one from the objects it is given.
- `tools/contract/base-paths.x` is regenerated: `slots` is no longer a
  path.
- `(heap tree-mark! obj flags)` passes its flags as the integer given
  ([#78]).
- The cost, measured on x-lang's helium boot from source and on a loop of
  300,000 calls, darwin/arm64: about 7% more user time than 0.2.17.

[#76]: https://github.com/jonruttan/x-engine-c/pull/76
[#78]: https://github.com/jonruttan/x-engine-c/pull/78
[#81]: https://github.com/jonruttan/x-engine-c/pull/81
[#82]: https://github.com/jonruttan/x-engine-c/pull/82

## 0.2.17 — 2026-09-28

**`%seq` walks its forms as an operative body is walked** ([#72]). `%seq` had
its own loop over its forms and checked the whole list before it evaluated
the first. It now calls `x_eval_op_body`, the walk an operative call uses for
its body, with the current environment as the one to make current when the
body is done. Two things change:

- A dotted list raises where the walk reaches the dot, as an operative body
  does, so the forms before the dot have been evaluated. The message is the
  same, `call: improper argument list (dotted tail)`.
- The environment `%seq` started in is current again when its last form is
  done, as after an operative or a procedure call. `%seq` handed the
  trampoline no environment, as `match` does, and the environment was left as
  its last form left it.

The forms run in order, the last in tail position, no forms answer nil, a
`def` binds in the environment `%seq` runs in, and a call allocates nothing,
as before. Covered by two bare smoke cases, both failed by v0.2.16: a dotted
`%seq` raising after the form before the dot has run, and a name read after a
`%seq` whose last form ran in another environment.

x-lang's image loader sequenced its install with `%seq` and carried on in the
image's environment; it uses `atomic` there from [x-lang#850].

[#72]: https://github.com/jonruttan/x-engine-c/pull/72
[x-lang#850]: https://github.com/jonruttan/x-lang/pull/850

## 0.2.16 — 2026-09-28

**`%seq` sequences any number of forms** ([#70]). It took exactly two:
`(%seq a b c)` evaluated `a`, tail-evaluated `b` and never read `c`, and
`(%seq a)` evaluated `a` and answered nil. It now evaluates each form but
the last in the current environment, so a `def` among them binds there, and
hands the last to the trampoline, so it stays in tail position; with no
forms it answers nil. The whole list is checked before any form is
evaluated: a dotted tail raises at the spine guard ([x-lang#487]),
`call: improper argument list (dotted tail)`, with nothing evaluated. Two
forms behave as they did. Covered by the prim-core C spec, where three forms
leave the third in the tail-call slot and none leave it nil, and by five
bare smoke cases: the order and the answer, no forms, a `def` binding where
`%seq` runs, a dotted list raising with nothing evaluated, and the last form
in tail position over 50,000 calls.

**The contract's names follow x-lang's glossary** ([#69], [x-lang#808]).
Every rename is breaking and keeps no alias:

- A set of fields is `fields`. The base-path routes `io-group`,
  `meta-group`, `heap-group`, `alloc-group`, `type-iter-group`,
  `type-ops-group` and `type-image-group` are `io-fields`, `meta-fields`,
  `heap-fields`, `alloc-fields`, `type-iter-fields`, `type-ops-fields` and
  `type-image-fields`. In C, `x_type_field_iter_group`,
  `x_type_field_ops_group` and `x_type_field_image_group` end in `_fields`,
  as do x-expr's `x_base_field_io_group`, `_meta_group`, `_heap_group` and
  `_alloc_group` ([x-expr#11]).
- A unit's ref, word, bytes or foreign is its label. `(type set-shape!)`
  and its bare name `type-set-shape!` are `(type set-unit-labels!)` and
  `type-set-unit-labels!`; `x_type_unit_kind` and `X_TYPE_UNIT_KIND_MASK`
  are `x_type_unit_label` and `X_TYPE_UNIT_LABEL_MASK`.
- The channel an analyser declares through carries a label. The export
  `jit_score_variant` is `jit_score_label`, the capability `tok/variant` is
  `tok/label`, and `x_token_read_arg_variant` is `x_token_read_arg_label`.
- The registry of instructions is the catalogue: `tools/contract/isa.x`
  defines `%isa-catalogue` in place of `%isa-catalog`.
- `tools/contract/obj-layout.x` drops x-expr's simple-type codes,
  `%obj-flag-simple-type`, `%obj-flag-prim`, `-fn`, `-int`, `-char`,
  `-str`, `-ptr` and `%obj-flag-type-mask`: the engine neither uses nor
  supports them.

The renames change no behaviour: the tree the routes walk, the mask's bits
and an image's bytes are what they were. The declaration's ISA and layout
digests follow the manifests, and its capabilities are the same set with
`tok/label` in place of `tok/variant`. Comments, tool-local names and the
earlier entries in this file use the glossary's words; an earlier entry
that quotes a renamed name says what it was released as.

x-lang takes the new names with the pin bump.

[#69]: https://github.com/jonruttan/x-engine-c/pull/69
[#70]: https://github.com/jonruttan/x-engine-c/pull/70
[x-expr#11]: https://github.com/jonruttan/x-expr/pull/11
[x-lang#808]: https://github.com/jonruttan/x-lang/pull/808

## 0.2.15 — 2026-09-27

**The release ships a profiling engine, and it counts evaluation per object**
([#66]). Every release now carries `x-bin-profile` beside `x-bin`: built with
`X_PROFILE` and `X_COV`, it differs from `x-bin` in those flags and nothing
else, and it is stripped and signed the same way. `make dist` packs it at the
tarball root, and `release.yml` checks it on unpack. Under `X_PROFILE` the
evaluator now also counts, in each object's flags word, how many times
evaluation reached it, at exactly the points `X_COV` marks, through one
helper, `x_eval_reached`. A procedure's first body cell holds its calls, and
the counts through its body are the evaluation it did itself. The count takes
bits 11 to 30 and stops at 1,048,575: x-expr's sweep clears bits 32 and up of
every object it keeps on a 64-bit host, and bit 31 is a 32-bit host's sign.
Bit 10 is the one the callers of `(heap tree-mark!)`, `(heap chain-clear!)`
and `(image write!)` mark with; it is named now, `X_OBJ_FLAG_TRACE`, and
`include/x-eval.h` asserts at compile time that the count sits above it.
`tools/contract/obj-layout.x` gains `%obj-flag-trace`, `%obj-evals-shift` and
`%obj-evals-bits`, `check-obj-layout` holds them against the header, and the
layout digest in `x-engine.xon` follows. The default build and `x-bin-cov`
are unchanged. A C spec covers the count, and `make test-bare-profile` runs
the bare specs against `x-bin-profile` and reads the count through the
contract, as x-lang will.

The variant builds (debug, profile, asan, cov) read their own dependency
files ([#66]). Each looked for the plain build's `.d` file, so a changed
header rebuilt none of them.

Also: five bare fixtures share `tests/bare/prim-ref.x`'s catalogue walk instead
of keeping their own copies, and the guarded cases in `env.spec.md`,
`image.spec.md` and `smoke.spec.md` check their primitives before the guard,
so a missing primitive fails the case instead of raising into the guard it
asserts on ([#65]).

x-lang's pin bump picks up the new layout digest.

[#65]: https://github.com/jonruttan/x-engine-c/pull/65
[#66]: https://github.com/jonruttan/x-engine-c/pull/66

## 0.2.14 — 2026-09-26

**The engine proper deals in no floats and loads no libraries** ([#63]).
`ffi-call` was a C double machine inside the engine, with arithmetic,
comparison, casts, libm calls through a function pointer, and string
conversion through `sprintf`, and `ffi.c` called libc for `sprintf`,
`memcpy`, `dlopen` and `dlsym`. The `sprintf` was what surfaced, as a macOS
deprecation warning in x-lang's ASan boot. The primitive is gone, with its
double helpers, its specs and the `(ffi call)` row of the ISA manifest;
floats belong to the language, and x-lang emits its double operations as
assembler stubs called through `(ptr call)` ([x-lang#796]). `dlopen` and
`dlsym` move to the CLI, `src/x-cli.c`, behind `X_DL`, on by default beside
`X_SYSCALL`, and keep their catalogue names `(ffi dlopen)` and `(ffi dlsym)`.
`ffi.c` and `callcc.c` copy through `x_lib_memcpy`.

A new gate holds the line: `make check-libc` (`tools/check/libc.sh`), in
`make gates`, refuses a libc header, a libc call, a feature-test macro or a
silenced diagnostic anywhere outside x-expr's `x_sys_*` and `x_lib_*`
wrappers. `src/x-cli.c`, the optional host modules under `opt/` (listed by
name), `ctype.h`, `setjmp.h`, the freestanding headers and code compiled
only under `DEBUG` are exempt. `--self-test` plants one offender per rule
and checks that the scan names exactly those. The rule was set before
0.2.9, but its gate was never committed, and the calls stayed.

x-lang reads the declaration's new ISA digest and drops `ffi/call` from its
contract files with the pin bump.

**A short call stops where its arguments do** ([#62]). A primitive that
unpacks a fixed prefix with `x_args` or `x_eargs` and then reads the rest of
its arguments as `x_11` or `x_111` of the list took the rest of nil when the
call was short, and segfaulted before any check ran: `(-)`, `(eval)`,
`(fn)`, `(op)`, a bare `ptr-call`. `x_args_tail`, in `include/x-prim.h`,
walks the list as `x_args` does, stops at nil and raises on a dotted tail
through `x_eval_spine_guard` ([x-lang#487]), and every such read in
`src/x-prim` and `src/x-syntax` goes through it: `-`, `apply`, `eval`,
`buf make`, `ptr-call`, `def`, `set!`, `fn`, `op` and `guard`. What a
primitive does with the nil it then receives is unchanged; `(apply)`,
`(def)`, `(set!)` and a bare `buf make` get past the walk and still fail on
nil, as they do when the nil is written out. Found by calling every catalogue
primitive with none to three arguments. Covered by a bare case that crashed
on 0.2.13.

**A profiling counter for the environment walk** ([#59]).
`profile-env-steps`, a tenth x-eval counter, counts under `X_PROFILE` each
binding `x_env_lookup` compares on its way to the root, the one lookup loop
no counter covered. The root's tree counts its own lookups
(`profile-bst-hits`, `profile-bst-misses`) and `profile-assoc-steps` counts
`assoc`, so nothing priced what scoping a module adds to a lookup, the
question standing between x-lang and scoping its hot modules. The base
layout gains the cell, with `x-eval-layout.h` regenerated and a new
`base-paths.x` row; the default build compiles none of it. A C spec,
built with `X_PROFILE`, checks a fresh counter reads zero, a hit on an
environment's only binding counts one, a miss counts every binding on the
way to the root, and a lookup starting at the root counts none.

Also: the bare smoke cases for the FFI nil-pointer and nil-operand raises
called `prim-ref`, which is x-lang's and unbound in a bare engine, so each
guard caught the unbound name and the case passed without reaching the
primitive ([#61]). `tests/bare/prim-ref.x` looks a primitive up through the
committed base paths, and each case checks the lookup is non-nil before
the guard.

[#59]: https://github.com/jonruttan/x-engine-c/pull/59
[#61]: https://github.com/jonruttan/x-engine-c/pull/61
[#62]: https://github.com/jonruttan/x-engine-c/pull/62
[#63]: https://github.com/jonruttan/x-engine-c/pull/63
[x-lang#487]: https://github.com/jonruttan/x-lang/issues/487
[x-lang#796]: https://github.com/jonruttan/x-lang/pull/796

## 0.2.13 — 2026-09-16

**`read` answers the EOF sentinel at end of input** ([#57]). The primitive
converted the reader's sentinel to nil before returning it, and nil is also
what a top-level `()` reads as, so a loop that read until nil could not tell
end of input from a `()` in its input and stopped at the first one. `read`
now answers the sentinel, the value x-lang binds as `%token-eof`, as
`repl-read` already did. x-lang's `(Io read)` answers nil at end of input
for the callers that loop until nil; the library's other callers of the
primitive, and x-sweet's two readers ([x-sweet#15]), stop at the sentinel.
Covered by a C case: a literal `()` reads as nil, and the read after it, at
end of input, answers the sentinel.

[#57]: https://github.com/jonruttan/x-engine-c/pull/57
[x-sweet#15]: https://github.com/jonruttan/x-sweet/pull/15

## 0.2.12 — 2026-09-16

**A guard's handler carries the handler it displaced** ([#54]). Installing a
guard's handler took the previous handler out of the error-handler slot and
kept it in a C local, which the collector cannot see. A collect inside the
body swept the enclosing guard's handler, and the base-eval handler under it
when the guard ran inside `(base eval ...)`; the pop on the way out wrote the
freed pair back into the slot, and the next raise longjmp'd through freed
memory. The handler's saved-environment cell had a nil slot beside the
environment, and it now holds the displaced handler, so every installed
handler stays reachable from the slot for as long as the innermost one is. A
base-eval handler leaves the slot nil, since it is consed onto the target's
stack and the one under it is reachable through the stack cell. The shortest
program that showed it is a nested guard whose body collects and then raises
to the outer guard, a segfault on 0.2.11. Found through [x-lang#728], whose
sweep after each module load during an image write ran inside the tower's
guard around its JIT probe and segfaulted the x-base, xe and rn writers on
macOS CI. Covered by a C spec beside the collect-in-load case, which asks the
allocation chain whether the enclosing handler and its saved-environment cell
survived a collect inside a guard body.

[#54]: https://github.com/jonruttan/x-engine-c/pull/54
[x-lang#728]: https://github.com/jonruttan/x-lang/pull/728

## 0.2.11 — 2026-09-15

**A name is found by identity, and a foreign symbol stands for the base's
own** ([#52]). 0.2.10's root tree took an equal spelling for a hit, so a
child's own symbol found a name the host had bound into it under the host's
symbol, which x-lang's conformance suite says it must not: names are found by
identity, and symbols intern per base. The old engine met that law by
accident, keeping a base-bound name on the alist where the walk compared
objects while the tree matched by spelling underneath; with every root
binding in the tree the accident was gone, and the first CI run of the pin
bump found it.

The tree now hits by identity only. An equal spelling that is not the same
object sorts to the right, so two bases' symbols of one spelling keep two
nodes and a rebinding updates its own. A symbol interned in another base has
no identity here, so lookup lets it stand for this base's own symbol of its
spelling, which is what lets `(base eval B (lit (+ 2 3)))` hand a child the
host's `+` and reach the child's binding of its own; the retry runs only when
the identity lookup at the root missed. Covered by the root-environment C
spec, which binds one spelling from three bases, and two bare cases. x-lang's
conformance suite passes against this release, 133 checks, and its spec suite
from source.

[#52]: https://github.com/jonruttan/x-engine-c/pull/52

## 0.2.10 — 2026-09-15

**An environment is a value** ([#49], closing [#46]; design note
[x-lang#718]). It is one pair, bindings and parent: the root's bindings are
a tree and its parent is nil; every other environment's bindings are an
alist and its parent is the environment it was made in. A procedure call makes a child of the
closure's environment, a parameterless one too. An operative body runs in
a child of its static environment and receives the caller's environment as
a value. `def` binds in the current environment, rebinding in place when
the name is already there and never touching a parent. `eval` with an
environment makes that one current, and a `def` inside stays bound,
because the binding is in the object. `eval!` and the loader evaluate in
the root. Every save and restore in the evaluator is one pointer.

That retires what compensated for a frame having no identity of its own:
the frame and function-frame flag bits, the shadow list, the local
boundary, the tree a closure carried and reinstalled on each call, the
operative restore's walk to decide whether the body had grown the caller's
chain, and the top-level bracket's stripping of a frame run. An operative
can now define for its caller with `(eval (list 'def n v) e)`, which is
the form every lang already wrote and which used to bind nothing inside a
frame ([x-lang#527]); `eval!` no longer binds a form's `def` somewhere
other than where evaluation said ([x-lang#644]). `def-global` is kept for
this release as `def` in the root, for the langs that reach it through the
catalogue; it is expressible without a primitive now and its row goes when
they have moved.

The base layout changes with it: the env fields are `env`, the current
environment, and `env-root`, in place of the alist, boundary, tree and
shadow slots, and the error handler's saved-boundary slot is nil. The
procedure state is `(params . (body . env))`. Both descriptors and the
declaration are regenerated in the same change; x-lang's readers of the
old rows move with the pin bump.

The environment operations live in `src/x-env.c`, the save and restore
around a tail call in `src/x-tco.c`, and the top-level bracket in
`src/x-toplevel.c`, one prefix per file; x-eval.c keeps the evaluator.

Covered by the C specs, which build environments the new way throughout,
a base spec of the environment operations, a root-environment spec with
real symbols, and `tests/bare/specs/env.spec.md`: a `define` built on
`eval` binds in the caller's frame, in body position, through a wrapper
operative, privately, in place on redefinition, visibly to a closure
captured earlier and globally at top level; a parameterless body keeps
its own definitions; a parameter named after a global shadows it for that
body only; a top-level name shared with an env parameter does not hijack
it; `set!` through two frames mutates the local; and an error handler runs
in a child of the guard's environment. x-lang's suite booted from source
against this engine passes but for the one spec that reads the retired
`env-alist` cell by name.

Also: the address table under `image write!` held its choices as bare
numbers ([#50]). The smallest slot count, the occupancy it keeps, the
address bits it shifts off and the naming cache's starting room are each a
named constant with its reason beside it, and the smallest slot count is
asserted a power of two at compile time, since the slot mask is that count
less one. Growth was quadrupling, the bigger table asked for twice the old
slot count in keys when room for n keys is 2n slots; it now asks for the
old slot count, which is twice the slots, as its comment said. A bare spec
writes an image whose spine refers to 300 objects outside it twice over,
past the cache's starting room, so the cache is shown to answer after it
grew.

[#46]: https://github.com/jonruttan/x-engine-c/issues/46
[#49]: https://github.com/jonruttan/x-engine-c/pull/49
[#50]: https://github.com/jonruttan/x-engine-c/pull/50
[x-lang#527]: https://github.com/jonruttan/x-lang/issues/527
[x-lang#644]: https://github.com/jonruttan/x-lang/issues/644
[x-lang#718]: https://github.com/jonruttan/x-lang/pull/718

## 0.2.9 — 2026-09-11

**An analyser tells the reader which of its states accepted** ([#43]). An
analyser knows things the token text does not say — which state accepted,
whether a numeric literal ran through a fraction or an exponent — and threw
that away, so the type's reader rescanned the text it had just read to find out
again. The score cell an analyser is handed now carries a **label cell**
(released as the variant cell) on its rest. A state writes an integer there as
it accepts; `x_token_analyse` resets both per handler (and puts the cell back
on the rest, since an analyser may set that slot itself — the C specs do, as a
reader side channel), records the winning handler's label through a new
out-parameter, and `x_token_read` hands it to the type's reader as its **second
argument**: the slot in `(buffer ())` that always held nil. It stays nil when
no state declared one, so a type that never heard of the channel reads exactly
what it always read, and every reader in x-lang and its bundles is variadic, so
no arity changes.

**A raw atom cell, not an int.** An int object only means anything in a base
that registered the int type, and `x_mkint` reaches it through the type's
`make`, which *registers the type on the base* as a side effect — and a
tokenizer base (`make-tok`) has no int type on purpose. Measured: the
registration put a built-in integer analyser into a custom tokenizer mid-read,
and the next token it read was the built-in's. The atom type is static and
lives everywhere, so the label travels the way the score does: a cell whose
value word is the integer, `x_atomint` in C, `%cell-int` in x-lang.

`jit_score_label` (released as `jit_score_variant`) is the compiled states'
door — `jit_score_set`'s three-line peer, one export. The channel is a protocol
extension of `(tok read)` rather than a primitive, so the ISA manifest cannot
describe it; it is claimed as `tok/label` (released as `tok/variant`) in
`claims.x`, the way `native/jit` is, and the declaration is regenerated in the
same commit — the step 0.1.5 and 0.2.1 each shipped without. x-lang spells the
two ends `%score-label!` and `%read-label` (released as `%score-variant!` and
`%read-variant`, [x-lang#671]) and gates its specs on `@requires tok/label`; an
engine without the symbol is unaffected, since the binding is optional and the
compile falls back. Covered by `tests/c/src/7.0.x-token.spec.c`: a
three-character token whose type declares label 7 arrives with its span whole
and its label delivered, and one whose type declares none hands the reader nil.

Also: three comments called an error's label its "kind". The engine's prose
follows x-lang's word for it (released as **tag**, [x-lang#672]; now
**label**) ([#44]). Comments only, no code change.

[#43]: https://github.com/jonruttan/x-engine-c/pull/43
[#44]: https://github.com/jonruttan/x-engine-c/pull/44
[x-lang#671]: https://github.com/jonruttan/x-lang/pull/671
[x-lang#672]: https://github.com/jonruttan/x-lang/pull/672

## 0.2.8 — 2026-09-06

**A def scopes by the live frame, not by the save stack.** `def` decided
top-level by "the save-stack is empty", which is true in a closure body's
TAIL position: the frame is popped before the deferred tail runs. So
`(fn (_) (if c (do (def x 1) ...)))` defined `x` for the whole base while the
same def one form earlier was frame-local -- position-dependent scope, and
the way every compile in x-lang's asm lane left its buffer, self-cell and
function in bare globals (`hit`, `cell`, `buf`), which is what kept the JIT
dialects out of a state image. `x_prim_define` now asks whether the env head
is a FRAME-marked cell. Two things had leaned on the old rule. `eval!`, the
REPL's evaluator, runs inside the frames of whatever called the REPL
(`(unless %batch? (do (%banner) (repl)))`), so it now evaluates its form as a
top-level form through the loader's own bracket, now one implementation
with two doors (`x_toplevel_enter`/`x_toplevel_leave`: save-stack hidden,
the leading FRAME run stripped from the head, the displaced state parked on
the root chain, all restored after).
And `x_op_restore` kept an inner operative's formals on the chain whenever
the caller's head was "reachable" from them, which after any load it always
is, since every chain ends at the same bottom cells: a `when` or `unless`
whose `if` took the empty branch left `test then else e` at the head of the
top-level environment for the rest of the session. The walk now stops at a
FRAME cell, so a foreign frame restores to the caller and only def cells
grown onto the caller's env are kept. Code that must bind globally from
inside a frame says so with `def-global`, which the langs already do.
Verified: x-lang's suite from source, all six amalgams, and an image of
`x-base.x` with nothing unnameable.

## 0.2.7 — 2026-09-05

**A type registered on another base outlives the collector again**
([x-lang#599]). `base-make-type` builds the name atom, the type struct and the
handler closures on the CALLING base and then files them in the TARGET base's
type alist — so the objects sit on one heap chain while their only referrer
sits on another. Pinning them as SHARED is the whole point of the mark at the
end of that primitive, and the mark started from the target's tree root, which
pinned **nothing**: `x_heap_tree_mark` stops at any object that already carries
the flag it is setting, and a base's tree root is born SHARED (x-expr's
`x_base_make` allocates every skeleton node that way), so the walk
short-circuited on its own first node. It now starts from the type struct, an
ordinary `X_OBJ_FLAG_NONE` pair tree, and reaches the name atom and every
handler.

The failure was delayed by exactly one collect, which is what made it read as
a collector bug rather than a missing pin. The calling base's mark walk reaches
the target base through whatever binding holds it and descends its tree, so the
first collect marked these objects by that route and retained them — but it
also left the mark bit set on the target's OWN skeleton cells, which live on
the target's chain and are never visited by the calling base's sweep. The
second walk took those still-marked cells for already-done and stopped short,
leaving the name atom unmarked and unpinned; the sweep freed it, and the next
read walked the type alist over a freed key (`x_alist_assoc`, a
heap-use-after-free under ASan).

A bundle that registers its own tokenizer types on an isolated `make-tok`
base paid for this: x-ash had to run its whole suite with the per-snippet
`SPEC_SEAM_COLLECT` off, because with it on the tokenizer specs died first.
Covered by a regression case in bare — three reads across two collects, the
shortest case that shows it.

[x-lang#599]: https://github.com/jonruttan/x-lang/issues/599

## 0.2.6 — 2026-09-05

**The loader roots what it parks** ([#38]). `x_eval_load` displaces two pieces
of the includer's state for the length of a load: it hides the save-stack so a
loaded file's top-level `def`s bind globally, and strips the includer's `FRAME`
cells off the env head so a closure the file defines does not capture them.
Both right. Both waited in C locals — and the collector is precise: it marks
from the base tree, the root chain and the registered roots, never the C stack.
A loaded file that collected swept the includer's frame cells and restore
compounds; the load returned, put the freed head back, and the includer walked
freed memory on its next symbol lookup.

Whether that walk crashed depended on the allocator. glibc reuses a freed cell
at once, so on x86-64 Linux it was a SIGSEGV in `x_type_symbol_eval`; macOS's
allocator mostly leaves the cell intact, so it answered right by luck — every
local run, every macOS CI job. That is how it presented as an "x86-64 JIT
crash" holding three x-lang pull requests red while their `main` stayed green
on an older pin: x-lang's `compile-asm` collects every `%asm-gc-window`
compiled expressions from inside the tower's own includes, and a core dump on
an x86-64 guest showed the includer's `(%io-path …)` frame cell with glibc's
safe-linked free-list pointer written through it.

The saves now ride the root chain for the loop — the mechanism built for a C
frame holding the only reference (`x_prims_add` roots a half-built catalogue
entry the same way). Two registered nodes rather than one pointing at the
other, because the chain's pre-clear pass strips stale marks only from
registered nodes. The error path needs nothing: the guard already restores the
root chain from its snapshot. No layout change, no new field, no new
coordinate.

Two specs. `tests/c/src/4.5.x-eval-load.spec.c` stands an includer up, loads a
file that only collects, and asks the allocation chain whether the frame and
compound survived — chain membership is the collector's own record and reads
no freed memory, so it is the same answer on every allocator; red before,
green after. `tests/bare/specs/smoke.spec.md` gains the case a program sees:
a procedure includes a file that collects, then reads its own formal. Verified
end to end: v0.1.6 plus this change, built on an x86-64 Linux guest, boots the
xenon tower cold with `%asm-gc-window` forced to 1 — the configuration that
crashed five of five — and prints `1`.

The original comment defended the locals against `longjmp`, correctly, and not
against collection: the engine never collects on its own, so a load loop
looked like it contained no collector. It contains whatever the host puts in
the file. That is the general lesson, and it is why x-lang now boots every
dialect on an ASan build of the pinned engine before a push (x-lang#615): a
live-but-unrooted object is invisible until something collects, and the first
collect anyone adds is the one that finds it.

[#38]: https://github.com/jonruttan/x-engine-c/pull/38

## 0.2.3 — 2026-09-04

**Ask the collector what is reachable** ([#33]). A consumer that needs to know
what a base can reach cannot work it out by walking. Measured against a booted
helium: a reachability walk written in x-lang reaches 33,823 objects of 85,466
live. It cannot see the base sentinel, the custom mark handlers, the mark hooks
or the root chain — and a structural pair in the base spine can hold a raw C
function pointer, which following dereferences.

The collector already computes the answer, and `x_heap_tree_mark` already takes
the flag it sets as a parameter. So two coordinates, and nothing more than the
functions behind them:

    (heap tree-mark! obj flags)    mark a tree with flags of the caller's choosing
    (heap chain-clear! flags)      clear them again, freeing nothing

Which flag is the caller's problem. The collector owns `SHARED` and `MARK`, so
a caller picks a bit above them and names it in its own source. There are two
ways to get that wrong worth knowing: the flag doubles as the traversal's
visited test, so `SHARED` halts at the first base-tree node — two objects
marked, out of 85,431 live — and a leftover `MARK` makes the next mark phase
stop short and its sweep free the children it missed.

A **chain** clear rather than a tree one, and rather than an unset mode on the
walker. The mark hooks call back into `x_heap_tree_mark` with the flags they
are handed, so an unset mode would have hooks *setting* the flag on children
unless the mode threaded through every hook signature. And a tree reaches only
what is still reachable, so anything that became garbage since the mark would
keep the flag for good.

Through the coordinates: 0 marked, then 79,407 of 83,521 chain objects after
marking from the base, then 0 again after the clear. The 4,114 not marked are
the caller's own state and garbage — what a heap reader wants left out.

x-expr gains `x_heap_chain_clear` and a fifth general-purpose attribute bit
(`X_OBJ_FLAG_5`, with `X_OBJ_FLAG_ATTR_MASK` widened to `0x1F`).

## 0.2.2 — 2026-09-03

The declaration matches the ISA again.

### Fixed

- **`x-engine.xon` declared 0.2.0's ISA through 0.2.1** ([#29]). 0.2.1 added
  `(type set-unit-labels! types)` (released as `(type set-shape! types)`) to
  `tools/contract/isa.x` and did not regenerate the declaration, so that
  release ships

      (isa "sha256:9ac3e2b2…")

  — the digest of the ISA *before* the primitive was added. The engine
  claimed one surface and carried another. This release carries the digest
  of the ISA it actually ships, `sha256:255c90b1…`.

  **This is the second time, and the same way both times.** 0.1.5 did it with
  `(base def-global)`; 0.1.6 fixed it and proposed the guard — run x-lang's
  generator from CI here against a cloned x-lang, the way the bundles clone
  the lang kit — and recorded that it was *not done in this release*. It is
  still not done. A known unguarded gap did what a known unguarded gap does,
  two releases after the fix.

  The reason the gap exists has not changed and is not an oversight:
  `x-engine.xon` is generated by x-lang's `tools/contract/gen-engine-xon.sh`
  deliberately, because generating the declaration needs the *vocabulary*, and
  an engine that generated its own would be choosing the terms it is judged
  by. The consequence is that CI here — the bare-engine spec suite — knows
  nothing about the digest, so a release can go out inconsistent and only the
  downstream discovers it. Twice now.

  Nothing in the built engine differs and `make test` is 12/0 on either side
  of the line; the cost is entirely downstream. 0.2.1 stays published and
  stays inconsistent: anything pinning it fails x-lang's
  `check-engine-contract`, so this is the release to pin.

  Regenerating is still one command, run from an x-lang checkout:

      sh tools/contract/gen-engine-xon.sh <engine-dir>

### Added

- **`jit_call_value`** ([#27]) — call a callee computed at run time.

  The JIT lane could branch only to an address fixed when the code was
  generated: the self-call's trampoline cell, or an fvar's prim baked as an
  immediate. It had no way to call a callable it *computed*, which is what a C
  function pointer is (x-lang#604) and what a dispatch table needs.

  `jit_call_value` takes `(p_base, p_args)` — a primitive's signature exactly —
  and calls the callee sitting in `p_args`' self slot. That placement is the
  point of the signature: the emitter already builds `(callee arg0 arg1 ...)`
  to hand the callee its own args list, so the callee needs no second
  register to survive the list construction, and a compiled callee that names
  its own self param still finds itself there.

  **The type check is why this is a function and not four instructions.**
  `x_primval` is an object's first word; on anything that is not a PRIMITIVE
  that word is a length or a character, and branching to it is a SIGSEGV with
  no relation to the call site — the failure the emitter's "refuse loudly at
  generation" rule exists to prevent, which a run-time head can only be refused
  at run time. It lives here rather than in emitted instructions because the
  type it consults is the type system's business, not a layout offset a code
  generator should bake in.

  Peer of `jit_make_prim` — same file, same `x-type/prim.h` surface. Twelve
  lines plus one export, and nothing else in the engine changes. x-lang
  resolves the symbol *optionally*, the way it resolves `jit_buffer_last_char`:
  an engine without it keeps compiling every form that does not need it, and
  only a call through a computed head refuses and falls back.

- **`(provides native/jit)`** ([#30]) — the in-process assembler lane,
  declared.

  The engine exports its `jit_*` runtime helpers from the running binary, so a
  `dlopen` of self resolves `jit_buffer_len` and its siblings, and it permits
  executing the pages the assembler writes. x-lang's `x/tool/asm-compile.x` and
  ten spec files carrying `# @requires native/jit` have consumed that lane for
  as long as it has existed — gating on a row no engine published.

  Claimed at the **implementation** level, for the reason `instr/cov` is: the
  repo builds it, and what a particular binary ended up with is a build fact
  recorded beside that binary. Two such facts are worth naming because both
  have bitten. On macOS the execute half needs the code signature the Makefile
  applies from `entitlements.plist` (`allow-jit`,
  `allow-unsigned-executable-memory`); a build that skips the codesign step has
  the symbols and cannot run the pages. And a bare `strip` drops the exported
  symbol table that `strip -x` keeps, so an installed engine had no `jit_*`
  symbols at all while the repo build was fine (x-lang#201) — which
  `asm-compile.x` refuses on, by probing every helper before it emits anything
  rather than compiling a `blr` to address 0.

[#27]: https://github.com/jonruttan/x-engine-c/pull/27
[#30]: https://github.com/jonruttan/x-engine-c/pull/30
[#33]: https://github.com/jonruttan/x-engine-c/pull/33

## 0.2.1 — 2026-09-03

**A unit declares what it IS, not just that it exists** ([#29]). `p_units` was
a count, and the collector's fallback traced every unit it named. That made
the slot undeclarable for any type whose units are not references — INTEGER,
STRING, SYMBOL, CHARACTER, PRIMITIVE and POINTER all declared nothing, so
their one-unit size lived in the C constructors and in no contract, and
nothing reflective could learn where such an object ends.

Declaring it was not merely useless but **unsafe**. `x_heap_tree_mark` sets
the mark bit *through* the pointer it is handed, before it can establish that
the pointer is on the heap, so `units 1` on STRING would OR a bit into memory
three words ahead of the string's bytes at the next collect.

So the slot **widens in place** — no new field. An INT atom keeps both of its
meanings exactly (N units all references; negative for the slot-0-counted
convention), and a structural pair `(count . mask)` adds two bits per unit
saying what each one is: `ref`, `word`, `bytes`, `foreign`. Units past the
described prefix take the label of the last one described, so a dynamic-size
type says what its payload units are without a repeat marker.
`X_TYPE_UNIT_REF` is 0, so a zero mask means "every unit a reference" and the
pair form degrades exactly onto the integer form.

**`word` means a raw machine value, not a small one.** The 0.2.1 entry as
first published claimed `(word ref)` over a count of `-1` "is the vector".
That is wrong, and wrong in the direction that corrupts: a vector's slot 0
holds a heap INTEGER *object*, not an immediate, so declaring it `word` tells
the collector not to trace it and the length is freed under the instance,
leaving the slot dangling rather than nil. A vector is `(ref ref)` — mask 0,
the bare count it already had. `word` is for a unit holding a machine value
the collector must not follow, which is what the engine's own atom types
carry: an int, a character code. Ask what the unit *holds*, not how big it
looks. The code was always right; only that sentence was not.

Three sites read the slot and each takes one `x_obj_type_isspair()` test per
object — the collector's traversal, the unit accessor, the spine guard — then
a shift and mask per unit. Nothing allocates and nothing interns.

**`(type set-unit-labels!)` (released as `(type set-shape!)`) is a primitive**
because the unit labels must be a *structural* pair: x makes list-pairs only,
the readers discriminate on `x_obj_type_isspair()`, and an x-built pair would
be read as a bare count whose value is the pair's first data word. The readable
spelling stays in x-lang and compiles to two integers before the engine sees
it.

`(type set-units!)` is untouched: the integer form is still the integer form.

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

The layout is a declared guarantee, `err/typed-raise`, claimed in `claims.x`
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
can. A base still in that window raises through a static fallback laid out
as an ERR, so every C consumer reads `x_err_code`/`x_err_subject` without
asking which window it came from.

### Added

- **`jit_buffer_last_char`** ([#24]) — the last-read character as a raw long,
  exposed to compiled code.

  The JIT lane already publishes the engine's buffer and score macros as real
  callable functions (`jit_score_set`, `jit_buffer_unread`, `jit_buffer_len`).
  This is the peer of `jit_buffer_len` — a different buffer macro
  (`x_bufferlastchar` vs `x_bufferlen`), the same three lines, not a
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
[#29]: https://github.com/jonruttan/x-engine-c/pull/29

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

  The dot was one of the single-character tokens: it sat in
  `X_SEXP_LIST_CHARS_STR` beside the brackets, so the analyser scored it on
  sight. That is correct for `(` and `)`, which really are always
  single-character tokens, and false for `.`, which is a separator only when
  nothing follows it. A token merely *beginning* with a dot was taken whole as
  the separator, and `x_sexp_list_read` returned `x_sexp_list_delimit_prim` for
  it — consumed inside a list, and returned to the caller at the head of one,
  where a raw C satom is not a value any x program can survive touching.

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

- **The FFI raises on a nil function pointer or operand instead of calling it**
  (one of the failures of [x-lang#171][i171]). A dlsym miss answers nil, and
  every call convention handed that nil straight to the machine —
  `x_ptrval(nil)` as a call target, `x_intval(nil)` as a memcpy source.
  x-lang's v0.5.0 release run died on exactly this: the first conformance run
  Linux ever saw resolved `sqrt` against an engine that links no libm, got nil,
  and called it. One door per harm: `x_ffi_fptr` guards the function-pointer
  conventions and `ptr-call`; the nil-operand check lives in `x_ffi_to_double`,
  which every double convention shares. The arithmetic and comparison
  conventions never touch the fptr and are untouched. Three bare specs pin the
  behaviour.

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
