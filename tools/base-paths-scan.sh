#!/bin/sh
# tools/base-paths-scan.sh -- source half of the base-paths contract.
#
# The interpreter-state field accessors are pure first/rest macro chains
# spread across four headers: include/x-eval-layout.h (generated from
# tools/base-layout.x), ext/x-expr/include/x-base.h (the x-expr spine),
# include/x-eval.h (the error-handler object), and include/x-type.h (the
# type-object tree).  This scan expands every chain-shaped macro into a
# flat f/r step path and diffs the result against the committed descriptor
# tools/base-paths.x, which reflective X code walks (lib/x/boot/reflect.x).
# Non-chain macros (value casts, predicates) are skipped -- not paths.
#
# Roots: x_type_field_* macros are rooted at a TYPE object; other (X)
# macros at the base object (%base); (H) macros at an error-handler.
#
# Usage:  sh tools/base-paths-scan.sh          # check (diff, exit 1 on drift)
#         sh tools/base-paths-scan.sh --gen    # print descriptor entries

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SCRATCH="${TMPDIR:-/tmp}"
SRC_LIST="$SCRATCH/base-paths-src.$$"
MAN_LIST="$SCRATCH/base-paths-man.$$"
trap 'rm -f "$SRC_LIST" "$MAN_LIST"' EXIT

extract() {
awk '
/^#define x_[a-z0-9_]+\((X|H)\)/ {
	line = $0
	sub(/\/\*.*\*\//, "", line)          # strip trailing comment
	name = line
	sub(/^#define[ \t]+/, "", name)
	param = name
	sub(/\(.*$/, "", name)
	sub(/^[^(]*\(/, "", param)
	sub(/\).*$/, "", param)
	body = line
	sub(/^#define[ \t]+[a-z0-9_]+\([XH]\)[ \t]*/, "", body)
	gsub(/[ \t]/, "", body)
	if (!(name in defs)) {
		defs[name] = body
		roots[name] = param
		order[++n] = name
	}
}
# path of an expression relative to its root param; "!" on failure
function pathof(expr, param,    op, inner, ip, st) {
	# unwrap redundant grouping parens: ((X)) -> (X) -> X
	while (expr ~ /^\([a-zA-Z0-9_()]*\)$/) {
		sub(/^\(/, "", expr); sub(/\)$/, "", expr)
	}
	if (expr == param) return ""
	if (expr !~ /^[a-z0-9_]+\(.*\)$/) return "!"
	op = expr
	sub(/\(.*$/, "", op)
	inner = expr
	sub(/^[a-z0-9_]+\(/, "", inner)
	sub(/\)$/, "", inner)
	ip = pathof(inner, param)
	if (ip == "!") return "!"
	st = steps(op, param)
	if (st == "!") return "!"
	return ip st
}
# the f/r steps one operator applies AFTER its argument
function steps(op, param,    d, i, s) {
	if (op == "x_firstobj" || op == "x_0") return "f "
	if (op == "x_restobj"  || op == "x_1") return "r "
	if (op ~ /^x_[01]+$/) {
		d = substr(op, 3)
		s = ""
		# digit string is outermost-first; application order is the reverse
		for (i = length(d); i >= 1; i--)
			s = s (substr(d, i, 1) == "0" ? "f " : "r ")
		return s
	}
	if (op in defs) {
		if (roots[op] != param) return "!"
		return pathof(defs[op], roots[op])
	}
	return "!"
}
function xname(c) {
	sub(/^x_eval_field_/, "", c); sub(/^x_base_field_/, "", c)
	sub(/^x_error_handler_/, "error-handler-", c)
	sub(/^x_type_field_/, "type-", c)
	sub(/^x_eval_/, "", c); sub(/^x_/, "", c)
	gsub(/_/, "-", c)
	return c
}
function rootof(name, param) {
	if (param == "H") return "handler"
	if (name ~ /^x_type_field_/) return "type"
	return "base"
}
END {
	for (i = 1; i <= n; i++) {
		name = order[i]
		p = pathof(defs[name], roots[name])
		if (p == "!") continue
		sub(/ $/, "", p)
		printf "(%s %s%s%s)\n", xname(name), rootof(name, roots[name]), \
			(p == "" ? "" : " "), p
	}
}' "$ROOT/include/x-eval-layout.h" \
   "$ROOT/ext/x-expr/include/x-base.h" \
   "$ROOT/include/x-eval.h" \
   "$ROOT/include/x-type.h"
}

if [ "$1" = "--gen" ]; then
	extract | sed 's/^/  /'
	exit 0
fi

extract | sort > "$SRC_LIST"
awk '/^  \(/ { s = $0; sub(/^  /, "", s); print s }' \
	"$ROOT/tools/base-paths.x" | sort > "$MAN_LIST"

if ! diff -u "$MAN_LIST" "$SRC_LIST" > "$SCRATCH/base-paths-diff.$$" 2>&1; then
	echo "Base paths and tools/base-paths.x disagree (-descriptor +headers):"
	grep '^[-+][^-+]' "$SCRATCH/base-paths-diff.$$"
	rm -f "$SCRATCH/base-paths-diff.$$"
	echo "FAIL: a base field path moved without a descriptor edit (or vice versa)."
	exit 1
fi
rm -f "$SCRATCH/base-paths-diff.$$"
echo "Base-paths check: headers and tools/base-paths.x agree."
exit 0
