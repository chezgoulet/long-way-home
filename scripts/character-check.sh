#!/usr/bin/env bash
# The character layer (O8, docs/character-derivation.md): the derivation, three generated crew
# members in full, and the manner at three morale levels. No game data and no engine -- the
# engine-independent model is exercised directly, so this runs anywhere a compiler does.
#
#   scripts/character-check.sh
#
# The unit tests themselves are in scripts/test.sh (tests/ship); this prints and checks the
# acceptance the owner judges: that a generated crew reads as people rather than a table of numbers.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
WORK="$(mktemp -d)"
trap 'rm -rf "${WORK:?}"' EXIT

cmake -S tests/ship -B "$WORK/ship" -DCMAKE_BUILD_TYPE=Debug >/dev/null
cmake --build "$WORK/ship" -j"$(nproc)" >/dev/null

echo "==> three generated crew members, in full"
"$WORK/ship/test_ship_core" --crew | tee "$WORK/crew.txt"
echo
echo "==> the manner at three morale levels"
"$WORK/ship/test_ship_core" --manner | tee "$WORK/manner.txt"
echo

fail() { echo "FAIL  $1" >&2; exit 1; }
# The four kinds and the drives are present.
grep -q 'skills:' "$WORK/crew.txt" || fail "no skills were printed"
grep -q 'traits:' "$WORK/crew.txt" || fail "no traits were printed"
grep -q 'drives:' "$WORK/crew.txt" || fail "no drives were printed"
# Morale is broken out into the three components, never a single number.
grep -q 'morale 0\.[0-9][0-9] ([a-z ]*) = deficit 0\.[0-9][0-9] (heart [0-9.]*) / outlook 0\.[0-9][0-9] / holdings 0\.[0-9][0-9]' "$WORK/crew.txt" \
  || fail "morale was not broken out into deficit, outlook and holdings"
# A condition names its source and its cure, and says who can see it.
grep -q 'conditions (' "$WORK/crew.txt" || fail "no conditions were printed"
grep -q 'from "' "$WORK/crew.txt" || fail "a condition did not name its source"
grep -q 'clears with' "$WORK/crew.txt" || fail "a condition did not name its cure"
grep -q 'visible to ' "$WORK/crew.txt" || fail "a condition did not state its visibility"
# The species are capabilities and needs, not bonuses.
grep -q 'capability:' "$WORK/crew.txt" || fail "no species capability was printed"
grep -q 'need:' "$WORK/crew.txt" || fail "no species need was printed"
# The manner reaches all three bands.
grep -q 'at breaking point' "$WORK/manner.txt" || fail "the manner did not reach the low band"
grep -q 'worn' "$WORK/manner.txt" || fail "the manner did not reach the middle band"

echo "PASS  three generated crew printed in full; morale broken into three components; every condition with source, cure and visibility; species as capabilities and needs; the manner across three levels"
