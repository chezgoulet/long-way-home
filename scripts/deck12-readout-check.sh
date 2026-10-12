#!/usr/bin/env bash
# The deck-12 environmental readout (G18, docs/evidence/the-deck12-readout.md): a person standing
# at the watch console in environmental control reads the deck's own air, the countdown to what the
# design calls unbreathable, and which source is holding that air -- the finding's "a surface in the
# room where the problem is".
#
#   scripts/deck12-readout-check.sh
#
# One headless run on the merged ship (`g_shipTest 101`). It photographs the readout twice -- a deck
# holding its air and the same deck failing -- and asserts it from both halves: the numbers the
# surface is drawn from (the log), and the picture on the surface (tools/frames/frame.py). The
# readout's region must be legible in both frames and must *differ* between them, in a region the
# HUD does not occupy and above a threshold a HUD cannot reach.
#
# LWH_ENV_BREAK=1 plants the break: the run is passed `g_shipEnvStatic 1`, which cuts the wiring from
# the deck's air to the surface, so the readout stops following the deck. The check must then FAIL --
# the two states photograph identically -- which is how it is shown to be able to fail. Restore by
# running it again with no environment.
#
# Needs the merged ship (scripts/build-ship.sh) and the built engine + modules. Writes under
# build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
PK3="$ROOT/build/ship/out/longway_voyager.pk3"
HOLD="$GAME_DIR/screenshots/lwh_env_hold.tga"
LOW="$GAME_DIR/screenshots/lwh_env_low.tga"
AUX="$GAME_DIR/screenshots/lwh_env_aux.tga"
OUT="$HOME_DIR/deck12-readout.out"
# The readout's own region on the screen, in the frame: measured from the frame (the screen's dark
# rectangle, 301..979 x 337..678) and inset to its text. Naming the region is what keeps the
# difference the claim's and not the room's: the HUD sits in the corners and the weapon along the
# bottom centre, none of it inside this rectangle. The second region is the readout's source line
# alone -- the third of the triad, which the deck's feed changes.
REGION="env:320,350,860,660"
SRC_REGION="src:325,598,860,668"

[ -f "$PK3" ] || { echo "no merged ship at $PK3 -- run scripts/build-ship.sh" >&2; exit 1; }
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

mkdir -p "$GAME_DIR/screenshots"
cp "$PK3" "$GAME_DIR/"
trap 'find "$GAME_DIR" -maxdepth 1 -name "longway_voyager.pk3" -delete' EXIT
rm -f "$HOLD" "$LOW" "$AUX"

BREAK=0
EXTRA=""
if [ "${LWH_ENV_BREAK:-0}" = "1" ]; then
  BREAK=1
  EXTRA="+set g_shipEnvStatic 1"
  echo "==> planted: the readout's wiring cut (g_shipEnvStatic 1); the check must fail"
fi

find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> the deck-12 environmental readout (g_shipTest 101)"
# shellcheck disable=SC2086
SDL_AUDIODRIVER=dummy timeout 240 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set com_hunkMegs 768 +set s_useOpenAL 0 +set g_ship 1 +set g_shipDeckPitch 3072 \
    $EXTRA +set g_shipTest 101 +map voyager >"$OUT" 2>&1 || true

grep -h '^SHIP: env readout' "$OUT" | sed 's/^SHIP: /    /' || true

fail() { echo "FAIL  $1"; exit 1; }

[ -f "$HOLD" ] || fail "the run did not photograph the readout holding (no $HOLD)"
[ -f "$LOW" ] || fail "the run did not photograph the readout failing (no $LOW)"
[ -f "$AUX" ] || fail "the run did not photograph the readout on its own source (no $AUX)"
grep -q '^SHIP: env readout: standing at ' "$OUT" || fail "the harness never stood at the watch post (see $OUT)"

# 1. The numbers, not a glance: the three states are the deck's own, and the third is the deck held
#    on an independent source -- the triad the air pass built (docs/evidence/the-air.md).
grep -qE '^SHIP: env readout: deck 12 air 100% holding, source the main grid$' "$OUT" \
  || fail "the deck did not read as holding its air on the main grid"
grep -qE '^SHIP: env readout: deck 12 air [0-9]+% [0-9]+\.[0-9] min to airless, source the main grid$' "$OUT" \
  || fail "the breached deck did not read a countdown to airless -- a static label would fail here"
grep -qE '^SHIP: env readout: deck 12 air [0-9]+% holding, source auxiliary fusion$' "$OUT" \
  || fail "the deck held on its own source did not read it"
grep -q 'aim yaw 180 pitch 0' "$OUT" || fail "the post did not face the readout"

# 2. The picture: the readout's own region is legible in every frame (not black, not a flat fill).
python3 "$ROOT/tools/frames/frame.py" floor --region "$REGION" \
    --min-median 0 --min-p90 80 --min-spread 40 --require "$HOLD" "$LOW" "$AUX" \
  || fail "a readout frame is dark or flat in the readout's own region: the surface does not read"

# 3. And it changes, in the region the readout owns, by more than a HUD can move it.
if python3 "$ROOT/tools/frames/frame.py" diff --region "$REGION" \
      --min-changed 0.10 --min-delta 24 --require "$HOLD" "$LOW"; then
  if [ "$BREAK" = "1" ]; then
    fail "with the wiring cut the two states still differed: the check is not reading the readout"
  fi
else
  if [ "$BREAK" = "1" ]; then
    echo "FAIL  with the wiring cut the two states photographed identically (the readout stopped"
    echo "      following the deck) -- the check caught the planted break, as it must"
    exit 1
  fi
  fail "the readout did not change between a deck holding its air and the same deck failing"
fi

# 4. The source line is drawn from the deck's feed: hold on the main grid and held on the auxiliary
#    differ there, and only the readout's own surface carries that difference.
python3 "$ROOT/tools/frames/frame.py" floor --region "$SRC_REGION" \
    --min-median 0 --min-p90 80 --min-spread 40 --require "$HOLD" "$AUX" \
  || fail "the source line does not read in a frame"
python3 "$ROOT/tools/frames/frame.py" diff --region "$SRC_REGION" \
    --min-changed 0.05 --min-delta 24 --require "$HOLD" "$AUX" \
  || fail "the source line did not change when the deck moved to an independent source"

echo "PASS  the deck's air, its countdown and its source are drawn at the deck-12 watch console;"
echo "      a holding deck, the same deck failing, and the deck on its own source photograph"
echo "      differently in the readout's own region and on its source line"
echo "      frames: $HOLD, $LOW, $AUX"
