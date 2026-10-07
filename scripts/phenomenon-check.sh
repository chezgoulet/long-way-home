#!/usr/bin/env bash
# Phenomena (docs/exploration-and-science.md): an anomaly with hidden attributes, revealed one scan
# at a time, whose correct response is the science reward.
#
#   scripts/phenomenon-check.sh
#
# One headless run on deck 4. The phenomenon beacon is scanned three times (one attribute each), then
# answered correctly. The rules are tested without the game in tests/ship (`TestPhenomenon`). Writes
# under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$HOME_DIR/phenomenon.out"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> phenomena"
SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipDayScale 60 +set g_shipTest 45 +map tour/deck04 >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h 'SHIP: phenomenon test\|SHIP: a phenomenon\|the phenomenon answers' "$OUT" | sed 's/^EFSP: /    /' || true

grep -q 'SHIP: a phenomenon: 3 of 3 attributes resolved' "$OUT" || fail "the phenomenon's attributes were not revealed by scanning (see $OUT)"
grep -q 'SHIP: the phenomenon answers; material' "$OUT" || fail "the correct response did not yield the science reward"
echo "PASS  a phenomenon's hidden attributes are revealed one scan at a time, and the right response rewards"
