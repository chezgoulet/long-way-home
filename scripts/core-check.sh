#!/usr/bin/env bash
# The warp core cascade (docs/failure-is-content.md): a failing core loses coolant and heats, and
# command can shut it down to stop the breach.
#
#   scripts/core-check.sh
#
# One headless run on deck 4. The warp drive is badly damaged, the core reports its coolant,
# temperature and containment, then command shuts it down. The rules (the chain to a breach, and the
# interrupts -- shutdown, restart, eject, coolant) are tested without the game in tests/ship
# (`TestCoreCascade`). Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$HOME_DIR/core.out"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> warp core cascade"
SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipRole 1 +set g_shipDayScale 300 +set g_shipTest 47 +map tour/deck04 >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h 'SHIP: core coolant' "$OUT" | sed 's/^EFSP: /    /' || true

grep -q 'SHIP: core coolant' "$OUT" || fail "the core's condition was not reported (see $OUT)"
grep -q '(shut down)' "$OUT" || fail "the core was not shut down to stop the cascade"
echo "PASS  the warp core's condition is shown, and command can shut it down to stop the cascade"
