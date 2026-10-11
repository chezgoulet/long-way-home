#!/usr/bin/env bash
# The crew's manner (G14's second half; docs/character-derivation.md, section 7): the selection from
# a person's state, the wiring that raises it where the state changes and where they act, the
# surface the player reads, the absence case, and the save. No game data and no engine: the
# engine-independent model is exercised directly, so this runs anywhere a compiler does.
#
#   scripts/manner-check.sh
#
# The unit tests themselves are in scripts/test.sh (tests/ship); this prints the selection for the
# owner and checks its own criteria, and plants the state before this pass to show the check can
# fail (the falsification).

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
WORK="$(mktemp -d)"
trap 'rm -rf "${WORK:?}"' EXIT

cmake -S tests/ship -B "$WORK/ship" -DCMAKE_BUILD_TYPE=Debug >/dev/null
cmake --build "$WORK/ship" -j"$(nproc)" >/dev/null

echo "==> the manner selected from the state, wired, surfaced, and degrading without audio"
"$WORK/ship/test_ship_core" --manners | tee "$WORK/wired.txt"
echo
echo "==> the falsification: the selection reading the morale band alone (the state before the wiring)"
"$WORK/ship/test_ship_core" --manners-broken | tee "$WORK/broken.txt"
echo

fail() { echo "FAIL  $1" >&2; exit 1; }
W="$WORK/wired.txt"
B="$WORK/broken.txt"

# The selection, for the good one and the four bad, each chosen by its own state.
grep -q 'the selection is wired: the whole state' "$W" || fail "the selection did not report itself wired"
grep -qE '^  steady +-> steady' "$W"   || fail "the good state did not select the steady manner"
grep -qE '^  tired +-> tired' "$W"     || fail "exhaustion did not select the tired manner"
grep -qE '^  afraid +-> afraid' "$W"   || fail "fear did not select the afraid manner"
grep -qE '^  grieving +-> grieving' "$W" || fail "grief did not select the grieving manner"
grep -qE '^  hurt +-> hurt' "$W"       || fail "injury did not select the hurt manner"
grep -qE '^  hungry +-> hungry' "$W"   || fail "hunger did not select the hungry manner"
grep -qE '^  breathless +-> breathless' "$W" || fail "a deck without air did not select the breathless manner"

# Each manner names the state that chose it, so the selection is explainable.
grep -q 'why: exhausted:' "$W" || fail "the tired manner did not name the state that chose it"
grep -q 'why: afraid:' "$W"    || fail "the afraid manner did not name the state that chose it"
grep -q 'why: grieving:' "$W"  || fail "the grieving manner did not name the state that chose it"
grep -q 'why: injured' "$W"    || fail "the hurt manner did not name the state that chose it"

# Several at once: the precedence picks one and names the rest.
grep -q 'several at once: grieving' "$W" || fail "the precedence did not pick the highest state"
grep -q 'also afraid, exhausted' "$W"  || fail "the precedence did not name the states it beat"

# The wiring: raised at the state change, and where the person acts.
grep -q 'raised by a state change' "$W" || fail "a recorded condition did not raise the manner"
grep -q 'raised by an order' "$W"       || fail "an order did not raise the manner where the person acts"

# The surface, and the absence case.
grep -q 'surface: .*, voice .*, delivery .*: "' "$W" || fail "the cue did not carry speaker, voice and delivery"
grep -q 'the cue: spoken' "$W"               || fail "with a bank and a render the cue was not spoken"
grep -q 'no bank: text (no bank)' "$W"       || fail "with no bank the cue did not degrade to text"
grep -q 'no render: text (no render)' "$W"   || fail "with no render the cue did not degrade to text"

# Save and determinism.
grep -q 'same state, same manner twice: yes' "$W" || fail "the same state did not select the same manner twice"
grep -q 'a save replays the manner: yes' "$W"     || fail "a save did not replay the manner"

# The falsification: with the selection reading the morale band alone, every bad state collapses to
# the band's manner -- and the check above can see it. A check that cannot fail is decoration.
grep -q 'the selection is NOT wired' "$B" || fail "the planted (unwired) run did not report itself"
grep -qE '^  tired +-> steady' "$B"       || fail "the planted run did not collapse the bad states to the band"
if grep -qE -- '-> tired$' "$B"; then fail "with the selection unwired the bad states still selected their own manner"; fi
if grep -qE -- '-> grieving$' "$B"; then fail "with the selection unwired grief still selected its own manner"; fi

echo "PASS  the manner is selected from the state (steady and the bad states, each named, with the"
echo "      precedence when several are true); raised where the state changes and where the person"
echo "      acts; written to the log and handed to the voice; degrades to text with no bank or no"
echo "      render; the same state selects the same manner and a save replays it -- and the planted"
echo "      state before this pass makes the check fail."
