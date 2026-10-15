#!/usr/bin/env bash
# The derelict interior -- O11's first archetype (docs/outside-the-ship.md: "wrecked corridors, failing
# light, no gravity in places"). Re-dressed from `scav2` by tools/away/dress_derelict.py.
#
#   scripts/derelict-check.sh
#
# What it does, and what each step proves:
#   re-dress   the source's own level is cropped, shelled and lit FAILING (tools/away/dress_derelict.py)
#   build      scripts/build-map.sh compiles it and tools/mapgen/check-bsp.py reads the artifact
#   census     tools/shipmap/parts_census.py: every material already exists in the game
#   load       the map loads, the player connects, the nav bakes
#   measure    `frame.py derelict` -- the archetype's OWN assertion: dark (failing, not a lit room),
#              STRUCTURED (an edge fraction, the read of wreckage) and MARKED (a lit pixel, failing
#              light not absent). The room floor (`frame.py floor`) is NOT used and NO per-frame dark
#              exemption is allowed for this archetype.
#   falsify    the SAME measure must FAIL on the un-lit capture -- the re-dress with no lights. A
#              measure that cannot fail on a frame that does not carry the claim is decoration.
#   zero-g     g_shipTest 102: the map's own trigger -> target_gravity_change, read from ps.gravity --
#              normal in the hall, zero in the zone (docs/evidence/derelict-interior.md)
#
# Writes under build/away/derelict* and build/away-home only. The installation is read-only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/away-home"
GAME_DIR="$HOME_DIR/baseEF"
SRC="$ROOT/build/gdk/maps/eliteforce_scav_maps/scav2.map"
MAP="lwh_derelict"
GEN="$ROOT/build/away/derelict"
OUT="$ROOT/build/away/derelict-out"
OUT_UNLIT="$ROOT/build/away/derelict-unlit-out"
FRAMES_OUT="${LWH_DERELICT_FRAMES:-/home/c/lwh-frames/the-derelict}"

