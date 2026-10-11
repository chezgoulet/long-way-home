#!/usr/bin/env bash
# The allocation console in the game (docs/power-assignment.md, Task B): a hand sets a system's
# share of its demand, turns automatic mode on and off, and the console shows committed against
# available and the shortfall as a number. Everything below is a key press on the Engineering
# screen; the ship reports what it heard.
#
#   scripts/power-check.sh [--map NAME]
#
# One headless engine run. Needs the game data (scripts/playtest-host-setup.sh) and xvfb-run.

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
mkdir -p "$OUT" "$GAME_DIR/screenshots"
rm -f "${OUT:?}/power.txt" "${GAME_DIR:?}/screenshots/lwh_power.tga" \
      "${GAME_DIR:?}/screenshots/lwh_power_closed.tga"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> the allocation console (g_shipTest 71)"
SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipMode 1 +set g_shipTest 71 +map "$MAP" \
    >"$HOME_DIR/power.out" 2>&1 || true

grep -h '^SHIP: allocation test' "$HOME_DIR/power.out" | sed 's/^/    /' || true

fail() { echo "FAIL  $1"; exit 1; }
[ -f "$OUT/power.txt" ] || fail "the console run did not finish: the game stopped while the console was open"
grep -q '^SHIP: allocation test: life support 80%' "$HOME_DIR/power.out" \
  || fail "the console's allocation key did not set life support to 80%"
grep -q 'life support .* alloc  80%' "$OUT/power.txt" || fail "the allocation did not persist in the ship's state"
grep -Eiq 'automatic mode|the player sets the allocation' "$OUT/power.txt" || fail "the console did not report the mode"
grep -q 'committed' "$OUT/power.txt" || fail "the console did not report committed against available"
[ -f "$GAME_DIR/screenshots/lwh_power.tga" ] || fail "no screenshot of the allocation console"
[ -f "$GAME_DIR/screenshots/lwh_power_closed.tga" ] || fail "no before frame for the console assertion"
# The picture, not the path: the frame must differ from a frame taken at the same vantage before
# the change, in the region the claim lives in. (tools/frames/frame.py; its limits in its header:
# a difference proves the region *changed*, not that it changed into the thing meant -- the owner
# judges that. A frame that does not differ is the defect an existence check cannot see.)
python3 "$ROOT/tools/frames/frame.py" diff --min-changed 0.05 --require \
  "$GAME_DIR/screenshots/lwh_power_closed.tga" "$GAME_DIR/screenshots/lwh_power.tga" \
  || fail "the console frame is unchanged from the world: the Engineering console did not open on screen"
echo "PASS  the Engineering console set an allocation key by key and showed committed against available"
echo "      screenshot: $GAME_DIR/screenshots/lwh_power.tga"
