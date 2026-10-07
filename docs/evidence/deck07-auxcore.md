# Evidence — deck 7, the auxiliary computer core, re-dressed from `tour/deck11`

Date: 2026-10-07. Branch: `feat/deck-seven-auxcore`, cut from `feature/g3-reactive-crew`. Brief:
`docs/locations/deck07-auxcore.brief.md`. Procedure: `docs/authoring-a-location.md`. What was copied
and every judgement call: the commit message, this document, and the ledger entry in
`docs/lore-ledger.md`.

It is deck 14's pattern, reused, not a fourth one: `tools/shipmap/dressdeck07.py` is the copy of
`dressdeck14.py` with deck 7's program, and `scripts/build-ship.sh` calls it in place of the deck-7
generated placeholder. Decks 12, 13 and 14's tools are untouched, and their output still builds and
still passes their own checks (A8).

Nothing here is asserted without the command that produced it. The command is given before its
output.

## What was copied, and what was changed

The brief names the reuse target (section 3): **copy `tour/deck11`**, main engineering, "for the
industrial read; the core column is the deck's spine", and change the warp core for an auxiliary core
column and add cargo islands. It copies the room's own brushes and entities — the consoles, railings,
wall panels, door hardware, props and "small untidiness" — and changes what makes this room *this*
room:

