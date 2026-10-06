#!/usr/bin/env bash
# S10: the player's character is the body the player walks in.
#
#   scripts/playerbody-check.sh
#
# One headless run on deck 4. The harness creates a character (Security, rank 2), gives them a known
# crew type (Tuvok), and applies the body: the module issues the game's own headModel/torsoModel/
# legsModel, so the player's head is the character's and the uniform is the department's. The
# transcript names the body the module asked for and a screenshot is left behind. Needs the built
# modules. Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
OUT="$HOME_DIR/playerbody.out"
SHOT="$GAME_DIR/screenshots/lwh_body.tga"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$GAME_DIR/screenshots"
rm -f "$SHOT"

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> player body"
SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipDayScale 1 +set g_shipTest 33 +map tour/deck04 >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h 'SHIP: .* walks as head' "$OUT" | sed 's/^EFSP: /    /' || true

grep -q 'SHIP: Tuvok Test walks as head tuvok/default, torso crewthin/gold, legs crewthin/default' "$OUT" \
  || fail "the player's body was not set from the crew record (see $OUT)"
if grep -qiE "couldn't find model|model .* not found|RegisterModel.*fail" "$OUT"; then
  grep -iE "couldn't find model|model .* not found|RegisterModel.*fail" "$OUT"
  fail "a model the body names could not be loaded"
fi
[ -f "$SHOT" ] || fail "no screenshot of the session"
echo "PASS  the player's character is the body walked in: the head is the character's, the uniform the department's"
echo "      screenshot: $SHOT"
