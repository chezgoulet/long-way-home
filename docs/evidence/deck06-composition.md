# Evidence — deck 6, the composition (Holodeck 2, the armory and the crew quarters)

Date: 2026-10-07. Branch: `feat/deck-six-composition`, cut from `feature/g3-reactive-crew`. Brief:
`docs/locations/deck06-holodecks.brief.md`. Procedure: `docs/authoring-a-location.md`. What was copied
and every judgement call: the commit message, this document, and the ledger entry in
`docs/lore-ledger.md`.

It is deck 7's pattern, reused, not a sixth one: `tools/shipmap/dressdeck06.py` composes three source
maps into one deck map and is called by `scripts/build-ship.sh`; `scripts/build-map.sh --q3game` and
the per-material detail mechanism are deck 7's, unchanged. Decks 12, 13 and 14's tools are untouched
except for the one marker move recorded below, and their checks still pass (A7).

Nothing here is asserted without the command that produced it. The command is given before its
output.

## What was copied, and what was changed

The brief (section 3) is not a single copy target: deck 6 carries **three functions on one deck**, so
its parts come from **three maps**, and the commit message names which zone came from which. It is the
fifth and last re-dress, and the only one that **composes** rather than crops:

- **the quarters frontage — `tour/deck09`** (the crew-quarters deck): a **1,088 x 336 slice of its
  hall** (x -3788..-2700, y -3300..-2964) is set along the deck's north side at true scale — the
  hall-and-alcoves read the spatial program asks for, so the corridor stays the *route* and not the
  room. 283 brushes survive the slice;
- **the armory — `_brig`**: the only security interior in the game, and the map that carries the brig.
  A **512 x 336 slice around the brig's own player start** (x -320..192, y -460..-124) is set into a
  south-west alcove, so the armory borrows its lockers, grating and lighting. 185 brushes;
- **the holodeck doorframes — `Tour/_holodeck_firingrange`** (a `_holodeck_*` programme map): the
  programme's **arch cluster** (x 32..152, y -96..112) is turned 90 degrees and set **twice** on the
  south-east wall — the arch read and the warm light, not a working programme, which is the brief's
  non-goal. 23 brushes each.

**Copy nothing exactly: three sources, composited.** The failure to beat, in deck 14's own words, is
that a copied interior can dominate the room it was copied into; this deck has three inside one
corridor. The lighting brief is the instrument — warm where people live (the quarters frontage and the
holodeck arch), working neutral light down the corridor, cool working light at the armory — and the
corridor is authored as the route: the turbolift at the east wall, a Jefferies tube at the west, the
three zones opening onto it.

