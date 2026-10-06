#!/usr/bin/env bash
# Shuttles: supported, not pilotable (docs/shuttles.md). The bay's contents, a launch with a
# manifest, and a recall, driven at the console.
#
#   scripts/shuttle-check.sh
#
# One headless run on deck 4. The rules (bay contents after save/load, a lost shuttle permanent as a
# build job, a hit on the bay) are tested without the game in tests/ship (`TestShuttles`).
# Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$HOME_DIR/shuttle.out"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> shuttles"
SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipDayScale 1 +set g_shipTest 39 +map tour/deck04 >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h 'SHIP: .*in the bay\|SHIP: Type 6' "$OUT" | sed 's/^EFSP: /    /' || true

grep -q 'SHIP: 4 in the bay, 0 away' "$OUT" || fail "the bay's starting complement was not reported (see $OUT)"
grep -q 'SHIP: Type 6 away to beacon 3, 2 aboard' "$OUT" || fail "the launch did not put a shuttle away with its manifest"
grep -q 'SHIP: 3 in the bay, 1 away' "$OUT" || fail "an away shuttle did not leave the bay short one"
grep -q 'SHIP: Type 6 is back in the bay' "$OUT" || fail "the recall did not bring the shuttle home"
echo "PASS  the ship knows where each shuttle is: in the bay, away (with its manifest), and home again"
