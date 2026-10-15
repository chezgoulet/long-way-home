#!/usr/bin/env bash
# The shuttle load screen (O9; docs/shuttles.md step 2): the manifest, the loadout, the destination
# with its chart confidence, and the risk, decided on the screen and reflected in the run.
#
#   scripts/launch-check.sh
#
# One headless run (g_shipTest 103) on deck 4. The screen's own key commands drive it, so what the
# test presses is what a hand presses; the state read back is what the ship published (lwh_ship_launch*)
# and what the model then holds. The screen renders, and its own pixels carry the decisions.
# Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$HOME_DIR/launch.out"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

fail() { echo "FAIL  $1"; exit 1; }
pass() { echo "PASS  $1"; }

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> the shuttle load screen"
SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipTest 103 +map tour/deck04 >"$OUT" 2>&1 || true
grep -hE 'launch test:' "$OUT" | sed 's/^EFSP: /    /' || true

# The load screen carries the decision: the bay, the destinations with their confidence and risk, the
# loadout that leaves stores, and the ways back. Every one is published by the ship.
grep -q 'launch test: the bay reads "0|Class 2|100|in the bay;' "$OUT" \
  || fail "the bay and its shuttles are not published to the load screen"
grep -qE 'launch test: destinations "[0-9]+\|(CHARTED|SURVEYED|UNCHARTED);' "$OUT" \
  || fail "the destinations carry no chart confidence"
grep -qE 'launch test: the risk table "[0-9]+\|[0-9]+\|(LOW|MODERATE|HIGH);' "$OUT" \
  || fail "no launch risk is published"
grep -qE 'launch test: the loadout "[0-9]+ material, [0-9]+ rations, [0-9]+ parts"' "$OUT" \
  || fail "the loadout out of stores is not published"
grep -q 'launch test: the ways back "0|a transporter window;1|a rendezvous;2|its own power;"' "$OUT" \
  || fail "the ways a shuttle comes back are not published"
pass "the load screen carries the manifest, the loadout, the destination and the risk"

# A real choice: the run must be at a beacon with more than one way on, so the destination is decided
# and not the only option.
DESTS="$(grep -o 'destinations "[^"]*"' "$OUT" | sed 's/^[^"]*"//; s/"$//')"
FIRST="$(printf '%s' "$DESTS" | cut -d';' -f1 | cut -d'|' -f1)"
SECOND="$(printf '%s' "$DESTS" | cut -d';' -f2 | cut -d'|' -f1)"
if [ -z "$FIRST" ] || [ -z "$SECOND" ] || [ "$FIRST" = "$SECOND" ]; then
  fail "the screen offered no second destination to choose (destinations \"$DESTS\")"
fi

# The decisions reach the run: the chosen shuttle (Type 6, not the Class 2 the cursor started on), the
# chosen destination (the second, not the first), the manifest, and the return condition.
grep -q "launch test: the screen put Type 6 away, beacon $SECOND, 2 aboard, back by a rendezvous" "$OUT" \
  || fail "the run does not reflect the screen's choice (Type 6, beacon $SECOND, 2 aboard, a rendezvous)"
grep -q "launch test: the log: Type 6 launches for beacon $SECOND with 2 aboard" "$OUT" \
  || fail "the launch is not in the record"
pass "the screen's choice is the run: the shuttle, the destination, the manifest and the return"
pass "the destination was a choice: the screen sent the shuttle to beacon $SECOND, not $FIRST"

# The screen rendered, and its own pixels carry the decisions (the picture, not the path).
for shot in lwh_launch lwh_launch_set; do
  [ -f "$GAME_DIR/screenshots/$shot.tga" ] || fail "no screenshot of $shot"
  echo "      $GAME_DIR/screenshots/$shot.tga"
done
python3 "$ROOT/tools/frames/frame.py" diff --min-changed 0.01 --require \
  "$GAME_DIR/screenshots/lwh_launch.tga" "$GAME_DIR/screenshots/lwh_launch_set.tga" \
  || fail "the load screen frame does not change when the manifest and the return are set"
pass "the load screen rendered, and its own pixels carry the decisions"

echo
pass "the shuttle load screen decides on the screen and the run reflects the choice"
echo "      render: build/g3-home/baseEF/screenshots/lwh_launch.tga (and ..._set.tga),"
echo "              1280x1024; the owner judges whether it reads as a pre-flight check"
echo "      evidence: docs/evidence/the-load-screen.md"
