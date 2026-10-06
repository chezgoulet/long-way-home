#!/usr/bin/env bash
# S8's first hard problem: runtime asset replacement. gi.RemapShader swaps a surface's shader for the
# Borg one live, and back. This is the engine capability; the score is the screenshot and the log.
#
#   scripts/borgfx-check.sh
#
# One headless run on deck 4. The Borg shader is written into the writable home path, the floor's
# shader is remapped to it, a screenshot is taken, and it is remapped back. On the generated decks of
# the merged ship the module does this per deck by itself (module/ship/g_ship.cpp, BorgAssets), which
# needs the merged map to see. Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
SHOT="$GAME_DIR/screenshots/lwh_borgfx.tga"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

fail() { echo "FAIL  $1"; exit 1; }

# The Borg shader must be in the search path the engine reads at map load.
mkdir -p "$GAME_DIR/scripts" "$GAME_DIR/screenshots"
cp "$ROOT/tools/shipmap/data/lwh_borg.shader" "$GAME_DIR/scripts/"
rm -f "$SHOT"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> runtime asset replacement"
SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipTest 24 +map tour/deck04 >"$HOME_DIR/borgfx.out" 2>&1 || true

grep -h '^SHIP: Borg asset replacement\|^SHIP: borgfx test' "$HOME_DIR/borgfx.out" | sed 's/^SHIP: /    /' || true
grep -q '^SHIP: Borg asset replacement on$' "$HOME_DIR/borgfx.out" || fail "the surface was not remapped to the Borg shader"
grep -q '^SHIP: Borg asset replacement off$' "$HOME_DIR/borgfx.out" || fail "the surface was not remapped back"
[ -f "$SHOT" ] || fail "no screenshot of the Borg surface"
echo "PASS  a surface's shader is swapped for the Borg one live and swapped back"
echo "      screenshot: $SHOT (a person judges the tint; the capability is the swap)"
