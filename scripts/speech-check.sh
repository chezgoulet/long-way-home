#!/usr/bin/env bash
# Memory reaching dialogue: a crew member speaks a line drawn from what they remember, when the
# player addresses them (docs/memory-and-consequence.md).
#
#   scripts/speech-check.sh
#
# One headless run on deck 4. With the crew layer embodying the ship's crew on the deck, the harness
# gives one of them a rescue in their marks and addresses them the way the player would; the crew
# layer makes them say their account. Needs the built modules. Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$HOME_DIR/speech.out"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> speech"
SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipDayScale 1 \
    +set g_crew 1 +set g_crewFromShip 1 +set g_crewDeck 11 \
    +set g_shipTest 37 +map tour/deck04 >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h 'SHIP: speech test\|CREW: .* says:' "$OUT" | sed 's/^EFSP: /    /' || true

grep -q 'SHIP: speech test: .* carries a rescue; addressing them' "$OUT" || fail "no embodied crew was found to speak (see $OUT)"
grep -q 'CREW: .* says: "You came back for us. I have not forgotten."' "$OUT" \
  || fail "the crew member did not speak a line drawn from their memory"
echo "PASS  a crew member speaks their account from memory when addressed"
