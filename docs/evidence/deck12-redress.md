# Evidence — deck 12, re-dressed from `tour/deck11`

Date: 2026-10-06. Branch: `feat/deck-twelve-redress`, cut from `feature/g3-reactive-crew`.
Brief: `docs/locations/deck12-environmental-control.brief.md`. Procedure:
`docs/authoring-a-location.md`. What was copied and every judgement call: the commit message, this
document, and the ledger entry in `docs/lore-ledger.md`.

Nothing here is asserted without the command that produced it. The command is given before its
output.

## What was copied, and what was changed

The brief names the reuse target (section 3): **copy `tour/deck11`, main engineering**, the densest
systems room in the deck set. `tools/shipmap/dressdeck12.py` reads the published `deck11.map` and
writes `deck12.map`; `scripts/build-ship.sh` calls it in place of the generated placeholder. It copies
the room's own brushes and entities — the consoles, railings, lighting fixtures, wall panels, door
hardware, props and "small untidiness" — and changes what makes this room *this* room:

- **the vertical scale is reduced** (deck 11's 404-unit room compressed to 192, toward the floor);
- **the warp core becomes an atmosphere plant** (the tall core-textured cluster is dropped and plant
  vessels built from the room's own chrome/glass/beam materials);
- **a Jefferies tube exit** is added on the west wall, opposite the main door, low and awkward, with a
  `target_level_change` to deck 11 — the route that works when the turbolifts are down;
- **the watch console** is moved to where the room can be entered, in the room's most open spot, with
  the life-support station marker in front of it.

The wiring is kept: the life-support station marker, section 42 sealed next door, the breach triggers
and field, the status panel, the lift edge and the navigation furniture.

```
$ python3 tools/shipmap/dressdeck12.py --deck11 build/gdk/maps/eliteforce_virtualvoyager_maps/deck11.map \
      --out build/ship/generated --report build/ship/deck12-dress.json --census build/ship/deck12-census.json
deck12 <- build/gdk/maps/eliteforce_virtualvoyager_maps/deck11.map
  room -4096..-2704 x -4032..-2784, height 404 -> 192 (x0.78)
  world brushes 1944, entities 312, waypoints 57, core dropped 28, patches dropped 45, breach True
  wrote build/ship/generated/deck12.map
```

The dress report: 264 entities kept (56 dropped — deck 11's scripts, NPC spawns, turbolift network,
pickup and editor tags), 1,972 brushes kept, 28 core brushes dropped, 45 patches dropped.

## A1 — the merged ship compiles and `check-bsp.py` reads the artifact

```
$ scripts/build-ship.sh
...
/tmp/tmp.DvB31DWQt2/maps/voyager.bsp         OK                                 (1/17 lumps empty)
    534 entities added to /tmp/tmp.DvB31DWQt2/maps/voyager.bsp
wrote /home/c/big/git/long-way-home/build/ship/out/longway_voyager.pk3
```

`OK` is `tools/mapgen/check-bsp.py`'s verdict on the compiled BSP, run inside `build-map.sh`: shaders,
brushes and surfaces are not empty. The exit status of q3map2 is not the evidence; this line is. The
same verdict from the artifact itself:

```
$ unzip -j build/ship/out/longway_voyager.pk3 maps/voyager.bsp -d /tmp/x
$ python3 tools/mapgen/check-bsp.py /tmp/x/voyager.bsp
/tmp/x/voyager.bsp                           OK                                 (1/17 lumps empty)
```

The full build runs `-vis` and `-light`, so the ship is sealed: q3map2 wrote a portal file (a leaked
map writes none and `-vis` fails). An earlier build of this deck leaked, from a light entity compressed
above the new ceiling and from a tube chamber open to the void; both were found and fixed before this
build.

## A2 — the deck loads, is reached on foot, and navigation bakes on first load

```
$ rm -f build/g3-home/baseEF/maps/voyager.nav
$ scripts/deck12-check.sh
==> the deck 12 re-dress
    deck 12 room: standing at (-2800 -3840 -37646)
PASS  the deck 12 re-dress loads, is reachable on foot, and is photographed for the walkthrough
      screenshot: /home/c/big/git/long-way-home/build/g3-home/baseEF/screenshots/lwh_deck12.tga
```

The load lines, from that run's log (`build/g3-home/deck12.out`):

```
EFSP: SP_SpawnServer: CM_LoadMap(maps/voyager.bsp)
EFSP: Munro connected
SHIP: deck 12 room: standing at (-2800 -3840 -37646)
```

`voyager.nav` was deleted before the run and the engine wrote it again (`build/g3-home/baseEF/maps/voyager.nav`,
2,012,788 bytes), so navigation is baked from the room's waypoints, not shipped by hand. The check also
fails on any "in solid" waypoint; the room's waypoints are placed on the local floor and filtered
against the final solids.

## A3 — reachable from the turbolift, proven

```
$ scripts/s3-check.sh
    deck 12: at (-2800 -3840 -37676)  standing, clear, on deck 12  ok
...
    turbolift deck 12: use tour_turbo_12
PASS  one map, fifteen decks, each reached by turbolift from the one before with no level load
PASS  the retail turbolift menu reads our fifteen-deck list, and opens on the merged ship
```

The turbolift reaches deck 12 and the player stands clear on the deck. The tube is the second route
(its `target_level_change` to `tour/deck11` is a real exit when the lifts are down).

## A4 — the life-support station and the wiring survive, and the breach triggers still fire

The station marker `lwh_station_0` is in the generated map, in front of the watch console, and the
compartment row is the ship model's own (`SYS_LIFE_SUPPORT`, deck 12) — a re-dress replaces geometry,
not wiring. The breach triggers and field are re-sited in open room and re-proven:

```
$ scripts/breach-check.sh
ENV: deck 1 is whole; no breach effect
SHIP: breach test: standing at (-3582 -3216 -37640)
SHIP: breach test: deck 12 hull 0.00, air 1.00, minutes of air 3.8
ENV: deck 12 is breached; the air is going (push and hurt on, field off); the push throws (366 0 288)
SHIP: breach test: field off, thrown: health 93, velocity (366 0 -80), at (-3453 -3216 -37650)
SHIP: breach test: field off: health 93, velocity (0 0 0), air 0.99, minutes of air 3.7
ENV: a force field is up on deck 12; the air holds
SHIP: breach test: field on: health 93, velocity (0 0 0), minutes of air -1.0, published field 1
PASS  a breach pushes and hurts where it is authored; the field stops it and holds the air; the gate holds
```

The authored push throws the body toward the hole (velocity 366 toward it), the hurt volume takes it to
93, the field stops both and holds the air, and with the environment layer off none of it runs.

## A5 — the emergency lighting state is observable in the world

```
$ scripts/emergency-check.sh
==> emergency lighting, g_env 1
ENV: emergency lighting off on deck 1 (life support 100%, 4 red and 4 working strips)
SHIP: emergency test: standing at (-3776 -3776 -37646)
SHIP: emergency test: alert 0, life support 100%
ENV: emergency lighting on on deck 12 (life support 100%, 4 red and 4 working strips)
ENV: emergency lighting off on deck 12 (life support 100%, 4 red and 4 working strips)

==> emergency lighting, g_env 0 (the extension off)
PASS  the emergency lighting state comes on at battle stations and stands down; the gate holds
      screenshots: .../screenshots/lwh_emergency.tga, lwh_emergency_off.tga
```

The strips are authored `func_usable` brushes running along the room's ceiling — red (`hall/hall_light_red`,
a shader the game already has) held off, working (`engineering/elight1`) on. The module
(`module/crew/g_crew.cpp`, `SyncEmergencyLight`) swaps them when the ship is at red alert or life
support falls below 60%, by the same pattern as the breach field. The screenshots
(`lwh_emergency.tga` / `lwh_emergency_off.tga`) show the red present and absent.

## A6 — the parts census: no new art

```
$ python3 tools/shipmap/parts_census.py --map build/ship/generated/deck12.map --game build/baseEF \
      --census build/ship/deck12-census.json
build/ship/generated/deck12.map: 82 distinct materials
  present as shipped images:      2694
  declared by shipped shaders:   2070
  our own aliases:               27
  in the reuse source census:    77
PASS  every material already exists in the game; no new art
```

The full list, with what each material is and where it comes from, is
`docs/locations/deck12-parts-list.md`. The build runs this check, so a deck 12 that named art the game
does not have would fail the build.

## A7 — the gates

```
$ scripts/test.sh
...
all checks passed            # exit 0

$ scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
    the 8 rejections are exactly the known, documented set   # exit 0
```

(`check.sh` needs the GDK entity dictionaries beside the build; they are copied into `../upstream`
before the run — see the note in the run notes below.)

## Judgement calls, named as calls

1. **The room is kept whole, not cut into two chambers by a new wall.** The brief's change-list names
   four changes; a dividing wall is not among them, and "keep the console arrangement, the railings,
   the lighting fixtures, the wall panels and the door hardware" argues for keeping deck 11's room.
   Its own raised deck and side bays are the asymmetry section 5 asks for. If the owner wants two
   rooms, that is the next pass.
2. **The room crop is a chosen line.** x -4096..-2704, y -4032..-2784, from the source's own
   detail-brush cluster. Everything outside is the deck's corridors and was not copied. Invented, in
   the ledger.
3. **The core drop is a bounding box plus a texture test.** Tall brushes using core materials inside
   the core cluster are dropped; the rest of the room is untouched. 28 brushes.
4. **The breach is re-sited by measurement, not left where it was.** Its push and hole keep the
   blockout's proportions so the shove is survivable; the field sits over the hole. Coordinates are
   invented.
5. **The tube routes to deck 11** (engineering), the deck next door and the source of the room. Canon
   makes Jefferies tubes the emergency movement route; the destination is a call.
6. **The emergency threshold** is red alert or life support below 60%. Invented.
7. **Deck 11's scripts, NPC spawns, pickup and turbolift network are dropped**, not copied: they are
   deck 11's business, and the room's crew come from the direction layer, not from the map.

## What could not be verified here

- **The owner's walkthrough.** The brief's closing question — does this read as somewhere a person
  watches over fifteen decks of air, or like a box with a console in it? — is not answered by any
  transcript. The compression from 404 to 192 is the biggest risk: a room can be made low and read as
  machinery, or as squashed. Walk it.
- **Crew post coverage on deck 12.** The station marker is present and the crew layer can use it, but
  the G3 measurement harness runs on the deck 4 scenario, and no deck-12 crew scenario exists yet, so
  coverage was not measured here. The marker is wired; the measurement waits on a scenario.
- **The tube as an actual walk.** The traversal entity is in the map and the stitch report counts it;
  no session walked the tube and rode it. It is a `target_level_change`, the same mechanism the
  turbolift uses, proven above.
- **The red state reading as an emergency rather than a glow.** The state is authored, switched and
  photographed; whether the room *feels* like it has gone to emergency is the walkthrough.

## Run notes (environment)

- `scripts/check.sh` failed at first because `../upstream` had no `SP_entities.def` /
  `HM_entities-def.txt`: `scripts/bootstrap-upstream.sh` does `git clean -qfd` on that checkout, which
  removes the untracked GDK dictionaries. Copying them from `build/gdk` (where `scripts/fetch-gdk.sh`
  puts them) and re-running gave exit 0. This is environment, not this work.
- The module is rebuilt with `cmake --build ../upstream/efgame/build-linux`, which compiles
  `module/` into `libefgame.so`; the engine binary is unchanged.
