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
