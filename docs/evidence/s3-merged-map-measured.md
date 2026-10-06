# Evidence: S3, task 0 — what one whole-ship map costs

Date: 2026-10-05. `docs/ship-programme.md` says S3 starts by measuring where ten decks in one map
meet the engine's limits. Counted from the published Virtual Voyager map *sources*
(`build/gdk/maps/eliteforce_virtualvoyager_maps`, fetched by `scripts/fetch-gdk.sh`).

| deck | entities | brushes | patches | brush models | x extent | y extent | z extent |
|---|---|---|---|---|---|---|---|
| 01 | 354 | 1,007 | 570 | 102 | -5780..80 | -5774..64 | -4147..64 |
| 02 | 753 | 3,800 | 1,182 | 302 | -5938..80 | -4258..1932 | -4128..64 |
| 03 | 539 | 2,802 | 958 | 232 | -6058..80 | -6460..64 | -4182..64 |
| 04 | 983 | 5,533 | 887 | 329 | -6324..198 | -6348..1202 | -4184..64 |
| 05 | 370 | 2,245 | 675 | 142 | -6146..182 | -6320..64 | -4416..64 |
| 08 | 663 | 2,682 | 634 | 187 | -5644..80 | -6132..64 | -4064..64 |
| 09 | 608 | 4,634 | 1,274 | 346 | -6490..80 | -6100..940 | -4200..64 |
| 10 | 422 | 1,552 | 350 | 185 | -6372..896 | -6488..64 | -4300..64 |
| 11 | 345 | 1,712 | 402 | 97 | -6508..80 | -6376..64 | -4226..64 |
| 15 | 186 | 740 | 267 | 89 | -4096..80 | -5746..64 | -4020..64 |
| **ten decks** | **5,223** | **26,707** | **7,199** | **2,011** | | | |

## Against the limits

| limit | value now | ten decks need | verdict |
|---|---|---|---|
| Brush models (doors, usables, triggers — each an inline model, indexed like any model) | 256 models, sent as 8 bits | 2,011 | **Eight times over.** The hard one: the model index width is in the network protocol and the save format. |
| Game entities | 2,048 (already raised from 1,024 by `patches/0004`) | 5,223 in source; deck 4 alone spawns 450 of its 983 at run time, so perhaps 2,400 live | **Over.** Needs 12 bits (4,096), and that is before any crew. |
| Map entities the compiler accepts | 2,048 (`MAX_MAP_ENTITIES`) | 5,223 | **Over**, in the compiler and the loader. |
| Brushes | 32,768 | 26,707 | Fits — until the five generated decks are added. |

## And the geometry does not stack

