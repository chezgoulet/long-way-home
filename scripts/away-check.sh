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
#   vantage    g_shipTest 96 -- the vantage test -- is pointed at *the site's own posts*, and at
#              nothing else: the caller names the post to stand at (g_shipVantagePost) and the stem
#              to call the frames (g_shipVantage), so the frames stand where the site works and are
#              named for the site rather than for the dark room's security office (the defect this
#              pass repairs; docs/evidence/the-away-vantages.md). Six vantages per post: the four
#              facings at the post and a pace each side. The post's own four facings must read
#              LEGIBLE (tools/frames/frame.py); the paces are measured and a dark one is named
#   outside    the frame that looks out into the debris field must differ from the frame that looks
#              at the site's own wall at the same post -- so "outside" is a picture of a world
#   crew       g_crew embodies the ship's roster on the site and every body must reach its place
#
# Writes under build/away and build/away-home only. The installation is read-only.
#
# LWH_VANTAGE_POST="name" overrides the post the vantage step stands at, so the anchor plumbing can
# be shown to *refuse* a post the map does not carry rather than quietly photographing from
# somewhere else (the falsification in docs/evidence/the-away-vantages.md).

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/away-home"
GAME_DIR="$HOME_DIR/baseEF"
GEN="$ROOT/build/away/generated"
OUT="$ROOT/build/away/out"
SRC="$ROOT/build/gdk/maps/eliteforce_ctf_maps/ctf_space.map"
MAP="lwh_away_depot"
# The one frame allowed to read dark: the stores post's outward (east) view, whose subject is the
# black debris field (docs/evidence/the-away-site-exterior.md). The facing rule and the dark-probe
# both read it, so it is set once.
ALLOWED_DARK="${MAP}_stores_2.tga"
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

# The site's own posts, and only these: the two the dress report names. The report carries their
# positions; the map carries the point entities themselves -- their names are what the vantage
# caller stands at, and their origins are the places the frames are shot from. The two must be the
# same two, or the caller would photograph somewhere the report does not call a post.
POSTS_RAW="$(python3 - "$REPORT" "$GEN/$MAP.map" <<'PY'
import json, re, sys
report = json.load(open(sys.argv[1]))
want = sorted((int(p[0]), int(p[1])) for p in report["posts"])
name, posts = None, []
for ln in open(sys.argv[2], encoding="latin-1"):
    m = re.search(r'"targetname"\s+"(lwh_post_[^"]+)"', ln)
    if m:
        name = m.group(1)
        continue
    m = re.search(r'"origin"\s+"(-?\d+) (-?\d+) (-?\d+)"', ln)
    if m and name:
        posts.append((name, int(m.group(1)), int(m.group(2)), int(m.group(3))))
        name = None
have = sorted((x, y) for _, x, y, _ in posts)
if have != want:
    sys.exit("the map's posts %s are not the dress report's %s" % (have, want))
for n, x, y, z in posts:
    print(n[len("lwh_post_"):], n, x, y, z)
PY
)" || fail "the map's posts are not the dress report's own two posts -- the caller would stand where the site does not work"

POSTS=()
while read -r label target x y z; do
    [ -n "$label" ] && POSTS+=("$label $target $x $y $z")
done <<<"$POSTS_RAW"
[ "${#POSTS[@]}" -eq 2 ] || fail "the site does not name exactly two posts (found ${#POSTS[@]})"

cp "$OUT/longway_$MAP.pk3" "$GAME_DIR/"
rm -f "$GAME_DIR/maps/$MAP.nav"
rm -rf "$GAME_DIR/screenshots"
mkdir -p "$GAME_DIR/screenshots"
# the frames of every vantage run this check takes, and no stale ones from an earlier pass
rm -rf "$HOME_DIR"/frames-*

