# Evidence — deck 14, stasis chambers and Holodeck 1, re-dressed from `tour/deck11`

Date: 2026-10-07. Branch: `feat/deck-fourteen-stasis`, cut from `feature/g3-reactive-crew`. Brief:
`docs/locations/deck14-stasis.brief.md`. Procedure: `docs/authoring-a-location.md`. What was copied
and every judgement call: the commit message, this document, and the ledger entry in
`docs/lore-ledger.md`.

It is deck 13's pattern, reused, not a third one: `tools/shipmap/dressdeck14.py` is the copy of
`dressdeck13.py` with deck 14's program, and `scripts/build-ship.sh` calls it in place of the deck-14
generated placeholder. Decks 12 and 13's tools are untouched, and their output still builds and
still passes their own checks (A8).

**The holodeck placement is honoured, not re-opened.** Deck 14 carries **Holodeck 1**; deck 6
carries Holodeck 2. The call is the brief's (section 2), and this deck is built to it. One
consequence is recorded under judgement calls: the `SYS_HOLODECKS` station marker, which the deck-6
*placeholder* used to define, is now defined by this re-dress.

Nothing here is asserted without the command that produced it. The command is given before its
output.

## What was copied, and what was changed

The brief names the reuse target (section 3): **copy `tour/deck11`**, main engineering, "for the
industrial read", and change the warp core for banks of stasis pods and add a holodeck doorframe. It
copies the room's own brushes and entities — the consoles, railings, lighting fixtures, wall panels,
door hardware, props and "small untidiness" — and changes what makes this room *this* room:

- **the vertical scale is reduced** (deck 11's 404-unit room compressed to 208 — between deck 12's
  low control room, 192, and deck 13's plant hall, 224);
- **the raised core deck is cleared**: main engineering's large raised machinery platform is dropped
  (38 brushes), leaving the base floor, so the room is the **one flat chamber** the brief asks for
  rather than the two-level engineering room. This is the main structural change, and it is deck 14's
  own (decks 12 and 13 kept the raised deck and built their plant on it);
- **the warp core becomes banks of stasis pods**: a row of the game's own pod
  (`models/mapobjects/stasis/pod.md3`, the brief's section 4) along the north wall, each on a small
  pedestal at its local floor, so nothing floats or is buried;
- **a holodeck doorframe** is added at the far (west) end of the pod wall — a closed frame of
  `voyager/voydoor3b` with a glowing `engineering/elight1` door and `engineering/holodecklights`
  strips, with the control panel (`lwh/panel`) in front of it, warm-lit. Holodeck 1's station marker
  (`lwh_station_17`) is at the panel; the deck's maintenance post (`lwh_stasis_post`, navgoal
  `stasiswatch`) faces the arch;
- **a Jefferies tube mouth** at the far end from the door (west), low and awkward, with a traversal to
  **deck 13** — the route up to the life-support plant when the turbolifts are down;
- **the room is re-lit by function**: deck 11's own 104 ceiling lights are kept but dimmed (they are
  engineering-white and light the copied structure), and **cold** (0.42 0.55 0.98), **neutral** and
  **warm** (1.0 0.78 0.48) lights are added on top — cold over the pods, warm at the holodeck, a
  neutral wash between;
- **the emergency lighting state**: red strips (`hall/hall_light_red`, a shader the game already has)
  held off until the module switches them on, and the normal strips (`engineering/elight1`) they
  replace, tagged `lwh_light_..._14_N`.

The wiring is kept: the room's own navigation furniture, the status panel and shader remap, the lift
edge, and the room's navigation. Decks 12 and 13's watch station, plant panel and breach stay next
door, untouched.

```
$ python3 tools/shipmap/dressdeck14.py --deck11 build/gdk/maps/eliteforce_virtualvoyager_maps/deck11.map \
      --out build/ship/generated --report build/ship/deck14-dress.json --census build/ship/deck14-census.json
deck14 <- build/gdk/maps/eliteforce_virtualvoyager_maps/deck11.map
  room -4096..-2704 x -4032..-2784, height 404 -> 208 (x0.72)
  world brushes 1922, entities 368, waypoints 86, core dropped 28, platforms dropped 38, patches dropped 45, lights kept 104, pods 8
  wrote build/ship/generated/deck14.map
```

The dress report: 160 entities kept (56 dropped — deck 11's scripts, NPC spawns, turbolift network,
pickup and editor tags), 1,956 brushes kept, 33 brushes dropped, 28 core brushes dropped, 38 raised
platform brushes dropped, 45 patches dropped, 104 lights kept (dimmed), 86 waypoints; the eight pods
at `(-3648 .. -3152, -2824)`, and the holodeck part at `(-3640, -3072)`, `HOLO_X = -3840`.

