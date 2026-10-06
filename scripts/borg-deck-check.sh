#!/usr/bin/env bash
# S8's first hard problem on the merged ship: a generated deck assimilated in the simulation turns
# Borg in the world, and is stripped back. The deck's own shaders (deck-unique since 2026-10-07) are
# remapped to textures/lwh/borg by the module (BorgAssets) and remapped back when it is reclaimed.
#
#   scripts/borg-deck-check.sh
#
# Needs the merged ship: run scripts/build-ship.sh first (its pak is copied into build/g3-home/baseEF,
# the writable home path). Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
PAK="$ROOT/build/ship/out/longway_voyager.pk3"
BEFORE="$GAME_DIR/screenshots/lwh_borg_deck_before.tga"
PARTIAL="$GAME_DIR/screenshots/lwh_borg_deck_partial.tga"
AFTER="$GAME_DIR/screenshots/lwh_borg_deck_after.tga"
[ -f "$PAK" ] || { echo "no merged ship at $PAK -- run scripts/build-ship.sh" >&2; exit 1; }
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

fail() { echo "FAIL  $1"; exit 1; }

mkdir -p "$GAME_DIR/scripts" "$GAME_DIR/screenshots"
cp "$PAK" "$GAME_DIR/"
cp "$ROOT/tools/shipmap/data/lwh_borg.shader" "$GAME_DIR/scripts/"
rm -f "$BEFORE" "$PARTIAL" "$AFTER"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> the Borg swap on a generated deck"
SDL_AUDIODRIVER=dummy timeout 260 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipDeckPitch 3072 +set g_shipTest 25 +map voyager >"$HOME_DIR/borgdship.out" 2>&1 || true

grep -h '^SHIP: borg deck test\|^SHIP: deck 6' "$HOME_DIR/borgdship.out" | sed 's/^SHIP: /    /' || true
grep -q '^SHIP: borg deck test: standing on deck 6' "$HOME_DIR/borgdship.out" || fail "the player did not reach deck 6 on the merged ship"
grep -q '^SHIP: deck 6 is turning Borg (2 of 4 sections)' "$HOME_DIR/borgdship.out" \
  || fail "a half-assimilated deck did not turn Borg part by part"
grep -q '^SHIP: deck 6 has turned Borg (4 of 4 sections)' "$HOME_DIR/borgdship.out" || fail "the assimilated deck did not turn wholly Borg in the world"
[ -f "$BEFORE" ] && [ -f "$PARTIAL" ] && [ -f "$AFTER" ] || fail "no before/partial/after screenshots of the deck"
echo "PASS  a deck the simulation assimilates turns Borg part by part in the world, and is stripped back"
echo "      screenshots: $BEFORE, $PARTIAL, $AFTER (a person judges the tint; the swap is in the log)"
