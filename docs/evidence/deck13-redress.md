# Evidence — deck 13, the life-support plant, re-dressed from `tour/deck11`

Date: 2026-10-07. Branch: `feat/deck-thirteen-lifesupport`, cut from `feature/g3-reactive-crew`. Brief:
`docs/locations/deck13-life-support.brief.md`. Procedure: `docs/authoring-a-location.md`. What was
copied and every judgement call: the commit message, this document, and the ledger entry in
`docs/lore-ledger.md`.

It is deck 12's pattern, reused, not a second one: `tools/shipmap/dressdeck13.py` is the copy of
`dressdeck12.py` with deck 13's program, and `scripts/build-ship.sh` calls both in place of the
generated placeholder. Deck 12's tool is untouched, and deck 12's output still builds and still
passes its own check (A8).

Nothing here is asserted without the command that produced it. The command is given before its
output.

## What was copied, and what was changed

The brief names the reuse target (section 3): **copy `tour/deck11`, main engineering**, "as for deck
12, with the warp core swapped for an atmosphere plant and the vertical scale reduced". It copies the
room's own brushes and entities — the consoles, railings, lighting fixtures, wall panels, door
hardware, props and "small untidiness" — and changes what makes this room *this* room:

- **the vertical scale is reduced** (deck 11's 404-unit room compressed to 224, less hard than deck
  12's 192: this is the hall, not the control room);
- **the warp core becomes a central machinery island**: the tall core-textured cluster is dropped
  and a plinth of atmosphere processors is raised down the middle from the room's own
  chrome/glass/beam/newgrey materials;
- **a raised machinery catwalk** is added along the north wall, with a rail and a stair of 16-unit
  steps at its east end, so the machinery reads and the watch position looks down the island;
- **a Jefferies tube mouth** is added at the far end from the door (west), low and awkward, with a
  `target_level_change` to deck 11 — the route down from Engineering when the turbolifts are down;
- **a local plant panel** stands on the catwalk (the brief's control surface), a console with the
  ship's live state drawn on it (`lwh/panel`), with the `lwh_plant_post` marker — the brief's "one
  watch/roving maintenance position on the catwalk" — and a `waypoint_navgoal_1` named `plantwatch`.

The wiring is kept: the room's own navigation furniture, the status panel and shader remap, the lift
edge, and the room's navigation. Deck 12's watch console, station marker and breach stay next door,
untouched.

```
$ python3 tools/shipmap/dressdeck13.py --deck11 build/gdk/maps/eliteforce_virtualvoyager_maps/deck11.map \
      --out build/ship/generated --report build/ship/deck13-dress.json --census build/ship/deck13-census.json
deck13 <- build/gdk/maps/eliteforce_virtualvoyager_maps/deck11.map
  room -4096..-2704 x -4032..-2784, height 404 -> 224 (x0.8)
  world brushes 1957, entities 315, waypoints 64, core dropped 28, patches dropped 45
  wrote build/ship/generated/deck13.map
```

The dress report: 264 entities kept (56 dropped — deck 11's scripts, NPC spawns, turbolift network,
pickup and editor tags), 1,972 brushes kept, 28 core brushes dropped, 45 patches dropped, 64
waypoints, the three processors at `(-3612, -3408)`, `(-3400, -3408)`, `(-3188, -3408)` and the
plant panel at `(-3776, -2880)` on the catwalk.

## A1 — the merged ship compiles and `check-bsp.py` reads the artifact

```
$ scripts/build-ship.sh
deck13 <- /home/c/big/git/long-way-home/build/gdk/maps/eliteforce_virtualvoyager_maps/deck11.map
  room -4096..-2704 x -4032..-2784, height 404 -> 224 (x0.8)
  world brushes 1957, entities 315, waypoints 64, core dropped 28, patches dropped 45
  wrote /home/c/big/git/long-way-home/build/ship/generated/deck13.map
/home/c/big/git/long-way-home/build/ship/generated/deck13.map: 81 distinct materials
  present as shipped images:      2694
  declared by shipped shaders:   2070
  our own aliases:               27
  in the reuse source census:    77
PASS  every material already exists in the game; no new art
  world brushes 36284, entities 5079, stray brushes dropped 11, folded into the world {'func_group': 1011, 'func_static': 98, 'func_wall': 3}
  ...
  deck 12: z -37925 .. -37496
  deck 13: z -41001 .. -40572
/tmp/tmp.OedtvNc7Z1/maps/voyager.bsp         OK                                 (1/17 lumps empty)
    572 entities added to /tmp/tmp.OedtvNc7Z1/maps/voyager.bsp
wrote /home/c/big/git/long-way-home/build/ship/out/longway_voyager.pk3
```

`OK` is `tools/mapgen/check-bsp.py`'s verdict on the compiled BSP, run inside `build-map.sh`:
shaders, brushes and surfaces are not empty. The exit status of q3map2 is not the evidence; this
line is. The same verdict from the artifact itself:

```
$ unzip -j build/ship/out/longway_voyager.pk3 maps/voyager.bsp -d /tmp/opencode/pk3x
$ python3 tools/mapgen/check-bsp.py /tmp/opencode/pk3x/voyager.bsp
/tmp/opencode/pk3x/voyager.bsp               OK                                 (1/17 lumps empty)
```

The full build runs `-vis` and `-light`, so the ship is sealed: q3map2 wrote a portal file (a leaked
map writes none and `-vis` fails). A first compile of `deck13.map` alone leaked from the `plantwatch`
navgoal, placed in the north wall; it was moved onto the catwalk and the leak cleared.

## A2 — the deck loads, is reached on foot, and navigation bakes on first load

```
$ rm -f build/g3-home/baseEF/maps/voyager.nav
$ scripts/deck13-check.sh
==> the deck 13 re-dress
    deck 13 room: standing at (-2800 -3840 -40716)
    deck 13 room: at the catwalk: standing at (-3776 -2880 -40716)
PASS  the deck 13 re-dress loads, is reachable on foot, and is photographed for the walkthrough
      screenshots: .../screenshots/lwh_deck13.tga, .../screenshots/lwh_deck13_catwalk.tga
```

The load lines, from that run's log (`build/g3-home/deck13.out`):

```
EFSP: SP_SpawnServer: CM_LoadMap(maps/voyager.bsp)
EFSP: Munro connected
SHIP: deck 13 room: standing at (-2800 -3840 -40716)
SHIP: deck 13 room: at the catwalk: standing at (-3776 -2880 -40716)
```

`voyager.nav` was deleted before the run and the engine wrote it again
(`build/g3-home/baseEF/maps/voyager.nav`, 2,087,044 bytes), so navigation is baked from the room's
waypoints, not shipped by hand. The check also fails on any "in solid" waypoint; the room's
waypoints are placed on the local floor — including the catwalk and its stair — and filtered against
the final solids.

## A3 — reachable from the turbolift, proven

```
$ scripts/s3-check.sh
    deck 12: at (-2800 -3840 -37676)  standing, clear, on deck 12  ok
    deck 13: at (-2800 -3840 -40746)  standing, clear, on deck 13  ok
    deck 14: at (-4512 -3584 -43911)  standing, clear, on deck 14  ok
...
    turbolift deck 13: use tour_turbo_13
PASS  one map, fifteen decks, each reached by turbolift from the one before with no level load
PASS  the retail turbolift menu reads our fifteen-deck list, and opens on the merged ship
INFO  game frame 2.0 ms average, 9.3 ms worst over 381 frames; 526 scripted entities live
PASS  the game's average frame fits a 60 fps budget (the worst frame is reported, not judged)
```

The turbolift reaches deck 13 and the player stands clear on the deck. The tube is the second route
(its `target_level_change` to `tour/deck11` is a real exit when the lifts are down). Frame time
holds.

## A4 — the life-support system, the deck's wiring, and the environment triggers

The system node `SYS_LIFE_SUPPORT` and the deck-13 compartment row are the ship model's own
(`module/ship/ship_core.*`); deck 12 is its control room and deck 13 its plant. The deck's own
markers survive into the merged map:

```
$ for n in lwh_lift_d13 lwh_tube_d13 lwh_d13_door lwh_plant_post plantwatch d13_arrival \
      lwh_light_normal_13_0 lwh_light_emergency_13_1; do grep -c "\"targetname\" \"$n\"" build/ship/voyager.map; done
1
1
1
1
1
1
1
1
```

Deck 12's environment triggers and field are untouched and still fire — the pair shares the
environment layer, so this is the non-regression run for the acceptance line:

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
...
PASS  a breach pushes and hurts where it is authored; the field stops it and holds the air; the gate holds
```

## A5 — the emergency lighting state is observable in the world, on both decks

```
$ scripts/emergency-check.sh
==> emergency lighting, g_env 1
ENV: emergency lighting off on deck 1 (life support 100%, 8 red and 8 working strips)
SHIP: emergency test: standing at (-3776 -3776 -37646)
SHIP: emergency test: alert 0, life support 100%
ENV: emergency lighting on on deck 12 (life support 100%, 8 red and 8 working strips)
ENV: emergency lighting off on deck 12 (life support 100%, 8 red and 8 working strips)
ENV: emergency lighting off on deck 1 (life support 100%, 8 red and 8 working strips)
SHIP: emergency test 13: standing at (-3776 -2880 -40716)
SHIP: emergency test 13: alert 0, life support 100%
ENV: emergency lighting on on deck 13 (life support 100%, 8 red and 8 working strips)
ENV: emergency lighting off on deck 13 (life support 100%, 8 red and 8 working strips)

==> emergency lighting, g_env 0 (the extension off)
PASS  the emergency lighting state comes on at battle stations and stands down on both decks; the gate holds
      screenshots: .../screenshots/lwh_emergency.tga, lwh_emergency_off.tga,
                   lwh_emergency_13.tga, lwh_emergency_13_off.tga
```

The strips are authored `func_usable` brushes running along the room's ceiling — red
(`hall/hall_light_red`, a shader the game already has) held off, working (`engineering/elight1`) on.
Deck 13's carry a deck tag (`lwh_light_emergency_13_1`, `lwh_light_normal_13_0`, ...): the module
(`module/crew/g_crew.cpp`, `SyncEmergencyLight`) toggles by prefix, and two decks defining one name
would make the stitcher rename both, leaving the module nothing to toggle. The count is now the
pair's eight red and eight working strips, and the state is ship-wide on red alert or when life
support falls below 60%. The screenshots (`lwh_emergency_13.tga` / `lwh_emergency_13_off.tga`) show
the red present and absent.

> Note (2026-10-07, after deck 14): deck 14's stasis chamber carries tagged strips too
> (`lwh_light_emergency_14_*`), toggled by the same prefix, so a fresh run of this check now reads
> "12 red and 12 working strips" — the three decks', not the pair's eight and eight. Deck 13's own
> state is unchanged; see `docs/evidence/deck14-stasis.md` (A5).
>
> Note (2026-10-07, after deck 7): deck 7's core deck carries tagged strips too
> (`lwh_light_emergency_07_*`), so a fresh run reads "16 red and 16 working strips" — the four decks'
> and deck 7's sessions were added to the check; see `docs/evidence/deck07-auxcore.md` (A5).

## A6 — the parts census: no new art

```
$ python3 tools/shipmap/parts_census.py --map build/ship/generated/deck13.map --game build/baseEF \
      --census build/ship/deck13-census.json
build/ship/generated/deck13.map: 81 distinct materials
  present as shipped images:      2694
  declared by shipped shaders:   2070
  our own aliases:               27
  in the reuse source census:    77
PASS  every material already exists in the game; no new art
```

The full list, with what each material is and where it comes from, is
`docs/locations/deck13-parts-list.md`. The build runs this check, so a deck 13 that named art the
game does not have would fail the build.

## A7 — the gates

```
$ scripts/test.sh
...
all checks passed            # exit 0

$ scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
    2024 files: 2016 compiled and read back, 8 rejected by the compiler, 0 read-back failure(s)
    the 8 rejections are exactly the known, documented set   # exit 0
```

## A8 — deck 12 still works

```
$ scripts/deck12-check.sh
==> the deck 12 re-dress
    deck 12 room: standing at (-2800 -3840 -37646)
PASS  the deck 12 re-dress loads, is reachable on foot, and is photographed for the walkthrough
```

`tools/shipmap/dressdeck12.py` is unchanged; the build runs it before deck 13's and deck 12's map
compiles and loads.

## Judgement calls, named as calls

1. **The room is kept whole and one chamber, not cut by a new wall.** The brief (section 5) asks
   for "a long plant hall, one chamber with a central machinery island"; this is that, and it is the
   difference from deck 12, which kept two chambers. The room crop is the same line deck 12 took —
   x -4096..-2704, y -4032..-2784, from the source's own detail-brush cluster — because it is the
   same source room. Invented, in the ledger.
2. **The vertical scale is 404 → 224 (x0.8).** The brief says "reduced" without a number; deck 12
   chose 192 for a low control room, and a plant hall wants more headroom over the catwalk. Invented,
   in the ledger. It is also this deck's walkthrough risk (below).
3. **The core drop is deck 12's bounding box plus a texture test**, unchanged: tall brushes using
   core materials inside the core cluster are dropped (28), the rest of the room is untouched, and
   the island is built in the cleared middle.
4. **The island is massing, not simulation.** Three processors on a plinth down the centre, from the
   room's own materials; it must read as plant, per the brief's non-goal.
5. **The catwalk is reachable by a stair of 16-unit steps**, at the catwalk's east end. The engine
   steps over 16 units, not over 24; the first draft's 24-unit steps were not walkable.
6. **Deck 13 gets no second hull breach.** Deck 12's brief lists the compartment row and the
   environment triggers; deck 13's section 7 lists only the system node, the job sites and the local
   plant panel. The environment layer is verified on deck 12 (A4) instead, and the plant's own
   "failure" is the emergency lighting state and the ship model's life-support output — not a new
   authored breach. If the owner wants a breach at the plant, that is the next pass.
7. **The local plant panel is a physical surface, not a new UI.** It is a console with the live
   `lwh/panel` state drawn on it, the same control surface deck 12's watch console is; operating the
   system is the engineering console (`ui_lwh_engineering`), where engineering authority is checked.
   Authored, not exercised (below).
8. **Deck 13's emergency strips carry a deck tag** (`lwh_light_..._13_N`) so they do not collide with
   deck 12's names, which the stitcher would otherwise rename on both decks. The module toggles them
   by prefix, so both decks' lights work.
9. **The tube routes to deck 11** (engineering), the deck above and the source of the room. Canon
   makes Jefferies tubes the emergency movement route; the destination is a call.
10. **The emergency threshold** stays red alert or life support below 60%, the pair's established
    value. Invented.

## What could not be verified here

- **The owner's walkthrough.** The brief's closing question — does it read as the plant that keeps
  the ship breathing? — is not answered by any transcript. The compression from 404 to 224 is the
  biggest risk, the same class deck 12 named: a hall can be made low and read as machinery, or as
  squashed. The screenshots (`lwh_deck13.tga`, `lwh_deck13_catwalk.tga`) show the room dark under the
  copied lighting, with the machinery reading as unlit masses; whether the working light over the
  plant should be added or brightened is the thing a person has to look at.
- **Crew post coverage on deck 13.** The `plantwatch` navgoal and `lwh_plant_post` marker are present
  and the crew layer can use them, but the G3 measurement harness runs on the deck 4 scenario, and no
  deck-13 crew scenario exists yet, so coverage was not measured here. The post is wired; the
  measurement waits on a scenario.
- **The local plant panel as an operating console.** Its `lwh/panel` surface renders the ship's live
  state (visible in the screenshots) and the console opens the engineering console by the same
  command the rest of the ship uses, but no session stood at it and operated a system through it.
  Authored, not exercised.
- **The tube as an actual walk.** The traversal entity is in the map and the stitch counts it; no
  session walked the tube and rode it. It is a `target_level_change`, the same mechanism the
  turbolift uses, proven above.
- **The catwalk as circulation, not just a stand.** The check stands on it and the waypoints cover
  it; that the stair reads as a stair rather than a ledge is the walkthrough.

## Run notes (environment)

- The module is rebuilt with `cmake --build ../upstream/efgame/build-linux`, which compiles
  `module/` into `libefgame.so`; the engine binary is unchanged.
- `scripts/emergency-check.sh` now runs four headless sessions (deck 12 and deck 13, environment on
  and off). The deck 12 evidence's "4 red and 4 working strips" is now the pair's 8 and 8; see the
  note added to `docs/evidence/deck12-redress.md`.
