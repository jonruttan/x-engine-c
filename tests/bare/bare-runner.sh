#!/bin/sh
# bare-runner.sh -- run a spec against the ENGINE ALONE.
#
# SCOPE.  This is x-engine-c's own smoke harness: it asks whether THIS engine
# stands up unaided.  It is deliberately NOT the cross-engine conformance
# suite -- that defines what any evaluator must do, so it belongs with the
# LANGUAGE and lives in x-lang, with engines vendoring a pinned copy.  A
# conformance suite owned by one implementation would make that
# implementation the arbiter of the contract every other one is judged
# against, which is the whole thing the split exists to avoid.
#
# X_BIN is honoured anyway, because the cost is nothing and pointing this at
# a different build (asan, cov) is useful on its own.
#
# Every other runner in tests/ cats a library onto stdin ahead of the test, so
# what it measures is the library's surface. For a primitive that is the wrong
# subject: reached through prim-ref, through a class, or through any name the
# library rebinds, it is not the primitive under test. `+` is the plain case --
# the primitive is BINARY, and the variadic `+` every other spec exercises is
# lib/x/core/arithmetic.x sitting on top of it.
#
# So this runner loads nothing. Each case is one engine process fed two lines:
# the allocation guard, then the source. Nothing else reaches the engine.
#
# It cannot use the shared harness, because that harness prints its own test
# boundaries with (display ...) -- an x-lang function from lib/x/boot/printer.x,
# absent from a bare engine, which is why driving these specs through it kills
# the interpreter. There are no in-band separators here: the engine echoes each
# result through its own C read-eval loop, and one process per case keeps the
# results unambiguous without any help from inside.
#
# alloc-limit! is a primitive the engine binds bare, so the ceiling is armed
# with no library present -- the same guard tests/spec-runner.sh feeds.
#
# Usage: sh tests/bare/bare-runner.sh [spec.md ...]   (default: tests/bare/specs/)
set -e

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
ENGINE="${X_BIN:-$ROOT/x-bin}"
LIMIT="${X_ALLOC_LIMIT_OBJS:-2000000}"

[ -x "$ENGINE" ] || { echo "bare-runner: no engine at $ENGINE" >&2; exit 2; }

# The one piece of context the harness supplies. A bare engine has no printer
# -- write and display are pure x-lang -- so a spec that wants to show a value
# writes it through the syscall door, and that number is per-OS. Supplying it
# here keeps the specs portable without loading anything: %write arrives as a
# plain def, evaluated by the engine like any other line of the test.
case "$(uname -s)" in
	Darwin|*BSD) WRITE_NR=4 ;;
	Linux)       WRITE_NR=1 ;;
	*) echo "bare-runner: unknown platform $(uname -s); no write syscall number" >&2
	   exit 2 ;;
esac

WORK="${TMPDIR:-/tmp}/bare-runner.$$"
mkdir -p "$WORK"
trap 'rm -rf "$WORK"' EXIT INT TERM

if [ $# -gt 0 ]; then SPECS="$*"; else SPECS=$(find "$ROOT/tests/bare/specs" -name "*.spec.md" | sort); fi

RED=$(printf "\033[1;31m"); GREEN=$(printf "\033[1;32m")
BLUE=$(printf "\033[1;34m"); OFF=$(printf "\033[0m")

total=0; failed=0

for spec in $SPECS; do
	[ -f "$spec" ] || { echo "bare-runner: no such spec: $spec" >&2; exit 2; }
	printf "%s%s%s\n" "$BLUE" "$(basename "$spec")" "$OFF"

	# Split the spec into one directory per case: code, the expected value,
	# and the name. Same shape as the other spec files -- ### name, a fenced
	# block, --- , then the expected value indented.
	rm -rf "$WORK/cases"; mkdir -p "$WORK/cases"
	awk -v out="$WORK/cases" '
		/^### / { n++; name = substr($0, 5); state = "pre"
		          printf "%s", name > (out "/" n ".name"); next }
		!n { next }
		/^```/ { if (state == "pre") { state = "code"; next }
		         if (state == "code") { state = "aftercode"; next } }
		state == "code" { print > (out "/" n ".code"); next }
		/^---[ \t]*$/ && state == "aftercode" { state = "want"; next }
		state == "want" && $0 ~ /[^ \t]/ {
			line = $0; sub(/^[ \t]+/, "", line)
			print line > (out "/" n ".want"); state = "done"; next }
	' "$spec"

	i=1
	while [ -f "$WORK/cases/$i.code" ]; do
		name=$(cat "$WORK/cases/$i.name")
		want=""
		[ -f "$WORK/cases/$i.want" ] && want=$(cat "$WORK/cases/$i.want")

		# One process, two inputs: the guard, then the source. No library.
		got=$( { printf "(alloc-limit! %s)\n(def %%write %s)\n" "$LIMIT" "$WRITE_NR"
		         cat "$WORK/cases/$i.code"; } \
			| "$ENGINE" 2>&1 | sed "/^[ \t]*$/d" | tail -1 )

		total=$((total + 1))
		if [ "$got" = "$want" ]; then
			printf "%s.%s" "$GREEN" "$OFF"
		else
			failed=$((failed + 1))
			printf "\n%sFAIL: %s\n  expected: %s\n  got:      %s%s\n" \
				"$RED" "$name" "$want" "$got" "$OFF"
		fi
		i=$((i + 1))
	done
	printf "\n"
done

if [ "$failed" -gt 0 ]; then
	printf "\n%s%d tests, %d failed%s\n" "$RED" "$total" "$failed" "$OFF"
	exit 1
fi
printf "\n%s%d tests, 0 failed%s\n" "$GREEN" "$total" "$OFF"
