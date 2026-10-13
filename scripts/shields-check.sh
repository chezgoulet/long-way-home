#!/usr/bin/env bash
# The shields are canon's six arcs (C-14; the owner's ruling 2026-10-12: "BUILD THE SET -- canon's
# six arcs"; docs/ship-systems.md, docs/designs-against-code.md).
#
#   scripts/shields-check.sh
#
# Part one -- the demonstration. The unit test suite runs, and `test_ship_core --shields` prints the
# set as evidence (docs/evidence/the-shield-sections.md): the six arcs named, a hit on one named arc
# with the other five unchanged, an unrouted hit and where it goes, the read-out the Tactical console
# draws, the whole-set gate, and the save's cost. The lines asserted here are the acceptance, so a
# silent regression fails the check. And the scalar is gone: the search below proves nothing reads it.
#
# Part two -- the falsification. The same test is compiled against a broken copy of ship_core.cpp in
# which the routing is broken -- a hit reaches every arc instead of one -- and must FAIL. A guard that
# cannot fail for the reason we care about is decoration.
#
# Static host: no engine, no game data, so the rendered console frame is the playtest host's and is
# named as such in the evidence; the read-out's own text is asserted here.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

CXX="${CXX:-g++}"
WORK="$(mktemp -d)"
trap 'rm -rf "${WORK:?}"' EXIT

echo "==> shields: the unit test suite, built here"
"$CXX" -std=c++17 -g -I module/ship -I module/ui \
  tests/ship/test_ship_core.cpp module/ship/ship_core.cpp module/ship/scenario_core.cpp -o "$WORK/test_ship_core"
"$WORK/test_ship_core" >/dev/null

echo
echo "==> shields: the demonstration (test_ship_core --shields)"
"$WORK/test_ship_core" --shields | tee "$WORK/shields.txt"
echo

fail() { echo "FAIL  $1" >&2; exit 1; }

# The six sections, named, in canon's order.
grep -q '^  the six sections (SHIELD_SECTION_COUNT 6): forward port starboard aft dorsal ventral$' "$WORK/shields.txt" \
  || fail "the six sections are not named in canon's order"
# The read-out a person sees: all six arcs, not one number.
grep -q '^  the read-out the Tactical console draws: SHIELDS  FWD 100%  PORT 100%  STBD 100%  AFT 100%  DORSAL 100%  VENTRAL 100%$' "$WORK/shields.txt" \
  || fail "the console read-out is not the six arcs"
# A hit on a named arc: the arc falls, the other five do not.
grep -q '^  a hit of 60% on the port arc: port 100% -> 40%, overflow 0%; the other five unchanged$' "$WORK/shields.txt" \
  || fail "a hit on a named arc did not leave the other five unchanged"
grep -q '^  after:  SHIELDS  FWD 100%  PORT 40%  STBD 100%  AFT 100%  DORSAL 100%  VENTRAL 100%$' "$WORK/shields.txt" \
  || fail "the read-out after the hit does not show the port arc holed"
# The unrouted hit, with where it goes.
grep -q '^  unrouted: hits 0, so a shot with no section named strikes the forward arc$' "$WORK/shields.txt" \
  || fail "an unrouted hit was not routed to the counter's arc"
grep -q '^  unrouted: hits 2, so the next such shot strikes the starboard arc$' "$WORK/shields.txt" \
  || fail "an unrouted hit did not follow the counter as it moved"
# The gate reads the whole set.
grep -q '^  all six arcs down: ShieldsDown 1' "$WORK/shields.txt" \
  || fail "the whole-set gate did not read every arc down"
# The save: the new format loads, and a save from before the set is refused. The current version is
# pinned only as "a number" -- the header owns it, and it has moved again since this lane (62) -- but
# the version-60 refusal is the tooth: a save before the six arcs must not load.
grep -qE '^  save: format [0-9]+, the blob is [0-9]+ bytes and loads 1; the same blob at version 60 loads 0$' "$WORK/shields.txt" \
  || fail "the save format did not move, or an older save was not refused"

# The scalar is gone: nothing in the module or the tests reads it.
if grep -rn 'shieldStrength' module/ tests/; then
  fail "the one scalar is still read somewhere above"
fi
echo "    the search proves the scalar is gone: no \`shieldStrength\` in module/ or tests/"

echo
echo "==> shields: the falsification (the routing broken so a hit reaches every arc; the test must fail)"
cp -r module/ship "$WORK/shipmod"
# Break it: the hit no longer routes to one arc -- it strips them all.
sed -i 's|s\.shields\[sec\] = was - held;|for (int i = 0; i < SHIELD_SECTION_COUNT; ++i) s.shields[i] = (s.shields[i] < held) ? 0.0f : s.shields[i] - held;|' \
  "$WORK/shipmod/ship_core.cpp"
grep -q 'for (int i = 0; i < SHIELD_SECTION_COUNT; ++i) s.shields\[i\] = (s.shields\[i\] < held)' "$WORK/shipmod/ship_core.cpp" \
  || fail "the plant did not take"
"$CXX" -std=c++17 -g -I "$WORK/shipmod" -I module/ui \
  tests/ship/test_ship_core.cpp "$WORK/shipmod/ship_core.cpp" module/ship/scenario_core.cpp -o "$WORK/test_broken"
if "$WORK/test_broken" >"$WORK/broken.txt" 2>&1; then
  echo "FAIL  the test passed on a broken core: the check has no teeth" >&2
  exit 1
fi
grep -q 'FAIL  shields are canon.s six arcs, and damage routes to one' "$WORK/broken.txt" \
  || fail "the broken copy failed for the wrong reason"
sed -n 's/^/      /p' "$WORK/broken.txt" | grep 'FAIL  shields are canon' || true
echo "    the broken copy fails on the shield routing; the tree passes"

echo
echo "PASS  canon's six arcs are the state, a hit routes to one and leaves the other five, an unrouted"
echo "      hit goes to the arc the counter names, the gate and the read-out are the whole set, the"
echo "      scalar is gone, and the guard fails on a planted break"
