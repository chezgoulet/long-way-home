#!/usr/bin/env bash
# The player in the world (docs/path-to-playtest.md Stage B): the model's causes reach the person
# holding the controls.
#
#   scripts/playerworld-check.sh
#
# One headless run on the merged ship. The harness stands the player at the life support console on
# a degraded grid and works it until it lets go at the operator, so the player's own record is hurt
# and their body follows it down; the ward carries and treats them; a breached, airless compartment
# injures them, and they can still leave it; and past the air limit the record closes -- death is
# reached, the engine's respawn is refused, and command passes to the senior officer. A second run
# with g_player 0 shows the extension changes nothing. Needs the built ship and engine. Writes under
# build/g3-home.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
PK3="$ROOT/build/ship/out/longway_voyager.pk3"
OUT="$HOME_DIR/playerworld.out"
OFF="$HOME_DIR/playerworld-off.out"
[ -f "$PK3" ] || { echo "no merged ship at $PK3 -- run scripts/build-ship.sh" >&2; exit 1; }
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$GAME_DIR/screenshots"
cp "$PK3" "$GAME_DIR/"
trap 'find "$GAME_DIR" -maxdepth 1 -name "longway_voyager.pk3" -delete' EXIT

run() { # run OUT G_PLAYER
	find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
	SDL_AUDIODRIVER=dummy timeout 260 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
		+set com_hunkMegs 768 +set s_useOpenAL 0 +set g_ship 1 +set g_shipDayScale 1 \
		+set g_crew 1 +set g_crewFromShip 1 +set g_env 1 +set g_player "$2" +set g_shipDeckPitch 3072 \
		+set g_shipTest 56 +map voyager >"$1" 2>&1 || true
}

fail() { echo "FAIL  $1"; exit 1; }

echo "==> the player in the world, g_player 1"
run "$OUT" 1
grep -hE '^SHIP: player test|^PLAYER:' "$OUT" | sed 's/^EFSP: /    /' || true

grep -q 'SHIP: player test: Test Player at the life support console, 40% health, 40% condition, red alert' "$OUT" \
	|| fail "the player was not stood at a console on a degraded grid (see $OUT)"
# The console lets go at the person holding the controls, and their own record carries it.
grep -qE '\[sickbay\] .+: Test Player was hurt when the life support console let go' "$OUT" \
	|| fail "the console did not let go at the player"
grep -qE 'the console let go after [0-9]+ draws: status 1' "$OUT" \
	|| fail "the player's record was not injured by the console"
grep -qE 'hurt by the console 1, attended 1; body health [1-9][0-9]? of 100, incapacitated 1' "$OUT" \
	|| fail "the player's body did not follow the record down, or no one attended"
# Someone comes, and the log names them: the same shape as any casualty.
grep -qE '\[sickbay\] .+: .+ attends Test Player on deck 12; they are carried to sickbay' "$OUT" \
	|| fail "no one was named as attending the downed player"
grep -q 'SHIP: player test: recovered: status 0, body health 100 of 100' "$OUT" \
	|| fail "the ward did not return the player to fit, body whole"
# A breached, airless compartment affects the person in it, and they can still leave.
grep -qE 'in the airless compartment: status 1, severity 0\.[0-9]+, deck 12, moving 1' "$OUT" \
	|| fail "the player was not affected in the airless compartment, or could not move"
grep -qE 'left the deck: at \(.+record deck 4, still moving 1, status 1' "$OUT" \
	|| fail "the player could not leave the airless deck"
# Death is reachable, is not a reload, and command passes on.
grep -qE '\[sickbay\] .+: Test Player dead, no air' "$OUT" \
	|| fail "the player did not die of the air the model tracks"
grep -q 'PLAYER: no reload; Test Player'\''s record is closed and the ship carries on' "$OUT" \
	|| fail "the engine's respawn was not refused (the dead were reloaded)"
grep -q 'SHIP: player test: dead 1, body health -\{0,1\}[0-9]\+, respawn blocked 1, command "Kathryn Janeway"' "$OUT" \
	|| fail "death did not close the record, or command did not pass to the senior officer"

echo
echo "==> the player in the world, g_player 0 (the extension off)"
run "$OFF" 0
grep -hE '^SHIP: player test' "$OFF" | sed 's/^EFSP: /    /' || true
if grep -q 'PLAYER:' "$OFF"; then fail "the player-in-the-world layer ran with the cvar off"; fi
grep -q 'nothing to operate it: g_player is off, or no character is the player' "$OFF" \
	|| fail "the console worked the system with the cvar off"
grep -q 'SHIP: player test: the console let go after 0 draws: status 0, severity 0.00' "$OFF" \
	|| fail "with the cvar off the player's record was still hurt by the console"
grep -q 'SHIP: player test: hurt by the console 0, attended 0; body health 100 of 100, incapacitated 0' "$OFF" \
	|| fail "with the cvar off the player's body or record was changed"
grep -qE 'SHIP: player test: dead 0, body health 100, respawn blocked 0, command "Test Player"' "$OFF" \
	|| fail "with the cvar off death or command was changed"

echo
echo "PASS  the console lets go at the operator and the body follows the record; someone attends and is"
echo "      named; the airless compartment affects the player, who can still leave; death closes the"
echo "      record and is not a reload; with the cvar off nothing above happens"
