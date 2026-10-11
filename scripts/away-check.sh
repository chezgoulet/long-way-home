#!/usr/bin/env bash
# The away site: an outpost's apron in a debris field, re-dressed from `ctf_space`.
#
#   scripts/away-check.sh
#
# Closes walkthrough G17's first half -- "there is no away site, so the away mission's acceptance run
# is impossible" -- as far as a map can close it: a walkable site that loads, reads as *outside*, is
# lit by measurement, and that the crew layer's own bodies reach. The design is
# `docs/outside-the-ship.md` (the archetype table and *Order of work* 2-3); the method is
# `docs/authoring-a-location.md`; the evidence is `docs/evidence/the-away-site.md`.
#
# What it does, in order, and what each step proves:
#   re-dress   tools/away/dress_away_site.py copies the source's own top deck and shells it
#   build      scripts/build-map.sh compiles it and `tools/mapgen/check-bsp.py` reads the artifact
#   census     tools/shipmap/parts_census.py: every material already exists in the game
#   vantage    g_shipTest 96 (the dark-room vantage test, reused as a general vantage tool) stands the
#              player at the site's post and at the arrival and photographs six vantages of each;
#              tools/frames/frame.py measures every frame and the room's own six must read LEGIBLE
#   outside    the frame that looks out into the debris field must differ from the frame that looks
#              at the site's own wall at the same vantage -- so "outside" is a picture of a world
#   crew       g_crew embodies the ship's roster on the site and every body must reach its place
#
# Writes under build/away and build/away-home only. The installation is read-only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/away-home"
GAME_DIR="$HOME_DIR/baseEF"
GEN="$ROOT/build/away/generated"
OUT="$ROOT/build/away/out"
SRC="$ROOT/build/gdk/maps/eliteforce_ctf_maps/ctf_space.map"
MAP="lwh_away_depot"
REPORT="$ROOT/build/away/dress.json"

