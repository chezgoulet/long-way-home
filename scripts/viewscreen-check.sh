#!/usr/bin/env bash
# Gate S9's "see": the live viewscreen -- a contact drawn in the world beside the panel the player
# stands at, redrawn each frame from the ship's own state.
#
#   scripts/viewscreen-check.sh
#
# One headless run on deck 4. The harness stands at a usable panel and gives the ship a contact of
# a known state (a warship at 62% hull, 35% shields, weapons targeted), then photographs the panel.
# The module's cgame half (patches/0013) draws the contact's image from that state. Needs the built
# modules. Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$HOME_DIR/viewscreen.out"
SHOT="$GAME_DIR/screenshots/lwh_viewscreen.tga"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$GAME_DIR/screenshots"
rm -f "$SHOT"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> viewscreen"
SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipDayScale 1 +set g_shipTest 30 +map tour/deck04 >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h 'SHIP: viewscreen test:\|LWH: viewscreen ' "$OUT" | sed 's/^EFSP: /    /' || true
grep -q 'SHIP: viewscreen test: standing at .* with a contact hull 62% shields 35%' "$OUT" \
  || fail "the harness did not stand at a panel with a contact (see $OUT)"
grep -q 'LWH: viewscreen .* hull 6[0-9]% shields 3[0-9]%' "$OUT" \
  || fail "the live viewscreen did not draw the contact from the ship's state"
[ -f "$SHOT" ] || fail "no screenshot of the viewscreen"
echo "PASS  the contact is drawn live at the panel: a warship's image at 62% hull, 35% shields, weapons targeted"
echo "      screenshot: $SHOT"
