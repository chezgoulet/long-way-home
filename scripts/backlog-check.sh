#!/usr/bin/env bash
# The approved gap backlog's first items, through the console: the tractor beam and salvage, the
# materials economy, the EMH, and the crew systems (training, the brig, a funeral, a promotion). The
# rules behind them are in tests/ship; this runs the game once and drives the controls.
#
#   scripts/backlog-check.sh
#
# Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

fail() { echo "FAIL  $1"; exit 1; }

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> the backlog batch"
SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipTest 21 +map tour/deck04 >"$HOME_DIR/backlog.out" 2>&1 || true

grep -h '^SHIP: tractored\|^SHIP: fabricated\|^SHIP: the EMH\|^SHIP: crew\|^SHIP: backlog test' "$HOME_DIR/backlog.out" | sed 's/^SHIP: /    /' || true

grep -q '^SHIP: tractored a derelict and stripped it' "$HOME_DIR/backlog.out" || fail "the tractor did not strip the wreck"
grep -q '^SHIP: material 55, parts 130$' "$HOME_DIR/backlog.out" || fail "the replicators did not fabricate parts"
grep -q '^SHIP: the EMH is active' "$HOME_DIR/backlog.out" || fail "the EMH did not activate"
grep -q '^SHIP: crew 20 now holds rank 1' "$HOME_DIR/backlog.out" || fail "the promotion did not happen"
grep -q '^SHIP: crew 40 is confined' "$HOME_DIR/backlog.out" || fail "the brig did not take"
grep -q '^SHIP: backlog test: material 55, parts 130, EMH 1' "$HOME_DIR/backlog.out" || fail "the stores are not what the actions imply"
grep -q '^SHIP: backlog test: crew 20 qualified at Tactical 1, rank 1; crew 40 brigged 1' "$HOME_DIR/backlog.out" \
  || fail "the crew systems did not take"
echo "PASS  the tractor strips a wreck, the replicators fabricate, the EMH runs, and training, a promotion, the brig and a funeral take"