## A1 — the merged ship compiles and `check-bsp.py` reads the artifact

```
$ scripts/build-ship.sh
deck14 <- /home/c/big/git/long-way-home/build/gdk/maps/eliteforce_virtualvoyager_maps/deck11.map
  room -4096..-2704 x -4032..-2784, height 404 -> 208 (x0.72)
  world brushes 1922, entities 368, waypoints 86, core dropped 28, platforms dropped 38, patches dropped 45, lights kept 104, pods 8
  wrote .../build/ship/generated/deck14.map
.../build/ship/generated/deck14.map: 81 distinct materials
  present as shipped images:      2694
  declared by shipped shaders:   2070
  our own aliases:               27
  in the reuse source census:    77
PASS  every material already exists in the game; no new art
  15 decks -> /home/c/big/git/long-way-home/build/ship/voyager.map
  world brushes 38192, entities 5372, stray brushes dropped 11, folded into the world {'func_group': 1011, 'func_static': 98, 'func_wall': 3}
  turbolift links between decks: 98 rewritten, 129 added; ...
  deck 14: z -44059 .. -43660
  ...
/tmp/tmp.8HzKKIkC1I/maps/voyager.bsp         OK                                 (1/17 lumps empty)
wrote /home/c/big/git/long-way-home/build/ship/out/longway_voyager.pk3
```

`OK` is `tools/mapgen/check-bsp.py`'s verdict on the compiled BSP, run inside `build-map.sh`:
shaders, brushes and surfaces are not empty. The exit status of q3map2 is not the evidence; this
line is. The full build runs `-vis` and `-light`, so the ship is sealed (a leaked map writes no
portal file and `-vis` fails).

## A2 — the deck loads, is reached on foot, and navigation bakes on first load

```
$ scripts/deck14-check.sh
==> the deck 14 re-dress
    deck 14 room: standing at (-2800 -3840 -43797)
    deck 14 room: at the holodeck: standing at (-3840 -3072 -43884)
PASS  the deck 14 re-dress loads, is reachable on foot, and is photographed for the walkthrough
      screenshots: .../screenshots/lwh_deck14.tga, .../screenshots/lwh_deck14_holodeck.tga
```

The load lines, from that run's log (`build/g3-home/deck14.out`):

```
EFSP: SP_SpawnServer: CM_LoadMap(maps/voyager.bsp)
EFSP: Munro connected
SHIP: deck 14 room: standing at (-2800 -3840 -43797)
SHIP: deck 14 room: at the holodeck: standing at (-3840 -3072 -43884)
```

`voyager.nav` was deleted before the `s3-check.sh` run (A3) and the engine wrote it again
(`build/g3-home/baseEF/maps/voyager.nav`, 2,192,092 bytes), so navigation is baked from the room's
waypoints, not shipped by hand. The check also fails on any "in solid" waypoint; the room's waypoints
are placed on the local floor — including in front of the pod row and the holodeck post — and
filtered against the final solids.

## A3 — reachable from the turbolift, proven

```
$ scripts/s3-check.sh
    deck 12: at (-2800 -3840 -37676)  standing, clear, on deck 12  ok
    deck 13: at (-2800 -3840 -40746)  standing, clear, on deck 13  ok
    deck 14: at (-2800 -3840 -43827)  standing, clear, on deck 14  ok
    deck 15: at (-3840 -3522 -46959)  standing, clear, on deck 15  ok
PASS  one map, fifteen decks, each reached by turbolift from the one before with no level load
    turbolift deck 14: use tour_turbo_14
PASS  the retail turbolift menu reads our fifteen-deck list, and opens on the merged ship
INFO  game frame 2.1 ms average, 5.4 ms worst over 381 frames; 530 scripted entities live
PASS  the game's average frame fits a 60 fps budget (the worst frame is reported, not judged)
```

The turbolift reaches deck 14 and the player stands clear on it. The tube is the second route (its
`target_level_change` to `tour/deck13` is a real in-map exit when the lifts are down).

## A4 — the deck's hook, and its markers in the merged map

The system node `SYS_HOLODECKS` is the ship model's own (`module/ship/ship_core.*`); Holodeck 1 is
this deck's placement. The deck's markers survive into the merged map, exactly once each:

