#!/usr/bin/env bash
# The security squad (docs/borg-incursion.md): a fireteam command sends to retake a deck the boarders
# hold, advancing a deck at a time and then holding it while the crew restore it.
#
#   scripts/squad-check.sh
#
# One headless run on deck 4. The deck is evacuated (so only the squad fights), boarded, and a squad
# sent to retake it; the controller report shows it ours again. The rules are tested without the game
# in tests/ship (`TestSquad`). Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$HOME_DIR/squad.out"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> security squad"
SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipRole 1 +set g_shipDayScale 60 +set g_shipTest 46 +map tour/deck04 >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h 'SHIP: a squad is advancing\|SHIP: .* deck(s) not wholly ours' "$OUT" | sed 's/^EFSP: /    /' || true

grep -q 'SHIP: a squad is advancing to retake deck 4' "$OUT" || fail "the squad was not ordered (see $OUT)"
grep -q 'SHIP: 0 deck(s) not wholly ours' "$OUT" || fail "the squad did not retake the deck"
echo "PASS  a squad advances to retake a deck the boarders hold, and the deck is ours again"
