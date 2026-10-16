#!/usr/bin/env bash
# O10 / G20, the panel half: the in-world control at the panel.
#
#   scripts/panel-control-check.sh
#
# One headless run on the merged ship (`g_shipTest 104`). The harness stands a facing-pace from a
# status screen, clears the script's own movement lock (the test's, not the game's -- a hand at a
# panel is not locked), and forges the use the way the engine's own use runs (ClientThink ->
# PM_Use -> TryUse). The panel the ship itself carries (the module's shader list, marked on the
# brush by the deck tooling as `lwh_panel*`) then changes the condition where the control is; a
# person who may not call it gets the refusal and the panel is unchanged; and the same forge at a
# brush that carries no panel shader (the deck-12 readout, `lwh/env`) changes nothing.
#
# LWH_PANEL_BREAK=1 plants the break: the run is passed `g_shipPanelBreak 1`, which aims the
# *primary* forge at that plain brush instead of a panel, so the "the condition changed" assertion
# has nothing to hold it up. The check must then FAIL -- which is how it is shown it can fail, and
# that a green line is not won by standing anywhere near a screen.
#
# The check is bound to the module's own list: `tools/shipmap/markpanels.py` reads the shader list
# out of `module/ship/lwh_panel.cpp` and asserts the built map has no readout brush left unmarked
# (`--require-marked`). Add a panel and the tooling, the module's remap and this check move together.
#
# Needs the merged ship (scripts/build-ship.sh) and the built engine + modules. Writes under
# build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
PK3="$ROOT/build/ship/out/longway_voyager.pk3"
MAP="$ROOT/build/ship/voyager.map"
OUT="$HOME_DIR/panel-control.out"
SHOT_B="$GAME_DIR/screenshots/lwh_panel_before.tga"
SHOT_A="$GAME_DIR/screenshots/lwh_panel_after.tga"
# The readout's own rectangle on the screen, in the frame: measured from the frames (the 128x128
# screen, x 435..900 y 540..665) and inset to the text and the alert band. The HUD and the weapon
# sit outside it (the crosshair above, the weapon below right), so what changes here is the panel's.
REGION="panel:440,540,900,660"

[ -f "$PK3" ] || { echo "no merged ship at $PK3 -- run scripts/build-ship.sh" >&2; exit 1; }
[ -f "$MAP" ] || { echo "no merged ship source at $MAP -- run scripts/build-ship.sh" >&2; exit 1; }
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

fail() { echo "FAIL  $1"; exit 1; }

# 1. The list, bound to the module: the tool reads the shader list from the module's own source, and
#    every readout brush must already be marked (or the affordance is not placed).
echo "==> the panels, from the module's list"
python3 "$ROOT/tools/shipmap/markpanels.py" --map "$MAP" --require-marked \
  || fail "a readout brush is not marked: the affordance is not placed (tools/shipmap/markpanels.py)"

# 2. The run.
mkdir -p "$GAME_DIR"
cp "$PK3" "$GAME_DIR/"
trap 'find "$GAME_DIR" -maxdepth 1 -name "longway_voyager.pk3" -delete' EXIT
find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
rm -f "$SHOT_B" "$SHOT_A"

BREAK=0
EXTRA=""
if [ "${LWH_PANEL_BREAK:-0}" = "1" ]; then
  BREAK=1
  EXTRA="+set g_shipPanelBreak 1"
  echo "==> planted: the primary forge is aimed at a brush with no panel shader; the check must fail"
fi

echo "==> the in-world control at the panel (g_shipTest 104)"
# shellcheck disable=SC2086
SDL_AUDIODRIVER=dummy timeout 240 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set com_hunkMegs 768 +set s_useOpenAL 0 +set g_ship 1 +set g_shipDeckPitch 3072 \
    $EXTRA +set g_shipTest 104 +map voyager >"$OUT" 2>&1 || true

grep -h '^SHIP: panel test' "$OUT" | sed 's/^SHIP: /    /' || true

grep -q '^SHIP: panel test: the map carries lwh_panel_' "$OUT" \
  || fail "the run did not find the panel screens in the map (see $OUT)"

