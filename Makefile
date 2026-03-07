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
CFLAGS+=-Wall -Wextra -Wno-unused-parameter
CFLAGS+=-DX_GC -DX_TYPE

# Smaller, faster, flakier?
# CFLAGS+=-Wl,--gc-sections -fdata-sections -fno-stack-protector -Wa,--noexecstack -fno-builtin -fno-unwind-tables -fno-asynchronous-unwind-tables -Os

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
ifeq ("$(CCOMPILER)", "c89")
CFLAGS+=-ansi -fdiagnostics-color=always -Wno-unused-result
DUMPMACHINE=$(shell $(CC) $(CFLAGS) -dumpmachine)
else ifeq ("$(CCOMPILER)", "c99")
CFLAGS+=-fdiagnostics-color=always -Wno-unused-result
DUMPMACHINE=$(shell $(CC) $(CFLAGS) -dumpmachine)
else ifeq ("$(CCOMPILER)", "gcc")
CFLAGS+=-ansi -fdiagnostics-color=always -Wno-unused-result
DUMPMACHINE=$(shell $(CC) $(CFLAGS) -dumpmachine)
else ifeq ("$(CCOMPILER)", "clang")
CFLAGS+=-ansi -fdiagnostics-color=always -Wno-array-bounds
DUMPMACHINE=$(shell $(CC) $(CFLAGS) -dumpmachine)
else ifeq ("$(CCOMPILER)", "tcc")
CFLAGS+=-fdiagnostics-color=always
endif

# Fallback command to use when compiler doesn't support `-dumpmachine`
ifndef DUMPMACHINE
# DUMPMACHINE=$(shell uname -m -o)
# DUMPMACHINE=$(shell echo $(shell uname -m -o) | tr A-Z a-z)
DUMPMACHINE=$(shell echo $(shell uname -m)-$(shell uname -s)-$(shell uname -o) | tr A-Z a-z)
endif

# Get the machine Target Triplet
X_MACHINE?=\"$(DUMPMACHINE)\"

# Uncomment the following, if you get "wrong interpreter" errors on OSX
#LDFLAGS+=-Wl,-no_pie

BASEDIR=.
INCDIR=$(BASEDIR)/include
SRCDIR=$(BASEDIR)/src

# x-expr foundation library
X_EXPR_DIR=ext/x-expr
X_EXPR_SOURCES=$(filter-out $(X_EXPR_DIR)/src/x-obj.c, $(wildcard $(X_EXPR_DIR)/src/*.c))
X_EXPR_OBJECTS=$(X_EXPR_SOURCES:.c=.o)

CFLAGS+=-I$(X_EXPR_DIR)/include -I$(INCDIR)

HEADERS=$(wildcard $(INCDIR)/*.h $(INCDIR)/**/*.h $(X_EXPR_DIR)/include/*.h)
SOURCES=$(wildcard $(SRCDIR)/*.c $(SRCDIR)/**/*.c)
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=x

# Options to be added to $(DEFS)
DEFS?=$(OSDEF) -DX_MACHINE="$(X_MACHINE)"

# Where to install the stuff
BINDIR?=$(PREFIX)/bin
LIBDIR?=$(PREFIX)/share/$(EXECUTABLE)
MANDIR?=$(PREFIX)/man/man1

# Set up environment to be used during the build process
BUILD_ENV=env X_LIBRARY_PATH=.:lib:ext:contrib

default: all strip

all: $(SOURCES) $(EXECUTABLE)

debug: $(EXECUTABLE)-debug

strip: $(EXECUTABLE)
	strip $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) $(X_EXPR_OBJECTS) $(EXTRA_OBJS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(X_EXPR_OBJECTS) $(EXTRA_OBJS) $(EXTRA_LIBS) -o $@

$(EXECUTABLE)-debug: CFLAGS += -g -Og -DDEBUG
$(EXECUTABLE)-debug: LDFLAGS += -g
$(EXECUTABLE)-debug: $(EXECUTABLE)

.c.o:
	$(CC) -c $(CFLAGS) $(DEFS) -o $@ $<

defs:
	ctags -f - src/**/*.c | awk 'BEGIN {FS = "\t"} /\/.*\$\/;"/ { printf("%s;\n", substr($$3,3,length($$3)-6)) }' | sort -u > defs

docs: README.md $(SOURCES)
	grock --glob='*.{h,c,md}' --index=README.md --style=thin --out=docs
	LD_LIBRARY_PATH=/usr/lib/llvm-12/lib/ cldoc generate -I./include -- --report --language=c --output doc src/*.c

lint:
	$(CC) -fsyntax-only $(CFLAGS) -g -Wall -pedantic $(SOURCES)
.PHONY: lint

valgrind:
	$(CC) $(CFLAGS) -g -Wall $(SOURCES) && valgrind -v --leak-check=full ./a.out && rm a.out
.PHONY: valgrind

ifndef PATH_TESTS_C
PATH_TESTS_C=tests/c
endif

ifndef TESTS
TESTS=$(PATH_TESTS_C)/src/*.spec.c
endif

test:
	CFLAGS="$(CFLAGS) -fno-common -g -Og -I. -DTESTS" sh $(PATH_TESTS_C)/test-runner/test-runner.sh $(TESTS)
.PHONY: test

test-quick:
	CFLAGS="$(CFLAGS) -fno-common -g -Og -I. -DTESTS" RUNNER=command sh $(PATH_TESTS_C)/test-runner/test-runner.sh $(TESTS)
.PHONY: test

test-x: $(EXECUTABLE)
	sh tests/x/spec-runner.sh
.PHONY: test-x

tests: test test-x
.PHONY: tests

watch:
	while true; do \
		fswatch -o --event Created --event Updated --event MovedTo $(HEADERS) $(SOURCES) tests/c | \
		make debug && make test && make docs; \
	done
.PHONY: watch

# old version of install(1) may need -c
#C=-c
install:	$(EXECUTABLE) lib/$(EXECUTABLE)-lib.l $(EXECUTABLE).sh
	install -d -m 0755 $(BINDIR)
	install -d -m 0755 $(LIBDIR)
	install $C -m 0755 $(EXECUTABLE) $(BINDIR)
	install $C -m 0755 $(EXECUTABLE).sh $(BINDIR)
	strip $(BINDIR)/$(EXECUTABLE)
	install $C -m 0644 lib/* $(LIBDIR)
.PHONY: install

uninstall:
	rm -f $(LIBDIR)/* && rmdir $(LIBDIR)
	rm -f $(BINDIR)/$(EXECUTABLE)
	rm -f $(BINDIR)/$(EXECUTABLE).sh
.PHONY: uninstall

clean:
	rm -f $(EXECUTABLE) $(EXECUTABLE)-debug *.out $(SRCDIR)/*.o $(SRCDIR)/**/*.o $(X_EXPR_DIR)/src/*.o *.core core
	rm -Rf docs/
.PHONY: clean
