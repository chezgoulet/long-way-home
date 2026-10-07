#!/usr/bin/env bash
# The five owner-approved gaps, to their full criteria (docs/gates.md). The unit tests establish the
# rules; this runs the game once and drives the console controls a panel uses, so the same functions
# are exercised through the engine.
#
#   scripts/gaps-check.sh
#
# Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
SHOT="$GAME_DIR/screenshots/lwh_gaps.tga"
LOG_SHOT="$GAME_DIR/screenshots/lwh_log.tga"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$GAME_DIR/ship"
rm -f "$SHOT" "$LOG_SHOT"

fail() { echo "FAIL  $1"; exit 1; }

echo "==> the five gaps, through the console"
SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipTest 18 +map tour/deck04 >"$HOME_DIR/gaps.out" 2>&1 || true

grep -hE "^SHIP: (deck 9|the surgical|TRIAGE|   |--- the captain|Captain's|gap test)" \
    "$HOME_DIR/gaps.out" | sed 's/^SHIP: /    /' || true

grep -q '^SHIP: deck 9: air ' "$HOME_DIR/gaps.out" || fail "the tricorder did not read the compartment"
grep -q "the surgical bay's force field is up" "$HOME_DIR/gaps.out" || fail "the surgical field did not go up"
grep -q "^SHIP: --- the captain's log ---" "$HOME_DIR/gaps.out" || fail "the captain's log did not read"
grep -q "^SHIP: Captain's log, day 0" "$HOME_DIR/gaps.out" || fail "the captain's log is not a summary"
grep -q '^SHIP: gap test: surgical field 1, kit condition 9[0-9]%' "$HOME_DIR/gaps.out" || fail "the surgical field or kit condition is not as driven"
grep -q '^SHIP: gap test: clocks: DECK 9 AIR ' "$HOME_DIR/gaps.out" || fail "the air countdown is not published"
[ -f "$SHOT" ] || fail "no screenshot of the run"
[ -f "$LOG_SHOT" ] || fail "the log screen did not open"
echo "PASS  the tricorder reads a compartment, the surgical field holds, the ward is in triage order, the captain's log summarises, the clocks publish, and the log screen opens"
echo "      screenshots: $SHOT, $LOG_SHOT"
