#!/bin/sh
# tools/check/base-slots.sh -- source half of the base-slots contract.
#
# The positions in the base's slot vector are the members of two enums:
# x_slot_enum in ext/x-expr/include/x-slots.h, which x-expr owns, and
# x_eval_slot_enum in include/x-eval-slots.h, which follows it.  Each member
# is documented with the arguments its slot function takes.  This scan reads
# the members in order, numbers them, and diffs the result against the
# committed descriptor tools/contract/base-slots.x, which x-lang reads a
# position from.
#
# A member is a line `X_SLOT_NAME,` or `X_SLOT_NAME = ...,`; the two length
# members, X_SLOT_EXPR_LEN and X_SLOT_LEN, are not slots.  The arguments are
# read from the comment above the member, which says `Arguments: (a, b).`
# A member whose comment has no such sentence fails the scan, so a slot
# cannot be added without saying what it takes.
#
# Usage:  sh tools/check/base-slots.sh          # check (diff, exit 1 on drift)
#         sh tools/check/base-slots.sh --gen    # print descriptor entries

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
. "$ROOT/tools/lib/contract-diff.sh"
contract_diff_setup base-slots

extract() {
awk '
# x-lang name of an argument list: the words, lower case, hyphenated.
function xargs(s,    n, i, w, out) {
	gsub(/[()]/, "", s)
	n = split(s, w, /,[ \t]*/)
	out = ""
	for (i = 1; i <= n; i++) {
		sub(/^pointer to the /, "", w[i])
		gsub(/^[ \t]+|[ \t]+$/, "", w[i])
		gsub(/[ \t]+/, "-", w[i])
		out = out (i > 1 ? " " : "") w[i]
	}
	return out
}
function xname(c) {
	sub(/^X_SLOT_/, "", c)
	c = tolower(c)
	gsub(/_/, "-", c)
	return c
}
/Arguments: \(/ {
	args = $0
	sub(/^.*Arguments: /, "", args)
	sub(/\)\..*$/, ")", args)
	have = 1
	next
}
/^\tX_SLOT_[A-Z_]+[ \t]*(=[^,]*)?,?[ \t]*$/ {
	name = $1
	sub(/,$/, "", name)
	if (name ~ /_LEN$/) { have = 0; next }
	if (!have) {
		printf "FAIL: slot %s has no `Arguments: (...)` sentence in its" \
			" comment.\n", name > "/dev/stderr"
		bad = 1
		next
	}
	printf "(%d %s (%s))\n", n++, xname(name), xargs(args)
	have = 0
}
END { if (bad) exit 1 }
' "$ROOT/ext/x-expr/include/x-slots.h" \
  "$ROOT/include/x-eval-slots.h"
}

# extract runs outside a pipeline so a failure (exit 1) is not swallowed by
# the downstream sort/sed.
if [ "$1" = "--gen" ]; then
	extract > "$SRC_LIST" || exit 1
	sed 's/^/  /' "$SRC_LIST"
	exit 0
fi

extract > "$SRC_LIST" || exit 1
awk '/^  \(/ { s = $0; sub(/^  /, "", s); print s }' \
	"$ROOT/tools/contract/base-slots.x" > "$MAN_LIST"

contract_diff_check "$MAN_LIST" "$SRC_LIST" \
	"Base slots and tools/contract/base-slots.x disagree (-descriptor +headers):" \
	"FAIL: a slot moved without a descriptor edit (or vice versa)." \
	"Base-slots check: headers and tools/contract/base-slots.x agree."
