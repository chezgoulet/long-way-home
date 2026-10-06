#!/usr/bin/env bash
# The emergency lighting state of deck 12 (docs/locations/deck12-environmental-control.brief.md):
# the room whose failure darkens other decks goes red first.
#
#   scripts/emergency-check.sh
#
# One headless run with the environment layer on and one with it off. The harness stands at the
# life-support watch station on deck 12, puts the ship at battle stations, and photographs the red
# state; then it stands down and photographs it off. The strips are authored func_usable brushes
# (hall/hall_light_red and engineering/elight1) and the module swaps them. With g_env 0 nothing
# happens. Needs the built ship and engine. Writes under build/g3-home.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
PK3="$ROOT/build/ship/out/longway_voyager.pk3"
OUT="$HOME_DIR/emergency.out"
OFF="$HOME_DIR/emergency-off.out"
[ -f "$PK3" ] || { echo "no merged ship at $PK3 -- run scripts/build-ship.sh" >&2; exit 1; }
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

mkdir -p "$GAME_DIR/screenshots"
cp "$PK3" "$GAME_DIR/"
rm -f "$GAME_DIR/screenshots/lwh_emergency.tga" "$GAME_DIR/screenshots/lwh_emergency_off.tga"

run() { # run OUT G_ENV
	find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
	SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
		+set s_useOpenAL 0 +set g_ship 1 +set g_env "$2" +set g_shipDeckPitch 3072 \
		+set g_shipTest 58 +map voyager >"$1" 2>&1 || true
}

fail() { echo "FAIL  $1"; exit 1; }

echo "==> emergency lighting, g_env 1"
run "$OUT" 1
grep -h 'ENV: emergency\|SHIP: emergency test' "$OUT" | sed 's/^EFSP: /    /' || true
grep -q 'ENV: emergency lighting on on deck 12 ' "$OUT" || fail "the red state did not come on at battle stations"
grep -q 'ENV: emergency lighting off on deck 12 ' "$OUT" || fail "the red state did not stand down"
[ -f "$GAME_DIR/screenshots/lwh_emergency.tga" ] || fail "no screenshot of the red state"
[ -f "$GAME_DIR/screenshots/lwh_emergency_off.tga" ] || fail "no screenshot of the stood-down state"

echo
echo "==> emergency lighting, g_env 0 (the extension off)"
run "$OFF" 0
if grep -q 'ENV: emergency' "$OFF"; then fail "the emergency lighting ran with the cvar off"; fi
grep -q 'SHIP: emergency test: alert 0, life support 100%' "$OFF" \
	|| fail "with the cvar off the harness did not reach deck 12"

find "$GAME_DIR" -maxdepth 1 -name 'longway_voyager.pk3' -delete
echo "PASS  the emergency lighting state comes on at battle stations and stands down; the gate holds"
echo "      screenshots: $GAME_DIR/screenshots/lwh_emergency.tga, lwh_emergency_off.tga"
