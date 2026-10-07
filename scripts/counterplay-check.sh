#!/usr/bin/env bash
# The counter-play kit (docs/borg-incursion.md): the phaser adapter's rotating modulation breaks the
# Borg's adaptation, so the crew can buy their weapons back.
#
#   scripts/counterplay-check.sh
#
# One headless run on deck 4. The rules (the cooldown, and a vinculum raid that suppresses adaptation
# at a cost) are tested without the game in tests/ship (`TestCounterPlay`). Writes under build/g3-home.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$HOME_DIR/counterplay.out"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> counter-play"
SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipDayScale 1 +set g_shipTest 41 +map tour/deck04 >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h 'SHIP: counterplay test\|SHIP: modulation rotated' "$OUT" | sed 's/^EFSP: /    /' || true

grep -q 'SHIP: counterplay test: adaptation 60% before remodulation' "$OUT" || fail "the Borg's adaptation was not set up (see $OUT)"
grep -q 'SHIP: modulation rotated; enemy adaptation now 10%' "$OUT" \
  || fail "rotating the modulation did not break the Borg's lock"
echo "PASS  the phaser modulation can be rotated, breaking the Borg's adaptation so the shots land again"
