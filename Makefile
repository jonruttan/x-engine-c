# # Computational Expressions in C
#
# ## Makefile
#
# @description Computational Expressions in C
# @author [Jon Ruttan](jonruttan@gmail.com)
# @copyright 2021 Jon Ruttan
# @license MIT No Attribution (MIT-0)
#
#     ., .,
#     {O,O}
#     (   )
#      " "
# Info on portable Makefiles:
# - [A Tutorial on Portable Makefiles « null program](http://nullprogram.com/blog/2017/08/20/)
# - [Makefile Assignments are Turing-Complete « null program](http://nullprogram.com/blog/2016/04/30/)
# - [os agnostic - OS detecting makefile - Stack Overflow](https://stackoverflow.com/questions/714100/os-detecting-makefile)
# - [Gagallium : Portable conditionals in makefiles](http://gallium.inria.fr/blog/portable-conditionals-in-makefiles/)
# - [make](http://pubs.opengroup.org/onlinepubs/009695399/utilities/make.html)

.POSIX:

# The install prefix
PREFIX?=/usr/local

# Override default compiler and flags
CC?=gcc
CFLAGS?=-O2
CFLAGS+=-Wall -Wextra -Wno-unused-parameter
CFLAGS+=-DX_HEAP -DX_TYPE -DX_SYS_CLOCK

# Dead code elimination: each function/data in its own section, stripped at link
CFLAGS+=-ffunction-sections -fdata-sections

# Get the compiler name
CCOMPILER=$(CC)
ifeq ("$(CCOMPILER)", "cc")

ifeq ($(shell diff $(shell which cc) $(shell which gcc)), )
CCOMPILER=gcc
else ifeq ($(shell diff $(shell which cc) $(shell which clang)), )
CCOMPILER=clang
endif

endif

# If there are no LDFLAGS, use the CFLAGS
LDFLAGS?=$(CFLAGS)

# Customise the settings for the compiler
CFLAGS+=-fdiagnostics-color=always
ifneq ("$(CCOMPILER)", "tcc")
DUMPMACHINE=$(shell $(CC) $(CFLAGS) -dumpmachine)
endif
ifeq ("$(CCOMPILER)", "c89")
CFLAGS+=-ansi -Wno-unused-result
else ifeq ("$(CCOMPILER)", "c99")
CFLAGS+=-Wno-unused-result
else ifeq ("$(CCOMPILER)", "gcc")
CFLAGS+=-ansi -Wno-unused-result
else ifeq ("$(CCOMPILER)", "clang")
CFLAGS+=-ansi -Wno-array-bounds
endif

# Fallback command to use when compiler doesn't support `-dumpmachine`
ifndef DUMPMACHINE
DUMPMACHINE=$(shell echo $(shell uname -m)-$(shell uname -s)-$(shell uname -o) | tr A-Z a-z)
endif

# Get the machine Target Triplet
X_MACHINE?=\"$(DUMPMACHINE)\"

# Dead strip unreferenced sections at link time
# Export dynamic symbols so dlopen'd bundles can call host functions
ifneq (,$(findstring darwin,$(DUMPMACHINE)))
LDFLAGS+=-Wl,-exported_symbols_list,exports.sym -Wl,-dead_strip -Wl,-dead_strip
else ifneq (,$(findstring linux,$(DUMPMACHINE)))
LDFLAGS+=-Wl,--gc-sections -rdynamic
endif

BASEDIR=.
INCDIR=$(BASEDIR)/include
SRCDIR=$(BASEDIR)/src
OPTDIR=$(BASEDIR)/opt

