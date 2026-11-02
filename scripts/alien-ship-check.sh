#!/usr/bin/env bash
# The alien ship interior -- O11's third archetype (docs/outside-the-ship.md, the archetype table:
# "unfamiliar layout, the species' own design language"), re-dressed from `scav3` by
# tools/away/dress_alien_ship.py.
#
#   scripts/alien-ship-check.sh
#
# What it does, and what each step proves:
#   re-dress   the source's own upper engineering deck is cropped, shelled, and its two galleries
#              opened onto a starfield (tools/away/dress_alien_ship.py)
#   build      scripts/build-map.sh compiles it, -vis proves it is sealed, and
#              tools/mapgen/check-bsp.py reads the artifact (never the exit code)
#   census     tools/shipmap/parts_census.py: every material already exists in the game
#   posts      the two posts the dress report names are the two the map carries, and the gallery
#              anchor is the report's own -- a caller cannot photograph somewhere else
#   measure    `frame.py alien` -- the archetype's OWN assertion: MAINTAINED (a lit working deck,
#              the inverse of the derelict's failing light), BUILT (a brightness-edge fraction) and
#              COLOURED (a design language in the palette). Applied to both posts' six vantages,
#              every one must pass; NO per-frame dark exemption is allowed.
#   outside    at the gallery, the outward frame is black space (median <= 5) and the inward frame
#              is a lit room (median >= 40), and the two differ by >= 30% -- the opening is a
#              picture of a world, not a fill
#   drab       the SAME lit map with every material grey and every light white: the measure must
#              fail on the palette clause, or the design-language assertion is decoration
#   unlit      the same map with no light and the galleries sealed: the measure must fail on the
#              maintained clause
#   sealed     the same map with its galleries sealed: the outward median must rise above 5, so the
#              outside assertion can be shown to reject a sealed site (the planted break)
#   nav/crew   the engine bakes maps/lwh_alien_ship.nav, and every body the crew layer embodies
#              walks to its place
#
# Writes under build/away/alien* and build/away-home only. The installation is read-only.
#
# Q3MAP2=/path/to/q3map2 overrides the compiler. The frames are preserved outside the repository
# (the repo carries no game media) at $LWH_ALIEN_FRAMES, default /home/c/lwh-frames/the-alien-ship.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/away-home"
GAME_DIR="$HOME_DIR/baseEF"
SRC="$ROOT/build/gdk/maps/eliteforce_scav_maps/scav3.map"
MAP="lwh_alien_ship"
GEN="$ROOT/build/away/alien"
OUT="$ROOT/build/away/alien-out"
REPORT="$GEN/dress.json"
FRAMES_OUT="${LWH_ALIEN_FRAMES:-/home/c/lwh-frames/the-alien-ship}"
Q3MAP2="${Q3MAP2:-$(command -v q3map2 || true)}"

