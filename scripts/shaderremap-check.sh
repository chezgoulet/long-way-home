#!/usr/bin/env bash
# Harvest B, first class: the adopted RPG-X target_shaderremap spawns and toggles a shader at run
# time in the single-player game.
#
#   scripts/shaderremap-check.sh
#
# One headless run on the merged ship. The generated decks carry a target_shaderremap entity
# (tools/shipmap/gendeck.py); the game spawns it (LWH: target_shaderremap falsename=... truename=...)
# and the harness fires it twice through the engine's own use-func dispatcher, which swaps the shader
# and swaps it back (gi.RemapShader). A shader the renderer cannot find would print
# "WARNING: RE_RemapShader ... not found", so its absence is the proof the remap landed.
# Needs the built ship (build-ship.sh) and the built engine + modules. Writes under build/g3-home.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
PK3="$ROOT/build/ship/out/longway_voyager.pk3"
OUT="$HOME_DIR/shaderremap.out"
[ -f "$PK3" ] || { echo "no merged ship at $PK3 -- run scripts/build-ship.sh" >&2; exit 1; }
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
mkdir -p "$GAME_DIR"
cp "$PK3" "$GAME_DIR/"
trap 'find "$GAME_DIR" -maxdepth 1 -name "longway_voyager.pk3" -delete' EXIT

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> shaderremap"
SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set com_hunkMegs 768 +set s_useOpenAL 0 +set g_ship 1 +set g_shipDeckPitch 3072 \
    +set g_shipTest 32 +map voyager >"$OUT" 2>&1 || true

fail() { echo "FAIL  $1"; exit 1; }
grep -h 'LWH: target_shaderremap\|SHIP: shaderremap test' "$OUT" | sed 's/^EFSP: /    /' || true

grep -q 'LWH: target_shaderremap falsename=textures/lwh/panel truename=textures/lwh/borg' "$OUT" \
  || fail "the target_shaderremap entity did not spawn (the class is not registered, or the fixture is missing)"
grep -q 'SHIP: shaderremap test: found target_shaderremap' "$OUT" \
  || fail "the harness did not find the entity in the map"
grep -q 'LWH: target_shaderremap on textures/lwh/borg' "$OUT" \
  || fail "firing the entity did not swap the shader to truename"
grep -q 'LWH: target_shaderremap off textures/lwh/borg' "$OUT" \
  || fail "firing the entity again did not swap the shader back"
if grep -q 'RE_RemapShader' "$OUT"; then
  grep 'RE_RemapShader' "$OUT"
  fail "the renderer reported the shader missing, so no remap landed"
fi
echo "PASS  the adopted target_shaderremap spawns, and firing it toggles a shader at run time and back"