- **the vertical scale is reduced** (deck 11's 404-unit room compressed to 240 — taller than deck
  13's plant hall, 224, because this is the depth deck and the core column must read tall);
- **the raised core deck is cleared** (38 large raised slabs dropped), so the room is the brief's
  **one chamber**;
- **the warp core becomes an auxiliary core column**: the tall core-textured cluster is dropped (28
  brushes) and a column of the room's own core materials is raised at the centre, floor to ceiling,
  with the **core panel** at its base facing the door;
- **two cargo islands** are added to the sides (the brief's change): low plinths with the game's own
  crates (`models/mapobjects/cargo/crate.md3`, the brief's section 4) stacked on them, each seated on
  its local floor;
- **an overhead escape-pod hatch** on a short walkable platform with 16-unit steps (the brief's
  "escape-pod access overhead"); no pod model ships, so the hatch is authored geometry;
- **a Jefferies tube mouth** at the far end from the door (west), low and awkward, with a traversal to
  the main computer core (`tour/deck10`) — the fallback route;
- **the computer-core station marker** (`lwh_station_3`, `SYS_COMPUTER_CORE`) at the panel and the
  deck's **core-watch post** (`lwh_core_post` / `corewatch`);
- **the room is re-lit by function**: deck 11's own 104 ceiling lights kept but dimmed, plus authored
  working light over the cargo floor and a blue glow at the core column;
- **the emergency lighting state**: red strips (`hall/hall_light_red`) held off until the module
  switches them on, and the normal strips (`engineering/elight1`) they replace, tagged
  `lwh_light_..._07_N`.

Two changes are **ship-wide, not deck 7 alone**, and both are needed for the fourth re-dress to
build (judgement calls 1 and 2): the stitcher drops degenerate brushes, and deck 7's copied interior
is **detail** geometry.

```
$ python3 tools/shipmap/dressdeck07.py --deck11 build/gdk/maps/eliteforce_virtualvoyager_maps/deck11.map \
      --out build/ship/generated --report build/ship/deck07-dress.json --census build/ship/deck07-census.json \
      --detail-shader build/ship/files/scripts/lwh_detail07.shader
deck07 <- build/gdk/maps/eliteforce_virtualvoyager_maps/deck11.map
  room -4096..-2704 x -4032..-2784, height 404 -> 240 (x0.8)
  world brushes 1927, entities 342, waypoints 67, core dropped 28, platforms dropped 38, patches dropped 45, lights kept 104 (3430 -> 4862 light), crates 8, detail materials 94
  wrote build/ship/generated/deck07.map
```

## A1 — the merged ship compiles, and `check-bsp.py` reads the artifact

```
$ scripts/build-ship.sh
deck07 <- /home/c/big/git/long-way-home/build/gdk/maps/eliteforce_virtualvoyager_maps/deck11.map
  room -4096..-2704 x -4032..-2784, height 404 -> 240 (x0.8)
  world brushes 1927, entities 342, waypoints 67, core dropped 28, platforms dropped 38, patches dropped 45, lights kept 104 (3430 -> 4862 light), crates 8, detail materials 94
  wrote .../build/ship/generated/deck07.map
.../build/ship/generated/deck07.map: 86 distinct materials
  present as shipped images:      2694
  declared by shipped shaders:   2070
  our own aliases:               122
  in the reuse source census:    77
PASS  every material already exists in the game; no new art
  15 decks -> /home/c/big/git/long-way-home/build/ship/voyager.map
  world brushes 40088, entities 5650, stray brushes dropped 11, degenerate brushes dropped 21, folded into the world {'func_group': 1011, 'func_static': 98, 'func_wall': 3}
  ...
==> q3map2 -meta
==> q3map2 -vis
==> q3map2 -light
/tmp/tmp.cusktysA6t/maps/voyager.bsp         OK                                 (1/17 lumps empty)
wrote /home/c/big/git/long-way-home/build/ship/out/longway_voyager.pk3
```

`OK` is `tools/mapgen/check-bsp.py`'s verdict on the compiled BSP, run inside `build-map.sh`:
shaders, brushes and surfaces are not empty. The exit status of q3map2 is not the evidence; this
line is. The full build runs `-vis` and `-light`, so the ship is sealed.

The four re-dresses now fill the ship to q3map2's vis-cluster ceiling. Read from the packed artifact
(lump 16, the visibility lump), not from the tool's claim:

```
$ unzip -j build/ship/out/longway_voyager.pk3 maps/voyager.bsp -d /tmp/opencode/pk3x
$ python3 - <<'PY'   # lump 16 = LUMP_VISIBILITY; its header is numclusters, clusterbytes
...
PY
/tmp/opencode/pk3x/maps/voyager.bsp visdata bytes 29481608 portalclusters 15355 clusterbytes 1920
```

`MAX_MAP_VISCLUSTERS` is 16384; the ship is at 15355 with deck 7's interior as detail. Before the
detail change it was 16737 and `q3map2 -vis` refused the map (`MAX_MAP_VISIBILITY exceeded`), which
is why the detail aliases exist (judgement call 2).

## A2 — the deck loads, is reached on foot, and navigation bakes on first load

```
$ scripts/deck07-check.sh
==> the deck 7 re-dress
    deck 7 room: standing at (-2800 -3840 -22284)
    deck 7 room: at the core: standing at (-3200 -3296 -22246)
PASS  the deck 7 re-dress loads, is reachable on foot, and is photographed for the walkthrough
      screenshots: .../screenshots/lwh_deck07.tga, .../screenshots/lwh_deck07_core.tga
```

The load lines, from that run's log (`build/g3-home/deck07.out`):

```
EFSP: SP_SpawnServer: CM_LoadMap(maps/voyager.bsp)
EFSP: Munro connected
SHIP: deck 7 room: standing at (-2800 -3840 -22284)
SHIP: deck 7 room: at the core: standing at (-3200 -3296 -22246)
```

`voyager.nav` was deleted by `scripts/s3-check.sh` and the engine wrote it again
(`build/g3-home/baseEF/maps/voyager.nav`, 2,245,588 bytes), so navigation is baked from the room's
waypoints, not shipped by hand. The check also fails on any "in solid" waypoint: the waypoint grid is
filtered with the engine's own standing box (roughly ±15 in x/y), not a bare point, because the core
panel is a large low brush near the grid (judgement call 5).

## A3 — reachable from the turbolift, proven

```
$ scripts/s3-check.sh
    deck  7: at (-2800 -3840 -22314)  standing, clear, on deck 7  ok
    ...
    turbolift deck 7: use tour_turbo_07
    turbolift deck list ext_data/sp_turbolift.dat: 15 decks, highest 15
PASS  one map, fifteen decks, each reached by turbolift from the one before with no level load
PASS  the retail turbolift menu reads our fifteen-deck list, and opens on the merged ship
INFO  game frame 2.1 ms average, 8.9 ms worst over 381 frames; 534 scripted entities live
PASS  the game's average frame fits a 60 fps budget (the worst frame is reported, not judged)
```

The turbolift reaches deck 7 and the player stands clear. The tube is the second route (its
`target_level_change` to `tour/deck10` is a real in-map exit when the lifts are down).

## A4 — the deck's hook, and its markers in the merged map

The system node `SYS_COMPUTER_CORE` is the ship model's own (`module/ship/ship_core.*`); this is its
auxiliary depth, the fallback for the main core on deck 9/10. The deck's markers survive into the
merged map, exactly once each — read from the artifact, not the source:

```
$ for n in lwh_lift_d07 lwh_tube_d07 lwh_d07_door lwh_station_3 lwh_core_post corewatch \
      lwh_pod_hatch lwh_light_normal_07_0 lwh_light_emergency_07_1 d07_arrival; do
      printf '%-28s %s\n' "$n" "$(grep -c "\"targetname\" \"$n\"" build/ship/voyager.map)"; done
lwh_lift_d07                 1
lwh_tube_d07                 1
lwh_d07_door                 1
lwh_station_3                1
lwh_core_post                1
corewatch                    1
lwh_pod_hatch                1
lwh_light_normal_07_0        1
lwh_light_emergency_07_1     1
d07_arrival                  1
```

The `lwh_station_3` count of 1 is the point: the name is defined on **one** deck (this one), so the
stitcher did not rename it and `StationFor` (g_crew.cpp) can find it. The emergency-light names carry
the `_07` deck tag for the same reason.

## A5 — the emergency lighting state is observable in the world, on all four decks

```
$ scripts/emergency-check.sh
...
SHIP: emergency test 7: standing at (-3200 -3296 -22246)
SHIP: emergency test 7: alert 0, life support 100%
ENV: emergency lighting on on deck 7 (life support 100%, 16 red and 16 working strips)
ENV: emergency lighting off on deck 7 (life support 100%, 16 red and 16 working strips)
...
PASS  the emergency lighting state comes on at battle stations and stands down on all four decks; the gate holds
      screenshots: .../screenshots/lwh_emergency_07.tga, lwh_emergency_07_off.tga
```

The strips are authored `func_usable` brushes along the room's ceiling — red (`hall/hall_light_red`)
held off, working (`engineering/elight1`) on — tagged `lwh_light_..._07_N` so they do not collide
with the other decks' names. The count is now the four decks' 16 red and 16 working strips; the state
is ship-wide on red alert or when life support falls below 60%. The deck-7 test (`g_shipTest 62`) is
the fourth session in `scripts/emergency-check.sh`.

## A6 — the parts census: no new art

```
$ python3 tools/shipmap/parts_census.py --map build/ship/generated/deck07.map --game build/baseEF \
      --census build/ship/deck07-census.json --shader build/ship/files/scripts/lwh_detail07.shader
build/ship/generated/deck07.map: 86 distinct materials
  present as shipped images:      2694
  declared by shipped shaders:   2070
  our own aliases:               122
  in the reuse source census:    77
PASS  every material already exists in the game; no new art
```

The full list, with what each material is and where it comes from, is
`docs/locations/deck07-parts-list.md`. The build runs this check, so a deck 7 that named art the game
does not have would fail the build. The additions to the source are the crate model
(`models/mapobjects/cargo/crate.md3`) and the generated detail aliases (`lwh/detail07/...`), both
shipped or derived from shipped images.

## A7 — the gates

```
$ scripts/test.sh
all checks passed            # exit 0

$ scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
    2024 files: 2016 compiled and read back, 8 rejected by the compiler, 0 read-back failure(s)
    the 8 rejections are exactly the known, documented set   # exit 0
```

## A8 — decks 12, 13 and 14 still work

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

$ scripts/deck14-check.sh
==> the deck 14 re-dress
    deck 14 room: standing at (-2800 -3840 -43797)
    deck 14 room: at the holodeck: standing at (-3840 -3072 -43884)
PASS  the deck 14 re-dress loads, is reachable on foot, and is photographed for the walkthrough
```

`tools/shipmap/dressdeck12.py`, `dressdeck13.py` and `dressdeck14.py` are unchanged; the build runs
all four re-dresses in order.

## A9 — Task B: the recreational-holodeck default renamed to say what it is

`module/ship/ship_core.cpp`'s `HOLODECK_DECK` is now `HOLODECK_RECREATION_DECK`, with the comment
that Holodeck 1 is on deck 14 and Holodeck 2 on deck 6, that `SYS_HOLODECKS` is one system, and that
this is the deck the routine sends people to for recreation (deck 6, where the quarters are). **No
behaviour changed**: the value is still 6 and it is used in the same three places. The module
compiled clean and the ship simulation's unit tests pass:

```
$ cmake --build ../upstream/efgame/build-linux
[100%] Built target efgame

$ scripts/test.sh
ship_core: all checks passed
...
all checks passed            # exit 0
```

## Judgement calls, named as calls

1. **The stitcher drops degenerate brushes (21 on the merged ship).** The re-dress crop clamps a
   corridor brush's points to the room faces, so some brushes collapse to a face with two identical
   points — a zero normal. q3map2's `CreateNewFloatPlane` returns -1 for those and its bevel pass
   then indexes `mapplanes[-1]`, which segfaults on a map this size (found building the full ship:
   `FloatPlane: bad normal` then SIGSEGV). The fix is in the shared merge step
   (`tools/shipmap/stitch.py`), so every deck is treated the same and no deck tool emits junk; the
   count is reported in the stitch line. The affected brushes are slivers, and the room's shell seals
   it regardless.
2. **Deck 7's copied interior is detail geometry.** `MAX_MAP_VISCLUSTERS` is 16384; the ship was at
   15366 with three re-dresses, and a fourth full re-dress reached 16737 and failed `-vis`. Detail
   brushes do not split the BSP tree and do not add clusters, so the re-dress emits its copied
   furniture (and the core column, islands, hatch, panel) through generated per-material detail
   aliases (`lwh/detail07/...`, 94 of them, same images plus `surfaceparm detail`), keeping the
   shell, door and tube structural. With them the ship is 15355 clusters — under the ceiling, and
   fewer than the three-re-dress ship. This is the change the whole deck turns on, and it is
   ship-wide in effect (it is deck 7's geometry). The aliases are shipped in the pak for the engine
   and mirrored into a compiler-only game dir (`build/lwhshaders`, passed to q3map2 with build-map's
   new `--q3game`) because q3map2 must see the flag; the engine renders the same images. Invented
   mechanism, recorded in the ledger.
3. **The vertical scale is 404 → 240 (x0.8).** The brief says "the depth deck" with a tall core
   column and overhead access; deck 12 chose 192, deck 13 224, deck 14 208. 240 is taller than the
   plant hall so the column reads as the spine and the escape hatch has headroom. Invented, in the
   ledger.
4. **The core column is brushes, not a model.** No computer-core model ships (the game's core is
   sector geometry); the column is built from the room's own core materials, as deck 13's island is.
   Invented placement; the materials are the source's.
5. **The waypoint grid is filtered with the engine's standing box (±15 x/y, −24..+32 z), not a bare
   point.** `SP_waypoint` checks `G_CheckInSolid` on that box (g_nav.cpp); a point test let one
   waypoint sit in the core panel's corner and the engine refused the map ("Waypoint ... in solid!").
   The filter is measured from the engine's own code. Deck 12/13/14's tools use the point test and
   were not changed.
6. **The tube routes to `tour/deck10`** (the main computer core), the fallback this auxiliary core
   exists for. Canon makes Jefferies tubes the emergency movement route; the destination is a call.
7. **The escape hatch is authored geometry with no pod.** Canon shows escape pods, but the game ships
   no pod asset, so the access is a hatch and a marker (`lwh_pod_hatch`), not a pod. No new art.
8. **The emergency threshold** stays red alert or life support below 60%, the decks-12/13/14 pair's
   established value. Invented.

## Task B, judged as a call

The brief said: rename `HOLODECK_DECK` to say it is the recreational default, comment the two
holodecks, and **do not change the behaviour**; if the behaviour should change (crew walking to the
nearer holodeck, two real nodes), stop and say so instead. **I did not change the behaviour.** I think
the behaviour *should* eventually change — with Holodeck 1 now real on deck 14 and Holodeck 2 on
deck 6, a crew member sent to "the holodeck" walks to deck 6 regardless of where they are, and
`SYS_HOLODECKS`'s `SystemSpec` still names deck 6 — but that is the owner's design decision (two
nodes, a nearest rule, and what a holodeck failure on one means for the other), so it is flagged here
and in the ledger, not done.

## What could not be verified here

- **The owner's walkthrough.** The brief's closing question — does the depth deck read as the core
  the ship falls back to? — is not answered by a transcript. The screenshots
  (`lwh_deck07.tga`, `lwh_deck07_core.tga`) show the room. **This deck's specific aesthetic risk is
  the same class as deck 14's, one worse:** marking the copied interior, the core column and the
  islands all **detail** is invisible at runtime, but it also means the room's *structure* is now
  only the shell; if the copied engineering interior + a new column reads as a place rather than a
  hall with a cylinder in it, that is the walkthrough's judgement. The compression from 404 to 240 is
  the second risk.
- **Crew post coverage on deck 7.** The `lwh_core_post` marker and `corewatch` navgoal are present and
  the crew layer can use them, but the G3 harness runs on the deck 4 scenario and no deck-7 scenario
  exists, so coverage was not measured. Authored, not exercised.
- **The core panel as an operating console.** Its `lwh/panel` surface renders the ship's live state;
  no session stood at it and operated a system through it. Authored, not exercised.
- **The tube as an actual walk.** The traversal entity is in the map and the stitch counts it; no
  session walked the tube and rode it. It is a `target_level_change`, the same mechanism the
  turbolift uses, proven above.
- **The escape hatch as an actual escape.** The platform and hatch are geometry and a marker; no
  scenario opens it. Authored, not exercised.
- **The red emergency state's visibility from the camera.** The module toggles it (proven in A5); the
  screenshots are taken but the red is not prominent from the core post, as deck 13's and 14's are
  not. The state is proven by the module, not the photograph.

## Run notes (environment)

- The module is rebuilt with `cmake --build ../upstream/efgame/build-linux`, which compiles `module/`
  into `libefgame.so`; the engine binary is unchanged.
- `scripts/emergency-check.sh` now runs eight headless sessions (decks 12, 13, 14 and 7, environment
  on and off). The deck 14 evidence's "12 red and 12 working strips" is now the four decks' 16 and
  16.
- `scripts/build-map.sh` gained `--q3game NAME`, which adds a game dir for the compiler to resolve
  shaders from; `scripts/build-ship.sh` uses it for deck 7's generated detail shader. It is off
  unless asked for, and no other build calls it.
