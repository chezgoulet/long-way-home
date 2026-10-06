#!/usr/bin/env bash
# Deck 12's blockout (docs/locations/deck12-environmental-control.brief.md, the first in the deck
# build order of docs/ship-master-map.md): the generated environmental-control deck is a working
# half-deck in two chambers with the plant on a raised deck, reachable and walkable, photographed for
# the owner's approval before any detail.
#
#   scripts/deck12-check.sh
#
# Needs the merged ship: run scripts/build-ship.sh first (its pak is copied into build/g3-home/baseEF,
# the writable home path). Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
PAK="$ROOT/build/ship/out/longway_voyager.pk3"
SHOT="$GAME_DIR/screenshots/lwh_deck12.tga"
[ -f "$PAK" ] || { echo "no merged ship at $PAK -- run scripts/build-ship.sh" >&2; exit 1; }
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

fail() { echo "FAIL  $1"; exit 1; }

mkdir -p "$GAME_DIR/scripts" "$GAME_DIR/screenshots"
cp "$PAK" "$GAME_DIR/"
cp "$ROOT/tools/shipmap/data/lwh_borg.shader" "$GAME_DIR/scripts/"
rm -f "$SHOT"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> the deck 12 blockout"
SDL_AUDIODRIVER=dummy timeout 300 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipDeckPitch 3072 +set g_shipTest 26 +map voyager >"$HOME_DIR/deck12.out" 2>&1 || true

grep -h '^SHIP: deck 12 blockout' "$HOME_DIR/deck12.out" | sed 's/^SHIP: /    /' || true
grep -q '^SHIP: deck 12 blockout: standing at ' "$HOME_DIR/deck12.out" || fail "the player did not reach deck 12 on the merged ship"
grep -q 'in solid' "$HOME_DIR/deck12.out" && fail "a waypoint is in solid on deck 12"
[ -f "$SHOT" ] || fail "no screenshot of the deck 12 blockout"
echo "PASS  the deck 12 blockout loads, is reachable on foot, and is photographed for approval"
echo "      screenshot: $SHOT"
