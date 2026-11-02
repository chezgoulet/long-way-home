# Evidence: the alien ship interior — O11's third archetype

**What this is.** `docs/outside-the-ship.md`'s **second** archetype row: *"alien ship interior — re-dress
`scav1`-`5` — unfamiliar layout, the species' own design language."* The derelict interior (the first
row) was built on `feat/derelict-interior`; this is the row after it. One archetype, taken to the same
standard as the two that exist: re-dressed from the design's own source family, its enclosed facings
opened, its own frame measure, a check that can fail, and this record.

**Source: `scav3`** (`build/gdk/maps/eliteforce_scav_maps/scav3.map`), chosen by the survey
(`docs/evidence/the-archetype-survey.md`, the alien-ship section: the densest compact single-storey
candidate, one 128-unit engineering deck of green ribbed machinery and gold lamp coves). `scav2` is the
derelict's source and is not reused. **Named, as `AGENTS.md` requires:** the re-dress copies `scav3`'s
own brushes, materials and lamps; every part it emits is the source's or a shipped game material, and
the parts census below is the proof. No part is from another map.

**Host:** `sasquatch`, the engine host. Branch `feat/o11-archetype`, cut from `origin/testing`
(`460deaf`). Map `lwh_alien_ship`.

## The measure came first, and it is the archetype's own

The room measure (`frame.py floor`) says a region is legible; the derelict measure says failing light.
This archetype is neither: it is a **working alien deck**, so its claim is the inverse of the derelict's
and the room floor does not carry it either. **No per-frame dark exemption is allowed for this
archetype.**

`frame.py alien` asserts what the archetype *means*:

- **maintained, not a wreck** — the typical pixel is lit (`--min-median 40`) and something is clearly lit
  (`--min-p90 60`). The same room with the lights out is a different archetype and fails, which is what
  the unlit capture below demonstrates;
- **built** — enough pixels sit on a brightness edge (`--min-edge 0.004`, a gradient above 16):
  bulkheads, ribs, machinery. A flat field — black, grey or a single colour — fails;
- **carrying a design language** — enough pixels carry colour (`--min-chroma 0.15`: the fraction whose
  brightest and dimmest channels differ by 24). A neutral grey box with the same geometry and light
  fails, which is what the drab capture demonstrates. The measure asserts *a* palette, not the *right*
  species' palette; that is the owner's eye.

The thresholds are calibrated on this map's own captures, named as calls below. The separate **outside**
assertion (the galleries) is not this measure and is described under *The open facings*.

## What changed

