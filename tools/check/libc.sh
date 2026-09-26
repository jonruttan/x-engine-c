#!/bin/sh
# tools/check/libc.sh -- libc stays behind x-sys and x-stdlib.
#
# Every function in the C standard library and in POSIX that this engine
# needs is reached through ONE of two doors: `x_sys_*` (ext/x-expr's
# x-sys.c, the OS/libc primitives, each redirectable through X_SYS_FUNC)
# or `x_lib_*` (its x-stdlib.c, the stdlib replacements, self-contained
# unless X_USE_STDLIB says otherwise).  A raw call anywhere else bypasses
# the redirect, breaks the freestanding build, and sits quietly until a
# newer SDK deprecates the name -- which is how the `sprintf` in
# src/x-prim/ffi.c announced itself at v0.2.8, as a compiler warning in
# x-lang's ASan boot gate, with memcpy, dlopen and sigaction sitting
# beside it.  It was not the first time.  This gate is so it is the last:
# it scans this engine's own sources.  Exempt by decision: src/x-cli.c, the
# OS-facing file, where the host doors (syscall, dlopen/dlsym) live; and the
# build-time-optional host modules under opt/ (opt/x-prim/signal.c, the
# SIGINT module `make X_SIGNAL=` leaves out), listed by name.
#
# THREE RULES, over every .c and .h under SCAN_DIRS (tests excluded --
# a spec may use whatever it likes to check the code under test):
#
#   1. `#include <...>` only in the WRAPPERS.  The ALLOWED headers are
#      exempt everywhere: the freestanding ones carry types and macros,
#      no libc code; setjmp.h because setjmp must be called in the frame
#      longjmp returns to, so it cannot sit behind a wrapper function;
#      ctype.h by decision.
#   2. No call to a BANNED libc name outside the WRAPPERS.  Comments and
#      string literals are stripped first; `.name(` and `->name(` are
#      struct fields, `x_sys_name(` is a wrapper, neither is a hit.
#   3. No feature-test macro (_GNU_SOURCE and friends) and no
#      `#pragma ... diagnostic ignored` outside the WRAPPERS.  Both are
#      how a raw call stays quiet.
#
# EXEMPT everywhere: the EXEMPT files, and any line inside a region that
# is compiled only when DEBUG is defined (`#ifdef DEBUG`, `#if
# defined(DEBUG)`, the `#else` of an `#ifndef DEBUG`, nested however deep).
# Debug output may format however it likes.
#
# Exit 0 when the tree is clean; exit 1 listing every hit, with the file
# and line, otherwise.  The BANNED list is a list, not a theory: add to
# it when a new name slips through, in the same commit as the fix.
#
# Usage: sh tools/check/libc.sh              # check the tree
#        sh tools/check/libc.sh --self-test  # prove each rule still bites

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"

SCAN_DIRS="src opt include"
WRAPPERS="ext/x-expr/src/x-sys.c ext/x-expr/src/x-stdlib.c ext/x-expr/src/x-lib.c ext/x-expr/include/x-sys.h"
EXEMPT="src/x-cli.c opt/x-prim/signal.c"
ALLOWED="ctype.h float.h iso646.h limits.h setjmp.h stdalign.h stdarg.h stdbool.h stddef.h stdint.h stdnoreturn.h"

BANNED="abort abs atexit atof atoi atol atoll bsearch calloc div exit free getenv labs ldiv llabs malloc qsort rand realloc srand strtod strtof strtol strtold strtoll strtoul strtoull system
memchr memcmp memcpy memmove memset strcat strchr strcmp strcoll strcpy strcspn strdup strerror strlen strncat strncmp strncpy strndup strpbrk strrchr strspn strstr strtok strxfrm
fclose feof ferror fflush fgetc fgets fopen fprintf fputc fputs fread freopen fscanf fseek ftell fwrite getc getchar gets perror printf putc putchar puts remove rename rewind scanf setbuf setvbuf snprintf sprintf sscanf tmpfile ungetc vfprintf vprintf vsnprintf vsprintf vsscanf
access chdir close creat dup dup2 execl execle execlp execv execve execvp _exit fork fsync getcwd getpid isatty kill lseek mkdir open pipe read rmdir sleep stat fstat syscall unlink usleep wait waitpid write
raise sigaction sigaddset sigdelset sigemptyset sigfillset signal sigprocmask
dlclose dlerror dlopen dlsym
mmap munmap mprotect clock time gettimeofday nanosleep"

FEATURE_MACROS="_GNU_SOURCE _POSIX_C_SOURCE _POSIX_SOURCE _DEFAULT_SOURCE _XOPEN_SOURCE _BSD_SOURCE _SVID_SOURCE _DARWIN_C_SOURCE"

