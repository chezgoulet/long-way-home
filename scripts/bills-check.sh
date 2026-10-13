#!/usr/bin/env bash
# The bills -- who does what when it goes wrong (docs/crew-manifest.md; the finding O6 in
# docs/omissions.md).
#
#   scripts/bills-check.sh
#
# Part one -- the demonstration. The unit test suite runs, and `test_ship_core --bills` prints the
# bills as evidence (`docs/evidence/the-bills.md`): the census, each bill called and observed with
# the crew's recorded state changing, and the officer of the deck's authority demonstrated both
# ways. The lines asserted here are the acceptance, so a silent regression fails the check.
#
# Part two -- the falsification. The same test is compiled against a broken copy of ship_core.cpp in
# which the battle bill's effect is disabled, and must FAIL. A guard that cannot fail for the reason
# we care about is decoration.
#
# Static host: no engine, no game data. The rendered frame of ui_lwh_bills is the playtest host's
# and is named as such in the evidence.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

CXX="${CXX:-g++}"
WORK="$(mktemp -d)"
trap 'rm -rf "${WORK:?}"' EXIT

echo "==> bills: the unit test suite, built here"
"$CXX" -std=c++17 -g -I module/ship -I module/ui \
  tests/ship/test_ship_core.cpp module/ship/ship_core.cpp module/ship/scenario_core.cpp -o "$WORK/test_ship_core"
"$WORK/test_ship_core" >/dev/null

echo
echo "==> bills: the demonstration (test_ship_core --bills)"
"$WORK/test_ship_core" --bills | tee "$WORK/bills.txt"
echo

fail() { echo "FAIL  $1" >&2; exit 1; }

# The census: the watch bill is the roster, the battle bill is grouped by role, and the watch's two
# named posts are read.
grep -q '^  ALPHA WATCH 0800-1600: .* on duty, .* at a post, .* off watch' "$WORK/bills.txt" \
  || fail "the watch bill is not read"
grep -q '^  battle bill by role: own post [0-9]*; damage control [0-9]*;' "$WORK/bills.txt" \
  || fail "the battle bill is not grouped by role"
grep -q '^  engineer of the watch: ' "$WORK/bills.txt" || fail "no engineer of the watch"
grep -q '^  officer of the deck: ' "$WORK/bills.txt" || fail "no officer of the deck"
grep -q '^  with all but one transporter hand lost: [1-9][0-9]* hole(s): transporters rests on one hand' "$WORK/bills.txt" \
  || fail "the holes read did not name the post loss leaves on one hand"

# Battle stations: called (condition red, hands off their posts and onto the repair parties), then
# stood down (the routine returns). The warp drive's own hands must fall and come back.
grep -q '^  before: warp drive manned 3, condition 0, battle 0' "$WORK/bills.txt" || fail "the before state is wrong"
grep -q '^  called: condition 2, battle 1; warp drive manned 0, sensors repairing [1-9]' "$WORK/bills.txt" \
  || fail "battle stations did not move the hands to their battle stations"
grep -q '^  stood down: condition 0, battle 0; warp drive manned 3' "$WORK/bills.txt" \
  || fail "standing down did not return the watch"

# The emergency bills: the fire clears its deck; abandon ship musters the crew.
grep -q '^  fire on deck 9: [0-9]* were on it, 0 non-party hand(s) left; the fire bill: damage control fights it on deck 9' "$WORK/bills.txt" \
  || fail "the fire bill did not clear its deck"
grep -q '^  abandon ship: 1[0-9][0-9] of 141 fit hands muster at the shuttlebay (deck 10)' "$WORK/bills.txt" \
  || fail "abandon ship did not muster the crew"

# The officer of the deck: authority over a senior officer who is not the captain, and the negative
# case; and authority kept apart from access.
grep -q 'may order Tuvok (rank 4): yes' "$WORK/bills.txt" \
  || fail "the deck did not order a senior officer"
grep -q 'may order the captain Kathryn Janeway: the captain is not the deck.s to order' "$WORK/bills.txt" \
  || fail "the deck was not refused the captain"
grep -q 'but the deck cannot operate the engine room: MayOperate 0' "$WORK/bills.txt" \
  || fail "authority was not kept apart from access"

echo "==> bills: the falsification (the battle bill's effect disabled, the test must fail)"
cp -r module/ship "$WORK/shipmod"
# Break it: the battle bill no longer reconfigures the watch.
sed -i 's/if (a == ACT_ON_DUTY && s.battleStations)/if (false)/' "$WORK/shipmod/ship_core.cpp"
grep -q 'if (false)' "$WORK/shipmod/ship_core.cpp" || fail "the plant did not take"
"$CXX" -std=c++17 -g -I "$WORK/shipmod" -I module/ui \
  tests/ship/test_ship_core.cpp "$WORK/shipmod/ship_core.cpp" module/ship/scenario_core.cpp -o "$WORK/test_broken"
if "$WORK/test_broken" >"$WORK/broken.txt" 2>&1; then
  echo "FAIL  the test passed on a broken core: the check has no teeth" >&2
  exit 1
fi
grep -q 'FAIL  the bills: called, observed, stood down' "$WORK/broken.txt" \
  || fail "the broken copy failed for the wrong reason"
grep -q 'FAIL  the bills: called, observed, stood down' "$WORK/broken.txt" && \
  sed -n 's/^/      /p' "$WORK/broken.txt" | grep 'FAIL  the bills' || true
echo "    the broken copy fails on the bills; the tree passes"

echo
echo "PASS  the watch bill reads, battle stations call and stand down with the hands moving, the"
echo "      emergency bills clear a deck and muster the crew, the holes appear under loss, and the"
echo "      deck orders a senior officer but not the captain; the guard fails on a planted break"
