#!/usr/bin/env bash
# Deck 13's re-dress (docs/locations/deck13-life-support.brief.md, second in the deck build order of
# docs/ship-master-map.md): the life-support plant is copied from tour/deck11 and changed to be this
# room -- a central machinery island, a raised catwalk, a Jefferies tube -- reachable at the
# turbolift arrival and up on the catwalk, and photographed for the owner's own walkthrough.
#
#   scripts/deck13-check.sh
#
# Needs the merged ship: run scripts/build-ship.sh first (its pak is copied into build/g3-home/baseEF,
# the writable home path). Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
PAK="$ROOT/build/ship/out/longway_voyager.pk3"
SHOT="$GAME_DIR/screenshots/lwh_deck13.tga"
SHOT_CATWALK="$GAME_DIR/screenshots/lwh_deck13_catwalk.tga"
[ -f "$PAK" ] || { echo "no merged ship at $PAK -- run scripts/build-ship.sh" >&2; exit 1; }
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

fail() { echo "FAIL  $1"; exit 1; }

mkdir -p "$GAME_DIR/scripts" "$GAME_DIR/screenshots"
cp "$PAK" "$GAME_DIR/"
cp "$ROOT/tools/shipmap/data/lwh_borg.shader" "$GAME_DIR/scripts/"
rm -f "$SHOT" "$SHOT_CATWALK"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> the deck 13 re-dress"
SDL_AUDIODRIVER=dummy timeout 300 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipDeckPitch 3072 +set g_shipTest 27 +map voyager >"$HOME_DIR/deck13.out" 2>&1 || true

grep -h '^SHIP: deck 13 room' "$HOME_DIR/deck13.out" | sed 's/^SHIP: /    /' || true
grep -q '^SHIP: deck 13 room: standing at ' "$HOME_DIR/deck13.out" || fail "the player did not reach deck 13 on the merged ship"
grep -q '^SHIP: deck 13 room: at the catwalk: standing at ' "$HOME_DIR/deck13.out" || fail "the player did not stand on the deck 13 catwalk"
grep -q 'in solid' "$HOME_DIR/deck13.out" && fail "a waypoint is in solid on deck 13"
[ -f "$SHOT" ] || fail "no screenshot of the deck 13 re-dress"
[ -f "$SHOT_CATWALK" ] || fail "no screenshot from the deck 13 catwalk"
echo "PASS  the deck 13 re-dress loads, is reachable on foot, and is photographed for the walkthrough"
echo "      screenshots: $SHOT, $SHOT_CATWALK"