# 3. The decisive assertion: a forge at a panel changed the condition *at the panel*.
PANEL_LINE="$(grep -E '^SHIP: panel test: forged at lwh_panel_[^ ]* \(panel\); condition [0-9]+ -> [0-9]+$' "$OUT" | tail -1 || true)"
panel_changed=0
if [ -n "$PANEL_LINE" ]; then
  before="$(printf '%s\n' "$PANEL_LINE" | sed -E 's/.*condition ([0-9]+) -> [0-9]+$/\1/')"
  after="$(printf '%s\n' "$PANEL_LINE" | sed -E 's/.*condition [0-9]+ -> ([0-9]+)$/\1/')"
  [ "$before" != "$after" ] && panel_changed=1
fi

if [ "$panel_changed" = "1" ]; then
  if [ "$BREAK" = "1" ]; then
    fail "with the break the panel still changed: the check is not reading the panel"
  fi
else
  if [ "$BREAK" = "1" ]; then
    echo "FAIL  with the break the primary forge was made at a brush with no panel shader and the"
    echo "      condition did not change -- the check caught the planted break, as it must"
    exit 1
  fi
  fail "a forge at a status screen did not change the condition (the panel did not respond)"
fi

# 3b. And the picture: the panel's own readout is legible in both frames and *differs* between them,
#     in the readout's own region -- the condition and the alert band, where the control is.
[ -f "$SHOT_B" ] || fail "the run did not photograph the panel before the use (no $SHOT_B)"
[ -f "$SHOT_A" ] || fail "the run did not photograph the panel after the use (no $SHOT_A)"
python3 "$ROOT/tools/frames/frame.py" floor --region "$REGION" \
    --min-median 0 --min-p90 60 --min-spread 30 --require "$SHOT_B" "$SHOT_A" \
  || fail "the panel's readout region is dark or flat: the surface does not read"
python3 "$ROOT/tools/frames/frame.py" diff --region "$REGION" \
    --min-changed 0.05 --min-delta 24 --require "$SHOT_B" "$SHOT_A" \
  || fail "the panel's readout did not change between the two frames"

# 4. The refusal: a person who may not call it, refused by name, and the panel unchanged.
REF_LINE="$(grep -E '^SHIP: panel test: refusal ".+"; condition [0-9]+ -> [0-9]+$' "$OUT" | tail -1 || true)"
[ -n "$REF_LINE" ] || fail "the run made no refusal (see $OUT)"
rbefore="$(printf '%s\n' "$REF_LINE" | sed -E 's/.*condition ([0-9]+) -> [0-9]+$/\1/')"
rafter="$(printf '%s\n' "$REF_LINE" | sed -E 's/.*condition [0-9]+ -> ([0-9]+)$/\1/')"
[ "$rbefore" = "$rafter" ] || fail "the refusal changed the condition: it should have changed nothing"
printf '%s\n' "$REF_LINE" | grep -q 'the condition is not yours to call' \
  || fail "the refusal did not name who may call it"

# 5. The falsification, in the same run: the same forge at a brush that carries no panel shader.
PLAIN_LINE="$(grep -E '^SHIP: panel test: the plain brush left the condition at [0-9]+ \(before [0-9]+\)$' "$OUT" | tail -1 || true)"
[ -n "$PLAIN_LINE" ] || fail "the run made no plain-brush forge (see $OUT)"
pnow="$(printf '%s\n' "$PLAIN_LINE" | sed -E 's/.*condition at ([0-9]+) \(before ([0-9]+)\)$/\1/')"
pbefore="$(printf '%s\n' "$PLAIN_LINE" | sed -E 's/.*condition at ([0-9]+) \(before ([0-9]+)\)$/\2/')"
[ "$pnow" = "$pbefore" ] || fail "the plain brush changed the condition: the forge is not reading the panel"
# And the plain forge really reached a brush (the world), not empty air in front of the player.
grep -qE '^SHIP: panel probe: .* traceEnt 8190 ' "$OUT" \
  || fail "the plain forge did not reach a world brush: the falsification hit nothing"

echo "PASS  the in-world use at a status screen changes the condition the panel draws, and the"
echo "      panel's own readout differs between the two frames in the readout's region; a person"
echo "      who may not call it is refused and the panel is unchanged; and the same forge at a"
echo "      brush with no panel shader changes nothing"
echo "      transcript: $OUT"
echo "      frames: $SHOT_B, $SHOT_A"