**The copied interiors are detail geometry** (deck 7's mechanism): the three zones are emitted through
generated per-material shader aliases (`lwh/detail06/...`, 35, the same images plus `surfaceparm
detail`), so they do not split the BSP tree and the fifth re-dress does not pass q3map2's vis-cluster
ceiling. The shell, the door and the tube stay structural and seal the deck.

The deck also carries the **holodeck station** (`lwh_station_17`, `SYS_HOLODECKS`) at the arch's
control panel, its **holodeck maintenance post** (`lwh_holo_post` / `holowatch`) before the arch, its
**security post** (`lwh_armory_post` / `armorywatch`) in the armory, the emergency lighting strips
tagged `_06`, and the deck wiring (the lift edge, the status panel, the navigation furniture).

```
$ python3 tools/shipmap/dressdeck06.py \
      --quarters build/gdk/maps/eliteforce_virtualvoyager_maps/deck09.map \
      --armory build/gdk/maps/brig-map/_brig.map \
      --holodeck build/gdk/maps/eliteforce_holodeck_maps/Tour/_holodeck_firingrange.map \
      --out build/ship/generated --report build/ship/deck06-dress.json \
      --census build/ship/deck06-census.json --detail-shader build/ship/files/scripts/lwh_detail06.shader
deck06 <- .../deck09.map + .../_brig.map + .../Tour/_holodeck_firingrange.map
  envelope -4608..-3072 x -4096..-3072, height 224
  zone quarters  <- deck09.map               slice [-3788, -2700, -3300, -2964] -> (-4592 -3424) rot 0, 283 brushes (5416 dropped)
  zone armory    <- _brig.map                slice [-320, 192, -460, -124] -> (-4592 -4080) rot 0, 185 brushes (336 dropped)
  zone holodeck  <- _holodeck_firingrange.map slice [32, 152, -96, 112] -> (-3900 -4060) rot 90, 23 brushes (190 dropped)
  zone holodeck2 <- _holodeck_firingrange.map slice [32, 152, -96, 112] -> (-3500 -4060) rot 90, 23 brushes (190 dropped)
  world brushes 418, entities 127, waypoints 74, lights kept 15, detail materials 35
  wrote build/ship/generated/deck06.map
```

## A1 — the merged ship compiles and `check-bsp.py` reads the artifact

```
$ scripts/build-ship.sh
deck06 <- .../deck09.map + .../_brig.map + .../Tour/_holodeck_firingrange.map
  envelope -4608..-3072 x -4096..-3072, height 224
  ...
  world brushes 418, entities 127, waypoints 74, lights kept 15, detail materials 35
  wrote .../build/ship/generated/deck06.map
.../build/ship/generated/deck06.map: 37 distinct materials
  present as shipped images:      2694
  declared by shipped shaders:   2070
  our own aliases:               63
  in the reuse source census:    116
PASS  every material already exists in the game; no new art
  15 decks -> .../build/ship/voyager.map
  world brushes 40439, entities 5696, stray brushes dropped 11, degenerate brushes dropped 74, folded into the world {'func_group': 1011, 'func_static': 98, 'func_wall': 3}
  brush models 541 (engine limit 256): {'func_usable': 249, 'func_door': 213, ...}
  turbolift links between decks: 100 rewritten, 129 added; ...
  deck  6: z -19372 .. -19116
  ...
/tmp/tmp.RN1Hb9RaIv/maps/voyager.bsp         OK                                 (1/17 lumps empty)
wrote /home/c/big/git/long-way-home/build/ship/out/longway_voyager.pk3
```

`OK` is `tools/mapgen/check-bsp.py`'s verdict on the compiled BSP, run inside `build-map.sh`:
shaders, brushes and surfaces are not empty. The exit status of q3map2 is not the evidence; this line
is. The full build runs `-vis` and `-light`, so the ship is sealed. The **74 degenerate brushes** the
stitcher dropped are the clamped slivers the composition crop produces (deck 7's judgement call 1, the
same shared merge step; it was 21 before deck 6). Read from the packed artifact, the ship is **15,321
portal clusters** of `MAX_MAP_VISCLUSTERS` 16,384 — the fifth re-dress's detail geometry did not pass
the ceiling:

```
$ python3 - <<'PY'   # lump 16 = LUMP_VISIBILITY; its header is numclusters, clusterbytes
import zipfile, struct
d = zipfile.ZipFile("build/ship/out/longway_voyager.pk3").read("maps/voyager.bsp")
o, l = struct.unpack_from("<ii", d, 8 + 16*8); ncl, cb = struct.unpack_from("<ii", d, o)
print("portalclusters", ncl, "clusterbytes", cb)
PY
portalclusters 15321 clusterbytes 1920
```

## A2 — the deck loads, is reached on foot, and navigation bakes on first load

```
$ scripts/deck06-check.sh
==> the deck 6 composition
    deck 6 room: standing at (-3168 -3904 -19308)
    deck 6 room: at the holodeck: standing at (-3796 -3968 -19308)
    deck 6 room: at the armory: standing at (-4336 -3904 -19308)
PASS  the deck 6 composition loads, is reachable on foot, and is photographed for the walkthrough
      screenshots: .../screenshots/lwh_deck06.tga, lwh_deck06_holodeck.tga, lwh_deck06_armory.tga
```

The load lines, from that run's log (`build/g3-home/deck06.out`):

```
EFSP: SP_SpawnServer: CM_LoadMap(maps/voyager.bsp)
EFSP: Munro connected
SHIP: deck 6 room: standing at (-3168 -3904 -19308)
SHIP: deck 6 room: at the holodeck: standing at (-3796 -3968 -19308)
SHIP: deck 6 room: at the armory: standing at (-4336 -3904 -19308)
```

`voyager.nav` was deleted by the `s3-check.sh` run (A3) and the engine wrote it again
(`build/g3-home/baseEF/maps/voyager.nav`, 2,245,588 bytes), so navigation is baked from the deck's
waypoints, not shipped by hand. The check also fails on any "in solid" waypoint: the waypoint grid is
filtered with the engine's own standing box (±15 x/y, -24..+32 z), not a bare point (deck 7's
judgement call 5), because the three clamped interiors leave pockets the grid must skip.

## A3 — reachable from the turbolift, proven

```
$ scripts/s3-check.sh
    deck  6: at (-3168 -3904 -19331)  standing, clear, on deck 6  ok
    ...
PASS  one map, fifteen decks, each reached by turbolift from the one before with no level load
    turbolift deck 6: use tour_turbo_06
PASS  the retail turbolift menu reads our fifteen-deck list, and opens on the merged ship
INFO  game frame 2.2 ms average, 15.3 ms worst over 381 frames; 534 scripted entities live
PASS  the game's average frame fits a 60 fps budget (the worst frame is reported, not judged)
```

