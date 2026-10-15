#!/usr/bin/env bash
# A deck keeps a residue -- what the place permanently is now (slate 8; docs/scenario-atlas.md,
# docs/evidence/the-deck-residue.md).
#
#   scripts/deck-residue-check.sh
#
# The state: a deck the ship gives up keeps the *place* -- sealed, stripped, uninhabitable or written
# off -- distinct from the live state beside it (hull, atmosphere), which can be restored. It is set by
# a write-off and by a fired scenario's residue, and it survives a save.
#
# The guard is exercised BOTH directions: it passes on the tree, and it fails on a plant that drops the
# residue the write-off sets -- a guard that cannot fail for the reason we care about is decoration.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

CXX="${CXX:-g++}"
WORK="$(mktemp -d)"
trap 'rm -rf "${WORK:?}"' EXIT
fail() { echo "FAIL  $1" >&2; exit 1; }

echo "==> deck residue: build the suite (the ship core and the scenario loader)"
"$CXX" -std=c++17 -g -I module/ship -I module/ui \
  tests/ship/test_ship_core.cpp module/ship/ship_core.cpp module/ship/scenario_core.cpp \
  -o "$WORK/test_ship_core"
"$WORK/test_ship_core" >/dev/null

echo
echo "==> deck residue: a deck given up keeps the place, and it survives a save"
"$WORK/test_ship_core" --residue | tee "$WORK/residue.txt"
grep -q "deck 8: residue 'sealed'" "$WORK/residue.txt" || fail "a sealed deck did not keep its residue"
grep -q "deck 12: residue 'written off'" "$WORK/residue.txt" || fail "a written-off deck did not keep it"
grep -q "reloaded: deck 8 'sealed', deck 12 'written off'" "$WORK/residue.txt" \
  || fail "the residue did not survive a save and a load"
echo "PASS  the place is kept, and the residue survives a save"

echo
echo "==> deck residue: the falsification -- drop the residue the write-off sets"
mkdir -p "$WORK/shipmod"
cp module/ship/ship_core.cpp "$WORK/shipmod/ship_core.cpp"
# The plant: the write-off no longer marks the place, so a deck given up keeps nothing.
sed -i 's/s\.decks\[target - 1\]\.residue = res;/\/\/ planted: no residue (deck-residue-check.sh)/' \
  "$WORK/shipmod/ship_core.cpp"
grep -q "planted: no residue" "$WORK/shipmod/ship_core.cpp" || fail "the plant did not take"
"$CXX" -std=c++17 -g -I module/ship -I module/ui \
  tests/ship/test_ship_core.cpp "$WORK/shipmod/ship_core.cpp" module/ship/scenario_core.cpp \
  -o "$WORK/test_broken"
if "$WORK/test_broken" --residue >/dev/null 2>&1; then
  fail "the plant passed: the residue check has no teeth"
fi
echo "PASS  the planted break fails the check, so the check has teeth (restored)"
echo
echo "all deck-residue checks passed"
