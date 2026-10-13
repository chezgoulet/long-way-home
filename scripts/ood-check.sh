#!/usr/bin/env bash
# The officer of the deck binds -- the consult, and the refusal's ground (C-11, docs/orders.md,
# docs/access-and-authority.md).
#
#   scripts/ood-check.sh
#
# Part one -- the demonstration. `test_ship_core --ood` prints the deck's line as evidence
# (`docs/evidence/the-ood-binds.md`): the deck orders a senior officer it could not reach by rank, so
# the consult binds; and the deck is refused the captain *with the deck's own ground*, not a generic
# denial, with the ship unchanged. The lines asserted here are the acceptance, so a silent regression
# fails the check.
#
# Part two -- the falsification. The same test is compiled against a broken copy of ship_core.cpp in
# which the deck's authority is unconditioned -- "the consult is always true" -- and must FAIL on the
# refusal leg (the order that should have been refused is given). A guard that cannot fail for the
# reason we care about is decoration.
#
# Static host: no engine, no game data. The console verb `ship order give` reads the same refusal
# (`g_ship.cpp`), but it was not driven through the engine here; that is named in the evidence.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

CXX="${CXX:-g++}"
WORK="$(mktemp -d)"
trap 'rm -rf "${WORK:?}"' EXIT

echo "==> ood: the unit test suite, built here"
"$CXX" -std=c++17 -g -I module/ship -I module/ui \
  tests/ship/test_ship_core.cpp module/ship/ship_core.cpp module/ship/scenario_core.cpp -o "$WORK/test_ship_core"
"$WORK/test_ship_core" >/dev/null

echo
echo "==> ood: the demonstration (test_ship_core --ood)"
"$WORK/test_ship_core" --ood | tee "$WORK/ood.txt"
echo

fail() { echo "FAIL  $1" >&2; exit 1; }

# The line: the deck may order a senior who is not the captain, may not order the captain, and its
# authority is not access (MayOperate is 0).
grep -q '^  the deck is .* (rank [0-9]*); MayOperate(engineering) 0' "$WORK/ood.txt" \
  || fail "the deck's authority was not kept apart from access"
grep -q '^  may order Tuvok (rank 4): yes' "$WORK/ood.txt" \
  || fail "the deck did not order a senior officer"
grep -q '^  may order the captain .*: the captain is not the deck.s to order' "$WORK/ood.txt" \
  || fail "the deck was not refused the captain"

# The consult binds: the deck's order to a senior is dispatched.
grep -q '^  the deck -> Tuvok: given; orderRepairFirst [0-9][0-9]*' "$WORK/ood.txt" \
  || fail "the consult did not bind: the deck's order to a senior was not dispatched"

# The refusal's ground: the deck's order to the captain is refused, in the deck's own words.
grep -q '^  the deck -> the captain: refused' "$WORK/ood.txt" \
  || fail "the deck's order to the captain was not refused"
grep -q '^  the rule: "the captain is not the deck.s to order"' "$WORK/ood.txt" \
  || fail "the refusal did not name the deck's own ground"
grep -q '^  the rule is the deck.s own words: yes' "$WORK/ood.txt" \
  || fail "the refusal is not OodRefusal's own words"
grep -q '^  the ship is unchanged: yes; nobody was given it: 0' "$WORK/ood.txt" \
  || fail "the refused order changed the ship, or was given anyway"

echo "==> ood: the falsification (the consult always true, the test must fail)"
cp -r module/ship "$WORK/shipmod"
# Break it: the deck's order is never checked against OodMayOrder -- the consult is always true.
sed -i 's/if (giver == OfficerOfTheDeck(s) && OodMayOrder(s, giver, recipient)) return true;/if (giver == OfficerOfTheDeck(s)) return true;/' "$WORK/shipmod/ship_core.cpp"
grep -q 'if (giver == OfficerOfTheDeck(s)) return true;' "$WORK/shipmod/ship_core.cpp" \
  || fail "the plant did not take"
"$CXX" -std=c++17 -g -I "$WORK/shipmod" -I module/ui \
  tests/ship/test_ship_core.cpp "$WORK/shipmod/ship_core.cpp" module/ship/scenario_core.cpp -o "$WORK/test_broken"
if "$WORK/test_broken" >"$WORK/broken.txt" 2>&1; then
  echo "FAIL  the test passed on a broken core: the check has no teeth" >&2
  exit 1
fi
grep -q "FAIL  the deck's refusal names its ground" "$WORK/broken.txt" \
  || fail "the broken copy failed for the wrong reason"
sed -n 's/^/      /p' "$WORK/broken.txt" | grep "FAIL  the deck's refusal names its ground" || true
echo "    the broken copy fails on the refusal leg; the tree passes"

echo
echo "PASS  the consult binds -- the deck orders a senior it cannot reach by rank -- and the refusal"
echo "      names the deck's own ground, leaves the ship unchanged, and the guard fails on a planted"
echo "      always-true consult"