The turbolift reaches deck 6 and the player stands clear. The tube is the second route (its
`target_level_change` to `tour/deck05`, sickbay, is a real in-map exit when the lifts are down).

## A4 — the deck's markers in the merged map, exactly once each

Read from the artifact, not the source (`build/ship/voyager.map`):

```
$ for n in lwh_lift_d06 lwh_tube_d06 lwh_d06_door lwh_station_17 lwh_holo_post holowatch \
      lwh_armory_post armorywatch lwh_light_normal_06_0 lwh_light_emergency_06_1 d06_arrival; do
      printf '%-28s %s\n' "$n" "$(grep -c "\"targetname\" \"$n\"" build/ship/voyager.map)"; done
lwh_lift_d06                 1
lwh_tube_d06                 1
lwh_d06_door                 1
lwh_station_17               1
lwh_holo_post                1
holowatch                    1
lwh_armory_post              1
armorywatch                  1
lwh_light_normal_06_0        1
lwh_light_emergency_06_1     1
d06_arrival                  1
```

The `lwh_station_17` count of 1 is the point of judgement call 4 below, and the emergency-light names
carry the `_06` deck tag for the reason the other decks' do: one name on two decks would make the
stitcher rename both, leaving the module nothing to find.

## A5 — the parts census: no new art

```
$ python3 tools/shipmap/parts_census.py --map build/ship/generated/deck06.map --game build/baseEF \
      --census build/ship/deck06-census.json --shader build/ship/files/scripts/lwh_detail06.shader
build/ship/generated/deck06.map: 37 distinct materials
  present as shipped images:      2694
  declared by shipped shaders:    2070
  our own aliases:                63
  in the reuse source census:     116
PASS  every material already exists in the game; no new art
```

The full list, with what each material is and where it comes from, is
`docs/locations/deck06-parts-list.md`. The build runs this check, so a deck 6 that named art the game
does not have would fail the build.

## A6 — the gates

```
$ scripts/test.sh
all checks passed            # exit 0

$ scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
    2024 files: 2016 compiled and read back, 8 rejected by the compiler, 0 read-back failure(s)
    the 8 rejections are exactly the known, documented set   # exit 0
```

## A7 — decks 12, 13 and 14 still work

```
$ scripts/deck12-check.sh
    deck 12 room: standing at (-2800 -3840 -37646)
PASS  the deck 12 re-dress loads, is reachable on foot, and is photographed for the walkthrough

$ scripts/deck13-check.sh
    deck 13 room: standing at (-2800 -3840 -40716)
    deck 13 room: at the catwalk: standing at (-3776 -2880 -40716)
PASS  the deck 13 re-dress loads, is reachable on foot, and is photographed for the walkthrough

$ scripts/deck14-check.sh
    deck 14 room: standing at (-2800 -3840 -43797)
    deck 14 room: at the holodeck: standing at (-3840 -3072 -43884)
PASS  the deck 14 re-dress loads, is reachable on foot, and is photographed for the walkthrough
```

`tools/shipmap/dressdeck12.py` and `dressdeck13.py` are untouched; `dressdeck14.py` changed in one
place only (judgement call 4). The build runs deck 6, then all four re-dresses in order.

## A8 — the entity count the composition adds

The brief asks this against the 81 the generated placeholder had, because deck 6 is the densest deck in
the ship. The generated `deck06.map` carries **128** entities (worldspawn plus 127 entity blocks)
against the placeholder's **81** — **+47**, not the near-doubling a full composition of a quarters deck
would cost, because the zones are cropped to their slices, the sources' NPCs, scripts, triggers and
turbolift networks are dropped, and each zone's own lights are kept only where they fall inside it. In
the merged ship, deck 6 holds **141** entities counted by `"lwh_deck" "6"`: the stitcher adds the
arrival marker and the fourteen turbolift links.

## Judgement calls, named as calls

1. **The envelope is one 224-unit hall, authored, and the three sources are slices at true scale, not
   stretched rooms.** The brief says "a long corridor, wider at the holodeck end" without a number;
   deck 12 chose 192, deck 13 224, deck 14 208, deck 7 240. 224 keeps headroom over the 192-unit
   holodeck arch. The zones are squeezed only in z (to 200/192/208), never in x or y, so no copied
   interior is distorted. Invented, in the ledger.
