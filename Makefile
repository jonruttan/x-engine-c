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
CFLAGS+=-DX_HEAP -DX_TYPE -DX_CLOCK

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

# x-expr foundation library
X_EXPR_DIR=ext/x-expr
X_EXPR_SOURCES=$(wildcard $(X_EXPR_DIR)/src/*.c)
X_EXPR_OBJECTS=$(X_EXPR_SOURCES:.c=.o)

CFLAGS+=-I$(X_EXPR_DIR)/include -I$(INCDIR)

HEADERS=$(wildcard $(INCDIR)/*.h $(INCDIR)/**/*.h $(INCDIR)/**/**/*.h $(X_EXPR_DIR)/include/*.h)
SOURCES=$(wildcard $(SRCDIR)/*.c $(SRCDIR)/**/*.c $(SRCDIR)/**/**/*.c)
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=x
OUTPUT=$(EXECUTABLE)

# Options to be added to $(DEFS)
DEFS?=$(OSDEF) -DX_MACHINE="$(X_MACHINE)" -DX_SYSCALL -DX_INCLUDE -DSYMBOL_FIND_REORDER
EXTRA_LIBS+=-ldl

# Where to install the stuff
BINDIR?=$(PREFIX)/bin
LIBDIR?=$(PREFIX)/share/$(EXECUTABLE)
MANDIR?=$(PREFIX)/man/man1

# Set up environment to be used during the build process
BUILD_ENV=env X_LIBRARY_PATH=.:lib:ext:contrib

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

x-debug: ## Build debug target
	$(MAKE) clean-obj
	$(MAKE) OUTPUT=$@ CFLAGS="$(CFLAGS) -g -Og -DDEBUG" $(EXECUTABLE)

x-profile: ## Build profiling binary (includes coverage)
	$(MAKE) clean-obj
	$(MAKE) OUTPUT=$@ CFLAGS="$(CFLAGS) -DX_PROFILE -DX_COV" $(EXECUTABLE)

clean-obj:
	rm -f $(SRCDIR)/*.o $(SRCDIR)/**/*.o $(SRCDIR)/**/**/*.o $(X_EXPR_DIR)/src/*.o

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

test: test-c test-x ## Run all tests
.PHONY: test

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

bench: x-profile ## Run benchmarks
	sh tools/bench.sh --no-build

cov-x: x-profile ## x-lang library coverage report
	sh tools/cov-lib.sh
.PHONY: bench

# ============================================================================
# Dev tools
# ============================================================================

defs: ## Generate ctags definitions
	ctags -f - src/**/*.c | awk 'BEGIN {FS = "\t"} /\/.*\$\/;"/ { printf("%s;\n", substr($$3,3,length($$3)-6)) }' | sort -u > defs

lint: ## Lint C sources
	$(CC) -fsyntax-only $(CFLAGS) -g -Wall -pedantic $(SOURCES)
.PHONY: lint

lint-x: $(EXECUTABLE) ## Lint x-lang files
	sh tools/lint.sh
.PHONY: lint-x

fmt-x: $(EXECUTABLE) ## Format x-lang files
	@for f in lib/x-core.x lib/x/*.x; do \
		sh tools/fmt.sh -i "$$f" && printf '  \033[1;32m.\033[0m %s\n' "$$f"; \
	done
.PHONY: fmt-x

fmt-check-x: $(EXECUTABLE) ## Check x-lang formatting
	@FAIL=0; for f in lib/x-core.x lib/x/*.x; do \
		if sh tools/fmt.sh --check "$$f" 2>/dev/null; then \
			printf '  \033[1;32m.\033[0m %s\n' "$$f"; \
		else \
			FAIL=1; printf '  \033[1;31mF\033[0m %s\n' "$$f"; \
		fi; \
	done; [ "$$FAIL" -eq 0 ]
.PHONY: fmt-check-x

doc-c: ## Generate C reference documentation (HTML + man pages)
	/opt/homebrew/bin/doxygen Doxyfile
.PHONY: doc-c

doc-x: $(EXECUTABLE) ## Generate x-lang documentation
	@mkdir -p docs/ref/x/boot docs/ref/x/core docs/ref/x/type \
		docs/ref/x/sys docs/ref/x/num docs/ref/x/doc docs/ref/x/tool \
		docs/ref/x/platform
	@for f in lib/x-core.x lib/x/*.x lib/x/**/*.x; do \
		rel=$$(echo "$$f" | sed 's|^lib/x/||; s|^lib/||; s|\.x$$||'); \
		out="docs/ref/x/$${rel}.md"; \
		mkdir -p "$$(dirname $$out)"; \
		sh tools/doc.sh "$$f" > "$$out" 2>/dev/null; \
		printf '  %s\n' "$$out"; \
	done
	@sh tools/doc-index.sh > docs/ref/x/index.md 2>/dev/null; \
		printf '  %s\n' "docs/ref/x/index.md"
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

install: $(EXECUTABLE) lib/$(EXECUTABLE).x $(EXECUTABLE).sh ## Install to PREFIX
	install -d -m 0755 $(BINDIR)
	install -d -m 0755 $(LIBDIR)/lib
	install $C -m 0755 $(EXECUTABLE) $(BINDIR)
	install $C -m 0755 $(EXECUTABLE).sh $(BINDIR)
	strip $(BINDIR)/$(EXECUTABLE)
	install $C -m 0644 lib/* $(LIBDIR)/lib
.PHONY: install

uninstall: ## Uninstall from PREFIX
	rm -f $(LIBDIR)/* && rmdir $(LIBDIR)
	rm -f $(BINDIR)/$(EXECUTABLE)
	rm -f $(BINDIR)/$(EXECUTABLE).sh
.PHONY: uninstall

clean: cov-clean ## Clean build artifacts
	rm -f $(EXECUTABLE) x-debug x-profile *.out $(SRCDIR)/*.o $(SRCDIR)/**/*.o $(SRCDIR)/**/**/*.o $(X_EXPR_DIR)/src/*.o *.core core
.PHONY: clean

help: ## Show targets
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z0-9_-]+:.*?## / {printf "\033[32m%-38s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)
.PHONY: help
