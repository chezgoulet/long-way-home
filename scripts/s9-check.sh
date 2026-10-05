#!/usr/bin/env bash
# Gates S7 and S9 at the consoles: a hijacked system won back through the breach screen, a jump
# made from the Conn, and a torpedo fired from Tactical -- all by key press, in the game.
#
#   scripts/s9-check.sh
#
# One headless engine run on deck 4. The sensors are put in the boarders' hands; the Operations
# console is opened and its countermeasures key pressed, which asks the ship for a breach puzzle and
# presents it; the harness solves the published puzzle by search and enters the solution as the
# operator's key presses. Then the Conn console jumps the ship to the first beacon listed (a hostile
# one, for the default seed), and the Tactical console goes to condition red and fires a torpedo.
#
# The rules behind all of it are tested without the game in tests/ship.
# Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$HOME_DIR/s9-console.out"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$GAME_DIR/screenshots"
rm -f "${GAME_DIR:?}/screenshots/lwh_breach.tga" "${GAME_DIR:?}/screenshots/lwh_combat.tga"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> consoles"
# The ship's clock is slowed to real time: the test is of the consoles, not of how the fight goes.
SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipDayScale 1 +set g_shipTest 9 +map tour/deck04 >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h '^SHIP: console test\|^SHIP: counter-hack\|^SHIP: .* refused' "$OUT" | sed 's/^SHIP: /    /' || true

grep -q '^SHIP: console test: breach puzzle solvable to 100% in 7 picks' "$OUT" || fail "the ship published no solvable puzzle"
grep -q '^SHIP: counter-hack on sensors scored 100%; control is now 70%' "$OUT" \
  || fail "the solution entered at the console was not scored in full by the ship"
grep -q '^SHIP: console test: sensors control 70%, ours again' "$OUT" || fail "the sensors were not won back"
echo "PASS  the breach screen presented the ship's puzzle, the solution keyed in scored 100%, and the sensors are ours again"

grep -q '^SHIP: console test: at beacon 1, in combat, condition 2, torpedoes 37' "$OUT" \
  || fail "the Conn's jump and Tactical's alert and torpedo did not leave the ship at beacon 1, in combat, at red, one torpedo spent"
echo "PASS  the Conn jumped the ship into contact; Tactical went to red alert and fired a torpedo"

for shot in lwh_breach lwh_combat; do
  [ -f "$GAME_DIR/screenshots/$shot.tga" ] || fail "no screenshot $shot"
done
echo "      screenshots: $GAME_DIR/screenshots/lwh_breach.tga, lwh_combat.tga"
