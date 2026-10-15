# Evidence: the derelict interior — O11's first archetype

**What this is.** `docs/outside-the-ship.md`'s **first** archetype row: *"derelict interior — re-dress
`dn*`, `borg*`, `scav*` — wrecked corridors, failing light, no gravity in places."* One archetype was
already built (the space station/outpost, `lwh_away_depot`); this is the next, the design's own first
row. Source: **`scav2`** (`build/gdk/maps/eliteforce_scav_maps/scav2.map`), chosen by the survey
(`docs/evidence/the-archetype-survey.md`: the densest single-level interior of the four candidates).
**Named, as `AGENTS.md` requires:** the re-dress copies `scav2`'s own brushes, materials and lamps; one
part is not from it, named in the report (the cargo rack `models/mapobjects/cargo/rack.md3`, shipped,
used in `scav1`).

**Host:** `sasquatch`, the engine host. Branch `feat/derelict-interior`, cut from `origin/testing`.

## The measure came first, and it is the archetype's own

The check family that decides "is this legible" is the **room** measure (`frame.py floor`, the
DARK/LEGIBLE verdict), and the away pass has just spent a per-frame dark exemption so a picture of the
sky may be dark. This archetype is that problem whole: **a derelict's defining property is failing
light**, so a luminance floor cannot express "it reads as wrecked", and a pass that discovered this frame
by frame would end with an allowlist a page long. So the **measure is the pass's first deliverable**, and
**no per-frame dark exemption is allowed for this archetype.**

`frame.py derelict` asserts what the archetype *means*:

- **failing light, not a lit room** — the typical pixel is dark (`--max-median 90`);
- **failing light, not absent light** — something is clearly lit (`--min-p99 80`), and it is not pitch
  black (`--min-median 8`);
- **structure, the read of wreckage** — enough pixels sit on a brightness edge (`--min-edge 0.004`, a
  gradient above 16): bulkheads, ribs, a floor. A flat field — black or uniform — fails.

The thresholds are calibrated on this map's two captures (lit and un-lit), named as calls below.

## What changed

