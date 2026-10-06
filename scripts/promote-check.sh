#!/usr/bin/env bash
# S10: the promotion's confirmation reaches a UI -- the command console's field promotion.
#
#   scripts/promote-check.sh
#
# One headless run on deck 4. The harness takes command, creates a character (Reyes, Command, rank 0)
# and issues the ship's own "promote" for them; the ship's answer is published for the command
# console to draw (lwh_ship_promote), and the harness reads it back. Needs the built modules.
# Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$HOME_DIR/promote.out"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> promotion"
SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipDayScale 1 +set g_shipTest 36 +map tour/deck04 >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h 'SHIP: promote test:' "$OUT" | sed 's/^EFSP: /    /' || true

grep -q 'SHIP: promote test: Reyes is crew number .* at rank 0' "$OUT" || fail "the character was not created (see $OUT)"
grep -q 'SHIP: promote test: the ship says "Reyes is promoted to Ensign"' "$OUT" \
  || fail "the promotion did not reach the UI as a confirmation"
echo "PASS  command's field promotion is confirmed by the ship, and the answer is published for the console to show"
