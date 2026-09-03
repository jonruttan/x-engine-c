#!/bin/sh
# tools/check/declaration.sh -- is x-engine.xon what the generator produces?
#
# x-engine.xon is GENERATED -- by x-lang's tools/contract/gen-engine-xon.sh --
# and committed here.  So every edit to tools/contract/isa.x or claims.x has a
# second half, regenerate and commit the declaration, and until this script
# nothing in this repository checked that the second half happened.  Twice it
# did not: 0.1.5 added (base def-global), 0.2.1 added (type set-shape! types),
# and both shipped declaring the digest of the ISA they had BEFORE the row.
# The built engine was right both times -- only the declaration lied -- so
# every gate here stayed green and x-lang's check-engine-contract found it on
# the first pin bump, one release too late.
#
# UNLIKE the three contract gates, this is NOT self-contained: it needs an
# x-lang checkout.  The generator is the language's on purpose -- producing the
# declaration needs the feature VOCABULARY, and an engine that generated its
# own would be choosing the terms it is judged by -- which is exactly why this
# repo could not catch its own staleness.  A check that borrows a tree from
# elsewhere is still a check this repository can run.  It is kept out of `make
# gates` and `make test` for that reason: those must work on a bare clone.
#
# X_REFERENCE_DIR IS SET TO THIS TREE, deliberately.  The reference supplies
# the roster a candidate's capability claims are measured against, and pointing
# it at the tree under test asks the one question this check exists to ask:
# "is the committed declaration what the generator produces from these
# manifests?"  Whether this engine covers the LANGUAGE's roster is a different
# question that needs a real reference and the vocabulary in hand, and
# x-lang's check-engine-contract is what asks it.  Conflating the two here
# would make this gate depend on which engine release x-lang happens to have
# unpacked, which is not a fact about this commit.
#
# Exit 0 when the committed declaration matches; exit 1 with a diff when it is
# stale; exit 2 when no x-lang checkout could be found.
#
# Usage: X_LANG_DIR=<x-lang checkout> sh tools/check/declaration.sh

set -e

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"

# A sibling checkout is the common local layout; CI passes X_LANG_DIR outright.
XLANG="${X_LANG_DIR:-}"
if [ -z "$XLANG" ]; then
	for d in "$ROOT/../x-lang" "$ROOT/../../x-lang"; do
		if [ -f "$d/tools/contract/gen-engine-xon.sh" ]; then XLANG="$d"; break; fi
	done
fi
if [ -z "$XLANG" ]; then
	echo "check-declaration: no x-lang checkout found." >&2
	echo "  looked beside this repo and one level up; set X_LANG_DIR to override." >&2
	exit 2
fi

GEN="$XLANG/tools/contract/gen-engine-xon.sh"
if [ ! -f "$GEN" ]; then
	echo "check-declaration: no generator at $GEN" >&2
	exit 2
fi

W="${TMPDIR:-/tmp}/checkdecl.$$"
mkdir -p "$W"
trap 'rm -rf "$W"' EXIT INT TERM

X_REFERENCE_DIR="$ROOT" sh "$GEN" "$ROOT" > "$W/generated"

if diff -u "$ROOT/x-engine.xon" "$W/generated" > "$W/diff" 2>&1; then
	echo "x-engine.xon is what the generator produces."
	exit 0
fi

# Name the likely cause rather than only the symptom: the digest line moving on
# its own means a manifest was edited without regenerating, which is the whole
# reason this check exists.
echo "STALE: x-engine.xon is not what the generator produces." >&2
echo >&2
sed 's/^/    /' "$W/diff" >&2
echo >&2
echo "A manifest under tools/contract/ was edited without regenerating the" >&2
echo "declaration.  Regenerate it and commit the result in the same change:" >&2
echo >&2
echo "    sh $XLANG/tools/contract/gen-engine-xon.sh $ROOT > $ROOT/x-engine.xon" >&2
exit 1
