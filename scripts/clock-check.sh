#!/usr/bin/env bash
# The three clocks and the two exits (docs/ship-model.md), driven in the game.
#
#   scripts/clock-check.sh
#
# One headless engine run with the ship simulation on: ironman refuses to suspend the world; a
# sleep in one jump, in steps and a played interval all arrive together; a standing order holds
# across a sleep with the night's work in the log; and in holodeck the run is left standing, marked.
# Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$HOME_DIR/clocks.out"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$GAME_DIR/saves"
find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> clocks and exits"
SDL_AUDIODRIVER=dummy timeout 240 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipTest 51 +map tour/deck04 >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h '^SHIP: clock test:' "$OUT" | sed 's/^SHIP: /    /' || true
grep -q '^SHIP: clock test: ironman may suspend 0' "$OUT" || fail "ironman did not refuse to suspend the world"
grep -q '^SHIP: clock test: sleep 6h -> day 0; twelve steps -> day 0; played -> day 0; identical 1' "$OUT" \
  || fail "a sleep did not converge with the played interval"
grep -q '^SHIP: clock test: after a 6h sleep, deck 11 crew 0' "$OUT" \
  || fail "the standing order did not hold across the sleep"
grep -q '^SHIP: clock test: holodeck suspend -> left standing 1' "$OUT" \
  || fail "holodeck did not permit the world to be suspended"
echo "PASS  ironman refuses suspension; a sleep converges with a played interval and carries a standing order; holodeck leaves the run marked"
