# tools/lib/isa-scan.awk -- the C binding-site scanner.
#
# Extracts every env-binding site from the C source: primitive tables, direct
# x_callable_bind/x_value_bind calls, and the x_prims_name_kept keep-list.
# ONE parser, because there are six registration shapes and a second reader of
# them would drift from this one. Consumers:
#
#   check/isa.sh       diffs the records against tools/contract/isa.x
#   check/prim-doc.sh  joins them to the entries in docs/primitives.md
#
# Records:
#   catalog <ns> <method>          a table entry filed in the prims catalog
#   bare <name>                    bound bare in the env
#   value <name>                   a non-prim value binding
#   keep <name>                    on the keep-list: binds bare even when its
#                                  namespace is de-registered
#
# With -v names=1, catalog records carry the entry's NAME as a fourth field:
#
#   catalog <ns> <method> <name> <c-function>
#   bare <name> <c-function>
#
# The name and the coordinate are different things -- `str-append` is FILED at
# `(str append)` -- and a doc that names a primitive needs the former, which
# the plain record drops. isa.sh must not see that field (its diff is against
# a manifest that has no names), so it is opt-in.

FNR == 1 { in_keep = 0; in_kept_fn = 0; in_dereg_fn = 0 }
/offsetof/ { next }
# x_prims_name_kept: the keep-list -- names that bind bare even when their
# namespace is de-registered.  Its array IS surface: extract every name as
# a keep record so growing the array requires a manifest edit.
/^static int x_prims_name_kept/ { in_kept_fn = 1 }
# x_prims_ns_deregistered: the namespaces whose bare names are DROPPED at
# registration, leaving the catalog as the only door. Whether a documented
# primitive is callable by name turns on this list, so prim-doc.sh needs it;
# isa.sh diffs against a manifest that has no such entries, hence names-only.
/^static int x_prims_ns_deregistered/ { in_dereg_fn = 1 }
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
	if (names && in_dereg_fn) {
		s = $0
		while (match(s, /"[^"]*"/)) {
			print "dereg " substr(s, RSTART + 1, RLENGTH - 2)
			s = substr(s, RSTART + RLENGTH)
		}
	}
	if (/};/) { in_keep = 0; in_kept_fn = 0; in_dereg_fn = 0 }
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
	fn = ""
	if (match(line, /x_(prim|syntax)_[A-Za-z_0-9]+/)) fn = substr(line, RSTART, RLENGTH)
	if (line ~ /x_callable_bind\(/) { print "bare " str[1] (names ? " " fn : ""); next }
	if (line ~ /x_value_bind\(/)    { print "value " str[1]; next }
	if (line !~ /\{[ \t]*"/) next
	if (n >= 3) {
		if (names) print "catalog " str[2] " " str[3] " " str[1] " " fn
		else       print "catalog " str[2] " " str[3]
		next
	}
	if (line ~ /x_prim_|x_syntax_/)      { print "bare " str[1] (names ? " " fn : ""); next }
	print "value " str[1]
}