#!/usr/bin/env bash
# Damage control is three tiers (C-12; the owner's ruling 2026-10-12, in his own words: "There should
# be standing repair team ready to dispatch, and more egregious repairs will take more people, which
# can take them off of their stations." docs/crew-work.md, docs/ship-systems.md,
# docs/designs-against-code.md).
#
#   scripts/damage-control-check.sh
#
# Part one -- the demonstration. The unit test suite runs, and `test_ship_core --damage-control`
# prints the three tiers as evidence (docs/evidence/the-damage-control-party.md): the standing party
# answering a dispatch; severity scaling the hands with the sizes shown; a job past the threshold
# drawing a post-holder off a station, with the station's emptiness and the cost of it in the
# simulation's own numbers; the party's departments named from the roster's nine; and the save's
# cost (none: the party is derived every tick). The lines asserted here are the acceptance, so a
# silent regression fails the check.
#
# Part two -- the falsification, the shape the brief names: force every repair down the first tier,
# with no scaling and no poaching, and require the test to FAIL on the three tiers; then restore.
# A guard that cannot fail for the reason we care about is decoration.
#
# Static host: no engine and no game data; the model is exercised directly, so this runs anywhere a
# compiler does.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

CXX="${CXX:-g++}"
WORK="$(mktemp -d)"
trap 'rm -rf "${WORK:?}"' EXIT

fail() { echo "FAIL  $1" >&2; exit 1; }

echo "==> damage control: the unit test suite, built here"
"$CXX" -std=c++17 -g -I module/ship -I module/ui \
  tests/ship/test_ship_core.cpp module/ship/ship_core.cpp module/ship/scenario_core.cpp -o "$WORK/test_ship_core"
"$WORK/test_ship_core" >/dev/null

echo
echo "==> damage control: the demonstration (test_ship_core --damage-control)"
"$WORK/test_ship_core" --damage-control | tee "$WORK/dc.txt"
echo

# The axis and the threshold, stated as numbers.
grep -q '^  severity is the system.s damage (1 - health), 0..1; the threshold REPAIR_SEVERITY_LARGE is 0.50$' "$WORK/dc.txt" \
  || fail "the severity axis or the threshold is not stated"
grep -q '^  the standing party as it stands (REPAIR_TEAM_MAX) is 3; a wholly destroyed system takes REPAIR_PARTY_LARGE 6$' "$WORK/dc.txt" \
  || fail "the party sizes are not the standing 3 and the large 6"
# Tier two: the sizes, as a scale, not one number.
grep -q '^  RepairPartySize(severity): 0.25 -> 3, 0.50 -> 3, 0.60 -> 4, 0.75 -> 5, 0.90 -> 6, 1.00 -> 6$' "$WORK/dc.txt" \
  || fail "severity does not scale the party as designed"
# Tier one: the standing party answers a dispatched repair.
grep -q '^  tier one: dispatched to the sensors, the standing party answers: 3 hands' "$WORK/dc.txt" \
  || fail "the standing party did not answer the dispatch"
# Tier two, observed on a real job.
grep -q '^  tier two: the sensors severed to severity 0.90 take 6 hands (the standing party.s 3, plus 3 off their posts)$' "$WORK/dc.txt" \
  || fail "a severe job did not take more hands than the standing party"
# Tier three: a post-holder off their station, and the station's emptiness and its cost in numbers.
grep -q '^  tier three: the party -- sensors 6 hands (Engineering 3, ' "$WORK/dc.txt" \
  || fail "the party line does not name the standing party and the drawn hands"
grep -q 'left the transporters: manned 1 -> 0 of 1, output 1.00 -> 0.50' "$WORK/dc.txt" \
  || fail "the emptied station's fall in manning and output is not shown"
# The departments: the party names its people through the nine, quoted.
grep -q '^  the departments (the roster.s nine), quoted: .*Engineering' "$WORK/dc.txt" \
  || fail "the party does not name its people by the roster's nine"
grep -q 'off their posts' "$WORK/dc.txt" \
  || fail "the party does not show which hands were drawn off their stations"
# The save: the format did not move, and the party is derived, not stored.
grep -qE '^  save: format [0-9]+ \(unchanged by this pass\), the blob is [0-9]+ bytes and loads 1; the party is re-derived, not stored$' "$WORK/dc.txt" \
  || fail "the save format moved, or the blob did not round-trip with a party standing"

echo
echo "==> damage control: the falsification (every repair down the first tier; the test must fail)"
cp -r module/ship "$WORK/shipmod"
# Plant: no job is ever large -- the threshold is raised past every severity, so there is no scaling
# and no poaching, and every repair is the standing party alone.
sed -i 's/const float REPAIR_SEVERITY_LARGE = 0.5f;/const float REPAIR_SEVERITY_LARGE = 1.1f;/' "$WORK/shipmod/ship_core.h"
grep -q 'REPAIR_SEVERITY_LARGE = 1.1f;' "$WORK/shipmod/ship_core.h" || fail "the threshold plant did not take"
"$CXX" -std=c++17 -g -I "$WORK/shipmod" -I module/ui \
  tests/ship/test_ship_core.cpp "$WORK/shipmod/ship_core.cpp" module/ship/scenario_core.cpp -o "$WORK/test_broken"
if "$WORK/test_broken" >"$WORK/broken.txt" 2>&1; then
  echo "FAIL  the test passed with every repair forced down the first tier: the check has no teeth" >&2
  exit 1
fi
grep -q 'FAIL  damage control: the three tiers' "$WORK/broken.txt" \
  || fail "the broken copy failed for the wrong reason"
sed -n 's/^/      /p' "$WORK/broken.txt" | grep 'FAIL  damage control: the three tiers' || true
echo "    the broken copy fails on the three tiers; the tree passes"

echo
echo "PASS  the standing party answers a dispatch, severity scales the hands with the sizes shown, a"
echo "      job past the threshold draws a post-holder off a station and the station's fall is in the"
echo "      model's own numbers, the party names its people from the roster's nine, the save did not"
echo "      move, and the guard fails on a planted break"
