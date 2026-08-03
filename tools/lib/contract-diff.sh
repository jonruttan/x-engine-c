# tools/lib/contract-diff.sh -- shared scaffold for the contract scans
# (check/isa.sh, check/obj-layout.sh, check/base-paths.sh).  Sourced, not
# run; the caller sets ROOT first:
#   ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
#   . "$ROOT/tools/lib/contract-diff.sh"
#
# contract_diff_setup NAME
#   Requires ROOT (set by the caller; a sourced file cannot self-locate in
#   POSIX sh); sets SCRATCH and the tempfiles SRC_LIST / MAN_LIST / DIFF_OUT (all
#   $SCRATCH/NAME-*.$$), and installs ONE EXIT trap covering all three --
#   including the diff output, which an interrupt between diff and an
#   inline rm used to leak.
#
# contract_diff_check MAN_LIST SRC_LIST HEADER FAIL_MSG OK_MSG
#   Sorts both lists in place, diffs the manifest against the source view,
#   and either prints HEADER + the changed (+/-) lines + FAIL_MSG and exits
#   1 on drift, or prints OK_MSG and exits 0 on agreement.

contract_diff_setup() {
	[ -n "${ROOT:-}" ] || { echo "contract-diff: caller must set ROOT" >&2; exit 2; }
	SCRATCH="${TMPDIR:-/tmp}"
	SRC_LIST="$SCRATCH/$1-src.$$"
	MAN_LIST="$SCRATCH/$1-man.$$"
	DIFF_OUT="$SCRATCH/$1-diff.$$"
	trap 'rm -f "$SRC_LIST" "$MAN_LIST" "$DIFF_OUT"' EXIT
}

contract_diff_check() {
	sort -o "$1" "$1"
	sort -o "$2" "$2"
	if ! diff -u "$1" "$2" > "$DIFF_OUT" 2>&1; then
		echo "$3"
		grep '^[-+][^-+]' "$DIFF_OUT"
		echo "$4"
		exit 1
	fi
	echo "$5"
	exit 0
}
