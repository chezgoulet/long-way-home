#!/usr/bin/env bash
# Gate S4's "glance": the ship's live state drawn at a panel in the world.
#
#   scripts/s4-glance-check.sh
#
# One headless run on deck 4. The harness stands at a usable panel, and a one-line cgame seam
# (patches/0013) lets our module draw the ship's live state there. The transcript names the panel
# the readout was drawn at and a screenshot is left behind. Needs the built modules.
#
# Not checked: the surface itself painted with state (the second, engine half of "both"); this is
# the anchored readout that stands in for it first. Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
SHOT="$GAME_DIR/screenshots/lwh_glance.tga"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$GAME_DIR/screenshots"
rm -f "$SHOT"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> glance"
SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipTest 13 +map tour/deck04 >"$HOME_DIR/s4-glance.out" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h 'SHIP: glance test: standing at\|LWH: ship glance at' "$HOME_DIR/s4-glance.out" | sed 's/^EFSP: /    /' || true
grep -q 'SHIP: glance test: standing at ' "$HOME_DIR/s4-glance.out" \
  || fail "the harness did not find a usable panel to stand at (see $HOME_DIR/s4-glance.out)"
grep -q 'LWH: ship glance at ' "$HOME_DIR/s4-glance.out" \
  || fail "the ship's state was not drawn at the panel"
[ -f "$SHOT" ] || fail "no screenshot of the glance"
echo "PASS  the ship's live state is drawn at the panel the player stands at"
echo "      screenshot: $SHOT"
