#!/usr/bin/env bash
# Gate S2, the part a machine can check: the ship simulation runs inside the game, what is done
# to it takes effect, and the save holds the ship.
#
#   scripts/s2-check.sh [--map NAME]
#
# Three headless engine runs. The first starts a ship on a deck, does what a console would (red
# alert, damage, a reordered power priority, a hull breach), writes the ship's state and saves.
# The second loads that save and writes the state it finds. The two must be identical, and must
# differ from an untouched ship in the ways the actions dictate. The third opens Main Engineering's
# console and operates it key by key -- condition red, a system switched off, another demoted in
# the power order -- while the game keeps running behind it, then takes a screenshot of the console.
#
# Writes under build/g3-home only, like the G3 measurement.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MAP="tour/deck04"
while [ $# -gt 0 ]; do
  case "$1" in
    --map) MAP="$2"; shift 2 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done

HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$GAME_DIR/ship"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$OUT" "$GAME_DIR/saves"
rm -f "${OUT:?}/acted.txt" "${OUT:?}/restored.txt" "${OUT:?}/operated.txt" "${GAME_DIR:?}/saves/shiprun.sav" \
      "${GAME_DIR:?}/screenshots/lwh_engineering.tga"

engine() {
  local label="$1"; shift
  find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
  echo "==> $label"
  SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
      +set s_useOpenAL 0 +set g_ship 1 "$@" >"$HOME_DIR/s2-$label.out" 2>&1 || true
  grep -h '^SHIP: \(simulation\|restored\|wrote\|carried\|the save\)\|^Wrote screenshots' "$HOME_DIR/s2-$label.out" | sed 's/^/    /' || true
}

engine act    +set g_shipTest 1 +map "$MAP"
engine reload +set g_shipTest 2 +load shiprun
engine operate +set g_shipTest 3 +map "$MAP"

fail() { echo "FAIL  $1"; exit 1; }
[ -f "$OUT/acted.txt" ] || fail "the first run wrote no ship state"
[ -f "$OUT/restored.txt" ] || fail "the reload wrote no ship state"

# The clock runs between the save and the report after the load, so the first two lines (time and
# fuel) may differ by a tick; everything a console changed is in the lines that follow.
if ! diff <(tail -n +3 "$OUT/acted.txt") <(tail -n +3 "$OUT/restored.txt"); then
  fail "the ship that was loaded is not the ship that was saved"
fi
echo "PASS  the reloaded ship matches the saved one, system by system"

grep -q 'condition red' "$OUT/restored.txt" || fail "red alert did not survive"
grep -q 'shields .* health  60%' "$OUT/restored.txt" || fail "shield damage did not survive"
grep -q 'warp core .* health  50%' "$OUT/restored.txt" || fail "warp core damage did not survive"
grep -q 'warp drive .* power 400/400' "$OUT/restored.txt" || fail "the reordered priority did not feed the warp drive"
grep -q 'holodecks .* output   0%' "$OUT/restored.txt" || fail "red alert did not shut the holodecks"
echo "PASS  alert, damage and the reordered priority all took effect and persisted"

# The console. Every change below was made by a key press on the Engineering screen, sent to the
# ship as the command a hand on the console would send.
[ -f "$OUT/operated.txt" ] || fail "the console run did not finish: the game stopped while the console was open"
grep -q 'condition red' "$OUT/operated.txt" || fail "the console's condition-red key did not reach the ship"
grep -q 'inertial dampers .* OFF' "$OUT/operated.txt" || fail "the console did not switch the selected system off"
grep -q '^SHIP: life support priority 2, structural integrity priority 1' "$HOME_DIR/s2-operate.out" \
  || fail "the console's priority key did not demote the selected system by one place"
grep -q 'structural integrity .* power  80/ 80' "$OUT/operated.txt" || fail "structural integrity lost power"
[ -f "$GAME_DIR/screenshots/lwh_engineering.tga" ] || fail "no screenshot of the console"
echo "PASS  the Engineering console, operated key by key with the game running, changed the ship"
echo "      screenshot: $GAME_DIR/screenshots/lwh_engineering.tga"
echo
sed 's/^/    /' "$OUT/operated.txt"
