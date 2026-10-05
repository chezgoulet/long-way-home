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

## What this means for S3

"Stitch the ten sources into one BSP" does not survive measurement as stated. Two of the three
excesses are in the engine's protocol-level limits, not in the map. The options, for the owner:

1. **Raise the limits and merge.** Model index to 12 bits, entities to 12 bits, compiler entity
   limit raised; touches the snapshot protocol, the save format and both copies of `q_shared.h`.
   Deepest fork, truest result. Frame time for 2,000 brush models in one visibility set is unknown.
2. **Merge with fewer brush models.** Many of the 2,011 are static decoration modelled as entities;
   a stitcher pass could fold those into world geometry and keep only what moves or is used.
   How many survive is a measurement not yet taken.
3. **Several loaded decks, not one map.** The option declined on 2026-10-05, mentioned because the
   numbers above are the argument for it: the engine holds a few adjacent decks at once and moves
   the set as the player moves.

None of these is started. The next step is the two missing measurements: what occupies each deck's
extra 4,000 units, and how many brush models are static.