2. **The three crop lines are measured from each source, not copied from one.** deck 12/13/14/7 all
   took the same line because they copy the same room; here the quarters slice is the hall beside the
   crew quarters (measured from the source's own player start and speaker line), the armory slice is
   the room around the brig's own player start, and the arch slice is the programme's own door cluster.
   Invented (the lines); the geometry is each source's.
3. **The copied interiors are detail geometry.** The ship is near `MAX_MAP_VISCLUSTERS` (16,384); a
   fifth full re-dress as structure would pass it. The three zones go through deck 7's generated
   per-material aliases (`lwh/detail06/...`, 35), with the shell, door and tube structural. Detail
   brushes do not split the BSP tree; the merged ship is 15,321 clusters. Invented mechanism (deck 7's),
   measured.
4. **The `SYS_HOLODECKS` station marker moved from deck 14 to deck 6.** `ship_core`'s `SystemSpec`
   names the system's station "Holodeck 2" on deck 6, where the recreational holodeck is, and this
   deck's brief (section 8) requires the station to be marked. Two decks cannot both define
   `lwh_station_17` (the stitcher would prefix both and `StationFor` would find neither), so
   `dressdeck14.py` no longer emits it, and `dressdeck06.py` does. Deck 14 keeps its frame, its
   control panel and the maintenance post `lwh_stasis_post`/`stasiswatch`, and `deck14-check.sh` still
   passes (A7); its evidence carries a dated note. This answers deck 14's own flagged judgement call 6,
   and is recorded in the ledger.
5. **The holodeck arch is the firing range's own arch, so its frame carries that programme's material.**
   The game's holodeck maps share a `cargo/panel1*` door; the firing range's is the most self-contained
   entrance (frame, lintel and the lit door leaf), and it is the map the brief names among the
   `_holodeck_*` sources. Its uprights are `borg/basemetal` because that programme is a Borg scenario,
   so the copied frame reads that way; a neutral `cargo/panel1` frame is a candidate for the next pass.
   It is a closed frame on the wall, not an opening, so it is not a dead end (the brief's non-goal
   forbids building a programme). Invented (the choice of programme map).
6. **The arch's unlit door leaf is dropped.** The programme ships two leaves in the same place
   (`paneloff` unlit, `panelon` lit); with no programme running both would render and z-fight, so the
   unlit one is dropped and the arch keeps the lit one. Invented.
7. **The tube routes to `tour/deck05`** (sickbay, the deck below), the fallback route when the
   turbolifts are down. Canon makes Jefferies tubes the emergency movement route; the destination is a
   call. Invented.
8. **The emergency threshold** stays red alert or life support below 60%, the other re-dresses'
   established value. The deck carries eight strips tagged `_06`; the module toggles them by prefix.
   Invented.

## What could not be verified here

- **The owner's walkthrough, and the thing this deck is really about.** The brief's closing question —
  whether the three zones read as three places, and whether the walk from the quarters to the holodeck
  with an armory off it feels like a deck rather than a corridor with three props in it — is the one
  check that decides whether the composition worked, and no transcript answers it. The screenshots
  (`lwh_deck06.tga`, `lwh_deck06_holodeck.tga`, `lwh_deck06_armory.tga`) show the corridor and the two
  ends. **This is the biggest risk.** The copied quarters hall and armory are real interiors, but a
  slice is not a room: the boundaries are the authored shell, and whether the cuts read as architecture
  or as a diorama is the walkthrough's judgement.
- **Crew post coverage on deck 6.** The `lwh_holo_post`/`holowatch` and `lwh_armory_post`/`armorywatch`
  markers are present and the crew layer can use them, but the G3 harness runs on the deck 4 scenario
  and no deck-6 scenario exists, so coverage was not measured. Authored, not exercised.
- **The holodeck control surface as an operating console.** Its `lwh/panel` surface renders the ship's
  live state; no session stood at it and operated a system through it. Authored, not exercised.
- **The tube as an actual walk.** The traversal entity is in the map and the stitch counts it; no
  session walked the tube and rode it. It is a `target_level_change`, the same mechanism the turbolift
  uses, proven above.
- **The red emergency state's visibility from the camera.** The module toggles the strips (the same
  mechanism deck 7/12/13/14 use); the screenshots are taken but the red is not prominent from the
  camera positions, as on the other decks. The state is proven by the module, not the photograph.
- **`scripts/emergency-check.sh` was not extended to a deck-6 session.** The strips exist and are
  tagged; the session that photographs them red is left to the next pass, because the acceptance for
  deck 6 is the walkthrough and the four decks' emergency checks are the regression set.

## Run notes (environment)

- The module is rebuilt with `cmake --build ../upstream/efgame/build-linux`, which compiles `module/`
  into `libefgame.so` (`g_shipTest 63` was added for this deck); the engine binary is unchanged.
- The deck-6 detail shader is generated to `build/ship/files/scripts/lwh_detail06.shader` (shipped in
  the pak) and mirrored to `build/lwhshaders/scripts/` for the compiler, sharing the deck-7 game dir
  (`build-map.sh --q3game lwhshaders`).
