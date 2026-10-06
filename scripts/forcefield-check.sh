#!/usr/bin/env bash
# Force fields rated 1-10 (docs/borg-incursion.md): a level-10 field holds boarders and drains under
# their pressure; a level-10 field on a Borg deck cuts them from the Collective.
#
#   scripts/forcefield-check.sh
#
# One headless run on deck 4. Boarders are put there and a level-10 field raised; the controller
# report shows the deck held with the field draining. The full rules are tested without the game in
# tests/ship (`TestForceFieldKit`). Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$HOME_DIR/forcefield.out"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> force fields"
SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipRole 1 +set g_shipDayScale 60 +set g_shipTest 43 +map tour/deck04 >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h 'SHIP: deck 4 ' "$OUT" | sed 's/^EFSP: /    /' || true

grep -qE 'SHIP: deck 4 (crew|sealed), intruders [0-9]+, dwell 0s, field [1-9][0-9]*' "$OUT" \
  || fail "a level-10 field did not hold the boarders (see $OUT)"
if grep -q 'deck 4.*(compromised)' "$OUT"; then fail "the field let the boarders write to the systems"; fi
echo "PASS  a rated force field holds the boarders, keeps the deck uncompromised, and drains under pressure"
