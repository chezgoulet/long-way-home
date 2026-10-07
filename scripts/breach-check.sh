#!/usr/bin/env bash
# The environment in the world: a breach you can feel, and a field you can see.
#
#   scripts/breach-check.sh
#
# One headless run on the merged ship. The harness stands in deck 12's authored breach compartment and
# opens the hull; the module activates the authored push (aimed at the hole) and hurt volume, so the
# body is thrown toward the hole and the air is going. Raise the force field and the push and hurt
# stop, the authored field brush is solid and visible, and the air stops going. A second run with
# g_env 0 shows the extension changes nothing. Needs the built ship and engine. Writes under build/g3-home.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
PK3="$ROOT/build/ship/out/longway_voyager.pk3"
OUT="$HOME_DIR/breach.out"
OFF="$HOME_DIR/breach-off.out"
SHOT="$GAME_DIR/screenshots/lwh_breach.tga"
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
		+set g_shipTest 55 +map voyager >"$1" 2>&1 || true
}

fail() { echo "FAIL  $1"; exit 1; }

echo "==> breach, g_env 1"
run "$OUT" 1
grep -h 'ENV:\|SHIP: breach test' "$OUT" | sed 's/^EFSP: /    /' || true
grep -q 'ENV: deck 12 is breached; the air is going (push and hurt on, field off); the push throws' "$OUT" \
	|| fail "the breach did not switch on the authored push and hurt"
grep -qE 'SHIP: breach test: field off, thrown: health [0-9]+, velocity \(-?[1-9][0-9]* ' "$OUT" \
	|| fail "the push did not throw the player toward the hole"
grep -qE 'SHIP: breach test: field off: health [0-9]+, ' "$OUT" \
	|| fail "the hurt volume did not hurt the player"
grep -qE 'minutes of air [0-9]+\.[0-9]' "$OUT" || fail "the breached deck's air was not being lost"
grep -q 'SHIP: breach test: field on: health [0-9]*, velocity (0 0 0), minutes of air -1.0, published field 1' "$OUT" \
	|| fail "the field did not stop the push and hold the air"
[ -f "$SHOT" ] || fail "no screenshot of the raised field"

echo
echo "==> breach, g_env 0 (the extension off)"
run "$OFF" 0
grep -h 'SHIP: breach test' "$OFF" | sed 's/^EFSP: /    /' || true
if grep -q 'ENV:' "$OFF"; then fail "the environment ran with the cvar off"; fi
grep -q 'SHIP: breach test: field off, thrown: health 100, velocity (0 0 0)' "$OFF" \
	|| fail "with the cvar off the ship hurt or threw the player"
grep -q 'SHIP: breach test: field off: health 100, ' "$OFF" \
	|| fail "with the cvar off the hurt volume still fired"

echo "PASS  a breach pushes and hurts where it is authored; the field stops it and holds the air; the gate holds"
