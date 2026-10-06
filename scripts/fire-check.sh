#!/usr/bin/env bash
# S6: a burning deck is seen to burn -- smoke and flame across the deck the player is on.
#
#   scripts/fire-check.sh
#
# One headless run on the merged ship. The harness stands on deck 12 and sets it alight; the crew
# layer spawns the game's own fx_smoke and fx_electricfire at points across the deck and says so.
# Needs the built ship (build-ship.sh) and the built engine + modules. Writes under build/g3-home.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
PK3="$ROOT/build/ship/out/longway_voyager.pk3"
OUT="$HOME_DIR/fire.out"
SHOT="$GAME_DIR/screenshots/lwh_fire.tga"
[ -f "$PK3" ] || { echo "no merged ship at $PK3 -- run scripts/build-ship.sh" >&2; exit 1; }
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$GAME_DIR/screenshots"
cp "$PK3" "$GAME_DIR/"
trap 'find "$GAME_DIR" -maxdepth 1 -name "longway_voyager.pk3" -delete' EXIT
rm -f "$SHOT"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> fire"
SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set com_hunkMegs 768 +set s_useOpenAL 0 +set g_ship 1 +set g_shipDayScale 1 \
    +set g_crew 1 +set g_crewFromShip 1 +set g_shipDeckPitch 3072 \
    +set g_shipTest 35 +map voyager >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h 'SHIP: fire test\|CREW: fire on deck' "$OUT" | sed 's/^EFSP: /    /' || true

grep -q 'SHIP: fire test: standing on deck 12' "$OUT" || fail "the harness did not reach deck 12 (see $OUT)"
grep -q 'SHIP: fire test: deck 12 alight at 60%' "$OUT" || fail "deck 12 was not set alight"
grep -qE 'CREW: fire on deck 12 \(60%\); [1-8] effects in the world' "$OUT" \
  || fail "the fire was not embodied in the world"
[ -f "$SHOT" ] || fail "no screenshot of the burning deck"
echo "PASS  a burning deck is embodied as smoke and flame across the deck"
echo "      screenshot: $SHOT"
