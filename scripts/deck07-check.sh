#!/usr/bin/env bash
# Deck 7's re-dress (docs/locations/deck07-auxcore.brief.md, the next in the deck build order of
# docs/ship-master-map.md): the auxiliary computer core, cargo and labs are copied from tour/deck11
# and changed to be this room -- a central auxiliary core column as the deck's spine, two cargo
# islands of the game's own crates, an overhead escape-pod hatch, working light and a blue glow at
# the core -- reached at the turbolift arrival and at the core-watch post, and photographed for the
# owner's own walkthrough.
#
#   scripts/deck07-check.sh
#
# Needs the merged ship: run scripts/build-ship.sh first (its pak is copied into build/g3-home/baseEF,
# the writable home path). Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
PAK="$ROOT/build/ship/out/longway_voyager.pk3"
SHOT="$GAME_DIR/screenshots/lwh_deck07.tga"
SHOT_CORE="$GAME_DIR/screenshots/lwh_deck07_core.tga"
[ -f "$PAK" ] || { echo "no merged ship at $PAK -- run scripts/build-ship.sh" >&2; exit 1; }
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

fail() { echo "FAIL  $1"; exit 1; }

mkdir -p "$GAME_DIR/scripts" "$GAME_DIR/screenshots"
cp "$PAK" "$GAME_DIR/"
cp "$ROOT/tools/shipmap/data/lwh_borg.shader" "$GAME_DIR/scripts/"
rm -f "$SHOT" "$SHOT_CORE"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> the deck 7 re-dress"
SDL_AUDIODRIVER=dummy timeout 300 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipDeckPitch 3072 +set g_shipTest 29 +map voyager >"$HOME_DIR/deck07.out" 2>&1 || true

grep -h '^SHIP: deck 7 room' "$HOME_DIR/deck07.out" | sed 's/^SHIP: /    /' || true
grep -q '^SHIP: deck 7 room: standing at ' "$HOME_DIR/deck07.out" || fail "the player did not reach deck 7 on the merged ship"
grep -q '^SHIP: deck 7 room: at the core: standing at ' "$HOME_DIR/deck07.out" || fail "the player did not stand at the deck 7 core-watch post"
grep -q 'in solid' "$HOME_DIR/deck07.out" && fail "a waypoint is in solid on deck 7"
[ -f "$SHOT" ] || fail "no screenshot of the deck 7 re-dress"
[ -f "$SHOT_CORE" ] || fail "no screenshot from the deck 7 core column"
echo "PASS  the deck 7 re-dress loads, is reachable on foot, and is photographed for the walkthrough"
echo "      screenshots: $SHOT, $SHOT_CORE"
