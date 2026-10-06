#!/usr/bin/env bash
# The Borg incursion's field (docs/borg-incursion.md): who holds each deck, whether its systems are
# compromised, and the clean intercepts recorded as wins.
#
#   scripts/controller-check.sh
#
# One headless run on deck 4. Boarders are put on a deck, the ship's security answers, and the
# controller field is read at the console before and after. The rules (a clean intercept recorded,
# a deck compromised and contested when unopposed) are tested without the game in tests/ship
# (`TestIncursion`). Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$HOME_DIR/controller.out"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> controller"
SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipDayScale 60 +set g_shipTest 40 +map tour/deck04 >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h 'SHIP: deck .* intruders\|SHIP: .* clean intercept' "$OUT" | sed 's/^EFSP: /    /' || true

grep -q 'SHIP: deck 4 ' "$OUT" || fail "the controller field was not reported for the boarded deck (see $OUT)"
grep -q 'clean intercept' "$OUT" || fail "the clean-intercept counter was not reported"
# Security boards deck 4 and, outnumbering two raiders, clears them before they touch a system.
grep -qE 'SHIP: [0-9]+ deck\(s\) not wholly ours; [1-9][0-9]* clean intercept\(s\)' "$OUT" \
  || fail "clearing the boarders was not recorded as a clean intercept"
echo "PASS  the ship holds each deck's controller, and beating the boarders before they write is recorded as a clean intercept"