| file | what |
|---|---|
| `tools/away/dress_alien_ship.py` | **new**: the re-dress (crop `scav3`'s upper engineering deck, shell it, two galleries with sky brushes, a working alien deck's light, the console, two posts, waypoints, arrival; `--drab`, `--unlit`, `--sealed`) |
| `tools/frames/frame.py` | the **`alien`** measure (the archetype's own assertion), the chroma fraction, and its stance in the docstring |
| `module/ship/g_ship.cpp` | harness input **`g_shipVantagePace`** (default 192): the vantage rig's pace distance. The cramped interior uses 96; the default leaves every existing caller unchanged |
| `scripts/alien-ship-check.sh` | **new**: the regression (re-dress, build, census, both posts, both galleries, three falsifications, nav, crew) |
| `tests/tools/test_frames.py` | five cases for the `alien` measure (lit/built/coloured pass; dark, flat and grey each fail their own clause) |
| `tests/tools/test_map_tools.py` | three cases for the re-dress (point-in-brush not AABB; a crossing brush dropped not clamped; `--drab` strips materials and light colour) |
| `docs/outside-the-ship.md`, `docs/locations/alien-ship-interior.brief.md`, `docs/evidence/the-archetype-survey.md`, `docs/gates.md` | the row, the brief, the source survey, the ledger |

## The re-dress, and what was copied

```
$ python3 tools/away/dress_alien_ship.py --source build/gdk/maps/eliteforce_scav_maps/scav3.map \
      --out build/away/alien --report build/away/alien/dress.json --census build/away/alien/census.json
lwh_alien_ship <- build/gdk/maps/eliteforce_scav_maps/scav3.map
  window x -224..512 y 560..1120, floor -128 roof 16 (sill -112 header -16)
  world brushes 298, brushes kept 245/3394, lights kept 11, waypoints 24, posts [(-64, 944), (0, 704)]
  galleries: south open east open; drab False, lit True
  wrote build/away/alien/lwh_alien_ship.map
```

- **245 of 3,394 world brushes and 24 patches** are kept: the room around the machine banks. The rest
  are other decks, the source's arena outside the window, or brushes that cross the shell. **A crossing
  brush is dropped, not clamped**: clamping a sloped brush's plane points turns a valid six-plane brush
  into a five-plane one, and a five-plane brush has no interior. That was this tool's first defect — it
  placed no waypoint at all — and `tests/tools/test_map_tools.py` now fails if it returns.
- **11 source lights** are kept inside the sealed room; 528 other entities (the source's spawns, items
  and machinery logic) are dropped.
- **Added:** floor/ceiling/west/north shell, the two gallery walls with sky brushes and mullions, a
  gallery rail, bulkhead ribs and trim, 10 added lights, the alien console, 24 waypoints, two posts,
  the two gallery anchors, the arrival and the site marker.
- **No new art:** 26 distinct materials, all present in the game or in the reuse source's census.

The gallery walls are the only openings. The **south** and **east** facings are galleries; the **west**
and **north** remain solid because the ship continues there and the source's machine banks stand against
them. Each gallery is a gap between a sill (`z −112`) and a header (`z −16`), closed by a sky brush
(`holodeck_temple/ctfc_sky_light2`, `skyParms env/stars`) so the map is sealed and `-vis` still writes a
portal file. There is no facing that was meant to be open and could not be: the two open ones are open,
and the two closed ones are the ship's structure, named rather than implied.

## What I verified by running it

`scripts/alien-ship-check.sh` exits **0**:

```
$ Q3MAP2=… scripts/alien-ship-check.sh
==> the re-dress: scav3's own upper engineering deck, shelled and galleries opened
    lwh_post_console at -64 944 -104
    lwh_post_bank at 0 704 -104
    gallery at [144, 1024, -104]
==> the build, the artifact, and the parts census (no new art)
    …/maps/lwh_alien_ship.bsp  OK                                 (1/17 lumps empty)
    26 distinct materials … PASS  every material already exists in the game; no new art
==> the archetype's own assertion on both posts' six vantages (must read ALIEN)
    … all twelve ALIEN …
PASS  both posts read as a maintained, built, coloured alien deck at all twelve vantages
==> the opening: black space outward, a lit room inward, at the gallery
    outward median 2 (black space), inward median 56 (lit room)
    lwh_alien_ship_gallery_0.tga  … changed 73.2% mean|d| 46.2 of 590080 px  DIFFERS
PASS  the gallery looks out onto black space, inward at a lit room, and the two differ by >= 30%
==> the second gallery, on the east wall: black space outward, the room inward
    outward median 0 (black space), inward median 60 (lit room)
    lwh_alien_ship_gallery_east_2.tga  … changed 86.4% mean|d| 60.0 of 590080 px  DIFFERS
PASS  both galleries (south and east) look out onto black space, inward at a lit room
==> the palette clause's falsification: the SAME lit room, every material grey and every light white
    … chroma 0.00 < 0.15 (a neutral grey box: no design language in the palette) …
PASS  the measure rejects the drab capture on the design-language clause
==> the maintained clause's falsification: the same map with no light and sealed galleries
    … median 0 < 40 (the deck is not maintained: too dark) …
PASS  the measure rejects the unlit capture on the maintained clause
==> the outside assertion's falsification: the same gallery with the openings sealed
    sealed outward median 14 (a wall where the window was)
PASS  sealing the galleries moves the outward median above 5, so the outside assertion rejects a sealed site
==> restore the lit map, bake the nav, and walk the crew
    9 embodied, 9 reached their place, 0 gave up
PASS  the engine baked maps/lwh_alien_ship.nav, and every crew member it embodied walked to its place
PASS  the alien ship interior loads, reads as a maintained alien deck by its own measure, opens two
      galleries onto black space, and its sources are named
```

**The measure, on the two posts' twelve vantages** (`frame.py alien --band 0.10,0.55`, the band
`y 102..563`, full width, 590,080 px):