[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
[ -f "$SRC" ] || { echo "no source at $SRC -- run scripts/fetch-gdk.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }

fail() { echo "FAIL  $1"; exit 1; }
pass() { echo "PASS  $1"; }

mkdir -p "$GEN" "$OUT" "$GAME_DIR/scripts" "$GAME_DIR/screenshots"

echo "==> the re-dress"
python3 "$ROOT/tools/away/dress_away_site.py" --source "$SRC" --out "$GEN" \
    --report "$REPORT" --census "$ROOT/build/away/census.json" || fail "the re-dress wrote no map"

echo "==> the build, and the artifact"
bash "$ROOT/scripts/build-map.sh" "$GEN/$MAP.map" --name "$MAP" --out "$OUT" --light || fail "the map did not build"

echo "==> the parts census: no new art"
python3 "$ROOT/tools/shipmap/parts_census.py" --map "$GEN/$MAP.map" --game "$ROOT/build/baseEF" \
    --census "$ROOT/build/away/census.json" || fail "the site names art the game does not have"

# The two anchors the harness stands the player at, from the tool's own report: the post a person
# works from, and the arrival the player beams in at.
anchor() { python3 -c "import json,sys;print(' '.join(str(int(v)) for v in json.load(open(sys.argv[1]))[sys.argv[2]]))" "$REPORT" "$1"; }
POST="$(anchor anchor)"
EDGE="$(anchor arrival)"
[ -n "$POST" ] && [ -n "$EDGE" ] || fail "the report has no anchor/arrival to stand at"

cp "$OUT/longway_$MAP.pk3" "$GAME_DIR/"
rm -f "$GAME_DIR/maps/$MAP.nav"
rm -rf "$GAME_DIR/screenshots"
mkdir -p "$GAME_DIR/screenshots"

# The vantage test -- g_shipTest 96, the dark-room test reused as a general tool (it accepts any
# "x y z" anchor, and its own header says so): six facings/positions per anchor, one frame each.
vantage() {  # vantage LABEL ANCHOR
  local label="$1" pos="$2"
  find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
  rm -f "$GAME_DIR/screenshots"/*.tga
  echo "==> the vantages, $label at $pos"
  SDL_AUDIODRIVER=dummy timeout 240 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
      +set s_useOpenAL 0 +set g_ship 1 +set g_shipTest 96 +set g_shipTestPos "$pos" \
      +map "$MAP" >"$HOME_DIR/vantage-$label.out" 2>&1 || true
  grep -q "CM_LoadMap(maps/$MAP.bsp)" "$HOME_DIR/vantage-$label.out" || fail "the map did not load ($label; see $HOME_DIR/vantage-$label.out)"
  grep -q "Munro connected" "$HOME_DIR/vantage-$label.out" || fail "the player never connected ($label)"
  grep -q "SHIP: dark room: the anchor given by g_shipTestPos: ($pos)" "$HOME_DIR/vantage-$label.out" \
      || fail "the player did not stand at $pos ($label)"
  grep -h "dark room: security frame" "$HOME_DIR/vantage-$label.out" | sed 's/^EFSP: /    /' || true
  mkdir -p "$HOME_DIR/frames-$label"
  cp "$GAME_DIR/screenshots"/*.tga "$HOME_DIR/frames-$label/"
  ls "$HOME_DIR/frames-$label"/*.tga >/dev/null 2>&1 || fail "no frames were photographed ($label)"
}

vantage post "$POST"
vantage edge "$EDGE"

echo "==> the light, measured per viewpoint (mean / median / p90 of the brightest channel, mid band)"
python3 "$ROOT/tools/frames/frame.py" floor --band 0.10,0.55 "$HOME_DIR"/frames-post/*.tga | sed 's/^/    /' || true
python3 "$ROOT/tools/frames/frame.py" floor --band 0.10,0.55 "$HOME_DIR"/frames-edge/*.tga | sed 's/^/    /' || true

# The room a person works in must read. Every one of the post's six vantages is required: if the
# room goes dark, this fails and names it. (The dark-room pass's rule is in hand -- a *facing* can be
# dark in a well-lit room -- but here the post's own four facings and the pace either side of it
# all read, so none needs excusing.)
python3 "$ROOT/tools/frames/frame.py" floor --band 0.10,0.55 --require "$HOME_DIR"/frames-post/*.tga \
    >/dev/null || fail "the site's own post does not read: a frame there is black or uniform"
pass "every vantage at the site's post is legible -- the room stands in the light"

# The site reads as *outside*: at one vantage, the picture looking out into the debris field and the
# picture looking at the site's own wall must differ over the region of the subject. A HUD cannot
# satisfy this -- the band excludes the console text above and the weapon below -- and a painted wall
# would not: what changes is the world.
python3 "$ROOT/tools/frames/frame.py" diff --band 0.10,0.55 --min-changed 0.5 --min-delta 24 --require \
    "$HOME_DIR/frames-edge/lwh_darkroom_security_3.tga" "$HOME_DIR/frames-edge/lwh_darkroom_security_2.tga" \
    | sed 's/^/    /' || fail "the opening into the debris field is not a picture of a world: it did not differ from the wall at the same vantage"
pass "the frame that looks out shows a world: it differs from the site's own wall at the same vantage"

# Navigation is baked by the engine on first load, from the map's own waypoints -- no crew can work a
# site with no nav, and the file was deleted before the run.
[ -f "$GAME_DIR/maps/$MAP.nav" ] || fail "the engine did not bake maps/$MAP.nav: the site has no navigation"
pass "the engine baked maps/$MAP.nav on first load"

# Reachability, by the project's own instrument: the crew layer embodies the ship's roster on the
# site (g_crewFromShip, the s5 shape) and every body walks to its place. Writes nothing but the log.
find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
echo "==> the crew on the site"
SDL_AUDIODRIVER=dummy timeout 120 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
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
[ "$EMBODIED" -ge 10 ] || fail "fewer than ten crew embodied on the site ($EMBODIED)"
[ "$STUCK" -eq 0 ] || fail "$STUCK crew could not reach their place on the site"
[ "$ARRIVED" -eq "$EMBODIED" ] || fail "only $ARRIVED of $EMBODIED crew reached their place"
pass "every crew member the ship put on the site walked to its place, and none is in solid"

echo
pass "the away site loads, walks, reads as outside, is lit by measurement, and its sources are named"
echo "      map:     $OUT/longway_$MAP.pk3  (in game: map $MAP)"
echo "      frames:  $HOME_DIR/frames-post/, $HOME_DIR/frames-edge/"
echo "      sources: docs/locations/away-site-outpost.brief.md; evidence: docs/evidence/the-away-site.md"