# x-expr foundation library
X_EXPR_DIR=ext/x-expr
X_EXPR_SOURCES=$(wildcard $(X_EXPR_DIR)/src/*.c)
X_EXPR_OBJECTS=$(X_EXPR_SOURCES:.c=.o)

CFLAGS+=-I$(X_EXPR_DIR)/include -I$(INCDIR)

HEADERS=$(wildcard $(INCDIR)/*.h $(INCDIR)/**/*.h $(INCDIR)/**/**/*.h $(X_EXPR_DIR)/include/*.h)
SOURCES=$(wildcard $(SRCDIR)/*.c $(SRCDIR)/**/*.c $(SRCDIR)/**/**/*.c)
OBJECTS=$(SOURCES:.c=.o)
# NAME is the PROJECT name: the wrapper's installed command (bin/x) and the
# install-tree dirs (share/x, libexec/x) -- x.sh's X_SHARE/X_ENGINE and the
# bootstrap tarball layout depend on it.  EXECUTABLE is the ENGINE BINARY's
# filename (x-bin), deliberately distinct from the repo root, the wrapper,
# and the .x extension so tooling can match it precisely.
NAME=x
EXECUTABLE=x-bin
OUTPUT=$(EXECUTABLE)

# Options to be added to $(DEFS)
DEFS?=$(OSDEF) -DX_MACHINE="$(X_MACHINE)" -DX_SYSCALL -DX_INCLUDE -DSYMBOL_FIND_REORDER

# SIGINT (Ctrl-C) handling, on by default (X_SIGNAL carries the -DX_SIGNAL
# flag).  The signal module lives under opt/ and is built only when enabled;
# `make X_SIGNAL=` leaves it out of the build and compiles the eval poll out
# too (x-lang REPLs fall back to no-ops).  DEFS is absent from TEST_CFLAGS, so
# the C unit tests always build without it.
X_SIGNAL?=-DX_SIGNAL
ifdef X_SIGNAL
DEFS+=$(X_SIGNAL)
SOURCES+=$(OPTDIR)/x-prim/signal.c
endif

# -ldl is the FFI/JIT layer's (dlopen/dlsym in src/x-prim/ffi.c and
# src/x-obj/jit.c) -- the expression engine ext/x-expr needs no libraries
# beyond libc.  Darwin and glibc >= 2.34 fold dl into libc, so the flag is
# a compat no-op there.  There is deliberately NO -lm: the one C fmod call
# was retired (float % goes through float.x's dlsym'd %libm handle, which
# dlopens libm at runtime like every other math function).
EXTRA_LIBS+=-ldl

# Where to install the stuff.  The user-facing command is the WRAPPER,
# installed as bin/x; the engine binary hides in libexec (without the
# library on its stdin pipe it cannot even print, so it is not a user
# command).  MANDIR is reserved for a future man page.
BINDIR?=$(PREFIX)/bin
LIBDIR?=$(PREFIX)/share/$(NAME)
LIBEXECDIR?=$(PREFIX)/libexec/$(NAME)
MANDIR?=$(PREFIX)/man/man1

# C test config
ifndef PATH_TESTS_C
PATH_TESTS_C=tests/c
endif
ifndef TESTS
TESTS=$(PATH_TESTS_C)/src/*.spec.c
endif
TEST_CFLAGS=$(CFLAGS) -fno-common -g -Og -I. -DTESTS


# Coverage
COVERAGE_DIR=.coverage

# ============================================================================
# Build
# ============================================================================

default: all strip ## Build and strip

all: $(SOURCES) $(EXECUTABLE) ## Build all

strip: $(EXECUTABLE) ## Strip non-global symbols (keep dynamic exports for dlopen)
	strip -x $(EXECUTABLE)
	@if [ -f entitlements.plist ]; then codesign -s - --entitlements entitlements.plist -f $(EXECUTABLE) 2>/dev/null || true; fi

$(EXECUTABLE): $(OBJECTS) $(X_EXPR_OBJECTS) $(EXTRA_OBJS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(X_EXPR_OBJECTS) $(EXTRA_OBJS) $(EXTRA_LIBS) -o $(OUTPUT)

# Variant builds (debug / profile / asan) share src/*.o with the normal build,
# so each brackets its work with clean-obj: the leading one forces a rebuild
# under the variant's flags; the trailing one removes those objects so a later
# plain `make` doesn't relink them -- silently picking up -DDEBUG, or hard-
# failing on the ASan runtime ("_asan.module_ctor ... symbol(s) not found").
x-bin-debug: ## Build debug target
	$(MAKE) clean-obj
	$(MAKE) OUTPUT=$@ CFLAGS="$(CFLAGS) -g -Og -DDEBUG" $(EXECUTABLE)
	$(MAKE) clean-obj

x-bin-profile: ## Build profiling binary (includes coverage)
	$(MAKE) clean-obj
	$(MAKE) OUTPUT=$@ CFLAGS="$(CFLAGS) -DX_PROFILE -DX_COV" $(EXECUTABLE)
	$(MAKE) clean-obj

# ASan flags go in CFLAGS only: 'LDFLAGS?=$(CFLAGS)' (above) carries them into
# the link too, so the runtime links while KEEPING the project's -dead_strip /
# exports.sym LDFLAGS (passing LDFLAGS on the command line would lose those).
x-bin-asan: ## Build with AddressSanitizer for memory-safety testing
	$(MAKE) clean-obj
	$(MAKE) OUTPUT=$@ CFLAGS="$(CFLAGS) -fsanitize=address -fno-omit-frame-pointer -g" $(EXECUTABLE)
	$(MAKE) clean-obj
.PHONY: x-bin-asan

# Not PHONY: once the file exists the target is satisfied, so the double
# rebuild (clean-obj brackets, like the variants above) is paid once per
# clean, not once per test run.  `make clean` removes it.
x-bin-cov: ## Build the coverage binary (-DX_COV flag-marking eval)
	$(MAKE) clean-obj
	$(MAKE) OUTPUT=$@ CFLAGS="$(CFLAGS) -DX_COV" $(EXECUTABLE)
	$(MAKE) clean-obj

clean-obj:
	rm -f $(SRCDIR)/*.o $(SRCDIR)/**/*.o $(SRCDIR)/**/**/*.o $(OPTDIR)/**/*.o $(X_EXPR_DIR)/src/*.o

.c.o:
	$(CC) -c $(CFLAGS) $(DEFS) -o $@ $<

# ============================================================================
# Test
# ============================================================================

test-c: ## Run C unit tests
	CFLAGS="$(TEST_CFLAGS)" RUNNER=command sh $(PATH_TESTS_C)/test-runner/test-runner.sh $(TESTS)
.PHONY: test-c

test-x: $(EXECUTABLE) ## Run x-lang tests
	sh tests/x/spec-runner.sh
.PHONY: test-x

# The tools' own spec suite (tools/tests), repaired from the post-overhaul
# rot (#180).  Two runners: spec-runner.sh takes the top-level specs on the
# plain engine; cov-spec-runner.sh takes specs/cov/ on x-bin-cov, because
# coverage marking only exists under -DX_COV.
test-tools: $(EXECUTABLE) x-bin-cov ## Run the tool suite's specs (tools/tests)
	sh tools/tests/spec-runner.sh
	sh tools/tests/cov-spec-runner.sh
.PHONY: test-tools

# The doctest ratchet (#16): every (example "in" "out") in the doc registry
# is an executable contract -- "out" must be the true echo.  tools/check/doctest.sh
# extracts them into a generated spec; the personality runner executes it.
# Illustrations that must not run are (sample ...) forms (see doc.x).
doctest: $(EXECUTABLE) ## Extract (example ...) forms and run them as doctests
	mkdir -p build/doctest-specs
	sh tools/check/doctest.sh > build/doctest-specs/doctests.spec.md
	sh tests/x/doctest-runner.sh
.PHONY: doctest

# The contract gates, ONE definition: `make test` runs them first and
# CI's "Contract gates" step runs exactly this target.  They must not
# drift -- ci.yml once hand-listed a subset, and check-pin's first run
# on Linux happened in the RELEASE job (where it promptly died).
gates: check-isa check-obj-layout check-base-paths check-boot-order check-path-literals check-boot-amalgam check-pin check-release-manifest check-bootstrap check-package check-doc-vocab check-dup-defs check-bare-globals check-percent-globals check-dialect-cover ## Run the contract gates
.PHONY: gates

# The local-latency split (2026-08-03 audit): `make test` grew past ten
# minutes (build/install/package smokes, amalgam boots, doctest walk,
# example programs) and the pre-push hook ran ALL of it inside the open
# push connection -- long enough for GitHub to hang up the idle SSH
# channel mid-hook.  gates-fast is the sub-minute subset: every scan
# ratchet, none of the targets that build or boot artifacts.  The hook
# runs test-fast; CI still runs the FULL `make test` on every push/PR
# (ci.yml unchanged -- it stays the enforcing gate for the heavy surface).
gates-fast: check-isa check-obj-layout check-base-paths check-boot-order check-path-literals check-doc-vocab check-dup-defs check-bare-globals check-percent-globals check-dialect-cover ## The fast contract gates (pre-push subset)
.PHONY: gates-fast

test-fast: gates-fast test-c test-x ## Pre-push gate: fast gates + both spec suites (CI runs full `make test`)
.PHONY: test-fast

# bootstrap.sh's build+install path (its coupling to the install layout);
# the clone path is exercised by the release workflow on a clean checkout.
check-bootstrap: $(EXECUTABLE) ## Smoke the one-command bootstrap install
	sh tools/check/bootstrap-smoke.sh
.PHONY: check-bootstrap

test: gates test-c test-x doctest spec-examples check-examples lint-x test-tools ## Run all tests
.PHONY: test

# The release manifest (SHASUMS + pin.release.xon over the amalgams;
# .github/workflows/release.yml publishes it on a version tag) -- gated
# here with a throwaway tag so the self-checking script cannot rot
# between releases.
check-release-manifest: boot ## Generate + self-check the release manifest
	sh tools/release/release-manifest.sh local
.PHONY: check-release-manifest

# The relocatable binary tarball (release.yml ships one per platform on a
# tag) -- gated with a throwaway tag: package.sh stages the install tree,
# tars it, and self-proves relocation (unpack elsewhere, run) so the
# packaging cannot rot between releases.  Output lands under build/.
check-package: $(EXECUTABLE) ## Build + self-check a relocatable binary tarball
	sh tools/release/package.sh check build/dist-check
.PHONY: check-package

# Project pinning (docs/modules.md "Pinning"): the wrapper's pin.xon probe
# and lib/x/tool/pin.x, end to end -- overlay resolution, root precedence,
# the unpinnable boot core, the closed manifest vocabulary, --no-pin.
check-pin: $(EXECUTABLE) ## Smoke the pin.xon probe + loader end to end
	sh tools/check/pin-smoke.sh
.PHONY: check-pin

# The examples ratchet: every file under examples/*/ runs under its documented
# dialect in batch mode; output-pinned where portable (sidecars in
# tests/examples/).  The examples are the first code a newcomer runs and were
# previously the only code with no gate.  UPDATE=1 regenerates sidecars.
check-examples: $(EXECUTABLE) ## Run every example under its documented dialect
	sh tools/check/examples.sh
.PHONY: check-examples

# The logo tty contract (#152/#157): expect-driven pty sessions pinning the
# interactive behaviors (ctrl-c cancel, exit paths, hooks, execute-once)
# that isatty guards hide from every batch suite.  Not part of `make test`
# (tty environments flake); CI runs it explicitly on both OSes.  Skips
# with a note when expect(1) is absent.  known-fail.txt entries pin the
# post-#157 ruling; a listed test PASSING is red (delete its line).
check-logo-tty: $(EXECUTABLE) ## Run the logo interactive-contract pty tests
	sh tools/check/logo-tty.sh
.PHONY: check-logo-tty

# The C-surface ratchet, source half: every binding site in the C source must
# appear in the committed manifest tools/contract/isa.x, so growing the C layer requires
# a deliberate manifest edit in the same commit.  The runtime half lives in
# tests/x/specs/meta/isa.spec.md (runs under test-x).
check-isa: ## Diff the C source's binding surface against tools/contract/isa.x
	sh tools/check/isa.sh
.PHONY: check-isa

# The object-layout contract, source half: the header-word layout parsed out
# of ext/x-expr/include/x-obj.h must match the committed descriptor
# tools/contract/obj-layout.x, which reflective X code reads its offsets from.  The
# runtime half is tests/x/specs/meta/obj-layout.spec.md (runs under test-x).
check-obj-layout: ## Diff x-obj.h's object layout against tools/contract/obj-layout.x
	sh tools/check/obj-layout.sh
.PHONY: check-obj-layout

# The base-paths contract, source half: every base-field accessor macro
# (x-eval-layout.h, x-base.h, the error-handler in x-eval.h) flattened to a
# first/rest path must match tools/contract/base-paths.x, which reflect.x walks.
# The runtime half is tests/x/specs/meta/base-paths.spec.md.
check-base-paths: ## Diff the base-field macro chains against tools/contract/base-paths.x
	sh tools/check/base-paths.sh
.PHONY: check-base-paths

# The boot-order lint: derives the effective load order from each boot entry
# (lib/x-core.x + the three dialect entries; include forms, the
# %include-list-cell pre-seed, import expansion) and flags (a) load-time
# class-calls whose def-class comes later in the order -- the silent
# class-call trap -- and (b) pre-seed drift: double loads and raw-included
# lib paths never registered (see tools/check/boot-order.x).
check-boot-order: $(EXECUTABLE) ## Lint the boot load order: class-call order + pre-seed drift
	sh tools/check/boot-order.sh
.PHONY: check-boot-order

# Doc-type vocabulary ratchet: the adjudicated losers (INTEGER/BOOLEAN/
# FUNCTION -- see contributing.md) must not reappear in (param ...)/
# (returns ...) forms; INT/BOOL/CALLABLE are the one-name-per-concept picks.
# The duplicate-global-def ratchet (#47): top-level redefinition updates the
# shared binding in place, so two modules defining one name with different
# meanings is a real collision (the %alist-find segfault).  tools/check/dup-defs.sh
# holds the rule + the adjudicated allowlist.
check-dup-defs: ## Lint lib+apps for cross-module duplicate global defs
	sh tools/check/dup-defs.sh
.PHONY: check-dup-defs

check-path-literals: ## Lint for root-relative load literals outside the boot closure
	sh tools/check/path-literals.sh
.PHONY: check-path-literals

# Amalgamated boot entries: each dialect's raw-include
# chain flattened into one self-ordered stream, plus one per app entry.
# Build products only -- never committed; regenerated on every call, so the
# amalgams cannot drift from the sources they are made of.
boot: ## Generate amalgamated boot entries into build/boot
	@mkdir -p build/boot
	@for e in x he xe rn x-base; do \
		sh tools/release/amalgamate.sh "lib/$$e.x" > "build/boot/$$e.x" || exit 1; done
	@for a in apps/*/run.x; do \
		n=$$(basename "$$(dirname "$$a")"); \
		sh tools/release/amalgamate.sh "$$a" > "build/boot/$$n.x" || exit 1; done
	@echo "boot: generated $$(ls build/boot | wc -l | tr -d ' ') entries"
.PHONY: boot

check-boot-amalgam: $(EXECUTABLE) boot ## Boot every amalgam in batch mode and pin a smoke expression
	sh tools/check/amalgam-smoke.sh
.PHONY: check-boot-amalgam

# THE TOP LEVEL IS SACRED (#108): the runtime library may bind only the names
# tools/contract/bare-globals.x sanctions; the manifest can only shrink.
check-bare-globals: ## Diff the runtime library's bare top-level defs against tools/contract/bare-globals.x
	sh tools/check/bare-globals.sh
.PHONY: check-bare-globals

# The %-global budget (the bare ratchet's closed exemption): per-file
# counts in tools/contract/percent-globals.x, shrink-only.
check-percent-globals: ## Diff every lib file's %-global count against its shrinking budget
	sh tools/check/percent-globals.sh
.PHONY: check-percent-globals

# The dialect coverage ratchet (#70): every lib/*.x entry point needs an
# end-to-end smoke group, so a new dialect cannot ship untested the way the
# tower launchers did (#49 -- both crashed at the exact invocation the README
# documents, while every numeric spec passed against a bespoke harness).
check-dialect-cover: $(EXECUTABLE) ## Assert every lib/*.x dialect has an end-to-end smoke group
	sh x.sh --no-pin -q -f tools/check/dialect-cover.x
.PHONY: check-dialect-cover

# spec.md's worked examples, extracted and executed -- the ratchet that keeps
# the normative spec honest (#70 seam 2, PROMOTED at 356/356 green).  It began
# report-only at 86 failures; the drift burned down through #72 #73 #76 the
# #31 order sweep, the regex-escaping fix, the reader-section repairs and
# depth-tracked quasi (#55), and on the stated criterion -- red would rot it
# like lint-x (#60) -- it joined `test` only once fully green.  A spec.md
# example that stops reproducing now fails the build with a file:line name.
spec-examples: $(EXECUTABLE) ## Run docs/spec.md's examples (gate: spec.md cannot drift silently)
	sh tools/check/spec-examples.sh
	sh tests/x/spec-example-runner.sh
.PHONY: spec-examples

check-doc-vocab: ## Lint doc forms for banned type-token aliases + retired names
	@if grep -rn 'INTEGER\|BOOLEAN\|FUNCTION' lib --include='*.x' \
		| grep '(param \|(returns '; then \
		echo "doc-vocab: FAIL (use INT/BOOL/CALLABLE; see contributing.md)" >&2; \
		exit 1; \
	else echo "doc-vocab: ok"; fi
	@# Retired/banned names from the #42/#44 adjudications (see contributing.md's
	@# adjudication block): shapes ride the name, synonyms stay dead.
	@if grep -rnw 'from-pairs\|->pairs' lib --include='*.x' \
		|| grep -rn '(method nth \|(method member? \|(method every? \|(method size ' lib --include='*.x'; then \
		echo "doc-vocab: FAIL (retired name; see contributing.md adjudications)" >&2; \
		exit 1; \
	else echo "retired-names: ok"; fi
	@# Retired dialect spellings (#95): the noble-gas names (he/xe/rn,
	@# modules x/xe, x/rn) replaced x-and/x-or; the compat shims are gone.
	@if grep -rnw 'x-and\|x-or\|x/and\|x/or' lib --include='*.x'; then \
		echo "doc-vocab: FAIL (retired dialect spelling; use he/xe/rn -- #95)" >&2; \
		exit 1; \
	else echo "retired-dialects: ok"; fi
	@# The quote-idiom ratchet (#45 R2/R8, added at the 2026-07-18 reopen):
	@# user-facing doc STRINGS -- (example ...), (sample ...), (note ...) --
	@# speak 'x, never the longhand (lit x), even inside boot-constrained
	@# files (strings never hit the boot reader).  Allowlist: doc-prims.x's
	@# definitional docs for lit itself.
	@if grep -rn '(example "\|(sample "\|(note "' lib --include='*.x' \
		| grep -v 'lib/x/doc/doc-prims\.x' \
		| grep '(lit '; then \
		echo "doc-vocab: FAIL (doc strings speak 'x, not (lit x); #45 R2/R8)" >&2; \
		exit 1; \
	else echo "doc-string-quotes: ok"; fi
.PHONY: check-doc-vocab

# Memory-safety gate: run BOTH suites against an AddressSanitizer build (reuses
# the x-asan target). Catches the crash class we keep hitting -- e.g. an
# unchecked `first` reading past a non-pair, which is silently wrong on 64-bit
# but SIGSEGVs on 32-bit/Pi -- on the dev box, before a Pi run surfaces it.
#   - address only: UBSan is deferred until its baseline noise on the C89
#     stack-pair pointer tricks is assessed (it would flag intentional UB).
#   - detect_leaks=0: the interpreter is a GC that does not free at exit, so
#     LeakSanitizer reports are not bugs.
#   - detect_stack_use_after_return=0: stack-copying call/cc cannot coexist
#     with ASan's fake stack -- intermediate frames' locals live in heap-side
#     fake frames that are recycled on return, so a continuation reinvoked
#     later restores real-stack bytes pointing at dead fake frames (the same
#     limitation every fiber/coroutine library documents). Off on some
#     arch/compiler defaults already; pinned off so behavior matches.
#   - WRAPPER= disables the C runner's valgrind auto-wrap (ASan != valgrind).
#   - TIMEOUT_UNIT_SECS raised: instrumentation slows each spec ~2-3x.
ASAN_RUN_OPTIONS=detect_leaks=0:detect_stack_use_after_return=0
test-asan: x-bin-asan ## Run both suites under AddressSanitizer (memory-safety gate)
	ASAN_OPTIONS=$(ASAN_RUN_OPTIONS) TIMEOUT_UNIT_SECS=180 X_BIN=./x-bin-asan sh tests/x/spec-runner.sh
	ASAN_OPTIONS=$(ASAN_RUN_OPTIONS) WRAPPER= CFLAGS="$(TEST_CFLAGS) -fsanitize=address -fno-omit-frame-pointer" sh $(PATH_TESTS_C)/test-runner/test-runner.sh $(TESTS)
.PHONY: test-asan

# Install the local pre-push gate (first line of defence before the Actions
# CI gates). Points git at the tracked .githooks/ dir so `make test` runs
# before every push. RUN_ASAN=1 in the environment also runs `make test-asan`
# as a non-blocking advisory.
install-hooks: ## Install the pre-push test gate (core.hooksPath=.githooks)
	git config core.hooksPath .githooks
	@chmod +x .githooks/* 2>/dev/null || true
	@echo "pre-push gate active (core.hooksPath=.githooks). Bypass: git push --no-verify. Uninstall: git config --unset core.hooksPath."
.PHONY: install-hooks

# ============================================================================
# Coverage
# ============================================================================

test-c-cov: cov-clean ## C tests with coverage
	COVERAGE_DIR=$(COVERAGE_DIR) CFLAGS="$(TEST_CFLAGS)" sh $(PATH_TESTS_C)/test-runner/test-runner-coverage.sh $(TESTS)
.PHONY: test-c-cov

test-x-cov: cov-clean $(EXECUTABLE) ## x-lang tests with coverage
	$(MAKE) clean
	CFLAGS="-Og --coverage" $(MAKE) $(EXECUTABLE)
	sh tests/x/spec-runner.sh
	mkdir -p $(COVERAGE_DIR)
	gcovr -r . --filter 'src/' --print-summary --html-details $(COVERAGE_DIR)/index.html
.PHONY: test-x-cov

test-cov: cov-clean ## All tests with coverage
	$(MAKE) clean
	CFLAGS="-Og --coverage" $(MAKE) $(EXECUTABLE)
	sh tests/x/spec-runner.sh
	CFLAGS="$(TEST_CFLAGS) -Og --coverage" RUNNER=command sh $(PATH_TESTS_C)/test-runner/test-runner.sh $(TESTS)
	mkdir -p $(COVERAGE_DIR)
	gcovr -r . --filter 'src/' --print-summary --html-details $(COVERAGE_DIR)/index.html
.PHONY: test-cov

cov-clean: ## Clean coverage artifacts
	rm -rf $(COVERAGE_DIR)
	find . -name '*.gcov' -o -name '*.gcda' -o -name '*.gcno' | xargs rm -f
.PHONY: cov-clean

# ============================================================================
# Performance
# ============================================================================

bench: x-bin-profile ## Run benchmarks
	sh tools/dev/bench.sh --no-build

cov-x: x-bin-profile ## x-lang library coverage report
	sh tools/dev/cov-lib.sh
.PHONY: bench

# ============================================================================
# Dev tools
# ============================================================================

defs: ## Generate ctags definitions
	ctags -f - src/**/*.c | awk 'BEGIN {FS = "\t"} /\/.*\$\/;"/ { printf("%s;\n", substr($$3,3,length($$3)-6)) }' | sort -u > defs

# The base-object layout -- the x_eval_field_* accessors and x_eval_make's
# construction skeleton -- is generated from the descriptor tools/contract/base-layout.x.
# include/x-eval-layout.h is committed so a plain checkout builds without awk;
# after editing the descriptor run `make gen-layout`, then `make clean && make`
# (header changes don't trigger object rebuilds on their own here).
$(INCDIR)/x-eval-layout.h: tools/contract/base-layout.x tools/contract/gen-base-layout.awk
	awk -f tools/contract/gen-base-layout.awk $< > $@

gen-layout: $(INCDIR)/x-eval-layout.h ## Regenerate the base-object layout header from the descriptor
.PHONY: gen-layout

lint: ## Lint C sources
	$(CC) -fsyntax-only $(CFLAGS) -g -Wall -pedantic $(SOURCES)
.PHONY: lint

# Promoted into `test` 2026-08-02 on the #60 criterion (red would rot it,
# so it joined only once fully green): lib AND apps both sweep clean since
# the sibling-preload / value-call linter round (#176).
lint-x: $(EXECUTABLE) ## Lint x-lang files
	PARALLEL=1 sh tools/dev/lint.sh
.PHONY: lint-x

fmt-x: $(EXECUTABLE) ## Format x-lang files
	@for f in lib/x-core.x lib/x/*.x; do \
		sh x.sh --no-pin -q -f tools/dev/fmt.x -- "$$f" > "$$f.fmt.tmp" \
			&& mv "$$f.fmt.tmp" "$$f" && printf '  \033[1;32m.\033[0m %s\n' "$$f" \
			|| { rm -f "$$f.fmt.tmp"; exit 1; }; \
	done
.PHONY: fmt-x

fmt-check-x: $(EXECUTABLE) ## Check x-lang formatting
	@FAIL=0; for f in lib/x-core.x lib/x/*.x; do \
		if [ "$$(sh x.sh --no-pin -q -f tools/dev/fmt.x -- "$$f" 2>/dev/null)" = "$$(cat "$$f")" ]; then \
			printf '  \033[1;32m.\033[0m %s\n' "$$f"; \
		else \
			FAIL=1; printf '  \033[1;31mF\033[0m %s\n' "$$f"; \
		fi; \
	done; [ "$$FAIL" -eq 0 ]
.PHONY: fmt-check-x

doc-c: ## Generate C reference documentation (HTML + man pages)
	doxygen Doxyfile
.PHONY: doc-c

# No stderr masking and fail on error/empty output: a 2>/dev/null here once
# hid a retired-constructor crash for weeks -- 77 of 79 ref files were 0
# bytes while the target reported success.
doc-x: $(EXECUTABLE) ## Generate x-lang documentation
	@mkdir -p docs/ref/x/boot docs/ref/x/core docs/ref/x/type \
		docs/ref/x/sys docs/ref/x/num docs/ref/x/doc docs/ref/x/tool \
		docs/ref/x/platform
	@for f in lib/x-core.x lib/x/*.x lib/x/**/*.x; do \
		rel=$$(echo "$$f" | sed 's|^lib/x/||; s|^lib/||; s|\.x$$||'); \
		out="docs/ref/x/$${rel}.md"; \
		mkdir -p "$$(dirname $$out)"; \
		sh x.sh --no-pin -q -f tools/dev/doc.x -- "$$f" > "$$out" || { \
			printf '  \033[1;31mFAIL\033[0m %s\n' "$$f"; exit 1; }; \
		if [ ! -s "$$out" ]; then \
			if grep -q '(doc (provide' "$$f"; then \
				printf '  \033[1;31mEMPTY\033[0m %s\n' "$$out"; exit 1; \
			else \
				rm -f "$$out"; \
				printf '  skip %s (no doc-provide)\n' "$$f"; continue; \
			fi; \
		fi; \
		printf '  %s\n' "$$out"; \
	done
	@sh x.sh --no-pin -q -f tools/dev/doc-index.x > docs/ref/x/index.md
	@printf '  %s\n' "docs/ref/x/index.md"
.PHONY: doc-x

doc: doc-c doc-x ## Generate all documentation
.PHONY: doc

valgrind: ## Run Valgrind
	$(CC) $(CFLAGS) -g -Wall $(SOURCES) && valgrind -v --leak-check=full ./a.out && rm a.out
.PHONY: valgrind

watch: ## Watch for changes
	while true; do \
		fswatch -o --event Created --event Updated --event MovedTo $(HEADERS) $(SOURCES) tests/c | \
		make debug && make test-c; \
	done
.PHONY: watch

# The installed library is BYTE-IDENTICAL to the repo's:
# lib/ and apps/ copy verbatim -- diff -r inside the recipe is the proof, and
# it fails the install if anything diverges.  Only the boot/ entries are
# generated (the amalgams; build products, like the binary itself).  The
# import root reaches the installed tree as data: the wrapper emits one
# (def %install-root ...) form at the top of the pipe (see x.sh + module.x).
install: $(EXECUTABLE) $(NAME).sh boot ## Install to PREFIX (DESTDIR honoured)
	install -d -m 0755 $(DESTDIR)$(BINDIR) $(DESTDIR)$(LIBEXECDIR) $(DESTDIR)$(LIBDIR)
	install $C -m 0755 $(EXECUTABLE) $(DESTDIR)$(LIBEXECDIR)/$(EXECUTABLE)
	# strip -x, NOT bare strip: the JIT resolves its runtime helpers
	# (jit_mkint, jit_atomint, ...) through dlsym on the engine itself,
	# and those live in exports.sym.  Bare strip removes the exported
	# symbol table, every dlsym then answers nil, and compiled code
	# called address 0 -- a SIGSEGV inside jit_atomint on every INSTALLED
	# engine while the repo build (which already used -x, line ~158) was
	# clean (x-lang#201).  Release tarballs come from this same target.
	strip -x $(DESTDIR)$(LIBEXECDIR)/$(EXECUTABLE)
	@if [ -f entitlements.plist ]; then codesign -s - --entitlements entitlements.plist -f $(DESTDIR)$(LIBEXECDIR)/$(EXECUTABLE) 2>/dev/null || true; fi
	install $C -m 0755 $(NAME).sh $(DESTDIR)$(BINDIR)/$(NAME)
	rm -rf $(DESTDIR)$(LIBDIR)/lib $(DESTDIR)$(LIBDIR)/apps $(DESTDIR)$(LIBDIR)/boot
	# The engine's ISA fingerprint travels WITH the engine.  An installed
	# tree has no source checkout, so before this there was no way to ask
	# "which engine contract is this?" -- fetch could not compare (#186)
	# and a pinned boot could not refuse a mismatched amalgam (#187); it
	# just ran it and segfaulted.  One precomputed hex line: the wrapper
	# needs a STRING compare against a release manifest, not a digester.
	install -d -m 0755 $(DESTDIR)$(LIBDIR)/contract
	@sh -c 'if command -v shasum >/dev/null 2>&1; then shasum -a 256 tools/contract/isa.x | cut -d" " -f1; 		else sha256sum tools/contract/isa.x | cut -d" " -f1; fi' 		> $(DESTDIR)$(LIBDIR)/contract/isa.sha256
	cp -R lib $(DESTDIR)$(LIBDIR)/lib
	cp -R apps $(DESTDIR)$(LIBDIR)/apps
	cp -R build/boot $(DESTDIR)$(LIBDIR)/boot
	diff -r lib $(DESTDIR)$(LIBDIR)/lib
	diff -r apps $(DESTDIR)$(LIBDIR)/apps
	diff -r build/boot $(DESTDIR)$(LIBDIR)/boot
.PHONY: install

uninstall: ## Uninstall from PREFIX
	rm -rf $(DESTDIR)$(LIBDIR)
	rm -rf $(DESTDIR)$(LIBEXECDIR)
	rm -f $(DESTDIR)$(BINDIR)/$(NAME)
.PHONY: uninstall

clean: cov-clean ## Clean build artifacts
	rm -f $(EXECUTABLE) x-bin-debug x-bin-profile x-bin-asan x-bin-cov *.out $(SRCDIR)/*.o $(SRCDIR)/**/*.o $(SRCDIR)/**/**/*.o $(OPTDIR)/**/*.o $(X_EXPR_DIR)/src/*.o *.core core
	@# Pre-rename binary names (engine was `x` until the x-bin rename): a
	@# checkout that built before the rename has stale copies at the root.
	rm -f x x-debug x-profile x-asan x-cov
.PHONY: clean

help: ## Show targets
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z0-9_-]+:.*?## / {printf "\033[32m%-38s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)
.PHONY: help
