#!/usr/bin/env bash
# Probes (docs/exploration-and-science.md): the safe way to look at something hostile -- a probe
# charts the target without the ship going there, and is sometimes lost.
#
#   scripts/probe-check.sh
#
# One headless run on deck 4. The rules (stores down, the chart up, no launchers no probe, none left
# no probe) are tested without the game in tests/ship (`TestProbes`). Writes under build/g3-home.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$HOME_DIR/probe.out"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> probes"
SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipDayScale 60 +set g_shipTest 44 +map tour/deck04 >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h 'SHIP: probe away' "$OUT" | sed 's/^EFSP: /    /' || true

grep -q 'SHIP: probe away; 5 left' "$OUT" || fail "a probe was not launched (see $OUT)"
echo "PASS  a probe is launched, spending one of the ship's complement"
