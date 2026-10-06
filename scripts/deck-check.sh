#!/usr/bin/env bash
# A generated deck's blockout on the merged ship: reach it on foot and photograph it, for the owner's
# approval before detail. The deck build order is in docs/ship-master-map.md; the briefs are in
# docs/locations/.
#
#   scripts/deck-check.sh DECK
#
# Needs the merged ship: run scripts/build-ship.sh first (its pak is copied into build/g3-home/baseEF,
# the writable home path). Writes under build/g3-home only.

set -euo pipefail

DECK="${1:?usage: deck-check.sh DECK (6, 13, 14, ...)}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
PAK="$ROOT/build/ship/out/longway_voyager.pk3"
SHOT="$GAME_DIR/screenshots/lwh_deck$(printf '%02d' "$DECK").tga"
[ -f "$PAK" ] || { echo "no merged ship at $PAK -- run scripts/build-ship.sh" >&2; exit 1; }
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

fail() { echo "FAIL  $1"; exit 1; }

mkdir -p "$GAME_DIR/scripts" "$GAME_DIR/screenshots"
cp "$PAK" "$GAME_DIR/"
cp "$ROOT/tools/shipmap/data/lwh_borg.shader" "$GAME_DIR/scripts/"
rm -f "$SHOT"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> the deck $DECK blockout"
SDL_AUDIODRIVER=dummy timeout 300 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipDeckPitch 3072 +set g_shipTestPos "$DECK" +set g_shipTest 28 +map voyager >"$HOME_DIR/deck$DECK.out" 2>&1 || true

grep -h "^SHIP: deck $DECK blockout" "$HOME_DIR/deck$DECK.out" | sed 's/^SHIP: /    /' || true
grep -q "^SHIP: deck $DECK blockout: standing at " "$HOME_DIR/deck$DECK.out" || fail "the player did not reach deck $DECK on the merged ship"
grep -q 'in solid' "$HOME_DIR/deck$DECK.out" && fail "a waypoint is in solid on deck $DECK"
[ -f "$SHOT" ] || fail "no screenshot of the deck $DECK blockout"
echo "PASS  the deck $DECK blockout loads, is reachable on foot, and is photographed for approval"
echo "      screenshot: $SHOT"
