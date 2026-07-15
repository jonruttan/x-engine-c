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
# The descriptor records the X_HEAP build (every shipped personality); for
# the conditional X_OBJ_UNITS_HEAP the first (X_HEAP) definition wins, and
# the build-variant X_OBJ_FLAG_MASK is skipped.
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
# Units defines; the first UNITS_HEAP definition is the X_HEAP branch.
/^#define X_OBJ_UNITS_(HEAP|TYPE|FLAGS|ATOM|PAIR)[ \t]/ {
	name = $2
	if (name in seen) next
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
	uh = units["X_OBJ_UNITS_HEAP"]
	ut = units["X_OBJ_UNITS_TYPE"]
	uf = units["X_OBJ_UNITS_FLAGS"]
	print "%obj-slot-heap 0"
	print "%obj-slot-type " uh
	print "%obj-slot-flags " uh + ut
	print "%obj-meta-len " uh + ut + uf
	print "%obj-slot-first 0"
	print "%obj-slot-rest 1"
}' "$HDR" > "$SRC_LIST"

# --- 2. the descriptor's view ------------------------------------------------
awk '/^\(def %obj-/ { gsub(/[()]/, ""); print $2 " " $3 }' \
	"$ROOT/tools/obj-layout.x" > "$MAN_LIST"

# --- 3. diff -------------------------------------------------------------------
contract_diff_check "$MAN_LIST" "$SRC_LIST" \
	"Object layout and tools/obj-layout.x disagree (-descriptor +header):" \
	"FAIL: x-obj.h layout changed without a descriptor edit (or vice versa)." \
	"Object-layout check: x-obj.h and tools/obj-layout.x agree."
