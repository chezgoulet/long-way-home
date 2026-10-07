#!/usr/bin/env bash
# A death's consequences in the game (docs/morale.md): a crew member dies and the quarters are sealed
# and readable, the name goes on the wall, and the funeral opens the quarters and leaves a positive
# mark in the crew who stood together. The rules are unit-tested by tests/ship
# (TestCrewJusticeAndBorg); this drives them through the game console, so what the player sees is
# observed rather than asserted.
#
#   scripts/grief-check.sh
#
# Needs the merged ship: run scripts/build-ship.sh first (its pak is copied into build/g3-home/baseEF,
# the writable home path). Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
PAK="$ROOT/build/ship/out/longway_voyager.pk3"
[ -f "$PAK" ] || { echo "no merged ship at $PAK -- run scripts/build-ship.sh" >&2; exit 1; }
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

fail() { echo "FAIL  $1"; exit 1; }

mkdir -p "$GAME_DIR/scripts"
cp "$PAK" "$GAME_DIR/"
cp "$ROOT/tools/shipmap/data/lwh_borg.shader" "$GAME_DIR/scripts/"
find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> a death and its consequences"
SDL_AUDIODRIVER=dummy timeout 300 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipDeckPitch 3072 +set g_shipTest 53 +map voyager >"$HOME_DIR/grief.out" 2>&1 || true

grep -h '^SHIP: grief:' "$HOME_DIR/grief.out" | sed 's/^SHIP: /    /' || true
grep -h "quarters, deck .* are sealed" "$HOME_DIR/grief.out" | sed 's/^SHIP: /    /' || true
grep -q '^SHIP: grief: .* is dead, [1-9][0-9]* quarters sealed, 1 on the wall' "$HOME_DIR/grief.out" \
    || fail "the death did not seal the quarters or reach the wall"
grep -q '^SHIP: grief: after the funeral 0 quarters sealed, 1 on the wall, crew 11 funeral mark 1' "$HOME_DIR/grief.out" \
    || fail "the funeral did not open the quarters or leave a positive mark"
echo "PASS  a death seals the quarters and puts a name on the wall; a funeral opens them and leaves a mark"