```
$ for n in lwh_lift_d14 lwh_tube_d14 lwh_d14_door lwh_station_17 lwh_stasis_post stasiswatch \
      lwh_light_normal_14_0 lwh_light_emergency_14_1; do printf '%-28s %s\n' "$n" "$(grep -c "\"targetname\" \"$n\"" build/ship/voyager.map)"; done
lwh_lift_d14                 1
lwh_tube_d14                 1
lwh_d14_door                 1
lwh_station_17               1
lwh_stasis_post              1
stasiswatch                  1
lwh_light_normal_14_0        1
lwh_light_emergency_14_1     1

$ grep -c '"model" "models/mapobjects/stasis/pod.md3"' build/ship/voyager.map
8
```

The `lwh_station_17` count of 1 is the point: the name is defined on **one** deck (this one), so the
stitcher did not rename it and `StationFor` (g_crew.cpp) can find it. See judgement call 6.

## A5 — the emergency lighting state is observable in the world, on all three decks

```
$ scripts/emergency-check.sh
==> emergency lighting, g_env 1
SHIP: emergency test 14: standing at (-3840 -3072 -43884)
SHIP: emergency test 14: alert 0, life support 100%
ENV: emergency lighting on on deck 14 (life support 100%, 12 red and 12 working strips)
ENV: emergency lighting off on deck 14 (life support 100%, 12 red and 12 working strips)

==> emergency lighting, g_env 0 (the extension off)
PASS  the emergency lighting state comes on at battle stations and stands down on all three decks; the gate holds
      screenshots: .../screenshots/lwh_emergency_14.tga, lwh_emergency_14_off.tga
```

The strips are authored `func_usable` brushes running along the room's ceiling — red
(`hall/hall_light_red`) held off, working (`engineering/elight1`) on. Deck 14's carry a deck tag
(`lwh_light_emergency_14_1`, `lwh_light_normal_14_0`, ...): the module
(`module/crew/g_crew.cpp`, `SyncEmergencyLight`) toggles by prefix, and three decks defining one name
would make the stitcher rename all of them, leaving the module nothing to toggle. The count is now
the three decks' twelve red and twelve working strips; the state is ship-wide on red alert or when
life support falls below 60%. The screenshots (`lwh_emergency_14.tga` / `lwh_emergency_14_off.tga`)
are taken at the deck-14 holodeck post, looking up the room. The red is not prominent from that
angle; deck 13's pair is the same (its screenshots show it barely). The state is proven by the
module's own line, not by the photograph.

## A6 — the parts census: no new art

```
$ python3 tools/shipmap/parts_census.py --map build/ship/generated/deck14.map --game build/baseEF \
      --census build/ship/deck14-census.json
build/ship/generated/deck14.map: 81 distinct materials
  present as shipped images:      2694
  declared by shipped shaders:   2070
  our own aliases:               27
  in the reuse source census:    77
PASS  every material already exists in the game; no new art
```

The full list, with what each material is and where it comes from, is
`docs/locations/deck14-parts-list.md`. The build runs this check, so a deck 14 that named art the
game does not have would fail the build. The two additions to the source are the pod model
`models/mapobjects/stasis/pod.md3` (in the shipped paks) and the holodeck materials
(`engineering/holodecklights`, `engineering/elight1`), both shipped.

## A7 — the gates

```
$ scripts/test.sh
all checks passed            # exit 0

$ scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
    2024 files: 2016 compiled and read back, 8 rejected by the compiler, 0 read-back failure(s)
    the 8 rejections are exactly the known, documented set   # exit 0
```

## A8 — decks 12 and 13 still work

```
$ scripts/deck12-check.sh
==> the deck 12 re-dress
    deck 12 room: standing at (-2800 -3840 -37646)
PASS  the deck 12 re-dress loads, is reachable on foot, and is photographed for the walkthrough

$ scripts/deck13-check.sh
==> the deck 13 re-dress
    deck 13 room: standing at (-2800 -3840 -40716)
    deck 13 room: at the catwalk: standing at (-3776 -2880 -40716)
PASS  the deck 13 re-dress loads, is reachable on foot, and is photographed for the walkthrough
```

`tools/shipmap/dressdeck12.py` and `dressdeck13.py` are unchanged; the build runs all three
re-dresses in order.

## Judgement calls, named as calls

1. **The room is kept one chamber, and the raised core deck is cleared (38 brushes).** The brief's
   section 5 asks for "one chamber, a row of stasis pods along a wall" and "verticality: flat". Deck
   11's main engineering is a two-level room whose raised core platform fills the middle; decks 12
   and 13 kept it and built their plant on it, and their evidence records the "unlit masses". To
   honour "flat", the large raised slabs are dropped (bottom above the floor, top below the ceiling,
   footprint >= 50,000), keeping the base floor, the walls and the smaller furniture. Invented.