[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
[ -f "$SRC" ] || { echo "no source at $SRC -- run scripts/fetch-gdk.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

fail() { echo "FAIL  $1"; exit 1; }
pass() { echo "PASS  $1"; }

mkdir -p "$GEN" "$OUT" "$OUT_UNLIT" "$GAME_DIR/scripts" "$GAME_DIR/screenshots" "$FRAMES_OUT/lit" "$FRAMES_OUT/unlit"

# One re-dress + build + load + shoot at the console post.
#   shoot OUTDIR SET [extra dress flags...]
shoot() {
  local outdir="$1" set="$2"; shift 2
  local -a flags=("$@")
  python3 "$ROOT/tools/away/dress_derelict.py" --source "$SRC" --out "$GEN" "${flags[@]}" \
      --report "$outdir/dress.json" --census "$outdir/census.json" >/dev/null || fail "the re-dress wrote no map ($set)"
  bash "$ROOT/scripts/build-map.sh" "$GEN/$MAP.map" --name "$MAP" --out "$outdir" --light >"$outdir/build.log" 2>&1 \
      || { tail -5 "$outdir/build.log"; fail "the map did not build ($set)"; }
  cp "$outdir/longway_$MAP.pk3" "$GAME_DIR/"
  rm -f "$GAME_DIR/maps/$MAP.nav"
  find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
  rm -rf "${GAME_DIR:?}/screenshots"; mkdir -p "$GAME_DIR/screenshots"
  SDL_AUDIODRIVER=dummy timeout 240 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
      +set s_useOpenAL 0 +set g_ship 1 +set g_shipTest 96 +set g_shipVantage "lwh_derelict_console" \
      +set g_shipVantagePost "lwh_post_console" +map "$MAP" >"$HOME_DIR/derelict-$set.out" 2>&1 || true
  grep -q "CM_LoadMap(maps/$MAP.bsp)" "$HOME_DIR/derelict-$set.out" || fail "the map did not load ($set)"
  grep -q "Munro connected" "$HOME_DIR/derelict-$set.out" || fail "the player never connected ($set)"
  rm -rf "${FRAMES_OUT:?}/$set"; mkdir -p "$FRAMES_OUT/$set"
  cp "$GAME_DIR/screenshots/lwh_derelict_console"*.tga "$FRAMES_OUT/$set/" || fail "no frames ($set)"
  [ "$(find "$FRAMES_OUT/$set" -name '*.tga' | wc -l)" -eq 6 ] || fail "the console post gave no six frames ($set)"
}

echo "==> the re-dress (lit): the source's own level, shelled and lit failing"
shoot "$OUT" lit

echo
echo "==> the build, the artifact, and the parts census (no new art)"
grep -q "OK" "$OUT/build.log" || fail "the BSP is empty or malformed"
grep "OK" "$OUT/build.log" | sed 's/^/    /'
python3 "$ROOT/tools/shipmap/parts_census.py" --map "$GEN/$MAP.map" --game "$ROOT/build/baseEF" \
    --census "$OUT/census.json" || fail "the derelict names art the game does not have"

echo
echo "==> the archetype's own assertion on its own frames (must read DERELICT)"
python3 "$ROOT/tools/frames/frame.py" derelict --band 0.10,0.55 --require "$FRAMES_OUT/lit"/*.tga \
    | sed 's/^/    /' || fail "a frame does not read as a derelict (failing light, structured, marked)"
pass "every frame reads as a derelict: dark, structured, and marked"

echo
echo "==> the falsification: the SAME measure must FAIL on the un-lit before-capture"
shoot "$OUT_UNLIT" unlit --unlit
if python3 "$ROOT/tools/frames/frame.py" derelict --band 0.10,0.55 --require "$FRAMES_OUT/unlit"/*.tga \
    >"$HOME_DIR/derelict-unlit-measure.txt" 2>&1; then
  sed 's/^/    /' "$HOME_DIR/derelict-unlit-measure.txt"
  fail "the measure PASSED the un-lit capture: it cannot tell failing light from absent light"
fi
sed 's/^/    /' "$HOME_DIR/derelict-unlit-measure.txt"
pass "the measure fails on the un-lit capture, so it has teeth"

echo
echo "==> no gravity in places: the zone's own trigger -> target_gravity_change, read from ps.gravity"
# Re-shoot the lit map (the unlit build overwrote the pk3) and run the zero-g harness.
shoot "$OUT" lit
find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipTest 102 +map "$MAP" >"$HOME_DIR/derelict-zerog.out" 2>&1 || true
grep -h "SHIP: zero-g:" "$HOME_DIR/derelict-zerog.out" | sed 's/^/    /'
HALL="$(grep -oE "zero-g: hall  ps.gravity [0-9-]+" "$HOME_DIR/derelict-zerog.out" | grep -oE "[0-9-]+$" || true)"
ZONE="$(grep -oE "zero-g: zone  ps.gravity [0-9-]+" "$HOME_DIR/derelict-zerog.out" | grep -oE "[0-9-]+$" || true)"
[ "${HALL:-x}" != "x" ] || fail "the hall gravity was not read"
[ "${ZONE:-x}" = "0" ] || fail "the zone's gravity is not zero (read '${ZONE:-none}')"
[ "$HALL" != "0" ] || fail "the hall reads zero gravity too -- the zone is not 'in places'"
pass "the zone has no gravity ($ZONE) and the hall does ($HALL): a zone, not the whole map"

[ -f "$GAME_DIR/maps/$MAP.nav" ] || fail "the engine did not bake maps/$MAP.nav"
pass "the engine baked maps/$MAP.nav on first load"

echo
pass "the derelict interior loads, reads as wrecked by its own measure, has failing light that the"
echo "      measure can tell from absent light, and a zero-g zone shown by a run"
echo "      map:    $OUT/longway_$MAP.pk3  (in game: map $MAP)"
echo "      frames: $FRAMES_OUT/lit/  (and the falsification set $FRAMES_OUT/unlit/)"
echo "      source: build/gdk/maps/eliteforce_scav_maps/scav2.map; docs/evidence/the-archetype-survey.md"
echo "      evidence: docs/evidence/derelict-interior.md"
