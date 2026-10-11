#!/usr/bin/env bash
# S6: damage is visible in the world -- a damaged system sparks where it is worked.
#
#   scripts/damage-check.sh
#
# One headless run on the merged ship. The harness stands where the life-support station is in view
# (the place the crew work the system), damages the system, and takes two frames at that one
# vantage; the crew layer spawns the game's own fx_spark at the station. The frame must carry the
# spark: the assertion is a difference in the region the camera aims at, not a file that exists --
# on the old vantage (a wall) that region changed 0.0% (docs/evidence/the-frame-assertions.md).
# Needs the built ship (build-ship.sh) and the built engine + modules. Writes under build/g3-home.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
PK3="$ROOT/build/ship/out/longway_voyager.pk3"
OUT="$HOME_DIR/damage.out"
SHOT="$GAME_DIR/screenshots/lwh_damage.tga"
BEFORE="$GAME_DIR/screenshots/lwh_damage_before.tga"
[ -f "$PK3" ] || { echo "no merged ship at $PK3 -- run scripts/build-ship.sh" >&2; exit 1; }
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$GAME_DIR/screenshots"
cp "$PK3" "$GAME_DIR/"
trap 'find "$GAME_DIR" -maxdepth 1 -name "longway_voyager.pk3" -delete' EXIT
rm -f "$SHOT" "$BEFORE"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> damage"
SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set com_hunkMegs 768 +set s_useOpenAL 0 +set g_ship 1 +set g_shipDayScale 1 \
    +set g_crew 1 +set g_crewFromShip 1 +set g_shipDeckPitch 3072 \
    +set g_shipTest 34 +map voyager >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h 'SHIP: damage test\|CREW: .* is damaged' "$OUT" | sed 's/^EFSP: /    /' || true

grep -q 'SHIP: damage test: standing at ' "$OUT" || fail "the harness did not take up a vantage on the deck (see $OUT)"
grep -q 'SHIP: damage test: life support at 60% health' "$OUT" || fail "life support was not damaged"
grep -q 'CREW: life support is damaged; sparks at .* on deck 12' "$OUT" \
  || fail "no visible damage was embodied at the damaged system"
[ -f "$BEFORE" ] || fail "no before-frame: a difference needs a reference at the same vantage"
[ -f "$SHOT" ] || fail "no screenshot of the damaged deck"

# The claim is a spark, so the frame must carry it where it is: assert the region the camera aims
# at, not the whole frame -- the console text and the HUD move whatever happens, and a whole-frame
# difference is a green stamp on a change no one claimed (docs/evidence/the-frame-assertions.md).
# That region on the old (wall) pair changed 0.0%; --min-changed 0.005 is above the scene's own
# stillness and below the spark this vantage actually carries.
python3 "$ROOT/tools/frames/frame.py" diff --region subject:615,525,665,585 --min-changed 0.005 \
  --require "$BEFORE" "$SHOT" || fail "the spark is not visible where the system is worked"
echo "PASS  a damaged system sparks where it is worked, in the world"
echo "      screenshot: $SHOT (against $BEFORE)"