# The vantage test -- g_shipTest 96, with the caller's post and frame stem. It stands at the point
# entity g_shipVantagePost names and photographs six vantages (four facings at the post and a pace
# each side), one frame each, named screenshots/<g_shipVantage>_<n>.tga. The post must be the map's
# own: the harness prints the anchor it honoured, and this fails if that is not the post's origin
# -- so a run that stood somewhere else, or renamed the frames for another room, is caught rather
# than captioned.
vantage() {  # vantage LABEL POST ORIGIN
  local label="$1" post="$2" origin="$3" name="${MAP}_$1"
  find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
  rm -f "$GAME_DIR/screenshots"/*.tga
  echo "==> the vantages, the $label post ($post at $origin)"
  SDL_AUDIODRIVER=dummy timeout 240 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
      +set s_useOpenAL 0 +set g_ship 1 +set g_shipTest 96 +set g_shipVantage "$name" \
      +set g_shipVantagePost "$post" +map "$MAP" >"$HOME_DIR/vantage-$label.out" 2>&1 || true
  grep -q "CM_LoadMap(maps/$MAP.bsp)" "$HOME_DIR/vantage-$label.out" || fail "the map did not load ($label; see $HOME_DIR/vantage-$label.out)"
  grep -q "Munro connected" "$HOME_DIR/vantage-$label.out" || fail "the player never connected ($label)"
  grep -q "SHIP: vantage: $name: the anchor at the post \"$post\": ($origin)" "$HOME_DIR/vantage-$label.out" \
      || fail "the harness did not stand at the site's own post $post ($label; see $HOME_DIR/vantage-$label.out)"
  grep -h "vantage: $name " "$HOME_DIR/vantage-$label.out" | sed 's/^SHIP: /    /' || true
  rm -rf "$HOME_DIR/frames-$label"
  mkdir -p "$HOME_DIR/frames-$label"
  cp "$GAME_DIR/screenshots/$name"_*.tga "$HOME_DIR/frames-$label/"
  # The frames are this site's, and all six: a run that named them for another room, or skipped a
  # vantage, is not the photograph this check is about.
  [ "$(ls "$HOME_DIR/frames-$label"/*.tga 2>/dev/null | wc -l)" -eq 6 ] \
      || fail "the $label post did not produce six frames named $name\_<n>.tga ($label)"
}

for post in "${POSTS[@]}"; do
    read -r label target x y z <<<"$post"
    vantage "$label" "${LWH_VANTAGE_POST:-$target}" "$x $y $z"
done
echo "==> the light, measured per viewpoint (mean / median / p90 / spread of the brightest channel, mid band)"
for post in "${POSTS[@]}"; do
    read -r label _ <<<"$post"
    python3 "$ROOT/tools/frames/frame.py" floor --band 0.10,0.55 "$HOME_DIR"/frames-"$label"/*.tga | sed 's/^/    /' || true
done

# A post is a position *and* a facing: the four facings at the post are the post's own claim, and
# every one of them must read -- **except the one whose subject is the sky**. The stores post now
# opens east onto the debris field (docs/evidence/the-away-site-exterior.md), so its outward facing
# is a picture of the black sky, the category the dark-room pass names and the away-site evidence
# accepts ("a legibility threshold written for a room cannot be passed by a picture of the sky").
# That one facing is named in ALLOWED_DARK and exempt; the post's other three facings -- where the
# station and the structure are -- must still read, so a post with no legible stand still fails.
echo "==> every post's own facings must read (the outward one may look at the sky)"
for post in "${POSTS[@]}"; do
    read -r label _ <<<"$post"
    required=()
    for i in 0 1 2 3; do
        f="$HOME_DIR/frames-$label/${MAP}_${label}_$i.tga"
        [ "$(basename "$f")" = "$ALLOWED_DARK" ] && continue
        required+=("$f")
    done
    python3 "$ROOT/tools/frames/frame.py" floor --band 0.10,0.55 --require "${required[@]}" >/dev/null \
        || fail "the $label post renders dark at one of its own facings -- the site's own post has no legible stand"
done
pass "both of the site's own posts read at their own facings -- the site stands in its light"

# The rig's probe: one frame reads dark, and it is *named* rather than dressed over. After the
# exterior pass (docs/evidence/the-away-site-exterior.md) the stores post's **outward (east)** frame
# looks out into the debris field, and the debris field is black; its own numbers show the deck under
# it is lit (median 41, spread 82 -- a lit foreground against a black sky, not a dark room). This is
# the same category the away-site evidence names for the console post's outward frame: **a
# legibility threshold written for a room cannot be passed by a picture of the sky.** (Before this
# pass the dark frame was `stores_5`, a pace against a machine block; the crate that blocked the
# stores post's outward view has been lowered and the block dropped, so that pace now reads and the
# dark frame moved to the outward view itself.)
# The rig's probe, made **two-way** (`tools/frames/dark_probe.py`; docs/evidence/the-two-way-fixes.md).
# The named frame -- the stores post's outward view, whose subject is the black debris field -- *must*
# read dark, and **no other** frame may. A one-way guard (fail only on an *unexpected* dark frame)
# cannot report the exemption ceasing to be needed: if a later pass seals the opening, the frame becomes
# a lit wall and a one-way guard says nothing while the outward view is gone. The probe fails on that,
# and names it, so the assertion is the one the pass actually makes -- the opening still exists.
echo "==> the dark probe, two ways: the named opening reads dark, and nothing else does"
python3 "$ROOT/tools/frames/dark_probe.py" --band 0.10,0.55 --named "$ALLOWED_DARK" \
    "$HOME_DIR"/frames-*/*.tga | sed 's/^/    /' \
    || fail "the dark probe failed: the named opening ($ALLOWED_DARK) is not dark, or another frame is"

# The site reads as *outside*: at the site's own post, the picture looking out into the debris field
# and the picture looking at the site's own wall must differ over the region of the subject. A HUD
# cannot satisfy this -- the band excludes the console text above and the weapon below -- and a
# painted wall would not: what changes is the world.
echo "==> the opening into the debris field, against the site's own wall at the same post"
python3 "$ROOT/tools/frames/frame.py" diff --band 0.10,0.55 --min-changed 0.5 --min-delta 24 --require \
    "$HOME_DIR/frames-console/${MAP}_console_3.tga" "$HOME_DIR/frames-console/${MAP}_console_2.tga" \
    | sed 's/^/    /' || fail "the opening into the debris field is not a picture of a world: it did not differ from the wall at the same post"
pass "the frame that looks out shows a world: it differs from the site's own wall at the same post"

# The stores post now opens onto the exterior too (docs/evidence/the-away-site-exterior.md): the
# crate that stood in its outward sightline is low, so the frame that looks east out into the debris
# field differs from the wall behind the post. Before the pass this view was blocked -- the crate
# filled the frame -- so this is the region that was blocked, now showing the world.
echo "==> the stores post's opening into the debris field, against the wall behind it"
python3 "$ROOT/tools/frames/frame.py" diff --band 0.10,0.55 --min-changed 0.3 --min-delta 24 --require \
    "$HOME_DIR/frames-stores/${MAP}_stores_3.tga" "$HOME_DIR/frames-stores/${MAP}_stores_2.tga" \
    | sed 's/^/    /' || fail "the stores post's outward view is not a picture of a world: it did not differ from the wall behind it"
pass "the stores post opens onto the world: its outward frame differs from the wall behind it"

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
# The ship's roster puts **9** crew on deck 1 at 08:00 -- the nine-departments pass (2026-10-12,
# 3.3) moved it from the 13 this check was written against. The site embodies whoever the routine
# has there, so 9 is the current figure, named rather than rounded; a site that embodies none, or
# leaves one stuck, still fails.
[ "$EMBODIED" -ge 9 ] || fail "fewer than nine crew embodied on the site ($EMBODIED)"
[ "$STUCK" -eq 0 ] || fail "$STUCK crew could not reach their place on the site"
[ "$ARRIVED" -eq "$EMBODIED" ] || fail "only $ARRIVED of $EMBODIED crew reached their place"
pass "every crew member the ship put on the site walked to its place, and none is in solid"

echo
pass "the away site loads, walks, reads as outside from its own posts (partly), is lit by measurement,"
echo "      and its sources are named"
echo "      map:     $OUT/longway_$MAP.pk3  (in game: map $MAP)"
echo "      frames:  $HOME_DIR/frames-console/, $HOME_DIR/frames-stores/"
echo "      sources: docs/locations/away-site-outpost.brief.md; evidence: docs/evidence/the-away-site.md,"
echo "               docs/evidence/the-away-vantages.md"
