#!/bin/sh
# tools/isa-scan.sh -- source-level half of the C ISA ratchet.
#
# Extracts every env-binding site from the C source (primitive tables,
# direct x_callable_bind/x_value_bind calls) and diffs the result against
# the committed manifest tools/isa.x.  Complements the runtime half
# (tests/x/specs/meta/isa.spec.md): the runtime walk sees the live catalog
# but cannot enumerate bare env bindings; this scan sees every binding in
# the source, including ones behind non-default compile flags.
#
# Exit 0 when the source and the manifest agree; exit 1 with a diff when
# the C surface grew (or the manifest went stale).
#
# Usage: sh tools/isa-scan.sh

. "$(dirname "$0")/lib/contract-diff.sh"
contract_diff_setup isa-scan

# --- 1. the C source's view -------------------------------------------------
# Table entries are one-per-line initializers.  Shapes:
#   { "name", x_prim_fn, "ns", "method" }   -> catalog entry
#   { "name", x_prim_fn, NULL, NULL }       -> bare binding
#   { "name", x_prim_fn }                   -> bare binding
#   { "name", <value expr> }                -> value binding (e.g. %O_RDONLY)
#   x_callable_bind(p_base, "name", fn)     -> bare binding
#   x_value_bind(p_base, "name", val)       -> value binding
# Lines with offsetof are struct-layout tables, not bindings.  #t/#f are
# bound from interned singletons (x_prim_register), not name literals, so
# they are seeded explicitly.
awk '
FNR == 1 { in_keep = 0; in_kept_fn = 0 }
/offsetof/ { next }
# x_prims_name_kept: the keep-list -- names that bind bare even when their
# namespace is de-registered.  Its array IS surface: extract every name as
# a keep record so growing the array requires a manifest edit.
/^static int x_prims_name_kept/ { in_kept_fn = 1 }
# The other de-registration name arrays are string tables, not binding
# tables; they start with "static const char *const" and are skipped.
/static const char \*const/ { in_keep = 1 }
in_keep {
	if (in_kept_fn) {
		s = $0
		while (match(s, /"[^"]*"/)) {
			print "keep " substr(s, RSTART + 1, RLENGTH - 2)
			s = substr(s, RSTART + RLENGTH)
		}
	}
	if (/};/) { in_keep = 0; in_kept_fn = 0 }
	next
}
{
	line = $0
	# The two call forms may wrap across lines (table entries stay
	# one-per-line by the manifest contract).  Join continuation lines
	# until the parens balance, then extract from the joined line.
	if (line ~ /x_callable_bind\(|x_value_bind\(/) {
		t = line; o = gsub(/\(/, "(", t); c = gsub(/\)/, ")", t)
		while (o > c) {
			if ((getline nxt) <= 0) {
				printf "FAIL: unterminated bind call in %s: %s\n", \
					FILENAME, line > "/dev/stderr"
				exit 1
			}
			line = line " " nxt
			t = nxt; o += gsub(/\(/, "(", t); c += gsub(/\)/, ")", t)
		}
	}
	n = 0
	s = line
	while (match(s, /"[^"]*"/)) {
		n++
		str[n] = substr(s, RSTART + 1, RLENGTH - 2)
		s = substr(s, RSTART + RLENGTH)
	}
	if (n == 0) next
	if (line ~ /x_callable_bind\(/) { print "bare " str[1]; next }
	if (line ~ /x_value_bind\(/)    { print "value " str[1]; next }
	if (line !~ /\{[ \t]*"/) next
	if (n >= 3)                          { print "catalog " str[2] " " str[3]; next }
	if (line ~ /x_prim_|x_syntax_/)      { print "bare " str[1]; next }
	print "value " str[1]
}' "$ROOT"/src/*.c "$ROOT"/src/x-prim/*.c "$ROOT"/src/x-syntax/*.c \
   "$ROOT"/opt/x-prim/*.c > "$SRC_LIST" || exit 1
printf 'value #t\nvalue #f\n' >> "$SRC_LIST"

# --- 2. the manifest's view -------------------------------------------------
awk '
# Section heads are anchored def forms: (def %isa-<name> (lit (
# aliases are X-level aliases of bare prims (not C binding sites;
# runtime-walk-only) and are ignored; values entries print as "value".
/^\(def %isa-/ {
	sect = $2
	sub(/^%isa-/, "", sect)
	if (sect == "aliases") sect = ""
	else if (sect == "values") sect = "value"
	next
}
/^  \(/ {
	line = $0
	gsub(/[()]/, "", line)
	split(line, f, " ")
	if (sect == "catalog") print "catalog " f[1] " " f[2]
	else if (sect != "")   print sect " " f[1]
}' "$ROOT"/tools/isa.x > "$MAN_LIST"

# --- 3. diff ------------------------------------------------------------------
contract_diff_check "$MAN_LIST" "$SRC_LIST" \
	"C surface and tools/isa.x disagree (-manifest +source):" \
	"FAIL: the C ISA changed without a manifest edit (or the manifest is stale)." \
	"ISA check: C source and tools/isa.x agree."
