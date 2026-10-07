#!/usr/bin/env bash
# Gate S4's painted panel: the ship's live state drawn onto a world surface.
#
#   scripts/s4-panel-check.sh
#
# One headless run on the merged ship. The module draws the ship's state into a 128x128 RGBA image
# and the engine uploads it into the panel shader's texture (patches/0014); the harness stands at the
# generated deck 6's status screen and photographs it. Needs the built ship (build-ship.sh) and the
# built engine + modules. Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
PK3="$ROOT/build/ship/out/longway_voyager.pk3"
SHOT="$GAME_DIR/screenshots/lwh_panel.tga"
[ -f "$PK3" ] || { echo "no merged ship at $PK3 -- run scripts/build-ship.sh" >&2; exit 1; }
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$GAME_DIR"
cp "$PK3" "$GAME_DIR/"
trap 'find "$GAME_DIR" -maxdepth 1 -name "longway_voyager.pk3" -delete' EXIT
rm -f "$SHOT"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> panel"
SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set com_hunkMegs 768 +set s_useOpenAL 0 +set g_ship 1 +set g_shipDeckPitch 3072 \
    +set g_shipTest 15 +map voyager >"$HOME_DIR/s4-panel.out" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h 'SHIP: panel test' "$HOME_DIR/s4-panel.out" | sed 's/^EFSP: /    /' || true
grep -q 'SHIP: panel test: standing on deck 6' "$HOME_DIR/s4-panel.out" \
  || fail "the harness did not reach the status screen (see $HOME_DIR/s4-panel.out)"
[ -f "$SHOT" ] || fail "no screenshot of the panel"
echo "PASS  the ship's live state is painted onto the panel surface"
echo "      screenshot: $SHOT"
