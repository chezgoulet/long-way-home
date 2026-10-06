# Evidence — the two decks that were still missing, and the ship's response to a death

Date: 2026-10-06. Branch: `feat/the-two-missing-decks`, cut from `feature/g3-reactive-crew`.
Brief: deck 12 (environmental control), deck 13 (life-support plant) as **places in the simulation**,
and a death's consequences as a **cheap** mechanic (funeral, sealed quarters, the wall), not a new
subsystem.

Nothing here is asserted without the command that produced it. The command is given before its
output.

## What was already there, and what this work adds

The generator (`tools/shipmap/gendeck.py`) and the blockout check scripts (`scripts/deck12-check.sh`,
`scripts/deck13-check.sh`) were already on the trunk from the `g3-wip` snapshot, and the blockouts
loaded and were photographed. They were **not** yet places in the simulation. This work adds:

- the life-support system's **gravity** state, alongside the atmosphere it already held, sited on
  deck 12 (`SYS_LIFE_SUPPORT`'s `SystemSpec` is already `deck 12`);
- a **declared turbolift edge** on each new deck (`target_level_change` carrying a `mapname`), so
  reachability is in the map data, not assumed from the stitcher;
- **section 42 closed off and recorded** (a sealed compartment, and one entry in the lore ledger);
- **grief that is metabolised**: `HoldFuneral` now leaves a positive `MEM_FUNERAL` mark and softens
  the death marks, the wall of names is a read, and the sealed quarters are legible to the player.

## Task A — deck 12, environmental control

### A1 — the merged ship rebuilds, and `check-bsp.py` reads the artifact

```
$ scripts/build-ship.sh --fast
...
/tmp/tmp.iMWh11erCa/maps/voyager.bsp         OK                                 (4/17 lumps empty)
    494 entities added to /tmp/tmp.iMWh11erCa/maps/voyager.bsp
wrote /home/c/big/git/long-way-home/build/ship/out/longway_voyager.pk3
```

`OK` is `tools/mapgen/check-bsp.py`'s verdict on the compiled BSP (shaders, brushes and surfaces are
not empty). The exit status of q3map2 is not the evidence; this line is.

### A2 — deck 12 loads, is reached, and navigation bakes on first load

```
$ rm -f build/g3-home/baseEF/maps/voyager.nav
$ scripts/deck12-check.sh
==> the deck 12 blockout
    deck 12 blockout: standing at (-4512 -3584 -37744)
PASS  the deck 12 blockout loads, is reachable on foot, and is photographed for approval
      screenshot: /home/c/big/git/long-way-home/build/g3-home/baseEF/screenshots/lwh_deck12.tga
```

The load lines, from that run's log (`build/g3-home/deck12.out`):

```
EFSP: SP_SpawnServer: CM_LoadMap(maps/voyager.bsp)
EFSP: Munro connected
SHIP: deck 12 blockout: standing at (-4512 -3584 -37744)
```

`voyager.nav` was deleted before the run and the engine wrote it again on the load (the file is back
at `build/g3-home/baseEF/maps/voyager.nav`), so navigation is baked, not shipped by hand.

### A3 — deck 13, the plant, the same way

```
$ scripts/deck13-check.sh
==> the deck 13 blockout
    deck 13 blockout: standing at (-4512 -3584 -40816)
PASS  the deck 13 blockout loads, is reachable on foot, and is photographed for approval
      screenshot: /home/c/big/git/long-way-home/build/g3-home/baseEF/screenshots/lwh_deck13.tga
```

```
EFSP: SP_SpawnServer: CM_LoadMap(maps/voyager.bsp)
EFSP: Munro connected
SHIP: deck 13 blockout: standing at (-4512 -3584 -40816)
```

### A4 — reachable from the turbolift, proven by the edge

Each generated deck now declares a `target_level_change` with a `mapname`
(`tools/shipmap/gendeck.py`). In the stitched map it is rewritten to a teleporter to the deck 4
arrival, and the reverse edges are added:

```
$ grep -A5 '"targetname" "lwh_lift_d12"' build/ship/voyager.map
"classname" "target_teleporter"
"targetname" "lwh_lift_d12"
"origin" "-4448 -3584 -37768"
"target" "d04_arrival"
"lwh_deck" "12"
```

