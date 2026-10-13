#!/usr/bin/env bash
# The evaluator -- what reads a scenario's breeding conditions and decides it fires (G4's second
# half; docs/scenario-manifest.md, the evaluator; docs/evidence/the-evaluator.md).
#
#   scripts/evaluator-check.sh
#
# Part one -- the decision, both ways, quoted. The worked example does not fire on a fresh ship and
# names its nearest miss, with the ship's own number; given the state its conditions ask for it fires,
# and names the condition that fired it.
#
# Part two -- the worked example end to end: recruited, instantiated (the systems it stresses, the
# staff it needs, every position and line resolved from its post to a person), its decision resolved,
# its cost applied and its residue written.
#
# Part three -- the post-not-name law holds *at runtime*, across rosters: the same content recruits
# under the canon crew and under an all-fictitious one, and each line resolves to whoever holds the
# post.
#
# Part four -- the falsification. A condition true for every value (a tautology) and one true for no
# value (a contradiction) are named as degenerate, not treated as ordinary; and a planted evaluator
# that drops the distinction must fail this check. Restore.
#
# Part five -- what no state carries: the scoped states the condition shape cannot pin, and the design
# facts no manifest key reaches at all. And the save: the ledger round-trips, and an older save does
# not load.
#
# Static host: no engine and no game data, so the rendered surfaces are the playtest host's and are
# named as such in the evidence; the evaluator's own output is asserted here.

set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

CXX="${CXX:-g++}"
WORK="$(mktemp -d)"
trap 'rm -rf "${WORK:?}"' EXIT

SCEN="scenarios/deck11-gel-pack"
fail() { echo "FAIL  $1" >&2; exit 1; }

echo "==> evaluator: build the ship, the loader and the evaluator"
"$CXX" -std=c++17 -g -I module/ship -I module/ui \
  tests/ship/test_ship_core.cpp module/ship/ship_core.cpp module/ship/scenario_core.cpp \
  -o "$WORK/test_ship_core"
"$WORK/test_ship_core" >/dev/null 2>&1 || fail "the ship unit suite failed"

echo
echo "==> evaluator: the worked example validates and compiles"
python3 tools/validator/validate.py "$SCEN" >"$WORK/validate.txt" 2>&1 || { cat "$WORK/validate.txt"; fail "the worked example was refused"; }
grep -q "0 error(s)" "$WORK/validate.txt" || { cat "$WORK/validate.txt"; fail "the worked example did not validate clean"; }
python3 tools/scenario/scenario_build.py "$SCEN" >/dev/null
FLAT="$SCEN/build/maps/deck11-gel-pack.scenario"
[ -f "$FLAT" ] || fail "the builder wrote no flat declaration"

echo
echo "==> evaluator: it decides, both ways, and instantiates end to end"
"$WORK/test_ship_core" --recruit "$FLAT" >"$WORK/recruit.txt" 2>&1 || { cat "$WORK/recruit.txt"; fail "the evaluator refused the built declaration"; }
cat "$WORK/recruit.txt"
assert_line() { grep -qF "$1" "$WORK/recruit.txt" || { cat "$WORK/recruit.txt"; fail "the transcript lacks: $1"; }; }

# One: the fresh ship does not fire, and the nearest miss is quoted with the ship's own number.
assert_line 'fresh ship: does not fire: nearest miss system_condition < 0.7 -- now 1 (short by 0.3)'
# Two: given the state it asks for, it fires and names the condition that fired it.
assert_line 'it decides: fires: patients >= 1 -- now 1'

# The instantiation: the systems, the staff, the warmth, and every line resolved from its post.
assert_line 'system replicators: condition 0.60 -> 0.30'
assert_line 'system holodecks: off'
assert_line 'staff: 3 post(s) required; the ship needs 22 posts fully manned'
assert_line 'order to the engineering seat ('
assert_line 'condolence to the medical seat ('

# The decision resolved: its cost applied and its residue written.
assert_line 'resolve "deck11-gel-pack": isolate the forward grid and treat the carriers'
assert_line 'effect fabricate_parts 2'
assert_line "residue: one deck's upkeep gone quiet"

# The post-not-name law at runtime, across rosters.
assert_line 'order to the engineering seat (B'\''Elanna Torres)'
assert_line 'order to the engineering seat (Crewman '
assert_line 'condolence to the medical seat (Crewman '

# The save: the ledger round-trips, and an older save does not load.
assert_line 'the ledger: deck11-gel-pack(answer)'
assert_line 'round-trips the ledger: yes'
assert_line 'an older save (v64) loads: no (refused)'

# What no state carries: the scoped states, and the design facts no manifest key reaches at all.
assert_line 'declared but not answerable as declared (no subject in the condition shape): system_condition deck_life_support minutes_of_air'
assert_line 'the design implies, and no manifest key reaches at all: contact in_battle beacon breach structural_integrity clock'
echo
echo "PASS  it decides both ways and instantiates the worked example, from post to person, in flight and saved"

echo
echo "==> evaluator: the falsification -- a tautology and a contradiction are named, not ordinary"
assert_line 'degenerate, and named (not ordinary): deuterium >= 0 -> always; deuterium > 1 -> never'
assert_line 'a state no reader carries cannot recruit: nebula_contact >= 1 -> unreadable (nebula_contact)'

# A planted evaluator that drops the distinction must fail the assertion above.
mkdir -p "$WORK/shipmod"
cp module/ship/scenario_core.cpp "$WORK/shipmod/scenario_core.cpp"
sed -i 's/r\.outcome = COND_ALWAYS;/r.outcome = COND_MET;/; s/r\.outcome = COND_NEVER;/r.outcome = COND_UNMET;/' "$WORK/shipmod/scenario_core.cpp"
grep -q 'r.outcome = COND_MET;' "$WORK/shipmod/scenario_core.cpp" || fail "the falsification plant did not take"
"$CXX" -std=c++17 -g -I module/ship -I module/ui \
  tests/ship/test_ship_core.cpp module/ship/ship_core.cpp "$WORK/shipmod/scenario_core.cpp" \
  -o "$WORK/test_flawed"
"$WORK/test_flawed" --recruit "$FLAT" >"$WORK/flawed.txt" 2>&1 || true
if grep -qF 'degenerate, and named (not ordinary): deuterium >= 0 -> always; deuterium > 1 -> never' "$WORK/flawed.txt"; then
  fail "the falsification did not fail: a planted evaluator still treated a tautology as ordinary"
fi
echo "PASS  the planted evaluator fails the degeneracy assertion, so the check has teeth (restored)"
echo
echo "all evaluator checks passed"