| file | what |
|---|---|
| `tools/away/dress_derelict.py` | **new**: the re-dress (crop `scav2`'s level, shell it, wreck details, failing light, the no-gravity zone, two posts, spawn, `--unlit`) |
| `tools/frames/frame.py` | the **`derelict`** measure (the archetype's own assertion), and its stance in the docstring |
| `module/ship/g_ship.cpp` | harness case **`g_shipTest 102`**: read `ps.gravity` in the hall and the zone, driving the map's trigger |
| `scripts/derelict-check.sh` | **new**: the regression (re-dress, build, census, load, the measure, the falsification, the zero-g run, nav) |
| `docs/outside-the-ship.md`, `docs/omissions.md` (O11), `docs/gates.md` | the row, the register entry, the ledger |

## What I verified by running it

`scripts/derelict-check.sh` exits **0**:

```
==> the re-dress (lit): the source's own level, shelled and lit failing
==> the build, the artifact, and the parts census (no new art)
    PASS  every material already exists in the game; no new art
==> the archetype's own assertion on its own frames (must read DERELICT)
    PASS  every frame reads as a derelict: dark, structured, and marked
==> the falsification: the SAME measure must FAIL on the un-lit before-capture
    PASS  the measure fails on the un-lit capture, so it has teeth
==> no gravity in places: the zone's own trigger -> target_gravity_change, read from ps.gravity
    PASS  the zone has no gravity (0) and the hall does (800): a zone, not the whole map
    PASS  the engine baked maps/lwh_derelict.nav on first load
PASS  the derelict interior loads, reads as wrecked by its own measure, ...
```

**The measure, on the lit derelict (must read DERELICT) and on the un-lit before-capture (must fail):**

```
$ python3 tools/frames/frame.py derelict --band 0.10,0.55 --require /home/c/lwh-frames/the-derelict/lit/*.tga
lwh_derelict_console_0.tga  band 0.1-0.55  median  50.0 p99  83.0 edge 0.007  DERELICT
lwh_derelict_console_1.tga  band 0.1-0.55  median  46.0 p99  84.0 edge 0.009  DERELICT
lwh_derelict_console_2.tga  band 0.1-0.55  median  36.0 p99  98.0 edge 0.009  DERELICT
lwh_derelict_console_3.tga  band 0.1-0.55  median  68.0 p99  88.0 edge 0.007  DERELICT
lwh_derelict_console_4.tga  band 0.1-0.55  median  42.0 p99  96.0 edge 0.006  DERELICT
lwh_derelict_console_5.tga  band 0.1-0.55  median  62.0 p99 109.0 edge 0.007  DERELICT
$ ... /home/c/lwh-frames/the-derelict/unlit/*.tga   # the same measure, the un-lit capture
lwh_derelict_console_0.tga  ...  median   0.0 p99   9.0 edge 0.006  NOT-WRECKED -- median 0 < 8 (pitch black: the light is absent, not failing)
```

The **room** measure on the same lit frames says DARK (median 14–26 < 35) — the reason `floor` cannot be
used here, and the reason this archetype carries its own measure.

**The zero-g zone, by a run** (the map's own trigger → `target_gravity_change`, read from `ps.gravity`):

```
$ ... +set g_ship 1 +set g_shipTest 102 +map lwh_derelict
    SHIP: zero-g: hall  ps.gravity 800 custom 1
    SHIP: zero-g: zone trigger lwh_zerog_trigger
    SHIP: zero-g: zone  ps.gravity 0 custom 1
```

**The frames are preserved** outside the repo (the repo carries no game media) at
`/home/c/lwh-frames/the-derelict/lit/` and `.../unlit/`, **1280×1024**, band `0.10,0.55` = **y 102..563,
x 0..1280 (461 rows, 590,080 px)**.

## What I could not verify, and why

- **Whether the derelict reads as `wrecked corridors` — the owner's eye.** The frame is delivered
  (`/home/c/lwh-frames/the-derelict/lit/lwh_derelict_console_0.tga`, PNG beside it): a dim rust-brown
  interior with a ceiling marker and a raised platform. **I have not captioned it as reading well.** The
  measure asserts failing light, structure and markers; it cannot assert *wreckage* rather than a dim
  room, and that judgement is his.
- **Whether `scav2` is the best source.** The survey named it on density and a single level; the other
  three are multi-level. A different source may read better, and that is content.
- **The no-gravity zone by *touch*.** A headless teleport does not reliably touch a brush trigger, so the
  harness **drives the map's own trigger** through `G_UseTargets` and reads `ps.gravity`. The trigger and
  the target are the map's; the *touch* path is a session's. Said plainly.

## What I deliberately left unbuilt, and why

- **The other four archetypes** (alien ship, industrial surface, cave/mine, colony) — the design's order,
  after this one.
- **The away-mission loop** — scenario content, registered and named, not started.
- **The structure's enclosed facings (the away site's first cause)** — the owner's shape judgement, left
  named and unbuilt.
- **Wreck *dressing* beyond a first pass** — a fallen girder, a buckled plate, a burnt bulkhead are in;
  the corridor is broken but sparse, and richer wreckage is content the owner's eye sizes.

## Judgement calls, named as calls

1. **The measure is applied to the derelict's own frames, not as a classifier across maps.** Its
   thresholds separate failing light from absent light (the un-lit capture); the "not a lit room" ceiling
   is a sanity clause and cannot by itself reject every other archetype's frames — named here so it is not
   over-read.
2. **The thresholds (median 8..90, p99 ≥ 80, edge ≥ 0.004, gradient 16) are calibrated on this map's lit
   and un-lit captures**, the two inputs the assertion must separate. Measured separation: lit median
   36–68 / unlit 0; lit p99 83–109 / unlit ≤ 70.
3. **`--unlit` is the falsification input, and it is this pass's own capture**, not a synthetic file: the
   re-dress with no lights is exactly "a frame that does not carry the claim".
4. **The zone is driven, not touched**, and said so — the honest limit of a headless trigger.
5. **I re-used the away-site tool's machinery** (crop/shell/drop/light/waypoints/posts), the house
   pattern for a location, rather than a new mechanism.
