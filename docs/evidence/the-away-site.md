# Evidence — the away site: an outpost in a debris field

Date: 2026-10-11. Branch: `feat/the-away-site`, cut from `testing` (`d219d2d`).
Brief: `BRIEF.md` (the pass) — the finding closed is `docs/walkthrough.md` **G17** (*"there is no away
site, so the away mission's acceptance run is impossible"*), with **G18** named in the same list.
Design: `docs/outside-the-ship.md` (authority — the archetype table and *Order of work* 2–3).
Location brief: `docs/locations/away-site-outpost.brief.md`. Procedure:
`docs/authoring-a-location.md`. The worked examples this method copies:
`docs/evidence/the-two-missing-decks.md`, `docs/evidence/deck12-redress.md`. The standing rule from a
defect: `docs/evidence/the-dark-room.md`.

Nothing here is asserted without the command that produced it. The command is given before its output.

## What this is, and what it is not

**One walkable away site**: the apron of an abandoned outpost standing in a debris field, re-dressed
from the shipped `ctf_space` source. It loads, walks, reads as *outside*, is lit by measurement, and
the crew layer's own bodies reach every place on it. The archetype is the design's **space station /
outpost**; the reason it was built first is in the brief, §2.

**It is not the away-mission loop.** The loop — beam down, load a kit, scan, carry back, leave before
the window closes — is *scenario content* and the design's *Order of work* item 4. This pass builds
the map the loop will run on, and closes as much of G17 as a map can.

## Task A — the archetype, and what was copied

`docs/outside-the-ship.md`'s table names six archetypes and this pass builds **one**: the **space
station / outpost**, and its source is the design's own pick, `ctf_space`. It was chosen first because
it is the archetype whose source **already carries the exterior** — `ctf_space`'s deck stands in an
open debris field under the game's own lit sky — so it is the cheapest place to prove the thing the
brief says a site exists to do: read as outside. The other five need the sky built from nothing.

```
$ python3 tools/away/dress_away_site.py --source build/gdk/maps/eliteforce_ctf_maps/ctf_space.map \
      --out build/away/generated --report build/away/dress.json --census build/away/census.json
lwh_away_depot <- build/gdk/maps/eliteforce_ctf_maps/ctf_space.map
  window x 400..1280 y 672..1144, floor 168 roof 560
  world brushes 60, brushes kept 31/520, lights kept 16, waypoints 26, posts [(720, 1036), (924, 780)]
  wrote build/away/generated/lwh_away_depot.map
```

**What was copied, per part** (the dress report's `parts`, and the brief's §4 table): the deck's own
floor plates and wall plates (`forge/marmetal2_b`, `forge/flooring_b`), 11 ribs/rails
(`forge/ribs2`), 11 wall/block faces (`forge/fmetal1`), 2 red trim strips (`forge/trimlight2`), 9
patches, 1 `common/caulk` step, and the source's own 16 light entities that fall on this deck. 43
patches and 489 world brushes outside the window were dropped (they are the arena's lower decks and
its machine blocks). 201 entities were dropped — the CTF spawns, weapons and items, which are the
arena's and not ours.

**The exterior is not built.** `common/sky_light` (2 brushes: the ceiling over the open half and the
wall beyond the parapet) is the source's own material — `skyParms env/junk`, a debris field, with
`q3map_sun` and `q3map_surfacelight` so it lights the open half as well as showing it.

## Task B — the map is verified, never the exit code

```
$ scripts/build-map.sh build/away/generated/lwh_away_depot.map --name lwh_away_depot \
      --out build/away/out --light
==> q3map2 -meta
==> q3map2 -vis
==> q3map2 -light
/tmp/tmp.taCS9uB8U8/maps/lwh_away_depot.bsp  OK                                 (1/17 lumps empty)
wrote /home/c/big/git/long-way-home/build/away/out/longway_lwh_away_depot.pk3
in game: map lwh_away_depot
```

`OK` is `tools/mapgen/check-bsp.py`'s verdict on the compiled BSP, run inside `build-map.sh`: shaders,
brushes and surfaces are not empty. The one empty lump is expected. **The exit status of q3map2 is not
the evidence; this line is** — brush winding and texture paths make q3map2 exit 0 with an empty BSP.
`-vis` also ran, and a leaky map writes no portal file and fails `-vis`; it did not.

No new art:

```
$ python3 tools/shipmap/parts_census.py --map build/away/generated/lwh_away_depot.map \
      --game build/baseEF --census build/away/census.json
build/away/generated/lwh_away_depot.map: 8 distinct materials
  present as shipped images:      2694
  declared by shipped shaders:   2070
  our own aliases:               27
  in the reuse source census:    17
PASS  every material already exists in the game; no new art
```

## Task C — the site loads, and the player stands on it

```
$ SDL_AUDIODRIVER=dummy xvfb-run -a scripts/run-engine.sh --home-dir build/away-home \
    +set g_ship 1 +set g_shipTest 96 +set g_shipTestPos "720 908 208" +map lwh_away_depot
EFSP: SP_SpawnServer: CM_LoadMap(maps/lwh_away_depot.bsp)
EFSP: SP_StartClient: first snapshot numEntities=7 ps.origin=(960 908 205) ps.weapon=2
EFSP: Munro connected
SHIP: dark room: the anchor given by g_shipTestPos: (720 908 208)
```

The load lines are quoted; `ps.origin` is the arrival the map's own `info_player_start` places. The
harness is `g_shipTest 96`, **the dark-room vantage test reused as a general vantage tool** — it
accepts any `"x y z"` in `g_shipTestPos`, which its own header says it added as a candidate-anchor
tool, so no engine or module change was needed to stand a person on a new map. It photographs six
vantages: four facings at the anchor and a pace each side.

Navigation bakes on first load (the file was deleted before the run and the engine wrote it again):

```
$ ls -la build/away-home/baseEF/maps/
-rw-rw-r-- 1 c c 3652 Oct 11 16:27 lwh_away_depot.nav
```

## Task D — the light, measured per viewpoint

The measure is `tools/frames/frame.py floor` (the generalisation of the dark-room legibility measure),
on the **middle band** (`0.10,0.55` of the height: above it is the console text, below it the weapon
and the band). A pixel's value is its **brightest channel**, so a red trim strip counts as light.

**The site's own post** — the console at `(720 908 208)`, the place a person works:

```
$ python3 tools/frames/frame.py floor --band 0.10,0.55 build/away-home/frames-post/*.tga
lwh_darkroom_security_0.tga   band 0.1-0.55  mean  67.0 median  56.0 p90 121.0 spread 140.0  LEGIBLE
lwh_darkroom_security_1.tga   band 0.1-0.55  mean  65.6 median  55.0 p90 113.0 spread 145.0  LEGIBLE
lwh_darkroom_security_2.tga   band 0.1-0.55  mean  56.5 median  55.0 p90  84.0 spread  74.0  LEGIBLE
lwh_darkroom_security_3.tga   band 0.1-0.55  mean  56.1 median  53.0 p90  81.0 spread  63.0  LEGIBLE
lwh_darkroom_security_4.tga   band 0.1-0.55  mean  60.3 median  50.0 p90 121.0 spread 170.0  LEGIBLE
lwh_darkroom_security_5.tga   band 0.1-0.55  mean  57.6 median  52.0 p90  87.0 spread  72.0  LEGIBLE
```

**Every one of the post's six vantages reads**, so the room stands in the light and none needs
excusing. The check requires all six, so a site that goes dark fails and names the frame.

**The arrival, on the open apron** — `(960 908 200)`:

```
lwh_darkroom_security_0.tga   band 0.1-0.55  mean  62.8 median  54.0 p90  97.0 spread 115.0  LEGIBLE
lwh_darkroom_security_1.tga   band 0.1-0.55  mean  64.9 median  55.0 p90 102.0 spread 128.0  LEGIBLE
lwh_darkroom_security_2.tga   band 0.1-0.55  mean  40.1 median  37.0 p90  75.0 spread  90.0  DARK -- p90 75 < 80 (nothing lit)
lwh_darkroom_security_3.tga   band 0.1-0.55  mean  64.9 median  56.0 p90 103.0 spread 119.0  LEGIBLE
lwh_darkroom_security_4.tga   band 0.1-0.55  mean  47.5 median  41.0 p90  84.0 spread 144.0  LEGIBLE
lwh_darkroom_security_5.tga   band 0.1-0.55  mean  56.8 median  50.0 p90  84.0 spread 124.0  LEGIBLE
```

**One vantage reads dark, and it is named rather than dressed over: `_2`, the frame that looks east,
straight out into the debris field.** Its subject is empty space and empty space is black; its own
numbers show the deck under it is lit (median 37, p90 75, spread 90 — a lit foreground against a black
sky, not a dark room). A legibility threshold written for a room cannot be passed by a picture of the
sky, and the answer is not to light the debris field. **The dark-room pass's own ruling is what applies
here** — `docs/evidence/the-dark-room.md`: the question is which of *vantage, bake or light* it is, and
this one was established before anything was changed: it is the **subject**, which is the sky.

**Two earlier families of frame were defects and were fixed in the tool, not excused:**

1. **Too dim to read at all.** The source's own deck is dark (its own frames measure median 17–35 on
   the same band), so a straight crop inherited it. The fix is section 6 of the brief: warm ceiling
   light and mid-height wall lamps in the sheltered half, cool floods over the open apron, marker lamps
   at the edge, all in the source's own light materials. The post went from 5-of-6 dark to 6-of-6
   legible by measurement.