```
lwh_alien_ship_console_0  median  70.0 p90 111.0 edge 0.018 chroma 0.56  ALIEN
lwh_alien_ship_console_1  median  52.0 p90  87.0 edge 0.041 chroma 0.35  ALIEN
lwh_alien_ship_console_2  median  56.0 p90  96.0 edge 0.050 chroma 0.62  ALIEN
lwh_alien_ship_console_3  median  52.0 p90  93.0 edge 0.017 chroma 0.41  ALIEN
lwh_alien_ship_console_4  median  58.0 p90 104.0 edge 0.057 chroma 0.56  ALIEN
lwh_alien_ship_console_5  median  56.0 p90 104.0 edge 0.035 chroma 0.44  ALIEN
lwh_alien_ship_bank_0     median  56.0 p90  90.0 edge 0.038 chroma 0.51  ALIEN
lwh_alien_ship_bank_1     median  50.0 p90  82.0 edge 0.012 chroma 0.33  ALIEN
lwh_alien_ship_bank_2     median  56.0 p90  84.0 edge 0.042 chroma 0.53  ALIEN
lwh_alien_ship_bank_3     median  46.0 p90  68.0 edge 0.017 chroma 0.25  ALIEN
lwh_alien_ship_bank_4     median  56.0 p90  90.0 edge 0.049 chroma 0.48  ALIEN
lwh_alien_ship_bank_5     median  54.0 p90  91.0 edge 0.032 chroma 0.33  ALIEN
```

The **drab** capture (same room, same light, every material `common/greysquare2`, every light white)
measures chroma **0.00–0.05** and fails the palette clause on five of six frames. The **unlit** capture
(no lights, galleries sealed) measures median **0** and fails the maintained clause on all six. The
**sealed** capture keeps the light and seals the galleries; the outward frame's median moves from **2**
to **14**, so the outside assertion sees the opening gone.

**The frames are preserved outside the repository** (the repo carries no game media) at
`/home/c/lwh-frames/the-alien-ship/{console,bank,gallery,gallery-east,drab,unlit,sealed}`, **1280×1024**,
band `0.10,0.55` = **y 102..563, x 0..1280 (461 rows, 590,080 px)**. The frames were looked at before
they were captioned: the console post's north-facing frame is the south gallery's starfield through a
green mullioned window; its other facings are the room's green ribbed machinery, gold lamp coves and
red ceiling pipes; the east gallery's outward frame is the starfield through the east window, and its
inward frame is the same machinery from the east.

## The planted break

The check's own falsifications are three planted breaks (drab, unlit, sealed). The one that exercises
the **outside** assertion was also planted by hand in the lit path: `Build(..., galleries=False, ...)`
was set in the tool, the check was run, and it failed where the claim lives:

```
    outward median 14 (black space), inward median 54 (lit room)
FAIL  the outward frame is not a picture of black space (median 14 > 5): the gallery is sealed
```

The planted line was removed, the tool was diffed against its copy (`identical`), and the check was run
again: **exit 0**, the output above. The measure's own two clauses are planted by `--drab` and
`--unlit`, and both are rejected as shown.

## The gates

```
$ scripts/test.sh
…
all checks passed            # exit 0

$ scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
…
PASS  the 2 unresolved references are exactly the recorded boundary   # exit 0

$ scripts/away-check.sh
…
PASS  the away site loads, walks, reads as outside from its own posts (partly), is lit by measurement,
      and its sources are named                                      # exit 0
```

`scripts/away-check.sh` was re-run **because the module changed**: the vantage rig now reads
`g_shipVantagePace` (default 192), and the away site's own run is the proof that the default left its
frames unchanged. It passes.

