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

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SCRATCH="${TMPDIR:-/tmp}"
SRC_LIST="$SCRATCH/isa-scan-src.$$"
MAN_LIST="$SCRATCH/isa-scan-man.$$"
trap 'rm -f "$SRC_LIST" "$MAN_LIST"' EXIT

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
FNR == 1 { in_keep = 0 }
/offsetof/ { next }
# The keep-list / de-registration name arrays in x-prim.c are string tables,
# not binding tables; they start with "static const char *const".
/static const char \*const/ { in_keep = 1 }
in_keep { if (/};/) in_keep = 0; next }
{
	line = $0
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
   "$ROOT"/opt/x-prim/*.c > "$SRC_LIST"
printf 'value #t\nvalue #f\n' >> "$SRC_LIST"

# --- 2. the manifest's view -------------------------------------------------
awk '
/%isa-catalog/ { sect = "catalog"; next }
/%isa-bare/    { sect = "bare"; next }
/%isa-values/  { sect = "value"; next }
/^  \(/ {
	line = $0
	gsub(/[()]/, "", line)
	split(line, f, " ")
	if (sect == "catalog") print "catalog " f[1] " " f[2]
	else if (sect != "")   print sect " " f[1]
}' "$ROOT"/tools/isa.x > "$MAN_LIST"

# --- 3. diff ------------------------------------------------------------------
sort -o "$SRC_LIST" "$SRC_LIST"
sort -o "$MAN_LIST" "$MAN_LIST"
if ! diff -u "$MAN_LIST" "$SRC_LIST" > "$SCRATCH/isa-scan-diff.$$" 2>&1; then
	echo "C surface and tools/isa.x disagree (-manifest +source):"
	grep '^[-+][^-+]' "$SCRATCH/isa-scan-diff.$$"
	rm -f "$SCRATCH/isa-scan-diff.$$"
	echo "FAIL: the C ISA changed without a manifest edit (or the manifest is stale)."
	exit 1
fi
rm -f "$SCRATCH/isa-scan-diff.$$"
echo "ISA check: C source and tools/isa.x agree."
exit 0