The stitch report counts the rewrite (`turbolift_links` 95 rewritten, 129 added across the fifteen
decks; five of the rewrites are the generated decks' own declared edges). And the ride itself, which
reaches every deck in one run with no level load:

```
$ scripts/s3-check.sh
    deck  1: at (-3654 -3203 -3981)  standing, clear, on deck 1  ok
    ...
    deck 12: at (-4512 -3584 -37767)  standing, clear, on deck 12  ok
    deck 13: at (-4512 -3584 -40839)  standing, clear, on deck 13  ok
    ...
PASS  one map, fifteen decks, each reached by turbolift from the one before with no level load
```

### A5 — the system and its compartment, and gravity located on deck 12

`SYS_LIFE_SUPPORT`'s `SystemSpec` already reads `{ "life support", 12, "Environmental Control", ... }`
and the generated deck already carries the station marker `lwh_station_0`. New this work: a per-deck
`gravity` field, held by the same system as the air. With life support down a deck loses hold
(`GRAVITY_FAIL_HOURS`, 6 h), and a supplied deck regains it (1 h); it is in the save (format
**version 46**). The unit test that already watched the air go stale now watches gravity go with it:

```
$ <build>/test_ship_core
ship_core: all checks passed
```

`scripts/test.sh` runs the suite and reports the same (`all checks passed`).

## Task B — the ship's response to a death

A death already notified command, sealed the casualty's quarters and logged it. New this work:

- `HoldFuneral` **metabolises** the loss (docs/morale.md): each attendee's death marks soften toward
  shared memory, and each takes a positive `MEM_FUNERAL` mark toward whoever held it — a bond that
  was not there before;
- `WallOfNames` is the wall of names (a read over the records);
- `SealedQuarters` names the shut doors and their decks; `ship wall` prints both, `ship status`
  carries them, and `Publish` exports `lwh_ship_sealed` / `lwh_ship_wall` for the screens.

The rules are unit-tested in `TestCrewJusticeAndBorg` (the funeral lifts morale, opens the quarters,
leaves the positive mark, softens the death mark, and the name stays on the wall). The
player-facing path is driven through the game console:

```
$ scripts/grief-check.sh
==> a death and its consequences
    grief: Les Foster is dead, 1 quarters sealed, 1 on the wall
    grief: after the funeral 0 quarters sealed, 1 on the wall, crew 11 funeral mark 1
      Les Foster's quarters, deck 3, are sealed
PASS  a death seals the quarters and puts a name on the wall; a funeral opens them and leaves a mark
```

## The gates

```
$ scripts/test.sh
...
all checks passed
$ echo $?
0

$ scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
...
    the 8 rejections are exactly the known, documented set
$ echo $?
0
```

## Judgement calls, named as calls

1. **Section 42 is a sealed compartment, not a room.** Solid bulkheads on its two open sides, the
   deck's own walls on the other two, no way in, and its inside is not waypointed. Recorded once in
   `docs/lore-ledger.md`.
2. **Gravity is life support's, at invented rates.** Canon groups life support and gravity; the
   rates are `[inv]` and in the ledger. `SYS_LIFE_SUPPORT` already names deck 12, so the state is
   located there by the existing system spec, not by a new table.
3. **The funeral's bond is toward the officer who held it**, one mark each, rather than every pair
   of attendees. A pairwise bond would be O(n²) marks and would evict the crew's other memories; the
   brief says keep it cheap. The shared resolve is the morale lift and the softened death marks.
4. **Deck 13's turbolift label was "Cargo and Stores"** in our fifteen-deck list; it is now "Life
   Support Plant", matching `docs/ship-master-map.md`. Recorded in the ledger.
5. **Sealed quarters are visible through the console and the HUD** (`ship wall`, `ship status`,
   `lwh_ship_sealed`), not through a per-door prompt in the world. The game has no per-door
   quarters interaction; a world prompt would be a new feature, and the brief says stop before that.
6. **The reuse target is not yet spent.** Deck 12/13 are still generated from bare brushes, not a
   re-dress of `tour/deck11`'s models and fixtures. That is the brief's *detail* work, which
   `docs/authoring-a-location.md` says waits on the owner's approval of the blockout; this work
   makes the blockouts places in the simulation instead.

## What could not be verified here

- **That the space reads as somewhere a person works.** That is the owner's walkthrough, and the
  procedure says it is the check that closes the deck.
- **A death's full in-world feel** (a player walking past the sealed door and knowing it) — the
  state and the console read are shown; the world prompt is the call above.
