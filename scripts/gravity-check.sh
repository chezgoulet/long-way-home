#!/usr/bin/env bash
# The environment in the world: per-person gravity, and the way back.
#
#   scripts/gravity-check.sh
#
# One headless run on the merged ship. The harness stands on deck 12 and drives its plating to zero;
# the module scales the player's own gravity (ps.gravity, SVF_CUSTOM_GRAVITY -- the engine's
# per-person field) to match, so the player floats. The magnetic boots give standard gravity back, and
# a deck that recovers hands gravity to the world's own value -- the case the engine's own FIXME says
# it has no way to do. A second run with g_env 0 shows the extension changes nothing.
# Needs the built ship (build-ship.sh) and the built engine + modules. Writes under build/g3-home.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
PK3="$ROOT/build/ship/out/longway_voyager.pk3"
OUT="$HOME_DIR/gravity.out"
OFF="$HOME_DIR/gravity-off.out"
SHOT="$GAME_DIR/screenshots/lwh_gravity.tga"
[ -f "$PK3" ] || { echo "no merged ship at $PK3 -- run scripts/build-ship.sh" >&2; exit 1; }
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$GAME_DIR/screenshots"
cp "$PK3" "$GAME_DIR/"
trap 'find "$GAME_DIR" -maxdepth 1 -name "longway_voyager.pk3" -delete' EXIT
rm -f "$SHOT"

run() { # run OUT G_ENV
	find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
	SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
		+set com_hunkMegs 768 +set s_useOpenAL 0 +set g_ship 1 +set g_shipDayScale 1 \
		+set g_crew 1 +set g_crewFromShip 1 +set g_env "$2" +set g_shipDeckPitch 3072 \
		+set g_shipTest 54 +map voyager >"$1" 2>&1 || true
}

fail() { echo "FAIL  $1"; exit 1; }

echo "==> gravity, g_env 1"
run "$OUT" 1
grep -h 'ENV:\|SHIP: gravity test' "$OUT" | sed 's/^EFSP: /    /' || true
grep -q 'SHIP: gravity test: floating: ps.gravity 0, custom 1' "$OUT" \
	|| fail "the player was not put in freefall (see $OUT)"
grep -q 'floating: .*crew floating 1' "$OUT" \
	|| fail "the crew on the deck did not float (BS_FLY)"
grep -q 'SHIP: gravity test: boots on: ps.gravity 800, custom 0' "$OUT" \
	|| fail "the magnetic boots did not give standard gravity back"
grep -q 'SHIP: gravity test: recovered: ps.gravity 800, custom 0, crew floating 0' "$OUT" \
	|| fail "a recovered deck did not get the world's gravity back (the engine's FIXME)"
[ -f "$SHOT" ] || fail "no screenshot of the recovered deck"

echo
echo "==> gravity, g_env 0 (the extension off)"
run "$OFF" 0
grep -h 'SHIP: gravity test' "$OFF" | sed 's/^EFSP: /    /' || true
if grep -q 'ENV:' "$OFF"; then fail "the environment ran with the cvar off"; fi
grep -q 'SHIP: gravity test: floating: ps.gravity 800, custom 0' "$OFF" \
	|| fail "with the cvar off the player's gravity was changed"

echo "PASS  a deck's plating scales one person's gravity; the boots and a recovered deck give it back; the gate holds"
