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

# The engine's RELEASE IDENTITY (#435).  Derived, never written down: a tag
# build reports the tag, any other build reports its distance from one, and
# a dirty tree says so.  Overridable, and the release pipeline does override
# it -- tools/release/package.sh passes the tag it was handed, so a published
# tarball's engine is stamped with exactly the tag it ships under instead of
# whatever a shallow checkout's `git describe` could reconstruct.
#
# This is what the ISA fingerprint could never be.  isa.x is byte-identical
# across v0.3.1-rc10, v0.4.0 and v0.5.0 -- correctly, it is the C surface --
# so an amalgam from one release booted on another's engine passed the
# pairing guard and segfaulted.  The tag is the one key that cannot silently
# under-approximate, so the engine now carries it.
#
# WHOSE tag, after the repo split: the LANGUAGE's, not this repo's.  The pin
# lock records the x-lang release, and lib/x/tool/pin.x REFUSES at boot when a
# pinned amalgam's release differs from the engine's -- so an engine stamped
# with x-engine-c's own `git describe` would fail every pinned project on sight.
# x-lang's Makefile therefore passes ITS `git describe` down when it builds
# this repo as a submodule, exactly as tools/release/package.sh already does
# for a tarball.  The default below is only what a STANDALONE build of this
# repo reports, where there is no library to pair with and nothing to refuse.
X_RELEASE?=$(shell git describe --tags --always --dirty 2>/dev/null || echo dev)

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
# Per-configuration object extension (#329).  The plain build owns .o;
# each variant compiles to its own suffix (.cov.o, .asan.o, ...) beside
# it, so no two configurations ever share an object path -- the old
# clean-obj brackets (which deleted EVERY object on both sides of a
# variant build, forcing the next plain-build-dependent target to
# silently re-pay the whole C compile) are gone, and variants rebuild
# incrementally like the plain build always has.
OBJ_EXT?=.o
X_EXPR_OBJECTS=$(X_EXPR_SOURCES:.c=$(OBJ_EXT))

CFLAGS+=-I$(X_EXPR_DIR)/include -I$(INCDIR)

