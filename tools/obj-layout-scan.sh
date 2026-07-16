#!/bin/sh
# tools/obj-layout-scan.sh -- source half of the object-layout contract.
#
# Parses the layout constants out of ext/x-expr/include/x-obj.h (units per
# header slot, the flags-word bits) and diffs them against the committed
# descriptor tools/obj-layout.x, so an x-expr bump that moves the object
# layout fails `make check-obj-layout` before anything runs.  The runtime
# half is tests/x/specs/meta/obj-layout.spec.md, which probes live objects
# word by word.
#
# The descriptor records the X_HEAP build (every shipped personality); the
# scan tracks the header's #ifdef X_HEAP/#ifndef X_HEAP/#else/#endif nesting
# and reads only the X_HEAP branch, and the build-variant X_OBJ_FLAG_MASK is
# skipped.  An X_HEAP conditional the tracker cannot classify fails loudly
# (same philosophy as base-paths-scan's SKIP_MACROS fail-on-unparseable).
#
# Exit 0 when header and descriptor agree; exit 1 with a diff otherwise.

. "$(dirname "$0")/lib/contract-diff.sh"
contract_diff_setup obj-layout
HDR="$ROOT/ext/x-expr/include/x-obj.h"

# --- 1. the header's view ----------------------------------------------------
awk '
function hex2dec(h,    n, i, c) {
	n = 0
	h = tolower(h)
	sub(/^0x/, "", h)
	for (i = 1; i <= length(h); i++) {
		c = index("0123456789abcdef", substr(h, i, 1)) - 1
		n = n * 16 + c
	}
	return n
}
function numval(s) {
	sub(/[,;].*$/, "", s)
	if (s ~ /^0x/) return hex2dec(s)
	return s + 0
}
# x-lang name: strip X_OBJ_, lowercase, underscores to hyphens.
function xname(c) {
	sub(/^X_OBJ_/, "", c)
	c = tolower(c)
	gsub(/_/, "-", c)
	return "%obj-" c
}
# --- X_HEAP preprocessor tracking -----------------------------------------
# The descriptor records the X_HEAP build, so the X_HEAP branch of every
# conditional is selected EXPLICITLY (never first-definition-wins).  heap is
# 0 outside any X_HEAP conditional, 1 inside the taken branch, -1 inside the
# excluded branch; kind[] stacks every open conditional so #else/#endif pair
# with the right #if.  Anything the tracker cannot classify (an X_HEAP
# conditional nested in another, #if/#elif expressions naming X_HEAP) fails
# loudly rather than guessing.
function ppfail(msg) {
	printf "FAIL: %s at %s:%d: %s -- teach tools/obj-layout-scan.sh the shape.\n", \
		msg, FILENAME, FNR, $0 > "/dev/stderr"
	bad = 1
	exit 1
}
/^[ \t]*#[ \t]*ifdef[ \t]+X_HEAP([ \t]|$)/ {
	if (heap != 0) ppfail("nested X_HEAP conditional")
	depth++; kind[depth] = "hy"; heap = 1; next
}
/^[ \t]*#[ \t]*ifndef[ \t]+X_HEAP([ \t]|$)/ {
	if (heap != 0) ppfail("nested X_HEAP conditional")
	depth++; kind[depth] = "hn"; heap = -1; next
}
/^[ \t]*#[ \t]*(if|ifdef|ifndef)([ \t]|$)/ {
	if ($0 ~ /X_HEAP/) ppfail("unclassifiable X_HEAP conditional")
	depth++; kind[depth] = "other"; next
}
/^[ \t]*#[ \t]*elif([ \t]|$)/ {
	if (depth == 0) ppfail("unmatched #elif")
	if (kind[depth] != "other" || $0 ~ /X_HEAP/)
		ppfail("unclassifiable X_HEAP conditional")
	next
}
/^[ \t]*#[ \t]*else([ \t]|$|\/)/ {
	if (depth == 0) ppfail("unmatched #else")
	if (kind[depth] == "hy")      { kind[depth] = "hn"; heap = -1 }
	else if (kind[depth] == "hn") { kind[depth] = "hy"; heap = 1 }
	next
}
/^[ \t]*#[ \t]*endif([ \t]|$|\/)/ {
	if (depth == 0) ppfail("unmatched #endif")
	if (kind[depth] != "other") heap = 0
	depth--; next
}
# Lines in the excluded (non-X_HEAP) branch are not part of the recorded
# build.
heap < 0 { next }
# Units defines (X_HEAP-branch only, per the tracking above); a second
# active definition would mean the tracker misread the header, so it fails.
/^#define X_OBJ_UNITS_(HEAP|TYPE|FLAGS|ATOM|PAIR)[ \t]/ {
	name = $2
	if (name in seen) ppfail("duplicate active definition of " name)
	seen[name] = 1
	units[name] = numval($3)
	print xname(name) " " units[name]
	next
}
# Flags enum members: explicit "=value" or auto-increment from the previous.
/^[ \t]*X_OBJ_FLAG_[A-Z0-9_]+/ {
	line = $0
	sub(/^[ \t]*/, "", line)
	name = line
	sub(/[=,].*$/, "", name)
	if (name == "X_OBJ_FLAG_NONE" || name == "X_OBJ_FLAG_OBJ" \
			|| name == "X_OBJ_FLAG_MASK") {
		if (line ~ /=/) { v = line; sub(/^[^=]*=/, "", v); prev = numval(v) }
		next
	}
	if (line ~ /=/) { v = line; sub(/^[^=]*=/, "", v); prev = numval(v) }
	else prev = prev + 1
	print xname(name) " " prev
	next
}
END {
	if (bad) exit 1
	if (depth != 0) {
		printf "FAIL: unbalanced preprocessor conditionals in %s" \
			" (%d left open) -- teach tools/obj-layout-scan.sh" \
			" the shape.\n", FILENAME, depth > "/dev/stderr"
		exit 1
	}
	uh = units["X_OBJ_UNITS_HEAP"]
	ut = units["X_OBJ_UNITS_TYPE"]
	uf = units["X_OBJ_UNITS_FLAGS"]
	print "%obj-slot-heap 0"
	print "%obj-slot-type " uh
	print "%obj-slot-flags " uh + ut
	print "%obj-meta-len " uh + ut + uf
	print "%obj-slot-first 0"
	print "%obj-slot-rest 1"
}' "$HDR" > "$SRC_LIST" || exit 1

# --- 2. the descriptor's view ------------------------------------------------
awk '/^\(def %obj-/ { gsub(/[()]/, ""); print $2 " " $3 }' \
	"$ROOT/tools/obj-layout.x" > "$MAN_LIST"

# --- 3. diff -------------------------------------------------------------------
contract_diff_check "$MAN_LIST" "$SRC_LIST" \
	"Object layout and tools/obj-layout.x disagree (-descriptor +header):" \
	"FAIL: x-obj.h layout changed without a descriptor edit (or vice versa)." \
	"Object-layout check: x-obj.h and tools/obj-layout.x agree."
