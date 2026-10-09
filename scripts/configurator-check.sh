#!/usr/bin/env bash
# The configurator (docs/the-entry-point.md, part three; docs/start-states.md): the player chooses
# the start state, and the three proving cases are shown in full. No game data and no engine -- the
# engine-independent model is exercised directly, so this runs anywhere a compiler does.
#
#   scripts/configurator-check.sh
#
# The unit tests themselves are in scripts/test.sh (tests/ship); this prints and checks the
# acceptance the owner judges: that the canon default is untouched, that the captain's chair is
# vacant and derived, and that an all-fictitious crew names no show character anywhere.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
WORK="$(mktemp -d)"
trap 'rm -rf "${WORK:?}"' EXIT

cmake -S tests/ship -B "$WORK/ship" -DCMAKE_BUILD_TYPE=Debug >/dev/null
cmake --build "$WORK/ship" -j"$(nproc)" >/dev/null

echo "==> the configurator: the three proving cases"
"$WORK/ship/test_ship_core" --starts | tee "$WORK/starts.txt"
echo

fail() { echo "FAIL  $1" >&2; exit 1; }

# The three cases are present, and the configurator opens on the canon default.
grep -q '\[0\] CANON' "$WORK/starts.txt" || fail "the canon default is not the first entry"
grep -q '\[1\] THE CHAIR' "$WORK/starts.txt" || fail "the captain case is missing"
grep -q '\[2\] ALL-FICTITIOUS' "$WORK/starts.txt" || fail "the all-fictitious case is missing"
# The canon default: the chain is intact and nobody died.
grep -q 'seat: the chair                = Kathryn Janeway' "$WORK/starts.txt" || fail "the canon chain is not intact"
grep -q 'no one in the command crew was lost' "$WORK/starts.txt" || fail "the canon log seed does not state the losses"
# The captain: the chair is vacant, derived, and the record names the casualty.
grep -q 'seat: the chair                = Reyes  (derived: the named holder was lost)' "$WORK/starts.txt" \
  || fail "the chair was not derived from a casualty"
grep -q 'lost with her: Kathryn Janeway (the chair)' "$WORK/starts.txt" \
  || fail "the record does not name the casualty that created the vacancy"
# The all-fictitious case: no show character anywhere, and the derivation filled the whole chain.
grep -q 'roster: generated entirely -- no show character appears' "$WORK/starts.txt" || fail "the fictitious roster was not generated"
for name in Janeway Chakotay Tuvok Torres Seven Chakotay Kim Paris; do
  grep -q "$name" "$WORK/starts.txt" || continue
  # the name may appear only as the casualty in the canon/chair cases, never in the fictitious block
  awk '/\[2\] ALL-FICTITIOUS/{f=1} f' "$WORK/starts.txt" | grep -q "$name" \
    && fail "a show character ($name) appears in the all-fictitious case"
done
# The log seed describes what was chosen: the losses, the condition, and that no rescue is coming.
grep -c 'No rescue is coming' "$WORK/starts.txt" | grep -q '^3$' || fail "not every case's log seed says no rescue is coming"
# The player's own position is stated: rank, post, what they may authorise, who reports to them.
grep -q 'in command: the ship.s orders are theirs to give' "$WORK/starts.txt" || fail "the captain's authority is not stated"
grep -q 'may authorise their station.s work' "$WORK/starts.txt" || fail "the junior's authority is not stated"

echo "PASS  the canon default is intact; the chair is vacant and derived from the casualty the record names;"
echo "      the all-fictitious crew names no show character anywhere; every log seed states no rescue is coming"