[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
[ -f "$SRC" ] || { echo "no source at $SRC -- run scripts/fetch-gdk.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

fail() { echo "FAIL  $1"; exit 1; }
pass() { echo "PASS  $1"; }

mkdir -p "$GEN" "$OUT" "$GAME_DIR/scripts" "$GAME_DIR/screenshots" "$FRAMES_OUT"

QFLAG=()
[ -n "$Q3MAP2" ] && QFLAG=(--q3map2 "$Q3MAP2")

dress() {  # dress DIR [extra flags...]
  local dir="$1"; shift
  python3 "$ROOT/tools/away/dress_alien_ship.py" --source "$SRC" --out "$dir" --name "$MAP" "$@" \
      --report "$dir/dress.json" --census "$dir/census.json" || fail "the re-dress wrote no map ($dir)"
}

build() {  # build DIR
  local dir="$1"
  local out="$dir-out"
  mkdir -p "$out"
  bash "$ROOT/scripts/build-map.sh" "$dir/$MAP.map" --name "$MAP" --out "$out" --light "${QFLAG[@]}" \
      >"$dir/build.log" 2>&1 || { tail -6 "$dir/build.log"; fail "the map did not build ($dir)"; }
  grep -q "OK" "$dir/build.log" || { tail -6 "$dir/build.log"; fail "the BSP is empty or malformed ($dir)"; }
  grep "OK" "$dir/build.log" | sed 's/^/    /'
  cp "$out/longway_$MAP.pk3" "$GAME_DIR/"
  rm -f "$GAME_DIR/maps/$MAP.nav"
}

shoot() {  # shoot DIR LABEL STEM POST [PACE]
  local dir="$1"
  local label="$2"
  local stem="$3"
  local post="$4"
  local pace="${5:-96}"
  local out="$HOME_DIR/frames-$label"
  find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
  rm -rf "$GAME_DIR/screenshots"; mkdir -p "$GAME_DIR/screenshots"
  SDL_AUDIODRIVER=dummy timeout 240 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
      +set s_useOpenAL 0 +set g_ship 1 +set g_shipTest 96 +set g_shipVantage "$stem" \
      +set g_shipVantagePost "$post" +set g_shipVantagePace "$pace" +map "$MAP" \
      >"$HOME_DIR/vantage-$label.out" 2>&1 || true
  grep -q "CM_LoadMap(maps/$MAP.bsp)" "$HOME_DIR/vantage-$label.out" \
      || fail "the map did not load ($label; see $HOME_DIR/vantage-$label.out)"
  grep -q "Munro connected" "$HOME_DIR/vantage-$label.out" || fail "the player never connected ($label)"
  grep -q "SHIP: vantage: $stem: the anchor at the post \"$post\":" "$HOME_DIR/vantage-$label.out" \
      || fail "the harness did not stand at the site's own post $post ($label)"
  grep -h "vantage: $stem " "$HOME_DIR/vantage-$label.out" | sed 's/^SHIP: /    /' || true
  rm -rf "$out"; mkdir -p "$out"
  cp "$GAME_DIR/screenshots/$stem"_*.tga "$out/" || fail "no frames ($label)"
  [ "$(find "$out" -name '*.tga' | wc -l)" -eq 6 ] || fail "the $label run gave no six frames"
  rm -rf "${FRAMES_OUT:?}/$label"; mkdir -p "$FRAMES_OUT/$label"
  cp "$out"/*.tga "$FRAMES_OUT/$label/"
}

measure() {  # measure LABEL -- prints, returns frame.py's status (--require: a failure is an exit)
  python3 "$ROOT/tools/frames/frame.py" alien --band 0.10,0.55 --require "$HOME_DIR/frames-$1"/*.tga
}

# The frame's median, from frame.py's own printed line: the outside assertion is about a picture of
# black space, and the median is the number that says the picture is black.
median() {
  python3 "$ROOT/tools/frames/frame.py" floor --band 0.10,0.55 "$1" \
    | awk '{for (i=1;i<=NF;i++) if ($i=="median") {print int($(i+1)); exit}}'
}

echo "==> the re-dress: scav3's own upper engineering deck, shelled and galleries opened"
dress "$GEN"
python3 - "$REPORT" "$GEN/$MAP.map" <<'PY'
import json, re, sys
report = json.load(open(sys.argv[1]))
want = sorted((int(p[0]), int(p[1])) for p in report["posts"])
name, posts = None, []
for ln in open(sys.argv[2], encoding="latin-1"):
    m = re.search(r'"targetname"\s+"(lwh_post_[^"]+)"', ln)
    if m:
        name = m.group(1); continue
    m = re.search(r'"origin"\s+"(-?\d+) (-?\d+) (-?\d+)"', ln)
    if m and name:
        posts.append((name, int(m.group(1)), int(m.group(2)), int(m.group(3)))); name = None
have = sorted((x, y) for _, x, y, _ in posts)
if have != want:
    sys.exit("the map's posts %s are not the dress report's %s" % (have, want))
for n, x, y, z in posts:
    print("    %s at %d %d %d" % (n, x, y, z))
print("    gallery at %s" % (report["gallery"],))
PY
POSTS_RAW="$(python3 - "$REPORT" "$GEN/$MAP.map" <<'PY'
import json, re, sys
report = json.load(open(sys.argv[1]))
want = sorted((int(p[0]), int(p[1])) for p in report["posts"])
name, posts = None, []
for ln in open(sys.argv[2], encoding="latin-1"):
    m = re.search(r'"targetname"\s+"(lwh_post_[^"]+)"', ln)
    if m:
        name = m.group(1); continue
    m = re.search(r'"origin"\s+"(-?\d+) (-?\d+) (-?\d+)"', ln)
    if m and name:
        posts.append((name, int(m.group(1)), int(m.group(2)), int(m.group(3)))); name = None
have = sorted((x, y) for _, x, y, _ in posts)
if have != want:
    sys.exit("the map's posts %s are not the dress report's %s" % (have, want))
for n, x, y, z in posts:
    print(n[len("lwh_post_"):], n, x, y, z)
PY
)" || fail "the map's posts are not the dress report's own two posts"
POSTS=()
while read -r label target x y z; do
  [ -n "$label" ] && POSTS+=("$label $target $x $y $z")
done <<<"$POSTS_RAW"
[ "${#POSTS[@]}" -eq 2 ] || fail "the site does not name exactly two posts (found ${#POSTS[@]})"
GALLERY="$(python3 -c 'import json,sys; g=json.load(open(sys.argv[1]))["gallery"]; print("%d %d %d" % (g[0],g[1],g[2]))' "$REPORT")"
GALLERY_MAP="$(python3 - "$GEN/$MAP.map" <<'PY'
import re, sys
name = None
for ln in open(sys.argv[1], encoding="latin-1"):
    m = re.search(r'"targetname"\s+"(lwh_gallery)"', ln)
    if m:
        name = m.group(1); continue
    m = re.search(r'"origin"\s+"(-?\d+) (-?\d+) (-?\d+)"', ln)
    if m and name:
        print("%d %d %d" % (int(m.group(1)), int(m.group(2)), int(m.group(3))))
        break
PY
)"
[ "$GALLERY_MAP" = "$GALLERY" ] || fail "the map's gallery $GALLERY_MAP is not the dress report's $GALLERY"
EAST="$(python3 -c 'import json,sys; g=json.load(open(sys.argv[1]))["gallery_east_anchor"]; print("%d %d %d" % (g[0],g[1],g[2]))' "$REPORT")"
EAST_MAP="$(python3 - "$GEN/$MAP.map" <<'PY'
import re, sys
name = None
for ln in open(sys.argv[1], encoding="latin-1"):
    m = re.search(r'"targetname"\s+"(lwh_gallery_east)"', ln)
    if m:
        name = m.group(1); continue
    m = re.search(r'"origin"\s+"(-?\d+) (-?\d+) (-?\d+)"', ln)
    if m and name:
        print("%d %d %d" % (int(m.group(1)), int(m.group(2)), int(m.group(3))))
        break
PY
)"
[ "$EAST_MAP" = "$EAST" ] || fail "the map's east gallery $EAST_MAP is not the dress report's $EAST"

echo
echo "==> the build, the artifact, and the parts census (no new art)"
build "$GEN"
python3 "$ROOT/tools/shipmap/parts_census.py" --map "$GEN/$MAP.map" --game "$ROOT/build/baseEF" \
    --census "$GEN/census.json" || fail "the alien ship names art the game does not have"
pass "every material already exists in the game; no new art"

echo
echo "==> the archetype's own assertion on both posts' six vantages (must read ALIEN)"
for post in "${POSTS[@]}"; do
  read -r label target _ <<<"$post"
  shoot "$label" "$label" "${MAP}_$label" "$target"
done
for post in "${POSTS[@]}"; do
  read -r label _ <<<"$post"
  measure "$label" | sed 's/^/    /' || fail "the $label post does not read as an alien ship interior"
done
pass "both posts read as a maintained, built, coloured alien deck at all twelve vantages"

echo
echo "==> the opening: black space outward, a lit room inward, at the gallery"
shoot gallery gallery "${MAP}_gallery" lwh_gallery
OUT_MED="$(median "$HOME_DIR/frames-gallery/${MAP}_gallery_0.tga")"
IN_MED="$(median "$HOME_DIR/frames-gallery/${MAP}_gallery_1.tga")"
echo "    outward median ${OUT_MED:-?} (black space), inward median ${IN_MED:-?} (lit room)"
[ "${OUT_MED:-x}" != "x" ] && [ "$OUT_MED" -le 5 ] \
    || fail "the outward frame is not a picture of black space (median ${OUT_MED:-none} > 5): the gallery is sealed"
[ "${IN_MED:-x}" != "x" ] && [ "$IN_MED" -ge 40 ] \
    || fail "the inward frame is not a lit room (median ${IN_MED:-none} < 40)"
python3 "$ROOT/tools/frames/frame.py" diff --band 0.10,0.55 --min-changed 0.3 --min-delta 24 --require \
    "$HOME_DIR/frames-gallery/${MAP}_gallery_1.tga" "$HOME_DIR/frames-gallery/${MAP}_gallery_0.tga" \
    | sed 's/^/    /' || fail "the outward view did not differ from the room at the same post"
pass "the gallery looks out onto black space, inward at a lit room, and the two differ by >= 30%"

echo
echo "==> the second gallery, on the east wall: black space outward, the room inward"
shoot gallery-east gallery-east "${MAP}_gallery_east" lwh_gallery_east 64
EAST_OUT="$(median "$HOME_DIR/frames-gallery-east/${MAP}_gallery_east_2.tga")"
EAST_IN="$(median "$HOME_DIR/frames-gallery-east/${MAP}_gallery_east_3.tga")"
echo "    outward median ${EAST_OUT:-?} (black space), inward median ${EAST_IN:-?} (lit room)"
[ "${EAST_OUT:-x}" != "x" ] && [ "$EAST_OUT" -le 5 ] \
    || fail "the east gallery's outward frame is not black space (median ${EAST_OUT:-none} > 5)"
[ "${EAST_IN:-x}" != "x" ] && [ "$EAST_IN" -ge 40 ] \
    || fail "the east gallery's inward frame is not a lit room (median ${EAST_IN:-none} < 40)"
python3 "$ROOT/tools/frames/frame.py" diff --band 0.10,0.55 --min-changed 0.3 --min-delta 24 --require \
    "$HOME_DIR/frames-gallery-east/${MAP}_gallery_east_3.tga" \
    "$HOME_DIR/frames-gallery-east/${MAP}_gallery_east_2.tga" \
    | sed 's/^/    /' || fail "the east gallery's outward view did not differ from the room at the same post"
pass "both galleries (south and east) look out onto black space, inward at a lit room"

echo
echo "==> the palette clause's falsification: the SAME lit room, every material grey and every light white"
dress "$ROOT/build/away/alien-drab" --drab
build "$ROOT/build/away/alien-drab"
shoot drab drab "${MAP}_drab" lwh_post_console
if measure drab >"$HOME_DIR/alien-drab-measure.txt" 2>&1; then
  sed 's/^/    /' "$HOME_DIR/alien-drab-measure.txt"
  fail "the measure PASSED the drab capture: it cannot tell a design language from a neutral box"
fi
sed 's/^/    /' "$HOME_DIR/alien-drab-measure.txt"
grep -q "neutral grey\|no design language" "$HOME_DIR/alien-drab-measure.txt" \
    || fail "the drab capture failed, but not on the palette clause"
pass "the measure rejects the drab capture on the design-language clause"

echo
echo "==> the maintained clause's falsification: the same map with no light and sealed galleries"
dress "$ROOT/build/away/alien-unlit" --unlit
build "$ROOT/build/away/alien-unlit"
shoot unlit unlit "${MAP}_unlit" lwh_post_console
if measure unlit >"$HOME_DIR/alien-unlit-measure.txt" 2>&1; then
  sed 's/^/    /' "$HOME_DIR/alien-unlit-measure.txt"
  fail "the measure PASSED the unlit capture: it cannot tell a working deck from a dark one"
fi
sed 's/^/    /' "$HOME_DIR/alien-unlit-measure.txt"
grep -q "too dark\|nothing clearly lit" "$HOME_DIR/alien-unlit-measure.txt" \
    || fail "the unlit capture failed, but not on the maintained clause"
pass "the measure rejects the unlit capture on the maintained clause"

echo
echo "==> the outside assertion's falsification: the same gallery with the openings sealed"
dress "$ROOT/build/away/alien-sealed" --sealed
build "$ROOT/build/away/alien-sealed"
shoot sealed sealed "${MAP}_sealed" lwh_gallery
SEALED_MED="$(median "$HOME_DIR/frames-sealed/${MAP}_sealed_0.tga")"
echo "    sealed outward median ${SEALED_MED:-?} (a wall where the window was)"
[ "${SEALED_MED:-x}" != "x" ] && [ "$SEALED_MED" -gt 5 ] \
    || fail "the sealed build still reads as black space (median ${SEALED_MED:-none} <= 5): the window was not the opening, or --sealed does nothing"
pass "sealing the galleries moves the outward median above 5, so the outside assertion rejects a sealed site"

echo
echo "==> restore the lit map, bake the nav, and walk the crew"
dress "$GEN"
build "$GEN"
find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
    +set s_useOpenAL 0 +set g_ship 1 +set g_crew 1 +set g_crewDebug 1 +set g_crewFromShip 1 \
    +set g_crewDeck 1 +map "$MAP" >"$HOME_DIR/crew.out" 2>&1 || true
grep -hE '^CREW: (direction layer|([0-9]+ post|ship time|lwh_crew_[0-9]+ at post))' "$HOME_DIR/crew.out" | head -8 | sed 's/^/    /' || true
grep -qE "^CREW: [0-9]+ post\(s\) from the map's own navigation goals" "$HOME_DIR/crew.out" \
    || fail "the crew layer found no posts on the site (see $HOME_DIR/crew.out)"
grep -q "in solid" "$HOME_DIR/crew.out" && fail "a waypoint or a body is in solid on the site"
EMBODIED="$(grep -cE '^CREW: lwh_crew_[0-9]+ \(ent [0-9]+\) -> place_' "$HOME_DIR/crew.out" || true)"
ARRIVED="$(grep -cE '^CREW: lwh_crew_[0-9]+ at post ' "$HOME_DIR/crew.out" || true)"
STUCK="$(grep -cE '^CREW: lwh_crew_[0-9]+ failed to reach ' "$HOME_DIR/crew.out" || true)"
echo "    $EMBODIED embodied, $ARRIVED reached their place, $STUCK gave up"
[ "$EMBODIED" -ge 1 ] || fail "the crew layer embodied nobody on the site"
[ "$STUCK" -eq 0 ] || fail "$STUCK crew could not reach their place on the site"
[ "$ARRIVED" -eq "$EMBODIED" ] || fail "only $ARRIVED of $EMBODIED crew reached their place"
[ -f "$GAME_DIR/maps/$MAP.nav" ] || fail "the engine did not bake maps/$MAP.nav: the site has no navigation"
pass "the engine baked maps/$MAP.nav, and every crew member it embodied walked to its place"

echo
pass "the alien ship interior loads, reads as a maintained alien deck by its own measure, opens two"
echo "      galleries onto black space, and its sources are named"
echo "      map:     $OUT/longway_$MAP.pk3  (in game: map $MAP)"
echo "      frames:  $FRAMES_OUT/{console,bank,gallery,gallery-east,drab,unlit,sealed}"
echo "      source:  build/gdk/maps/eliteforce_scav_maps/scav3.map"
echo "      evidence: docs/evidence/alien-ship-interior.md"
