#!/usr/bin/env bash
# The reachability pass's first slice: the Tactical console's combat kit, reached by a key on an
# in-game screen rather than only by the developer console (docs/evidence/reachability.md).
#
# Before this pass, `SetTarget`, `Remodulate`, `RaidVinculum` and `OrderAdvance` were reached only
# by `ship target|remodulate|vinculum|advance`. The console now carries them: the screen's key sends
# `ship as 1 ...` for Tactical's acts and `ship advance ...` for command's, and the ship holds each
# to the station and the person exactly as it holds every other control.
#
#   scripts/reachability-check.sh
#
# One headless run (g_shipTest 72) on the measurement deck: a Borg fight is set up on the spot, the
# Tactical console is opened, and the target / remodulate / vinculum keys are pressed; then the
# command console is opened and the squad key is pressed. The engine's own output is the evidence.
# Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

fail() { echo "FAIL  $1"; exit 1; }
pass() { echo "PASS  $1"; }

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipTest 72 +map tour/deck04 >"$HOME_DIR/reachability.out" 2>&1 || true

echo "==> the Tactical console's combat kit, by key"
grep -h '^SHIP: counterplay test' "$HOME_DIR/reachability.out" | sed 's/^SHIP: /    /' || true

grep -qE 'counterplay test: aiming at (WEAPONS|ENGINES|SHIELD GENERATOR|HULL)' "$HOME_DIR/reachability.out" \
  || fail "the target key did not change what Tactical aims at (see $HOME_DIR/reachability.out)"
grep -qE 'counterplay test: adaptation [0-9]+% after remodulation \(cooldown [0-9]+ s\)' "$HOME_DIR/reachability.out" \
  || fail "the remodulate key did not rotate the modulation"
grep -qE 'counterplay test: vinculum suppressed for [0-9]+ s' "$HOME_DIR/reachability.out" \
  || fail "the vinculum key did not raid the vinculum"
grep -qE 'counterplay test: squad ordered to retake deck ([1-9]|1[0-5])' "$HOME_DIR/reachability.out" \
  || fail "the squad key did not order the security squad"
[ -f "$GAME_DIR/screenshots/lwh_counterplay_after.tga" ] || fail "no screenshot of the Tactical console after the kit"
pass "Tactical's combat kit is reachable in play: a key picks the target, remodulates and raids the vinculum, and command sends the squad"
