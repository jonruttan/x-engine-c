#!/bin/sh
# tools/check/isa.sh -- source-level half of the C ISA ratchet.
#
# Extracts every env-binding site from the C source (primitive tables,
# direct x_callable_bind/x_value_bind calls) and diffs the result against
# the committed manifest tools/contract/isa.x.  Complements the runtime half
# (tests/x/specs/meta/isa.spec.md): the runtime walk sees the live catalog
# but cannot enumerate bare env bindings; this scan sees every binding in
# the source, including ones behind non-default compile flags.
#
# Exit 0 when the source and the manifest agree; exit 1 with a diff when
# the C surface grew (or the manifest went stale).
#
# Usage: sh tools/check/isa.sh

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
. "$ROOT/tools/lib/contract-diff.sh"
contract_diff_setup isa-scan

# --- 1. the C source's view -------------------------------------------------
# The scanner is tools/lib/isa-scan.awk -- shared with check/prim-doc.sh, which
# joins the same records to docs/primitives.md.  #t/#f are bound from interned
# singletons rather than name literals, so they are seeded explicitly below.
awk -f "$ROOT/tools/lib/isa-scan.awk" "$ROOT"/src/*.c "$ROOT"/src/x-prim/*.c "$ROOT"/src/x-syntax/*.c \
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
}' "$ROOT"/tools/contract/isa.x > "$MAN_LIST"

# --- 3. diff ------------------------------------------------------------------
contract_diff_check "$MAN_LIST" "$SRC_LIST" \
	"C surface and tools/contract/isa.x disagree (-manifest +source):" \
	"FAIL: the C ISA changed without a manifest edit (or the manifest is stale)." \
	"ISA check: C source and tools/contract/isa.x agree."