**Save format: unchanged.** No persistent state is added. The map is content, the tools are build-time,
and the module change is a developer-harness cvar that is not serialised. There is no version bump and
no old-blob risk; an older save loads exactly as before.

## What I could not verify, and why

- **Whether it reads as an *alien* place — the owner's eye.** The frame is delivered
  (`/home/c/lwh-frames/the-alien-ship/console/lwh_alien_ship_console_0.tga`, PNG beside it). The measure
  can assert a lit, built, coloured interior; it cannot assert that the palette is the right species' or
  that the room is good. **I have not captioned it as reading well.**
- **Whether `scav3` is the best source.** The survey names it on density and a single storey; `scav4`
  is larger and multi-deck, and a different crop may read better. That is content.
- **The starfield is the game's generic `env/stars`,** not *this* system's. Nothing ties it to a chart
  row. The wider sky pass is the design's *Order of work* item 3 and remains.
- **The gallery frames are not measured by `frame.py alien`, deliberately.** They are the openings, not
  the site's posts: their outward frames are black space, and the outside assertion asserts that
  blackness rather than exempting a post frame from the measure. **The archetype's measure has no
  per-frame exemption** — all twelve post frames pass, and a dark one fails.
- **The away mission's acceptance run** — *"an away mission can be solved by scanning rather than by
  shooting"* (G17's own words) — is not run and could not be. The tricorder's `Scan` reads a *chart
  row*, not a place, and no scenario loads this map on a transporter delivery. This pass builds the
  map the loop is designed to run on; the loop is scenario content.
- **Performance on the site** — no frame-time measurement was taken.
- **The crew floor** — the site embodies the ship's roster (9 at 08:00); this check requires every
  embodied body to arrive and does not introduce a floor. The away site's floor dispute
  (`docs/gates.md`, *Residuals*) is the owner's and is not touched here.

## What I deliberately left unbuilt, and why

- **The other four archetypes** (industrial surface, cave/mine, colony) — the design's order, after
  this one. The station/outpost and derelict interior already exist.
- **The away-mission loop** — scenario content, registered and named, not started.
- **Wreck dressing** — this is an intact ship; the derelict's broken girders would be the wrong
  archetype.
- **A site failure mode** — an away site's air/endurance is the loop's, which is not built.

## Judgement calls, named as calls

1. **The crop window is a chosen line** (`x −224..512, y 560..1120`, floor −128, roof 16). It is the
   source's upper engineering storey, snapped to a 16-grid; everything below is another deck, and
   everything outside is the source's arena. The coordinates are invented, in the brief.
2. **A crossing brush is dropped, not clamped.** This is the fix to the tool's first defect; the shell
   replaces the boundary. Named because it is the largest difference from the derelict's copy rule.
3. **The shell, galleries and console are new construction from the source's own materials** — the same
   move as the outpost's shell and deck 12's plant.
4. **The lighting is added and is ours.** The source's deck is dim by its own design; a working alien
   deck's lights are not. Intensities and colours are invented, in the brief.
5. **The two open facings are south and east; west and north are solid.** A ship interior is not open on
   every side; the galleries are the ship's windows, and the other two walls are the ship continuing.
6. **The pace is 96 for this site** (`g_shipVantagePace`), not the default 192: the room is cramped, and
   the rig's long pace would photograph the far side of its own wall. The default is unchanged for
   every existing caller, and `scripts/away-check.sh` re-run proves it.
7. **The `alien` measure's thresholds (median 40, p90 60, edge 0.004, chroma 0.15 at 24) are calibrated
   on this map's twelve post frames and its two falsification captures**, the inputs the assertion must
   separate. Measured separation: post frames median 46–70 / drab 104–120 / unlit 0; chroma 0.25–0.62 /
   drab 0.00–0.05; edge 0.012–0.057. It is a call, not a universal classifier.
8. **The gallery is a viewpoint, not a post.** The site's two posts are the console and the machine
   bank; the galleries are where a person looks out, and the outside assertion is made there.
9. **`scav3`'s source is named, and no species is invented.** The site does not claim an owner, a
   species or an episode; the source's scavenger materials are the design's own pick.