2. **A vantage inside a solid** — the dark-room defect, on a new map. The first waypoints the tool
   placed were "in solid" (`Waypoint (null) at (944 736 192) in solid!`) and the engine aborted the
   load. The tool now tests each point entity's **own bounding box** against the final solids, not a
   point with a pad, and it will not place a waypoint or a post on a ledge a body cannot climb.

## Task E — the site reads as outside

A frame from a person's eye height, brought home as a PNG and looked at before it is captioned:

```
$ python3 tools/frames/frame.py diff --band 0.10,0.55 --min-changed 0.5 --min-delta 24 \
      build/away-home/frames-edge/lwh_darkroom_security_3.tga \
      build/away-home/frames-edge/lwh_darkroom_security_2.tga
lwh_darkroom_security_2.tga   band 0.1-0.55  changed  71.3% mean|d|  50.5 of 590080 px  DIFFERS
```

**What it shows, having looked at the frame:** the outpost's deck in the foreground — dark metal
grating, rust-streaked plate on either side, a red trim strip along the deck — and beyond an opening in
the structure, filling the upper half of the frame, **the debris field**: a black sky with a field of
tumbling wreckage and, centre, a broken hull tumbling in it. It is the same debris field the source's
own sky carries (`env/junk`), seen from the outpost's deck. **It does not read as a corridor.**

