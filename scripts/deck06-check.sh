#!/usr/bin/env bash
# Deck 6's composition (docs/locations/deck06-holodecks.brief.md, the fifth and last re-dress in the
# deck build order of docs/ship-master-map.md, and the only one that composes): a quarters slice from
# tour/deck09, an armory room from _brig and a holodeck arch from a _holodeck_* programme map are
# carried into one corridor -- reached at the turbolift arrival, at the holodeck arch and at the
# armory, and photographed for the owner's own walkthrough (the check that decides whether the three
# read as three places).
#
#   scripts/deck06-check.sh
#
# Needs the merged ship: run scripts/build-ship.sh first (its pak is copied into build/g3-home/baseEF,
# the writable home path). Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
PAK="$ROOT/build/ship/out/longway_voyager.pk3"
SHOT="$GAME_DIR/screenshots/lwh_deck06.tga"
SHOT_HOLO="$GAME_DIR/screenshots/lwh_deck06_holodeck.tga"
SHOT_ARM="$GAME_DIR/screenshots/lwh_deck06_armory.tga"
[ -f "$PAK" ] || { echo "no merged ship at $PAK -- run scripts/build-ship.sh" >&2; exit 1; }
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

fail() { echo "FAIL  $1"; exit 1; }

mkdir -p "$GAME_DIR/scripts" "$GAME_DIR/screenshots"
cp "$PAK" "$GAME_DIR/"
cp "$ROOT/tools/shipmap/data/lwh_borg.shader" "$GAME_DIR/scripts/"
rm -f "$SHOT" "$SHOT_HOLO" "$SHOT_ARM"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> the deck 6 composition"
SDL_AUDIODRIVER=dummy timeout 300 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipDeckPitch 3072 +set g_shipTest 63 +map voyager >"$HOME_DIR/deck06.out" 2>&1 || true

grep -h '^SHIP: deck 6 room' "$HOME_DIR/deck06.out" | sed 's/^SHIP: /    /' || true
grep -q '^SHIP: deck 6 room: standing at ' "$HOME_DIR/deck06.out" || fail "the player did not reach deck 6 on the merged ship"
grep -q '^SHIP: deck 6 room: at the holodeck: standing at ' "$HOME_DIR/deck06.out" || fail "the player did not stand at the deck 6 holodeck arch"
grep -q '^SHIP: deck 6 room: at the armory: standing at ' "$HOME_DIR/deck06.out" || fail "the player did not stand at the deck 6 armory post"
grep -q 'in solid' "$HOME_DIR/deck06.out" && fail "a waypoint is in solid on deck 6"
[ -f "$SHOT" ] || fail "no screenshot of the deck 6 corridor"
[ -f "$SHOT_HOLO" ] || fail "no screenshot from the deck 6 holodeck end"
[ -f "$SHOT_ARM" ] || fail "no screenshot from the deck 6 armory"
echo "PASS  the deck 6 composition loads, is reachable on foot, and is photographed for the walkthrough"
echo "      screenshots: $SHOT, $SHOT_HOLO, $SHOT_ARM"