2. **The vertical scale is 404 → 208 (x0.72).** The brief says "flat" without a number; deck 12 chose
   192 for a low control room and deck 13 chose 224 for a hall. A stasis ward is a low, clinical room;
   208 gives headroom over the 96-unit pod banks. Invented, in the ledger. It is this deck's
   walkthrough risk (below).
3. **The lighting is the change the BRIEF's notes asked about.** Deck 11's 104 ceiling lights are
   kept (dimmed to 70%) because dropping them left the copied structure black (tried and measured:
   the room was darker than deck 13's). On top of them, the room is lit by function: cold over the
   pods, warm at the holodeck, neutral between. Authored; the *read* is the walkthrough's.
4. **The pods are the game's own model, each seated on its local floor.** `misc_model_breakable` with
   `models/mapobjects/stasis/pod.md3` and the shipped stasis maps' own `mins`/`maxs`; a small
   pedestal at each pod's `floor_at` so a pod in the room's rising floor is neither buried nor
   floating (deck 12's plant vessels are placed the same way). Invented placements.
5. **The holodeck is a closed frame, not a programme.** The brief's non-goal forbids scripting the
   holodeck, so the arch is a doorframe on an intact wall (the north wall, at the far end of the pod
   row), with the control panel and the glowing door representing it. It is deliberately not an
   opening: a door that led nowhere would be a dead end.
6. **The `SYS_HOLODECKS` station marker is on this deck; deck 6's placeholder no longer defines it.**
   The decided placement is Holodeck 1 = deck 14, and `ship_core`'s own station name is "Holodeck 1",
   so `lwh_station_17` belongs here. The deck-6 generated blockout used to define `lwh_station_17`
   (`tools/shipmap/gendeck.py`, `STATION_SYSTEM`); with both decks defining it the stitcher would
   rename both and `StationFor` would find neither — the same trap as the emergency-light names. The
   placeholder entry is removed (deck 6 still gets no station marker until its own re-dress, which is
   the later stage). Recorded in the ledger. **Not changed:** `ship_core`'s `SystemSpecs` deck for
   `SYS_HOLODECKS` and `HOLODECK_DECK` still say deck 6, which is where crew take recreation in the
   model; moving that is a behaviour change the brief did not ask for, and it is flagged for the
   owner in the report.
7. **The tube routes to deck 13** (the life-support plant above), the route up when the turbolifts are
   down. Canon makes Jefferies tubes the emergency movement route; the destination is a call (decks
   12 and 13 route to deck 11).
8. **The emergency threshold** stays red alert or life support below 60%, the decks-12/13 pair's
   established value. Invented.

## What could not be verified here

- **The owner's walkthrough**, and the thing this deck is really about: does the walk from the cold
  pods to the warm holodeck *read* as the contrast the brief intends? The screenshots
  (`lwh_deck14.tga`, `lwh_deck14_holodeck.tga`) show the room after the platforms were cleared and
  the function lighting added; the **cold and warm zones are present** (the pod end is lit cold, the
  holodeck end warm), but the copied engineering interior still dominates, and the central dark
  masses remain. Whether the contrast is strong enough, and whether 208 reads as a ward rather than a
  squashed hall, is the thing a person has to look at. This is the biggest risk and the next pass.
- **Crew post coverage on deck 14.** The `lwh_stasis_post` marker and the `stasiswatch` navgoal are
  present and the crew layer can use them, but the G3 measurement harness runs on the deck 4
  scenario, and no deck-14 crew scenario exists, so coverage was not measured. Authored, not
  exercised.
- **The control panel as an operating console.** Its `lwh/panel` surface renders the ship's live
  state; no session stood at it and operated a system through it. Authored, not exercised.
- **The tube as an actual walk.** The traversal entity is in the map and the stitch counts it; no
  session walked the tube and rode it. It is a `target_level_change`, the same mechanism the
  turbolift uses, proven above.
- **The pod row as a walk, and the holodeck frame as a door.** The check stands at the arrival and at
  the holodeck post; that the walk past the pods reads as a ward is the walkthrough.
- **The red emergency state's visibility from the camera.** The module toggles it (12/12 strips,
  proven in A5), the screenshots are taken, but the red does not stand out from the deck-14 angle —
  deck 13's do not either. The state is proven by the module, not the photograph.

## Run notes (environment)

- The module is rebuilt with `cmake --build ../upstream/efgame/build-linux`, which compiles `module/`
  into `libefgame.so`; the engine binary is unchanged.
- `scripts/emergency-check.sh` now runs six headless sessions (decks 12, 13 and 14, environment on
  and off). The decks-12/13 evidence's "8 red and 8 working strips" is now the three decks' 12 and
  12; see the note added to `docs/evidence/deck13-redress.md`.