**The assertion, and what it does not prove.** At one vantage, the picture that looks out into the
debris field and the picture that looks at the site's own wall are compared over the middle band: they
differ by **71.3 % of the region's pixels** (`mean|d| 50.5`). The band excludes the console text above
and the weapon below, so a HUD cannot satisfy it, and a painted wall in place of the opening would not
change that much. **It proves the opening is a picture of a world and not a fill; it does not prove the
world is the right one, and the owner judges whether it reads as *somewhere*.** The tool says so in its
own header and the frame is left where he can look at it.

## Task F — reachability, by the project's own instruments

**The player arrives:** `info_player_start` places the party on the apron — `ps.origin=(960 908 205)`
in the load above.

**Crew reach their places:** the crew layer embodies the ship's roster on the site (`g_crew` with
`g_crewFromShip`, the `scripts/s5-check.sh` shape) and every body walks to its place:

```
$ SDL_AUDIODRIVER=dummy xvfb-run -a scripts/run-engine.sh --home-dir build/away-home \
    +set g_ship 1 +set g_crew 1 +set g_crewDebug 1 +set g_crewFromShip 1 +set g_crewDeck 1 +map lwh_away_depot
CREW: direction layer active on lwh_away_depot
CREW: 2 post(s) from the map's own navigation goals, on 1 deck(s)
CREW: 2 post(s), up to 24 crew
CREW: ship time 08:00, deck 1: the ship has 13 here (showing up to 24), 13 embodied
CREW: lwh_crew_000 at post 'place_lwh_crew_000' after 1000 ms
...
    13 embodied, 13 reached their place, 0 gave up
```

**Thirteen bodies, thirteen arrivals, none in solid, none gave up.** The check requires all of them and
fails if one does not arrive, so a site with an unreachable pocket fails and names it.

## The check, and the gates

`scripts/away-check.sh` runs the whole chain (re-dress, build, artifact, census, vantages, light,
outside, nav, crew) and is the regression:

