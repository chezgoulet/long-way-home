#!/usr/bin/env bash
# The emergency lighting state of the decks that carry it: deck 12 environmental control and deck 13
# life support plant (docs/locations/deck12-environmental-control.brief.md,
# docs/locations/deck13-life-support.brief.md), and deck 14 stasis (docs/locations/deck14-stasis.brief.md).
# The rooms whose failure darkens other decks go red first; the stasis deck goes red with the ship.
#
#   scripts/emergency-check.sh
#
# Three headless runs with the environment layer on and three with it off. The harness stands at the
# deck 12 watch station (g_shipTest 58), at the deck 13 plant panel on its catwalk (g_shipTest 59)
# and at the deck 14 holodeck post (g_shipTest 61), puts the ship at battle stations, and photographs
# the red state; then stands down and photographs it off. The strips are authored func_usable brushes
# (hall/hall_light_red and engineering/elight1) and the module swaps them. With g_env 0 nothing
# happens. Needs the built ship and engine. Writes under build/g3-home.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
PK3="$ROOT/build/ship/out/longway_voyager.pk3"
OUT="$HOME_DIR/emergency.out"
OFF="$HOME_DIR/emergency-off.out"
OUT13="$HOME_DIR/emergency13.out"
OFF13="$HOME_DIR/emergency13-off.out"
OUT14="$HOME_DIR/emergency14.out"
OFF14="$HOME_DIR/emergency14-off.out"
[ -f "$PK3" ] || { echo "no merged ship at $PK3 -- run scripts/build-ship.sh" >&2; exit 1; }
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

mkdir -p "$GAME_DIR/screenshots"
cp "$PK3" "$GAME_DIR/"
rm -f "$GAME_DIR/screenshots/lwh_emergency.tga" "$GAME_DIR/screenshots/lwh_emergency_off.tga" \
      "$GAME_DIR/screenshots/lwh_emergency_13.tga" "$GAME_DIR/screenshots/lwh_emergency_13_off.tga" \
      "$GAME_DIR/screenshots/lwh_emergency_14.tga" "$GAME_DIR/screenshots/lwh_emergency_14_off.tga"

run() { # run OUT G_ENV TEST
	find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
	SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
		+set s_useOpenAL 0 +set g_ship 1 +set g_env "$2" +set g_shipDeckPitch 3072 \
		+set g_shipTest "$3" +map voyager >"$1" 2>&1 || true
}

fail() { echo "FAIL  $1"; exit 1; }

echo "==> emergency lighting, g_env 1"
run "$OUT" 1 58
grep -h 'ENV: emergency\|SHIP: emergency test' "$OUT" | sed 's/^EFSP: /    /' || true
grep -q 'ENV: emergency lighting on on deck 12 ' "$OUT" || fail "the red state did not come on at battle stations on deck 12"
grep -q 'ENV: emergency lighting off on deck 12 ' "$OUT" || fail "the red state did not stand down on deck 12"
[ -f "$GAME_DIR/screenshots/lwh_emergency.tga" ] || fail "no screenshot of the deck 12 red state"
[ -f "$GAME_DIR/screenshots/lwh_emergency_off.tga" ] || fail "no screenshot of the deck 12 stood-down state"

run "$OUT13" 1 59
grep -h 'ENV: emergency\|SHIP: emergency test 13' "$OUT13" | sed 's/^EFSP: /    /' || true
grep -q 'ENV: emergency lighting on on deck 13 ' "$OUT13" || fail "the red state did not come on at battle stations on deck 13"
grep -q 'ENV: emergency lighting off on deck 13 ' "$OUT13" || fail "the red state did not stand down on deck 13"
[ -f "$GAME_DIR/screenshots/lwh_emergency_13.tga" ] || fail "no screenshot of the deck 13 red state"
[ -f "$GAME_DIR/screenshots/lwh_emergency_13_off.tga" ] || fail "no screenshot of the deck 13 stood-down state"

run "$OUT14" 1 61
grep -h 'ENV: emergency\|SHIP: emergency test 14' "$OUT14" | sed 's/^EFSP: /    /' || true
grep -q 'ENV: emergency lighting on on deck 14 ' "$OUT14" || fail "the red state did not come on at battle stations on deck 14"
grep -q 'ENV: emergency lighting off on deck 14 ' "$OUT14" || fail "the red state did not stand down on deck 14"
[ -f "$GAME_DIR/screenshots/lwh_emergency_14.tga" ] || fail "no screenshot of the deck 14 red state"
[ -f "$GAME_DIR/screenshots/lwh_emergency_14_off.tga" ] || fail "no screenshot of the deck 14 stood-down state"

echo
echo "==> emergency lighting, g_env 0 (the extension off)"
run "$OFF" 0 58
if grep -q 'ENV: emergency' "$OFF"; then fail "the emergency lighting ran with the cvar off"; fi
grep -q 'SHIP: emergency test: alert 0, life support 100%' "$OFF" \
	|| fail "with the cvar off the harness did not reach deck 12"

run "$OFF13" 0 59
if grep -q 'ENV: emergency' "$OFF13"; then fail "the emergency lighting ran with the cvar off on deck 13"; fi
grep -q 'SHIP: emergency test 13: alert 0, life support 100%' "$OFF13" \
	|| fail "with the cvar off the harness did not reach deck 13"

run "$OFF14" 0 61
if grep -q 'ENV: emergency' "$OFF14"; then fail "the emergency lighting ran with the cvar off on deck 14"; fi
grep -q 'SHIP: emergency test 14: alert 0, life support 100%' "$OFF14" \
	|| fail "with the cvar off the harness did not reach deck 14"

find "$GAME_DIR" -maxdepth 1 -name 'longway_voyager.pk3' -delete
echo "PASS  the emergency lighting state comes on at battle stations and stands down on all three decks; the gate holds"
echo "      screenshots: $GAME_DIR/screenshots/lwh_emergency.tga, lwh_emergency_off.tga,"
echo "                   $GAME_DIR/screenshots/lwh_emergency_13.tga, lwh_emergency_13_off.tga,"
echo "                   $GAME_DIR/screenshots/lwh_emergency_14.tga, lwh_emergency_14_off.tga"
