#!/usr/bin/env bash
# The navigation counter (docs/navigation-counter.md): how far home, how long, and the arrow moving.
#
#   scripts/nav-check.sh
#
# One headless run on deck 4. The counter is read at the console three times -- before anything, with
# the crystal wrecked, and after a jump toward home -- and the transcript must show the distance, both
# figures and the forecasts, and must show the estimate worsen when the crystal does. The projection
# itself is tested without the game in tests/ship (`TestNavigation`) and printed by
# `test_ship_core --nav`. Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$HOME_DIR/nav.out"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> navigation counter"
SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipDayScale 1 +set g_shipTest 52 +map tour/deck04 >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h 'SHIP: nav test\|SHIP: [0-9]* light years from home\|SHIP:   forecast:\|SHIP:   [+-]' "$OUT" | sed 's/^EFSP: /    /' || true

grep -q 'SHIP: 75000 light years from home; 75 years nominal, 77 at current capability' "$OUT" \
  || fail "the counter did not read the distance and both figures (see $OUT)"
grep -q 'SHIP: 75000 light years from home; 75 years nominal, 8[0-9] at current capability' "$OUT" \
  || fail "wrecking the crystal did not worsen the estimate on the spot"
grep -q 'SHIP: 72727 light years from home' "$OUT" \
  || fail "a jump toward home did not reduce the distance"
grep -q 'SHIP:   forecast: beacon ' "$OUT" \
  || fail "command did not see the forecasts under the available courses"
echo "PASS  the counter reads the distance and both figures, worsens when the crystal is wrecked, falls when the ship jumps, and command sees the forecasts"