```
$ scripts/away-check.sh
...
PASS  every vantage at the site's post is legible -- the room stands in the light
PASS  the frame that looks out shows a world: it differs from the site's own wall at the same vantage
PASS  the engine baked maps/lwh_away_depot.nav on first load
PASS  every crew member the ship put on the site walked to its place, and none is in solid

PASS  the away site loads, walks, reads as outside, is lit by measurement, and its sources are named
```

The gates the pass could disturb:

```
$ scripts/test.sh
...
all checks passed            # exit 0

$ scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
...
PASS  the 2 unresolved references are exactly the recorded boundary   # exit 0
```

## Judgement calls, named as calls

1. **One archetype, not five.** The design names six; this pass builds the station/outpost because its
   source already carries the exterior, so it is the cheapest proof that a site can read as outside.
   The derelict interior — the design's own first row — is next. Recorded in the brief, §2.
2. **The crop window is a chosen line** (`x 400..1280, y 672..1144`, floor z 168). It is the source's
   own top deck, snapped to a 16-grid; everything below is the arena's lower levels, and is not this
   site. The coordinates are invented, in the ledger.
3. **The machine blocks that stand on the deck are dropped.** The source's arena is packed with them,
   and a crop that keeps them is a maze rather than a place. The rule is mechanical — a body over 64
   units square, sitting on the deck rather than forming it — and the thin ribs, rails, floor plates
   and walls stay. This is the largest single change to what was copied.
4. **The shell (walls, roof, sky wall, parapet) and the console are new construction from the source's
   own materials** — the same move as deck 12's plant and shell (`docs/evidence/deck12-redress.md`).
5. **The lighting is added and is ours.** The source's deck is dark by its own design (an arena); a
   working outpost's lights are not. Intensities and colours are invented, in the ledger.
6. **The roof covers the west half only.** This is the asymmetry the template asks for, and it is what
   makes the sheltered/open threshold and the exterior read. A fully roofed site would be a room.
7. **One part is not from `ctf_space`**: the cargo rack (`models/mapobjects/cargo/rack.md3`, shipped,
   used in `scav1`). It is named because the brief's rule is that a machine checks per-part sources,
   and this part's source is not the map's.
8. **The site marker (`lwh_away_site`) has no reader.** It is placed so the away loop has something to
   key on; the loop is scenario content and is not this pass. Named rather than hidden.
9. **No emergency lighting state, and no failure mode of the site's own.** The emergency-lighting and
   breach mechanisms are the ship's and a site off the ship is not on its grid; an away site's own
   endurance is the loop's, which is not built.
10. **The extracted sources and the built map are not committed** — `build/` is ignored, and the re-dress
    is a script. Nothing here is a game asset in the repository.

## What could not be verified here — including the biggest risk

- **Whether the site reads as *somewhere*, rather than as assembled parts. That is the owner's
  walkthrough, and it is the biggest risk in this pass.** This is a slice of one source butted against
  new shell and new light; each source was built to be a whole place, and a slice of one can read as a
  diorama. **The seam is the risk, not the rooms** — every frame here shows a room that reads on its own
  terms, and none of them answers whether the whole holds together. It is a call, not a claim.
- **Whether the debris field reads as *this* system's** rather than as the game's generic debris. It is
  the game's generic debris (`env/junk`); nothing ties it to a chart row.
- **The away mission's acceptance run** — *"an away mission can be solved by scanning rather than by
  shooting"* (G17's own words) — **is not run and could not be**. The tricorder's `Scan` reads a *chart
  row*, not a place (`module/ship/ship_core.cpp`), and no scenario loads this map on a transporter
  delivery. So this pass closes the map half of G17 and leaves the acceptance run to the loop, exactly
  as `docs/evidence/gap-tricorders-and-kit.md` names it.
- **Kit loss on the site** (a property the away site is supposed to have,
  `docs/evidence/gap-tricorders-and-kit.md`) — not built; there is no kit on the map.
- **The `crew-door-check.sh` census** — the brief names it, but its census is of the merged ship's
  doors and this site has no doors; the reachability instrument used here is the `s5-check.sh` one (the
  crew layer embodying and walking), which the brief also names.
- **Performance on the site** — no frame-time measurement was taken.
