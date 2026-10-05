#!/usr/bin/env bash
# Gate S10, the part that is enforced in the game: ironman.
#
#   scripts/s10-check.sh
#
# One headless engine run with the ship simulation in its default mode. It tries a save by hand and
# a load of the level's autosave, both of which ironman must refuse, and then the one save the game
# itself is allowed to write, which must succeed.
#
# Clearance by rank is checked in scripts/s4-check.sh; the rules for modes, clocks, roles and
# character creation are tested without the game in tests/ship. Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$HOME_DIR/s10-ironman.out"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$GAME_DIR/saves"
rm -f "${GAME_DIR:?}/saves/byhand.sav" "${GAME_DIR:?}/saves/ironman.sav"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> ironman"
SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipTest 10 +map tour/deck04 >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h '^Ironman:' "$OUT" | sed 's/^/    /' || true
grep -q "^Ironman: save 'byhand' refused" "$OUT" || fail "a save by hand was not refused"
[ ! -f "$GAME_DIR/saves/byhand.sav" ] || fail "a save by hand was written"
grep -q "^Ironman: load 'auto' refused" "$OUT" || fail "going back to the level's autosave was not refused"
grep -q '^SHIP: ironman test done' "$OUT" || fail "the run did not continue after the refused load"
[ -f "$GAME_DIR/saves/ironman.sav" ] || fail "the game's own forward save was not written"
echo "PASS  ironman refuses a save and a load by hand, and writes its one save forward"
