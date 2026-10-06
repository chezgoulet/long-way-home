#!/usr/bin/env bash
# S6: damage is visible in the world -- a damaged system sparks where it is worked.
#
#   scripts/damage-check.sh
#
# One headless run on the merged ship. The harness stands on deck 12 (Life Support) and damages
# that system; the crew layer spawns the game's own fx_spark at its station marker and says so.
# Needs the built ship (build-ship.sh) and the built engine + modules. Writes under build/g3-home.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
PK3="$ROOT/build/ship/out/longway_voyager.pk3"
OUT="$HOME_DIR/damage.out"
SHOT="$GAME_DIR/screenshots/lwh_damage.tga"
[ -f "$PK3" ] || { echo "no merged ship at $PK3 -- run scripts/build-ship.sh" >&2; exit 1; }
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$GAME_DIR/screenshots"
cp "$PK3" "$GAME_DIR/"
trap 'find "$GAME_DIR" -maxdepth 1 -name "longway_voyager.pk3" -delete' EXIT
rm -f "$SHOT"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> damage"
SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set com_hunkMegs 768 +set s_useOpenAL 0 +set g_ship 1 +set g_shipDayScale 1 \
    +set g_crew 1 +set g_crewFromShip 1 +set g_shipDeckPitch 3072 \
    +set g_shipTest 34 +map voyager >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h 'SHIP: damage test\|CREW: .* is damaged' "$OUT" | sed 's/^EFSP: /    /' || true

grep -q 'SHIP: damage test: standing on deck 12' "$OUT" || fail "the harness did not reach deck 12 (see $OUT)"
grep -q 'SHIP: damage test: life support at 60% health' "$OUT" || fail "life support was not damaged"
grep -q 'CREW: life support is damaged; sparks at .* on deck 12' "$OUT" \
  || fail "no visible damage was embodied at the damaged system"
[ -f "$SHOT" ] || fail "no screenshot of the damaged deck"
echo "PASS  a damaged system sparks where it is worked, in the world"
echo "      screenshot: $SHOT"