HEADERS=$(wildcard $(INCDIR)/*.h $(INCDIR)/**/*.h $(INCDIR)/**/**/*.h $(X_EXPR_DIR)/include/*.h)
SOURCES=$(wildcard $(SRCDIR)/*.c $(SRCDIR)/**/*.c $(SRCDIR)/**/**/*.c)
OBJECTS=$(SOURCES:.c=$(OBJ_EXT))
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
# command).  MANDIR is the man HIERARCHY ROOT, not a section dir: the pages
# `make install-man` ships are Doxygen's C reference (section 3), and its
# alias pages are `.so man3/<page>` -- a source directive resolved against
# the hierarchy root, so it only works if the pages land in $(MANDIR)/man3.
BINDIR?=$(PREFIX)/bin
LIBDIR?=$(PREFIX)/share/$(NAME)
LIBEXECDIR?=$(PREFIX)/libexec/$(NAME)
MANDIR?=$(PREFIX)/share/man

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

all: $(SOURCES) $(EXECUTABLE) x-engine-build.xon ## Build all

# THE BUILD'S OWN FACTS.  x-engine.xon is generated from SOURCE and carries no
# (param ...) rows on purpose -- word size, byte order and architecture are facts
# of a BINARY, and a 32-bit and a 64-bit build of this tree differ there and
# nowhere else in the contract.  This is the other half, regenerated whenever the
# engine relinks, installed beside it, and read by x-lang instead of inferred from
# a triple at runtime.
#
# The compiler answers, not uname: a cross-compiled engine must report its TARGET,
# and __SIZEOF_POINTER__ / __BYTE_ORDER__ come from the compiler producing this
# binary.  A fact that cannot be established is recorded as `unknown` rather than
# guessed -- x-lang would believe a wrong row.
x-engine-build.xon: $(EXECUTABLE) tools/contract/gen-build-params.sh
	@X_RELEASE="$(X_RELEASE)" sh tools/contract/gen-build-params.sh $(X_MACHINE) "$(CC)" > $@

# Stamp-gated (#367): the bare `strip` recipe ran on EVERY make,
# mutating the binary (strip + macOS ad-hoc re-codesign) even when
# nothing had rebuilt -- churning the content hash that #325's
# boot-order verdict cache keys on, and making "did that target dirty
# the tree?" undiagnosable by a follow-up make.  The stamp is touched
# AFTER the strip mutates the binary, so it lands newer and the next
# make no-ops; a real relink leaves the binary newer than the stamp and
# re-strips exactly once.  `make strip` stays the manual entry.
strip: build/.strip-stamp ## Strip non-global symbols (keep dynamic exports for dlopen)
.PHONY: strip

build/.strip-stamp: $(EXECUTABLE)
	strip -x $(EXECUTABLE)
	@if [ -f entitlements.plist ]; then codesign -s - --entitlements entitlements.plist -f $(EXECUTABLE) 2>/dev/null || true; fi
	@mkdir -p build && touch $@

# Keyed on $(OUTPUT), not $(EXECUTABLE): a variant recursion passes
# OUTPUT=x-bin-<variant> and names ITSELF as the goal, so make checks
# the variant binary's staleness against the variant's own objects.
# (Under the old shared-object scheme the inner goal was the plain
# binary and only linked because clean-obj had just made every object
# newer than it.)
$(OUTPUT): $(OBJECTS) $(X_EXPR_OBJECTS) $(EXTRA_OBJS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(X_EXPR_OBJECTS) $(EXTRA_OBJS) $(EXTRA_LIBS) -o $(OUTPUT)

# Variant builds (debug / profile / cov / asan): each compiles to its
# own object suffix (#329), so nothing collides with the plain build's
# .o and nothing needs the old clean-obj brackets.  Every variant is
# PHONY -- the recursion is the freshness check: the inner make no-ops
# in milliseconds when the variant binary is newer than its own
# objects, and rebuilds exactly the stale ones otherwise (the coverage
# binary used to go STALE after its first build instead, because the
# bracket dance made rebuilding cost the whole tree twice).
#
# Defined ONLY in the plain (.o) universe: the inner make's goal IS the
# variant's filename, which must resolve to the $(OUTPUT) link rule --
# were this phony also defined there, it would override that rule
# (later definition wins) and recurse forever.
ifeq ($(OBJ_EXT),.o)
x-bin-debug: ## Build debug target
	$(MAKE) OUTPUT=$@ OBJ_EXT=.debug.o CFLAGS="$(CFLAGS) -g -Og -DDEBUG" $@
.PHONY: x-bin-debug

x-bin-profile: ## Build profiling binary (includes coverage)
	$(MAKE) OUTPUT=$@ OBJ_EXT=.profile.o CFLAGS="$(CFLAGS) -DX_PROFILE -DX_COV" $@
.PHONY: x-bin-profile

# ASan flags go in CFLAGS only: 'LDFLAGS?=$(CFLAGS)' (above) carries them into
# the link too, so the runtime links while KEEPING the project's -dead_strip /
# exports.sym LDFLAGS (passing LDFLAGS on the command line would lose those).
x-bin-asan: ## Build with AddressSanitizer for memory-safety testing
	$(MAKE) OUTPUT=$@ OBJ_EXT=.asan.o CFLAGS="$(CFLAGS) -fsanitize=address -fno-omit-frame-pointer -g" $@
.PHONY: x-bin-asan

x-bin-cov: ## Build the coverage binary (-DX_COV flag-marking eval)
	$(MAKE) OUTPUT=$@ OBJ_EXT=.cov.o CFLAGS="$(CFLAGS) -DX_COV" $@
.PHONY: x-bin-cov
endif

# The *.o globs also catch every variant suffix (x.cov.o ends in .o).
clean-obj:
	rm -f $(SRCDIR)/*.o $(SRCDIR)/**/*.o $(SRCDIR)/**/**/*.o $(OPTDIR)/**/*.o $(X_EXPR_DIR)/src/*.o

%$(OBJ_EXT): %.c
	$(CC) -c $(CFLAGS) $(DEFS) -o $@ $<

# ============================================================================
# Distribute
# ============================================================================

# THE ENGINE AS AN ARTIFACT.  x-lang consumes an engine as a DIRECTORY: a
# checkout of this repo is one, an unpacked release tarball is the other, and
# X_ENGINE_DIR picks between them.  That only holds if the tarball carries
# every path a consumer reaches for, spelled the way a checkout spells it --
# then a checkout is a SUPERSET of a release and nothing downstream needs to
# know which one it has.  This target stages exactly that directory.
#
# WHAT GOES IN, and who asks for it:
#   x-bin                  the engine; x-lang copies it to its own root, where
#                          its spec runner derives the awk harness path from it
#   x-engine-build.xon     this BINARY's params -- word size, endian, os, arch
#   x-engine.xon           what this engine provides, and what it claims
#   tools/contract/*.x     x-lang's boot INCLUDES base-paths.x and obj-layout.x
#                          before anything else; pin.x reads isa.x; compliance
#                          falsifies claims.x; base-layout.x pairs with them
#   include/               the JIT compiles C against these headers AT RUNTIME
#   ext/x-expr/include/    (lib/x/tool/compile.x's -I flags name both dirs, so
#                          the nesting has to survive packaging)
#   entitlements.plist     macOS re-signs the engine after install
#   LICENSE                it travels with the bytes
#
# WHAT STAYS OUT: sources, tests, build tooling.  A consumer needing those
# needs a checkout, and saying so is more honest than a half-source tarball
# that looks buildable and is not.
#
# THE NAME COMES FROM THE BUILD PARAMS, NOT uname.  A cross-compiled engine
# must be named for its TARGET, and x-engine-build.xon is where the compiler
# already answered that -- naming it from the packaging host would put the
# wrong platform on the file every consumer selects by.
DIST_DIR?=build/dist
DIST_OS=$(shell sed -n 's/^(param os \(.*\))$$/\1/p' x-engine-build.xon)
DIST_ARCH=$(shell sed -n 's/^(param arch \(.*\))$$/\1/p' x-engine-build.xon)
DIST_NAME=x-engine-c-$(X_RELEASE)-$(DIST_OS)-$(DIST_ARCH)

# The manifest is a LIST, not a comment: the staging step copies these paths
# and the verify step re-reads the same list, so a file that stops being
# copied fails the target instead of shipping as a hole.  Consumers meet a
# missing contract file as a segfault in field access, which is the worst
# possible place to learn that packaging drifted.
DIST_REQUIRED=x-bin x-engine.xon x-engine-build.xon LICENSE \
	tools/contract/isa.x tools/contract/obj-layout.x \
	tools/contract/base-paths.x tools/contract/base-layout.x \
	tools/contract/claims.x \
	include/x-eval.h ext/x-expr/include/x-obj.h

dist: all strip ## Package this engine as a consumable artifact (tar.gz + sha256)
	@rm -rf $(DIST_DIR)/$(DIST_NAME)
	@mkdir -p $(DIST_DIR)/$(DIST_NAME)/tools/contract $(DIST_DIR)/$(DIST_NAME)/ext/x-expr
	@cp $(EXECUTABLE) x-engine.xon x-engine-build.xon LICENSE $(DIST_DIR)/$(DIST_NAME)/
	@cp tools/contract/isa.x tools/contract/obj-layout.x tools/contract/base-paths.x \
		tools/contract/base-layout.x tools/contract/claims.x \
		$(DIST_DIR)/$(DIST_NAME)/tools/contract/
	@cp -R $(INCDIR) $(DIST_DIR)/$(DIST_NAME)/include
	@cp -R $(X_EXPR_DIR)/include $(DIST_DIR)/$(DIST_NAME)/ext/x-expr/include
	@# Darwin-only, and absent elsewhere: copy it when it exists rather than
	@# failing a Linux package for a file Linux has no use for.
	@if [ -f entitlements.plist ]; then cp entitlements.plist $(DIST_DIR)/$(DIST_NAME)/; fi
	@missing=; for f in $(DIST_REQUIRED); do \
		[ -f "$(DIST_DIR)/$(DIST_NAME)/$$f" ] || missing="$$missing $$f"; \
	done; \
	if [ -n "$$missing" ]; then \
		echo "dist: FAIL -- staged tree is missing:$$missing" >&2; exit 1; \
	fi
	@tar -czf $(DIST_DIR)/$(DIST_NAME).tar.gz -C $(DIST_DIR) $(DIST_NAME)
	@# Digest from INSIDE the dist dir so the sidecar names the tarball by its
	@# basename -- `shasum -c` resolves the name it reads relative to itself,
	@# and a path-qualified line only verifies from one directory.
	@cd $(DIST_DIR) && { \
		if command -v shasum >/dev/null 2>&1; then shasum -a 256 $(DIST_NAME).tar.gz; \
		else sha256sum $(DIST_NAME).tar.gz; fi; } > $(DIST_NAME).tar.gz.sha256
	@echo "dist: $(DIST_DIR)/$(DIST_NAME).tar.gz"
	@cat $(DIST_DIR)/$(DIST_NAME).tar.gz.sha256
.PHONY: dist

# ============================================================================
# Test
# ============================================================================

test-c: ## Run C unit tests
	CFLAGS="$(TEST_CFLAGS)" RUNNER=command sh $(PATH_TESTS_C)/test-runner/test-runner.sh $(TESTS)
.PHONY: test-c

# The contract gates, ONE definition: `make test` runs them first and CI's
# "Contract gates" step runs exactly this target, so the two cannot drift.
#
# These three are SELF-CONTAINED: each scans this repo's C and diffs it
# against a committed manifest in tools/contract/, with no x-lang checkout
# anywhere in the loop.  That is the point of them living here -- the engine
# refuses its own drift instead of waiting to be told about it downstream.
#
# The manifests are ALSO the engine's published description of itself.
# x-lang's boot loads base-paths.x and obj-layout.x to learn this engine's
# field offsets, and pin.x reads isa.x to answer which C surface a tree
# carries -- so they ship with the engine rather than being copied into the
# library, which is what kept a library's private layout copy from silently
# disagreeing with the engine actually running it.
#
# NOT here: check-prim-coverage, which asks whether every primitive is
# EXERCISED by a spec.  Most primitives are reachable only through the
# library, so the honest answer needs both spec suites at once and only the
# repo holding both trees can ask it.  It runs in x-lang, over this
# submodule's sources and both suites.
gates: check-isa check-obj-layout check-base-paths ## Run the contract gates
.PHONY: gates

# The C-surface ratchet: every binding site in the C source must appear in
# the committed manifest tools/contract/isa.x, so growing the C layer takes a
# deliberate manifest edit in the same commit.  The runtime half -- a walk of
# the LIVE catalog -- is x-lang's tests/x/specs/meta/isa.spec.md.
check-isa: ## Diff the C source's binding surface against tools/contract/isa.x
	sh tools/check/isa.sh
.PHONY: check-isa

# The object-layout contract: the header-word layout parsed out of
# ext/x-expr/include/x-obj.h must match tools/contract/obj-layout.x, which
# reflective x-lang code reads its offsets from.  Runtime half:
# x-lang's tests/x/specs/meta/obj-layout.spec.md.
check-obj-layout: ## Diff x-obj.h's object layout against tools/contract/obj-layout.x
	sh tools/check/obj-layout.sh
.PHONY: check-obj-layout

# The base-paths contract: every base-field accessor macro (x-eval-layout.h,
# x-base.h, the error handler in x-eval.h) flattened to a first/rest path
# must match tools/contract/base-paths.x, which x-lang's reflect.x walks.
# Runtime half: x-lang's tests/x/specs/meta/base-paths.spec.md.
check-base-paths: ## Diff the base-field macro chains against tools/contract/base-paths.x
	sh tools/check/base-paths.sh
.PHONY: check-base-paths

# The bare harness: every case is one engine process fed the allocation
# guard and the source, with NO library on stdin.  Every other runner in
# the world cats a library ahead of the test, so what it measures is the
# library's surface -- for a primitive that is the wrong subject.  This is
# the only place the engine is asked to stand up unaided.
#
# NOT the cross-engine conformance suite: that defines what ANY evaluator
# must do, so it belongs with the language and lives in x-lang.  See the
# header of tests/bare/bare-runner.sh.
test-bare: $(EXECUTABLE) ## Run the bare-engine smoke specs (no library)
	sh tests/bare/bare-runner.sh
.PHONY: test-bare

test: gates test-c test-bare ## Run all tests
.PHONY: test

# Memory-safety gate: run the C suite against an AddressSanitizer build.
# Catches the crash class we keep hitting -- e.g. an unchecked `first`
# reading past a non-pair, which is silently wrong on 64-bit but SIGSEGVs
# on 32-bit/Pi -- on the dev box, before a Pi run surfaces it.  x-lang's
# repo runs the same ASan ENGINE against the x spec suite (its own
# `make test-asan`); this half is the C specs, which link the sources
# directly and need no engine binary at all.
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
ASAN_RUN_OPTIONS=detect_leaks=0:detect_stack_use_after_return=0
test-asan: ## Run the C suite under AddressSanitizer (memory-safety gate)
	ASAN_OPTIONS=$(ASAN_RUN_OPTIONS) WRAPPER= CFLAGS="$(TEST_CFLAGS) -fsanitize=address -fno-omit-frame-pointer" sh $(PATH_TESTS_C)/test-runner/test-runner.sh $(TESTS)
.PHONY: test-asan

# ============================================================================
# Coverage
# ============================================================================

test-c-cov: cov-clean ## C tests with coverage
	COVERAGE_DIR=$(COVERAGE_DIR) CFLAGS="$(TEST_CFLAGS)" sh $(PATH_TESTS_C)/test-runner/test-runner-coverage.sh $(TESTS)
.PHONY: test-c-cov

cov-clean: ## Clean coverage artifacts
	rm -rf $(COVERAGE_DIR)
	find . -name '*.gcov' -o -name '*.gcda' -o -name '*.gcno' | xargs rm -f
.PHONY: cov-clean

# ============================================================================
# Performance
# ============================================================================

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

# The release identity reaches the C as a GENERATED header rather than a
# -D on the command line, because nothing here tracks CFLAGS: a -D whose
# value changed would leave the stale x-cli.o in place and the engine would
# keep reporting the release it was first built under.  A header can be a
# prerequisite, and one is declared below.
#
# Rewritten only when the VALUE changes (the cmp guard), so an unchanged
# `git describe` costs one stat and rebuilds nothing -- without it every
# make would relink, since the rule itself must run every time to notice a
# new commit or tag.  Generated, so it is not committed: a checked-in copy
# is stale the moment anything is committed.
$(INCDIR)/x-release.h: FORCE
	@tmp=$@.$$$$.tmp; \
		printf '/* GENERATED by the Makefile -- DO NOT EDIT, DO NOT COMMIT. */\n#define X_RELEASE "%s"\n' "$(X_RELEASE)" > $$tmp; \
		if cmp -s $$tmp $@ 2>/dev/null; then rm -f $$tmp; else mv $$tmp $@; fi

# The one object that reads it.  (No object here depends on any header --
# see the note above gen-layout -- so this dependency is spelled out.)
$(SRCDIR)/x-cli$(OBJ_EXT): $(INCDIR)/x-release.h

FORCE:
.PHONY: FORCE

lint: ## Lint C sources
	$(CC) -fsyntax-only $(CFLAGS) -g -Wall -pedantic $(SOURCES)
.PHONY: lint

# docs/ref/c/ is gitignored, so a fresh clone does not have it, and Doxygen
# refuses to create its own OUTPUT_DIRECTORY (docs/ref/README.md quotes the
# error).  The target makes its own output root rather than leaving that to
# whoever cloned -- CI runs this on a fresh checkout.
doc-c: ## Generate C reference documentation (HTML + man pages)
	@mkdir -p docs/ref/c
	X_RELEASE="$(X_RELEASE)" doxygen Doxyfile
.PHONY: doc-c

doc: doc-c ## Generate all documentation
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

# Doxygen's C reference as man pages, deliberately NOT wired into `install`
# and `uninstall`: they are a separate, opt-in pair.  Three reasons.  The
# pages are a build product of doc-c, so `install` would grow a hard Doxygen
# dependency it does not have today.  They are not part of the x-lang tree
# whose bytes contract/payload.sha256 attests, and `install` writes that
# digest AFTER its copies precisely so it describes the shipped payload --
# man pages under MANDIR are outside $(LIBDIR) and would not belong to it.
# And MANDIR is a SHARED hierarchy: an unconditional install would make
# `make install` scatter files outside the three dirs `uninstall` owns.
#
# Two classes of page come out of doc-c and both must ship:
#
#   real pages    112 of them -- one per file, group and struct.
#   alias pages   964 one-line `.so man3/<real>.3` stubs, one per documented
#                 entity (MAN_LINKS=YES in the Doxyfile).  Without these
#                 `man x_lib_strlen` finds nothing, because the symbol is
#                 documented INSIDE its file's page.  Their `.so` argument
#                 is resolved against the man hierarchy ROOT, which is why
#                 MANDIR is that root and the pages land in its man3/.
#
# What does NOT ship: Doxygen's 18 directory pages.  It builds those file
# names from the ABSOLUTE build path (`_Users_jon_Workspace_x_src_.3`), and
# STRIP_FROM_PATH does not reach them -- it rewrites titles, not man file
# names (A/B tested; blank and `.` produce identical output).  Installing
# them would publish this checkout's path into a shared man tree, and they
# hold nothing but a subdirectory listing.  The filter keys on the page
# TITLE, not the file name, so it stays right on whatever box ran Doxygen.
MANSRC=docs/ref/c/man/man3

# The C reference needs Doxygen; x-lang's library reference needs only the
# engine, and it ships from the x-lang repo (`make install-man-x` there).
# Keeping them apart is what lets someone install the library pages without
# a Doxygen dependency -- and lets this repo ship its half with no x-lang
# checkout at all.
install-man-c: doc-c ## Install the C reference man pages (section 3, needs Doxygen)
	install -d -m 0755 $(DESTDIR)$(MANDIR)/man3
	@n=0; \
	for page in $(MANSRC)/*.3; do \
		if head -n 1 "$$page" | grep -q 'Directory Reference" 3'; then \
			continue; \
		fi; \
		install -m 0644 "$$page" $(DESTDIR)$(MANDIR)/man3/ || exit 1; \
		n=`expr $$n + 1`; \
	done; \
	echo "  $$n C reference pages -> $(DESTDIR)$(MANDIR)/man3"
.PHONY: install-man-c

install-man: install-man-c ## Alias for install-man-c (this repo ships only the C half)
.PHONY: install-man

# Removal is BY NAME, from the same generated tree install-man read, so it
# needs doc-c output to exist -- it says so rather than silently removing
# nothing.  Note the shared-hierarchy hazard this inherits: Doxygen names
# the real pages after their source files (`atom.c.3`, `buffer.h.3`), so if
# another package owns a page of the same name, install-man overwrote it and
# this removes it.  Point MANDIR at a private tree (and add it to MANPATH)
# to keep both sides out of the shared namespace:
#
#   make install-man MANDIR=$(PREFIX)/share/$(NAME)/man
#
uninstall-man-c: ## Remove the C reference man pages from MANDIR
	@if [ ! -d $(MANSRC) ]; then \
		echo "uninstall-man-c reads $(MANSRC) to know what install-man-c shipped; run 'make doc-c' first" >&2; \
		exit 1; \
	fi
	@n=0; \
	for page in $(MANSRC)/*.3; do \
		installed=$(DESTDIR)$(MANDIR)/man3/`basename "$$page"`; \
		if [ -f "$$installed" ]; then \
			rm -f "$$installed" || exit 1; \
			n=`expr $$n + 1`; \
		fi; \
	done; \
	echo "  removed $$n C reference pages from $(DESTDIR)$(MANDIR)/man3"
.PHONY: uninstall-man-c

uninstall-man: uninstall-man-c ## Alias for uninstall-man-c
.PHONY: uninstall-man

clean: cov-clean ## Clean build artifacts
	rm -f $(EXECUTABLE) x-bin-debug x-bin-profile x-bin-asan x-bin-cov build/.strip-stamp *.out $(SRCDIR)/*.o $(SRCDIR)/**/*.o $(SRCDIR)/**/**/*.o $(OPTDIR)/**/*.o $(X_EXPR_DIR)/src/*.o *.core core
	@# Pre-rename binary names (engine was `x` until the x-bin rename): a
	@# checkout that built before the rename has stale copies at the root.
	rm -f x x-debug x-profile x-asan x-cov
.PHONY: clean

help: ## Show targets
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z0-9_-]+:.*?## / {printf "\033[32m%-38s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)
.PHONY: help