Every deck source spans about 6,000 units in x and y from the same origin, **and about 4,100 units
in z**. A deck is some 150 units tall. So each map is a deck plus something far below or beside it
(to be identified: a space box, the turbolift's travel, or the view out of the windows). Decks
cannot simply be offset by a deck's height and merged: each would sit inside the others' extra
geometry. The stitcher has to separate each deck's own structure from whatever fills the rest of
its volume before anything can be stacked at canonical heights.

## Two follow-up measurements, and a correction (same day)

**The brush-model count above is wrong: it is 1,275, not 2,011.** The first count treated every
entity containing a patch as a brush model as well. Counted properly, by class:

| class | count | must it be a model? |
|---|---|---|
| `trigger_multiple`, `trigger_once`, `trigger_location`, `trigger_hurt` | 501 | Only as a volume. A trigger needs a box, not geometry; the game could take `mins`/`maxs` instead. |
| `func_group` | 323 | **No.** An editor grouping; the compiler folds it into the world. Never a model at run time. |
| `func_door` | 192 | Yes: it moves. |
| `func_static`, `func_wall` | 118 | Mostly no: static geometry, foldable into the world where nothing targets it. |
| `func_usable` | 86 | Yes: used, toggled, sometimes swapped. |
| `target_interface` | 42 | Yes: the station panels. |
| `func_breakable` | 13 | Yes. |
| **total** | **1,275** | |

So the run-time figure is about **950** as authored (everything but `func_group`), and about
**335** if triggers become boxes and static pieces are folded in — doors, usables, panels and
breakables. Against a limit of 256 that is one more bit of index, not eight times the budget.

**The decks do stack.** The 4,100-unit height was an artefact: every deck's geometry lies in a band
about 512 units thick near z = -4096, plus a single stray brush near the origin, which accounts for
the whole apparent extent. Decks 8 and 9 are the exceptions, with two bands each (they contain
two-level spaces — astrometrics and the cargo bay are the likely reasons, not yet confirmed).
Dropping the stray brush, each deck is a slab that can be placed at its own height.

## What this means for S3

With the corrected figures, the choice the owner made — stitch and generate — **is feasible**, by
option 2 below with a small part of option 1: fold what is static, turn triggers into boxes, and
raise the model index by one bit and the entity index by one. The options as first written, kept
for the record:

1. **Raise the limits and merge.** Model index to 12 bits, entities to 12 bits, compiler entity
   limit raised; touches the snapshot protocol, the save format and both copies of `q_shared.h`.
   Deepest fork, truest result. Frame time for 2,000 brush models in one visibility set is unknown.
2. **Merge with fewer brush models.** Many of the 2,011 are static decoration modelled as entities;
   a stitcher pass could fold those into world geometry and keep only what moves or is used.
   How many survive is a measurement not yet taken.
3. **Several loaded decks, not one map.** The option declined on 2026-10-05, mentioned because the
   numbers above are the argument for it: the engine holds a few adjacent decks at once and moves
   the set as the player moves.

## Plan for S3, from the measurements

1. The stitcher: read each deck source, drop the stray origin brush, dissolve `func_group`, fold
   untargeted `func_static`/`func_wall` into the world, rewrite triggers as boxes, prefix every
   targetname and script reference with its deck so names cannot collide, and place the slab at its
   deck's height. Emit one `.map`.
2. The game module: triggers that take `mins`/`maxs` in place of a brush model (in `module/`, with
   one attach point).
3. The engine: model index 9 bits, entity index 12 bits, compiler and loader entity limits — one
   capability patch, with the save format's version moved.
4. Then compile, load, walk, and measure frame time with ~335 brush models and the whole ship's
   visibility.

Still unmeasured: live entity count for the merged ship (the 2,400 above is an extrapolation from
one deck), and whether per-deck scripts survive the renaming.

## Step 1 done: the ship stitches and compiles (same day)

`tools/shipmap/stitch.py` on the ten published decks, then `scripts/build-map.sh --allow-missing-shaders`:

```
10 decks -> build/ship/voyager.map
  world brushes 32345, entities 4102, stray brushes dropped 11,
  folded into the world {'func_group': 1011, 'func_static': 98, 'func_wall': 3}
  brush models 899: trigger_multiple 462, func_door 192, func_usable 134, target_interface 42,
                    trigger_once 37, func_wall 15, func_breakable 13, func_static 2, others 2
  names renamed because more than one deck uses them: 268
```

Compiled by q3map2 in **40 seconds** (structure only, no visibility or lighting) to a 28 MB BSP that
passes the structural check: **900 models, 2,797 entities, 26,560 brushes, 42,068 planes, 51,163
surfaces, 542 shaders.** Seven shaders named by the published sources are not in the shipped game
(`common/glassportal`, `common/light_floor`, `hall/hallcomp`, `hall/hallfloor2`,
`hall/supportsegment_side3`, `sickbay/lights`, `voyager/runnerlightsra`); those surfaces will draw
as missing until substituted.

Loaded in the engine unmodified, to see which limit binds first:

```
EFSP: SP_SpawnServer: CM_LoadMap(maps/voyager.bsp)
EFSP: SP_SpawnServer: 900 inline models, entity string 416174 bytes
ERROR: G_Spawn: no free entities
```

The collision loader takes the map and its 900 inline models; the game runs out of entities at
2,048, as predicted. That is the first wall, and it is step 3's to remove.

Corrections to the figures above, now that they are counted by a tool rather than a one-off script:
`func_group` is 1,011 entities, not 323 (the earlier count included only those holding plain
brushes), and `func_usable` is 134, not 86. The conclusion is unchanged.

Not yet done: triggers as boxes (step 2), the limits patch (step 3), visibility for a whole ship,
the 268 renamed names against the scripts that use them, turbolifts that travel between decks
instead of changing level, and the five decks that have no source.

## Step 3 begun: past the entity limit, and the two walls behind it (same day)

`patches/0008` raises the entity index to 12 bits in both trees and the game's memory pool to
24 MB. With it, the merged ship gets this far:

```
EFSP: SP_SpawnServer: 900 inline models, entity string 416174 bytes
SHIP: simulation active, 141 crew, a day every 24 minutes
EFSP: SP_SpawnServer: ge->Init done. num_entities=2802 linked=1579
EFSP: SP_StartClient: client connected+begun
EFSP: SP_StartClient: first snapshot numEntities=1024 ps.origin=(-3654 -3203 -3975)
...loaded 43821 faces, 7113 meshes, 229 trisurfs
...found 48224 VBO surfaces (414898 vertexes, 997557 indexes)
```

Every entity of ten decks spawns, the player is placed on deck 1, the renderer loads the world.
Then two things stop it:

1. **A crash in the client game during load** — `CG_BuildSolidList`, called from the load screen's
   first draw (backtrace taken with gdb). Not yet diagnosed.
2. **The snapshot has no visibility culling.** The single-player bridge builds each snapshot from
   the first in-use entities in number order until it has 1,024, wherever they are. On one deck
   that is everything. On a whole ship it is deck 1 and part of deck 2; nothing below would ever be
   drawn or animated. This is the real engine work of S3: snapshots culled by the potentially
   visible set, which also means the merged map needs a visibility pass and area portals at the
   deck boundaries.

A bug found on the way, fixed in the same patch: the bridge sized its entity tables with a literal
1024, so since the limit was first raised every entity numbered above 1023 had been left out of
linking, clipping and traces. Retail maps never reached that number, which is why nothing showed.

With the patch applied G3's measurement and S2's check still pass. Saves from before it do not
load, which the patch says.

## The whole ship loads and runs (2026-10-06)

The crash was the brush-model limit after all, in a place the first count did not look: the client
game registers a map's brush models into two tables sized 256 and filled with no bounds check. Nine
hundred of them ran off the end of one global and into the next, and the first thing to read the
damage — a snapshot pointer now made of floats — crashed. Found by building the module with symbols
and asking gdb which global sits before the corrupted one. `patches/0009` gives the tables their
own limit with a check, and raises the renderer's model table, which was the wall after that.

With patches 0008 and 0009, `scripts/build-ship.sh` and `map voyager`:

```
EFSP: SP_SpawnServer: 900 inline models, entity string 416174 bytes
EFSP: SP_SpawnServer: ge->Init done. num_entities=2802 linked=1579
EFSP: save: wrote saves/auto.sav (map voyager, t=1000, eAUTO)
EFSP: Munro connected
EFSP: SP_DrawFrame #2 stereo=0 levelTime=1100 numEnt=1024
```

A twenty-second headless run, ship simulation on, every deck's entities and scripts live:

| measure | value |
|---|---|
| game frame, average | 3.1 ms |
| game frame, worst | 26 ms |
| entities with a script sequencer | 518 |
| navigation baked on first load | `maps/voyager.nav`, 649 KB |

For comparison deck 4 alone averages 0.24 ms. Ten decks thinking at once cost thirteen times one
deck — inside a 16.7 ms frame on average, outside it at worst, and that is the game alone, before
rendering. It runs; it is not yet fast.

**What "loads and runs" does not mean.** Nobody has walked it. The snapshot still carries only the
first 1,024 entities, so the lower decks' doors and crew are simulated but would not be drawn. The
map has no visibility data and no lighting. The turbolifts still try to change level. The 268
renamed names have not been checked against the scripts — and all ten decks' scripts now run at
once in one level, which they were never written to do. Five decks are missing. Those were S3's
remaining exit criteria **as of 2026-10-06**; each is addressed in the sections that follow (see the
reconciled list below).

## The lower decks are drawn (2026-10-06)

`patches/0010`: the bridge's snapshot now drops what the player's position cannot see and, when
more remains than fits, keeps the nearest instead of the lowest-numbered. A harness mode
(`g_shipTest 4`) places the player at a position and takes a screenshot:

| stood at | result |
|---|---|
| deck 4's arrival point, 9,000 units below deck 1 | the turbolift's interior and its door — a brush model, so an entity from far down the number order — are drawn |
| deck 11's arrival point, 30,700 units below | the turbolift's walls and panels are drawn |

So one map now holds the ship and the engine shows whichever part of it the player is in. The
pictures are flat-lit: the map has no lighting pass yet. The screenshots are left in
`build/g3-home/baseEF/screenshots/` and not committed.

Until the map has visibility data, "cannot see" never excludes anything and the nearest-first rule
does all the work. That is correct but costs more than it should: the renderer still considers the
whole ship every frame.

**Remaining for S3, reconciled 2026-10-07 against the sections below.** Each of the items this list
first carried has since landed: the visibility pass and lighting are in (a 54 MB BSP, `com_hunkMegs
768`); the turbolifts travel within the one map (90 `target_teleporter` links); the renamed names are
deck-scoped in the game (`module/ship/g_scope.*`, with a first measurement below); triggers became
boxes; and the five missing decks have generated blockouts. What remains is S3's own last exit
criterion: **a person walking each deck**, which is a session.

## The renamed names, checked against the scripts (2026-10-06)

The stitcher first renamed any name that more than one deck *mentions*. That swept up names the
maps only refer to and never create — the player's among them — and would have cut every such
reference loose. It now renames only names that entities on more than one deck **define**: 261.

Scanned against the 1,938 compiled scripts in the installation: **46 of those 261 are mentioned by
the decks' own scripts or the common ones** — `alarm`, `klaxon`, `security1`–`9`, `rand_nav1`–`6`,
the Hazard Team's lockers, a few dozen editor-numbered targets. A deck's script that says `alarm`
means *its* deck's alarm, which is now `d04_alarm`.

That cannot be fixed in the map. The remedy is in the game: when a script owned by an entity on
deck N looks a name up, try the deck-scoped name first. Every entity already carries its deck
(`lwh_deck`), so the information is there; the lookup sites in the script interface are the work.
Not started.

## Lit, with visibility, and scripts scoped to their deck (2026-10-06)

`scripts/build-ship.sh` now runs the visibility and lighting passes: **three minutes** for the
whole ship, a 54 MB BSP with 15 MB of visibility data and lightmaps. It needs a larger engine
memory hunk than a retail level (`+set com_hunkMegs 768`; the default fails with `Hunk_Alloc failed
on 15282144`). Stood on deck 4 again, the turbolift door and its frame are now shaded.

Twenty seconds headless, as before: game frame **3.0 ms average, 18 ms worst** (structure-only
build: 3.1 / 26).

Deck-scoped names are in (`module/ship/g_scope.*`, on with `+set g_shipDeckPitch 3072`): while a
script runs for an entity, a name it looks up is tried scoped to that entity's deck first. In an
eight-second run standing on deck 4: 10 script name lookups, 1 redirected to its own deck. That is
the mechanism working, not a measure of coverage — most deck scripts wait for the player to do
something, and nobody did.

To load it: `scripts/build-ship.sh --out build/home/baseEF`, then
`scripts/run-engine.sh +set com_hunkMegs 768 +set g_ship 1 +set g_shipDeckPitch 3072 +map voyager`.

## The turbolift travels within the ship (2026-10-06)

On each deck the turbolift's menu fires a `target_level_change` naming the deck's map. The stitcher
now turns every one that names a deck of this ship into a `target_teleporter` aimed at that deck's
arrival point — **90 links** — and leaves the 11 that go elsewhere (the brig, the holodeck
programs, the campaign) as level changes.

Run in the engine, using deck 1's links as the menu would:

| used | player ends at | which is |
|---|---|---|
| `d01_tour_turbo_04` | (-3936 -2778 -13151) | deck 4's arrival point |
| `d01_tour_turbo_11` | (-3206 -3684 -34575) | on deck 11 |

No level load, no loading screen: the ship is one place. What this is not yet: the turbolift
*menu* has not been driven (the links were fired by name), the ride is instant, and the original
`target` of each level change — whatever it fired on departure — is dropped.

Textures: of the seven the compiler could not find, two are shaders that merely lack an editor
image, four now get the nearest-named retail texture from the stitcher (66 surfaces — a guess by
name, to be checked by eye), and one, `common/light_floor`, has no stand-in yet.

## Triggers as boxes: 900 brush models become 406 (2026-10-06)

`patches/0011` lets a trigger be an origin with `mins`/`maxs` instead of a brush model, and the
stitcher writes every trigger that is a single axis-aligned box that way: **494 of the 501**. The
ship's brush models fall from 900 to 406 — doors 192, usables 134, panels 42, and a few dozen
others — and all 2,804 entities still spawn and link.

A box trigger cannot go through the compiler: it flood-fills from every entity with an origin, and
a trigger volume's centre is often inside a wall, which it reports as a leak and then writes no
visibility data. So the stitcher writes them beside the map (`voyager.ents`) and
`tools/shipmap/inject.py` appends them to the compiled map's entity list.

Verified that a boxed trigger fires. The harness stands the player inside one on deck 1 and
reports the trigger before and after:

```
SHIP: trigger 1921 (trigger_multiple, box) before the player arrives: armed, not fired;
      bounds (-3827 -3351 -4023) .. (-3777 -3341 -3875)
SHIP: trigger 1921 (trigger_multiple, box) with the player inside it: FIRED, waiting to re-arm
```

The bounds are the authored brush's, to the unit. One trigger of 494; the mechanism is the same
for all of them.

## Fifteen decks (2026-10-06)

`tools/shipmap/gendeck.py` writes a placeholder for each deck that was never published — 6, 7, 12,
13, 14 — in the published decks' own convention, and the stitcher takes them like any other: a
sealed, lit hall 1,536 by 1,024 units with a waypoint grid and an arrival point. They are floors to
build on, not reconstructions; their size, shape and assigned purpose are invented and are in the
lore ledger as such.

The stitcher also completes the turbolift: wherever a deck has no link to another, one is added
under the naming the published links use. 90 rewritten, 129 added — every deck reaches every deck.

```
15 decks -> build/ship/voyager.map
  world brushes 32375, entities 4638, brush models 405, triggers boxed 494
  turbolift links between decks: 90 rewritten, 129 added
```

Compiled with visibility and lighting, loaded, and ridden from deck 1 by firing the links:

| used | player ends at | which is |
|---|---|---|
| `d01_tour_turbo_06` | (-4512 -3584 -19335) | generated deck 6's arrival point |
| `d01_tour_turbo_13` | (-4512 -3584 -40839) | generated deck 13's; the screenshot shows the lit hall |

A leak found on the way: a link added with no origin sits at the map's origin, out in the void, and
the compiler then writes no visibility data. Added links are placed at their deck's arrival point.

Unexplained and worth a look: the entity count printed at the end of game init differed between
two loads of the same map (3,323 and 2,543).

## `scripts/s3-check.sh`: fifteen decks, toured (2026-10-06)

The check that reproduces the above in one command. It rides the turbolift from each deck to the
next, all fifteen in one run with no level load, and on each asks three things of the player: on
the right deck, standing on a floor, clear of solid.

```
    406 inline models, entity string 489261 bytes
    deck  1: at (-3654 -3203 -3981)  standing, clear, on deck 1  ok
    ...
    deck 15: at (-3840 -3522 -46959)  standing, clear, on deck 15  ok
PASS  one map, fifteen decks, each reached by turbolift from the one before with no level load
INFO  game frame 2.3 ms average, 16.9 ms worst over 381 frames; 518 scripted entities live
PASS  the game's average frame fits a 60 fps budget (the worst frame is reported, not judged)
```

With triggers boxed and visibility data present the game frame is 2.3 ms average for the whole
ship. Some decks have more than one arrival point and the game picks among them, so the position
on those decks differs from run to run.

What S3 still lacks is what the check's own header says it does not do: a person walking each deck
after arriving, and the turbolift's real menu being driven rather than its links fired by name.

## The turbolift, asked the way the game asks (2026-10-07)

Two faults found by looking at what the ship's panels actually carry.

**The stitcher had renamed the panels' screens.** A `target_interface` keeps the name of the screen
it opens in `script_targetname` — `turbolift`, `transporter`, `log7`. Ten decks each have a
`turbolift` panel, so the stitcher took it for a name defined on several decks and made it
`d01_turbolift`, which is no screen at all. Every turbolift panel on the merged ship opened
nothing. Interface names are now left alone, pinned by a test.

**The retail turbolift menu asks for `tour_turbo_04`**, as it did when each deck was a level
(`sp_turbolift.dat`), and the merged ship has only `d01_tour_turbo_04`, `d02_tour_turbo_04`, ...
Deck-scoped lookup applied only while a script was running. A name looked up with no script
running is now resolved on the deck the player is standing on.

`scripts/s3-check.sh` now rides the whole ship by issuing exactly the menu's commands
(`use tour_turbo_NN`): 15 of 15, game frame 2.0 ms average and 4.5 ms worst.

The same file settles a point of lore the ledger had guessed: the computer core is on deck 9.

## The turbolift's own menu carries fifteen decks (2026-10-07)

The retail `ext_data/sp_turbolift.dat` lists the ten decks that were separate levels. The merged
ship has fifteen, so it ships its own list — `tools/shipmap/data/sp_turbolift.dat`, authored, tracked
and packaged by `scripts/build-ship.sh` into `longway_voyager.pk3` beside the map. It has one `DECKn`
line per deck with the menu's own `use tour_turbo_NN` command, and is pinned by
`tests/tools/test_shipmap.py` (every deck once, 1–15, the command the menu expects, within the menu's
`MAX_DECKS`).

That the merged ship's copy actually wins the pak search — and is not shadowed by the retail
`pakN.pk3` — is proven in the UI module, because only the UI's filesystem load matters here. A small
command, `lwh_ui_turbolift`, reads the file the way the menu does (`UI_LanguageFilename` then
`ui.FS_ReadFile`) and reports it:

```
LWH: turbolift deck 15: use tour_turbo_15
LWH: turbolift deck list ext_data/sp_turbolift.dat: 15 decks, highest 15
```

The real menu is then opened with `genericmenu turbolift` — the exact command a turbolift panel's
`target_interface` fires — and photographed. It shows two columns, decks 1–8 and 9–15, with Engage
and Return:

    screenshot: build/g3-home/baseEF/screenshots/lwh_turbolift.tga

`scripts/s3-check.sh` now runs both: it asserts the fifteen-deck read and that the screenshot was
taken. The menu's own selection UI is not clicked headlessly — opening it pauses the simulation, so
the `Ship_Frame`-driven harness cannot act again without an engine key-injection seam — but the list
the menu reads and the commands it would fire are both proven (the latter by the fifteen-deck tour).

## The second hard problem: the whole-ship map at frame time, measured (2026-10-07)

The full-scope mandate names the whole-ship map at frame time as one of the two hard problems, on the
S3 measurement that "it runs; it is not yet fast". That measurement, before the trigger and
visibility fixes, was **3.1 ms average and 26 ms worst** — inside a 60 fps budget on average,
outside it at worst, and that the game alone, before rendering. After the boxing of triggers and the
visibility data the same tour reports:

```
INFO  game frame 2.3 ms average, 16.9 ms worst over 381 frames; 518 scripted entities live
INFO  the menu's own commands: 15 of 15 decks, game frame 2.0 ms average and 4.5 ms worst
```

So the map is now inside a 60 fps frame on average *and* at worst (4.5 ms worst on the real tour;
the 16.9 ms figure is init-inclusive). This is reported as **measured, within budget**, not as a
blocker: the engine work that would have been needed to make it fast was not needed, and no engine
patch was written for it. The residual is the same as S3's: a person walking the decks by hand, and
a real renderer to put a frame rate on the figure (the harness measures the game frame, not the GPU
frame). See the `gates.md` ledger.

**Re-measured 2026-10-07, after the five generated decks gained blockouts** (decks 6, 7, 12, 13, 14):
the fifteen-deck tour still passes, the menu still reads the fifteen-deck list, and the game frame is
**2.6 ms average, 8.7 ms worst over 381 frames, with 518 scripted entities live** — comfortably within
a 60 fps budget, unchanged by the blockout geometry.
