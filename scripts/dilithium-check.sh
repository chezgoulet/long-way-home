#!/usr/bin/env bash
# The dilithium constraint that forces exploration (docs/exploration-and-science.md): the crystal,
# recomposition, and the journey's scoreboard, driven at the console.
#
#   scripts/dilithium-check.sh
#
# One headless run on deck 4. The system is exercised at the console -- the status, then a
# recomposition -- and the transcript must name the crystal's life and the range it buys. The rules
# (spend on jump, ceiling drop, acquisition by mine/trade/salvage/research, no crystal no warp) are
# tested without the game in tests/ship (`TestDilithium`). Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$HOME_DIR/dilithium.out"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> dilithium"
SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipDayScale 1 +set g_shipTest 38 +map tour/deck04 >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h 'SHIP: dilithium\|SHIP: dilithium source charted' "$OUT" | sed 's/^EFSP: /    /' || true

grep -q 'SHIP: dilithium 100% .*range 3000 ly, warp possible' "$OUT" || fail "the crystal's life and range were not reported (see $OUT)"
grep -q 'SHIP: dilithium now .*% (ceiling .*%)' "$OUT" || fail "recomposition was not worked at the console"
grep -q 'SHIP: dilithium source charted at beacon ' "$OUT" || fail "the survey did not locate a source"
echo "PASS  the dilithium crystal reports its life and range, Engineering can recomposite it, and a survey locates a source"
