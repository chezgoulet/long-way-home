#!/usr/bin/env bash
# Destinations as readers, the neutral string table, and the three-second arrival -- Stage E
# (docs/endings.md; docs/path-to-playtest.md, Stage E).
#
#   scripts/ending-check.sh
#
# Static host: no engine, no game data, no model, no network. It compiles the ship tests on the host
# and runs the suite, then `test_ship_core --endings` prints the observable criteria:
#
#   more than one destination is reachable from one run; the same record is read differently by two
#   readers, each naming the evidence it reads; the shipped ending strings carry no comparative or
#   tone word; the arrival is 3000 ms and the log is the last frame.
#
# Then it falsifies twice: a destination named "the best Earth" must fail the neutrality assertion,
# and an arrival shortened to 2500 ms must fail the arrival assertion. A guard that cannot fail for
# the reason we care about is decoration.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

CXX="${CXX:-g++}"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/lwh-ending-check.XXXXXX")"
trap 'rm -rf "${WORK:?}"' EXIT

fail() { echo "FAIL  $1" >&2; exit 1; }

build() { # <source dir> <binary>
	"$CXX" -std=c++17 -g -I "$1" -I module/ui \
		tests/ship/test_ship_core.cpp "$1/ship_core.cpp" "$1/scenario_core.cpp" -o "$2"
}

echo "==> endings: the unit test suite, built here"
build module/ship "$WORK/test_ship_core"
"$WORK/test_ship_core" >/dev/null

echo
echo "==> endings: destinations, readers, neutrality and the arrival (test_ship_core --endings)"
"$WORK/test_ship_core" --endings | tee "$WORK/endings.txt"
echo

# The set: one run reaches more than one destination, and Earth is always in it.
grep -qF -- '  EARTH ALWAYS: 1' "$WORK/endings.txt" \
	|| fail "Earth is not always open"
grep -qE '^  OFFERED \(earned run\): Earth, .+, No reader$' "$WORK/endings.txt" \
	|| fail "one run does not reach more than one destination"

# The readers: the same record, two different readings, each naming its own evidence.
grep -qE '^  Earth line: .+' "$WORK/endings.txt" || fail "Earth reads no evidence"
grep -qE '^  Klingon line: .+' "$WORK/endings.txt" || fail "the Klingons read no evidence"
grep -qF -- '  readings differ: 1' "$WORK/endings.txt" || fail "the two readers reach the same reading"
grep -qF -- '  each names what it reads: 1' "$WORK/endings.txt" \
	|| fail "a reading does not name the part of the record it reads"

# Neutrality: an assertion over the shipped strings, not a grep of the pull-request body.
grep -qE '^  NEUTRALITY: 0 findings' "$WORK/endings.txt" \
	|| fail "a comparative or tone word is in a shipped ending string"
grep -qF -- '  RANK FIELD: none' "$WORK/endings.txt" \
	|| fail "the destination table carries a rank"

# The arrival: three seconds, and the log is what follows.
grep -qE '^  ARRIVAL: 3000 ms \(3 s\)' "$WORK/endings.txt" \
	|| fail "the arrival is not three seconds"
grep -qE '^  THE LOG: the last frame is the official log' "$WORK/endings.txt" \
	|| fail "the log is not the last frame"

echo
echo "==> endings: falsification one (a destination named 'the best Earth'; the neutrality assertion must fail)"
cp -r module/ship "$WORK/ship_best"
sed -i 's/"Earth",/"the best Earth",/' "$WORK/ship_best/ship_core.cpp"
grep -q '"the best Earth",' "$WORK/ship_best/ship_core.cpp" || fail "the neutrality plant did not take"
build "$WORK/ship_best" "$WORK/test_best"
if "$WORK/test_best" --endings >"$WORK/best.txt" 2>&1; then
	fail "the neutrality assertion passed with a destination named 'the best Earth': it has no teeth"
fi
grep -qE '^  NEUTRALITY: [1-9]' "$WORK/best.txt" \
	|| fail "the planted tone word did not surface as a neutrality finding"
grep -qF -- 'best in "the best Earth"' "$WORK/best.txt" \
	|| fail "the planted tone word was not the one found"
echo "    the planted destination fails the same assertion; the tree passes"

echo
echo "==> endings: falsification two (the arrival shortened to 2500 ms; the arrival assertion must fail)"
cp -r module/ship "$WORK/ship_short"
sed -i 's/const int ARRIVAL_MS = 3000;/const int ARRIVAL_MS = 2500;/' "$WORK/ship_short/ship_core.h"
grep -q 'const int ARRIVAL_MS = 2500;' "$WORK/ship_short/ship_core.h" || fail "the arrival plant did not take"
build "$WORK/ship_short" "$WORK/test_short"
if "$WORK/test_short" --endings >"$WORK/short.txt" 2>&1; then
	fail "the arrival assertion passed at 2500 ms: it has no teeth"
fi
grep -qE '^  ARRIVAL: 2500 ms' "$WORK/short.txt" \
	|| fail "the shortened arrival did not print its own length"
grep -qF -- 'the arrival is not three seconds' "$WORK/short.txt" \
	|| fail "the shortened arrival failed for some other reason"
echo "    the shortened arrival fails the same assertion; the tree passes"

echo
echo "PASS  one run reaches more than one destination; the same record is read differently by two"
echo "      readers, each naming its evidence; no comparative or tone word is in the shipped ending"
echo "      strings; the arrival is 3000 ms and the log follows it; and the guard fails on a planted"
echo "      'best' and on a 2500 ms arrival"