# One line in, one line out -- block comments, line comments and string
# and character literals blanked, and every line inside a DEBUG-only
# region blanked whole -- so a mention of sprintf(3) in a doc comment is
# not a call, debug output is not scanned, and line numbers still point
# at the source.  A conditional stack tracks nesting: an entry is "debug"
# when its live branch exists only under DEBUG.
prepare() {
	awk '
	function is_dbg_test(t) {
		return t ~ /^ifdef[ \t]+DEBUG([^A-Za-z0-9_]|$)/ ||
		       t ~ /^if[ \t]+defined[ \t]*\(?[ \t]*DEBUG[ \t]*\)?[ \t]*$/ ||
		       t ~ /^if[ \t]+DEBUG[ \t]*$/
	}
	function is_ndbg_test(t) {
		return t ~ /^ifndef[ \t]+DEBUG([^A-Za-z0-9_]|$)/ ||
		       t ~ /^if[ \t]+![ \t]*defined[ \t]*\(?[ \t]*DEBUG[ \t]*\)?[ \t]*$/
	}
	function in_debug(   k) {
		for (k = 1; k <= depth; k++) if (dbg[k]) return 1
		return 0
	}
	{
		line = $0
		d = line
		if (sub(/^[ \t]*#[ \t]*/, "", d)) {
			if (d ~ /^if(def|ndef)?([ \t]|$)/) {
				depth++
				dbg[depth] = is_dbg_test(d) ? 1 : 0
				inv[depth] = is_ndbg_test(d) ? 1 : 0
			}
			else if (d ~ /^else([^A-Za-z0-9_]|$)/ && depth > 0) {
				if (inv[depth]) { dbg[depth] = 1 }
				else { dbg[depth] = 0 }
			}
			else if (d ~ /^elif([^A-Za-z0-9_]|$)/ && depth > 0) {
				sub(/^el/, "", d)
				dbg[depth] = is_dbg_test(d) ? 1 : 0
				inv[depth] = 0
			}
			else if (d ~ /^endif([^A-Za-z0-9_]|$)/ && depth > 0) {
				depth--
			}
			print ""
			next
		}
		if (in_debug()) { print ""; next }
		out = ""
		i = 1
		n = length(line)
		while (i <= n) {
			two = substr(line, i, 2)
			c = substr(line, i, 1)
			if (inc) {
				if (two == "*/") { inc = 0; i += 2 } else { i++ }
				continue
			}
			if (two == "/*") { inc = 1; i += 2; continue }
			if (two == "//") { break }
			if (c == "\"" || c == "\047") {
				q = c
				i++
				while (i <= n && substr(line, i, 1) != q) {
					if (substr(line, i, 1) == "\\") { i++ }
					i++
				}
				i++
				out = out q q
				continue
			}
			out = out c
			i++
		}
		print out
	}' "$1"
}

# Directives, with DEBUG regions blanked: rules 1 and 3 read these.
directives() {
	awk '
	function is_dbg_test(t) {
		return t ~ /^ifdef[ \t]+DEBUG([^A-Za-z0-9_]|$)/ ||
		       t ~ /^if[ \t]+defined[ \t]*\(?[ \t]*DEBUG[ \t]*\)?[ \t]*$/ ||
		       t ~ /^if[ \t]+DEBUG[ \t]*$/
	}
	function is_ndbg_test(t) {
		return t ~ /^ifndef[ \t]+DEBUG([^A-Za-z0-9_]|$)/ ||
		       t ~ /^if[ \t]+![ \t]*defined[ \t]*\(?[ \t]*DEBUG[ \t]*\)?[ \t]*$/
	}
	function in_debug(   k) {
		for (k = 1; k <= depth; k++) if (dbg[k]) return 1
		return 0
	}
	{
		d = $0
		if (!sub(/^[ \t]*#[ \t]*/, "", d)) { print ""; next }
		if (d ~ /^if(def|ndef)?([ \t]|$)/) {
			depth++
			dbg[depth] = is_dbg_test(d) ? 1 : 0
			inv[depth] = is_ndbg_test(d) ? 1 : 0
			print ""; next
		}
		if (d ~ /^else([^A-Za-z0-9_]|$)/ && depth > 0) {
			if (inv[depth]) { dbg[depth] = 1 } else { dbg[depth] = 0 }
			print ""; next
		}
		if (d ~ /^elif([^A-Za-z0-9_]|$)/ && depth > 0) {
			sub(/^el/, "", d)
			dbg[depth] = is_dbg_test(d) ? 1 : 0
			inv[depth] = 0
			print ""; next
		}
		if (d ~ /^endif([^A-Za-z0-9_]|$)/ && depth > 0) { depth--; print ""; next }
		if (in_debug()) { print ""; next }
		print "#" d
	}' "$1"
}

is_listed() {
	for w in $2; do
		[ "$w" = "$1" ] && return 0
	done
	return 1
}

# scan ROOT DIRS -> appends hits to $HITS, returns 1 when there were any
scan() {
	root="$1"
	dirs="$2"
	banned_re="$(printf '%s\n' $BANNED | paste -sd'|' -)"
	feature_re="$(printf '%s\n' $FEATURE_MACROS | paste -sd'|' -)"

	for d in $dirs; do
		[ -d "$root/$d" ] || continue
		for f in $(find "$root/$d" -type f \( -name '*.c' -o -name '*.h' \) | sort); do
			rel="${f#$root/}"
			if is_listed "$rel" "$WRAPPERS" || is_listed "$rel" "$EXEMPT"; then
				continue
			fi

			# Rule 1: libc headers
			directives "$f" | grep -nE '^#include[[:space:]]*<' | while IFS= read -r line; do
				hdr="$(printf '%s\n' "$line" | sed 's/.*<\([^>]*\)>.*/\1/')"
				if ! is_listed "$hdr" "$ALLOWED"; then
					printf '%s:%s: libc header <%s> outside the wrappers\n' \
						"$rel" "${line%%:*}" "$hdr"
				fi
			done >> "$HITS"

			# Rule 2: libc calls
			prepare "$f" | grep -nE "(^|[^A-Za-z0-9_.>])($banned_re)[[:space:]]*\(" | \
				sed "s|^\([0-9]*\):.*|$rel:\1: raw libc call -- route it through x_sys_* or x_lib_*|" >> "$HITS"

			# Rule 3: feature-test macros and silenced diagnostics
			directives "$f" | grep -nE "^#define[[:space:]]+($feature_re)([^A-Za-z0-9_]|$)" | \
				sed "s|^\([0-9]*\):.*|$rel:\1: feature-test macro outside the wrappers|" >> "$HITS"
			directives "$f" | grep -nE '^#pragma[[:space:]].*diagnostic[[:space:]]+ignored' | \
				sed "s|^\([0-9]*\):.*|$rel:\1: silenced diagnostic outside the wrappers|" >> "$HITS"
		done
	done

	[ ! -s "$HITS" ]
}

HITS="${TMPDIR:-/tmp}/check-libc.$$"
: > "$HITS"
trap 'rm -f "$HITS"' EXIT

if [ "$1" = "--self-test" ]; then
	# A throwaway tree: one offender per rule, one exempt file, one file of
	# DEBUG regions that must not fire, one clean file.  The scan must name
	# every offender and nothing else.
	T="${TMPDIR:-/tmp}/check-libc-self.$$"
	rm -rf "$T"
	mkdir -p "$T/src"
	trap 'rm -rf "$T"; rm -f "$HITS"' EXIT

	cat > "$T/src/header.c" <<'EOF'
#include <stdio.h>
#include <stddef.h>
#include <ctype.h>
EOF
	cat > "$T/src/call.c" <<'EOF'
/* sprintf(3) in a comment is not a call. */
static void f(char *b, double a) { len = sprintf(b, "%.15g", a); }
static void g(void) { table.free(p); q->free(p); x_sys_free(p); isdigit(c); }
EOF
	cat > "$T/src/macro.c" <<'EOF'
#define _GNU_SOURCE
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
EOF
	cat > "$T/src/debug.c" <<'EOF'
#ifdef DEBUG
#include <stdio.h>
static void a(void) { sprintf(b, "x"); }
#endif
#ifndef DEBUG
static void b(void) { x_sys_write(1, "x", 1); }
#else
static void c(void) { vsprintf(b, f, ap); }
#endif
#if defined(DEBUG)
# ifdef X_HEAP
static void d(void) { printf("x"); }
# endif
#endif
#ifdef DEBUG
static void e(void) { }
#else
static void e(void) { sprintf(b, "x"); }
#endif
EOF
	cat > "$T/src/exempt.c" <<'EOF'
#include <unistd.h>
static long s(void) { return syscall(1, 2); }
EOF
	cat > "$T/src/clean.c" <<'EOF'
#include <setjmp.h>
#include <ctype.h>
#include "x-sys.h"
static void f(const char *b) { x_sys_write(1, b, 1); }
static void g(void) { x_lib_memcpy(d, s, n); if (isxdigit(c)) return; }
EOF
	EXEMPT="src/exempt.c"
	scan "$T" "src" > /dev/null 2>&1
	want='src/call.c:2: raw libc call -- route it through x_sys_* or x_lib_*
src/debug.c:18: raw libc call -- route it through x_sys_* or x_lib_*
src/header.c:1: libc header <stdio.h> outside the wrappers
src/macro.c:1: feature-test macro outside the wrappers
src/macro.c:2: silenced diagnostic outside the wrappers'
	got="$(sort "$HITS")"
	if [ "$got" != "$want" ]; then
		echo "check-libc self-test: FAIL -- the scan did not name exactly the planted offenders" >&2
		echo "--- wanted" >&2; printf '%s\n' "$want" >&2
		echo "--- got" >&2; printf '%s\n' "$got" >&2
		exit 1
	fi
	echo "check-libc self-test: ok (each rule bites; ctype, DEBUG regions and the exempt file pass)"
	exit 0
fi

if scan "$ROOT" "$SCAN_DIRS" > /dev/null; then
	echo "check-libc: ok (libc is reached only through x_sys_* and x_lib_*)"
	exit 0
fi

echo "check-libc: FAIL -- libc reached directly, outside x-sys/x-stdlib:" >&2
sort "$HITS" >&2
echo "Use an existing x_sys_*/x_lib_* door, or move the host call into src/x-cli.c." >&2
exit 1
