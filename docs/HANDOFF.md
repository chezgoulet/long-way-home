# Hand-off — where the work stands and how to continue

Written 2026-10-06 at the owner's request, for whoever picks this up next (a new session has no
memory of this one). Read this, then `docs/ship-programme.md` (the plan and the owner's decisions),
then `docs/gates.md` (the ledger). Keep this file current: update it at the end of every session.

## In one paragraph

Every gate S1-S10 now has something real behind it, and none of S3-S10 is finished. S1-S2 are done;
S3 (the one-map ship) and S4-S5 have slices that run in the engine with check scripts; S6-S10 are
**rules in the ship core with unit tests, reachable only from the console** — nothing of them is
embodied, drawn or enforced in the game. The honest shape of what remains is therefore the same for
each: take the core's state and make it visible and operable (bodies, consoles, assets), in the
order the hand-off's per-gate notes give. The core (`module/ship/ship_core.*`, ~1,100 lines, 24
tests) is the part to trust; read `tests/ship/test_ship_core.cpp` to see what it guarantees.

## The standing instruction

The owner's goal for the session: *fully understand the goals of this repo and fully implement
them to professional standards; fix every bug encountered.* On 2026-10-05 he redirected the
programme to an operational, lore-accurate Voyager (gates S1–S10) and said: time is not a
constraint, "just gate milestones and keep going forward". He asked to be questioned first; that
round is done and its answers are in `docs/ship-programme.md`. **Do not re-ask those questions.**

## State of the repository

- Branch **`feature/g3-reactive-crew`**, branched from `testing`. **All work is local commits;
  nothing has been pushed and no PR is open.** Pushing needs the owner's go-ahead.
- The upstream checkout is `../upstream` (pinned commit, with `patches/0001`–`0011` applied as
  uncommitted changes — that is how `scripts/bootstrap-upstream.sh` leaves it). The engine build is
  `build-engine/` (plain `make -j12` there rebuilds it); the game modules build in
  `../upstream/efgame/build-linux`.
- **After adding a new `.cpp` under `module/`, re-run cmake configure**
  (`cmake -S ../upstream/efgame -B ../upstream/efgame/build-linux -DLWH_MODULE_DIR=$PWD/module`):
  the sources are globbed at configure time, and the module links with unresolved symbols ignored,
  so a forgotten file shows up only as "undefined symbol" when the engine loads the module.
- **Patches are generated, not hand-written.** Each is a diff of the upstream working tree against
  pre-edit copies. Those copies lived in a scratch directory that will not survive the session. To
  change a patch in a new session: clone upstream fresh, apply the patches *before* the one being
  changed, copy the files it touches aside, apply it, edit, and diff. Then verify the whole series
  on another fresh clone (apply all, build with and without `-DLWH_MODULE_DIR`).

## Commands that prove things

| command | what it proves | needs |
|---|---|---|
| `scripts/test.sh` | unit tests (crew layer, ship core), tool tests, lint, patch series shape. Check its **exit status**, not its output. | nothing |
| `scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts` | G0. Exits 1 by design while 8 corpus files are rejected. | `scripts/fetch-gdk.sh` |
| `scripts/g3-measure.sh` (`--seconds 30` for a quick regression) | G3, eleven criteria | game data, built engine |
| `scripts/s2-check.sh` | S2: ship in game, save/reload, console operated | same |
| `scripts/build-ship.sh` | S3: stitches and compiles the whole ship (~3 min with vis+light) | `fetch-gdk.sh`, `fetch-map-tools.sh` |
| `scripts/s9-check.sh` | S7/S9 at the consoles: breach puzzle solved by key, jump from the Conn, torpedo from Tactical | game data, built engine |
| `scripts/s7-check.sh` | S7/S8 in the game: boarders and drones embodied, hostile, and tied to the ship's count | game data, built engine |
| `scripts/s5-check.sh` | S5 so far: embodied crew follow the ship's roster through meals; most walk to their place | game data, built engine |
| `scripts/s4-check.sh` | S4 so far: Tactical opened by the retail panel command, operated, authority enforced | game data, built engine |
| `scripts/s3-check.sh` | S3: one map, fifteen decks, each reached by turbolift, standing and clear; game frame time | the built ship (builds it if absent) |

All engine runs are headless (`xvfb-run`, software Vulkan, `SDL_AUDIODRIVER=dummy`) and write under
`build/g3-home`, never the owner's `build/home`. A stale `*.pid` file in the home's `baseEF` makes
the engine wait at a dialog forever; the scripts delete it first.

**Run the regressions after any engine or module change**: `scripts/s2-check.sh`,
`scripts/s4-check.sh`, `scripts/g3-measure.sh --seconds 30`, and `scripts/s3-check.sh` if the ship,
the stitcher or the engine limits were touched. Do not edit `scripts/` or rebuild the module while a
measurement is running — bash reads scripts incrementally and the module is loaded per run.

## Gates

| gate | state |
|---|---|
| G0 | re-run 2026-10-05, matches recorded evidence |
| G1, G2, G6 | closed / reported working by the owner earlier |
| G7 | server half only; needs two machines and a person |
| G3 | **implemented, measured, passing; NOT signed off.** The owner said it stays open. Two judgement calls are his: whether the deck feels inhabited (`scripts/run-scenario.sh`), and whether six *spawned* crew satisfy "named crew already placed" (deck04's own eight cannot hold posts). |
| S1 | done — `module/ship/ship_core.*`, `tests/ship` |
| S2 | built and verified headless; the owner has not operated the console. Open by command only (`ship console`), keyboard only, flat colour. |
| S3 | **in progress — see below** |
| S4 | **first slice done** (`scripts/s4-check.sh`): stations in the core; one screen serves Engineering, Tactical, Ops, Conn (and Sickbay); the retail panel commands `ui_engineeringstatus`/`ui_tactical`/`ui_ops`/`ui_navigation` open them when `g_ship 1`. Remaining: each station's real controls (needs core state: targets, course, transporter, medical), live status drawn on panels in the world (engine work), the other retail panels, mouse, artwork. See `docs/evidence/s4-station-consoles.md`. |
| S5 | **first slice** (2026-10-06): with `g_crew 1 g_ship 1 g_crewFromShip 1` the crew on the player's deck are whoever the ship has there (`SyncShipRoster` in `g_crew.cpp`): they arrive and leave with their routine, each is given a place from the deck's navigation, and they walk to it. Seen on `tour/deck04` standing in for deck 2 (`+set g_crewDeck 2`): 41 aboard at breakfast, none an hour later, back at each meal; 7 of 10 embodied reached their places within 35 s, one failed. `scripts/s5-check.sh` checks it (count follows the ship through four meals; most reach their place; those leaving walk out of sight). **On the merged ship it runs** (`+set g_shipDeckPitch 3072 +map voyager`): at 08:00 the ship has 13 on deck 1 — the bridge watch, Janeway and Chakotay among them — and ten are embodied; five reached their places and four gave up, because a "place" is an arbitrary navigation node and some are behind doors that stay shut. **That is the next thing to fix: real stations for places** (a table of positions per deck). Cap is 10. Plan below. |
| S6 | **first slice, core only** (`tests/ship`): damage-control parties (engineers, parts, time), exposure casualties, sickbay beds and recovery, `RepairDeck`. Save format is now version 2. Not in the game beyond `ship status`. Next: show it (crew walking to repairs = S5's places becoming real), give hull repair a crew and a cost, make fatigue matter, more causes of injury. See `docs/evidence/s6-damage-and-casualties.md`. |
| S7 | **first slice, core only** (`tests/ship`): `Board`, per-system `control`, `Hijacked` (refuses console, delivers nothing), `CounterHack`, security response and attrition, boarders advancing, `MakeBreach`/`BreachScore`. Save format is version 3. Console: `ship board <deck> <n>`, `ship counterhack <system> <0..1>`. **Bodies are done** (`SyncIntruders` in `g_crew.cpp`, checked by `scripts/s7-check.sh`): the ship's boarders on the player's deck are Klingon raiders or Borg drones, hostile to the crew, capped at six, and one killed is one fewer in the ship's count. **The breach screen is done** (`H` on any console; `ship as N breach <sys>` / `solve <cells>`; checked by `scripts/s9-check.sh`). Next: a timer on the puzzle, security fighting as bodies. See `docs/evidence/s7-intruders-and-control.md`. |
| S8 | **first slice, core only** (`tests/ship`): `BoardBorg`, per-deck `assimilated`, crew taken become drones, assimilated systems locked, stripping by engineers for parts. Save format is version 4. Console: `ship borg <deck> <n>`. Next: the visible half — swapping a section's shaders/models by `assimilated` is an engine capability to design; drones as NPCs (the game has Borg NPC types). See `docs/evidence/s8-the-borg.md`. |
| S9 | **first slice, core only** (`tests/ship`): `sector` of 12 beacons from the seed, `Jump`, `Enemy`, `FireTorpedo`, `shieldStrength`, hits landing on decks and systems, boarding when shields fall, salvage. Save format is version 5. Console: `ship chart`, `ship jump <n>`, `ship fire`. **On the consoles in outline** (Tactical: contact readout, `F` fires; Conn: beacons listed, `J`/`K`/`L` jump; checked by `scripts/s9-check.sh`). Next: an enemy with systems, the pressure director, a viewscreen. See `docs/evidence/s9-the-outside.md`. |
| S10 | **first slice, core + cvars** (`tests/ship`): `PlayMode`, `ClockMode` + `CatchUp`, `MayOperate`/`MayCallAlert`/`MayCommand`, `PlayerRole`, `CreateCharacter`. Save format is version 6. Cvars `g_shipMode`, `g_shipClock`, `g_shipRole`; console `ship character <name> <dept> <rank>`. **Clearance is enforced** (consoles send `ship as <station> ...`; refusals come from `Svcmd_Ship_f` and show on the screen; checked by `scripts/s4-check.sh`). **Not enforced yet**: deny manual saves in ironman (`GameAllowedToSaveHere` in `g_savegame.cpp` is the place), and a creation screen. See `docs/evidence/s10-modes-and-roles.md`. |

## S3 in detail (the active work)

Done and committed:
- `tools/shipmap/stitch.py` (+ `tests/tools/test_shipmap.py`): merges the ten published deck sources
  into one map at a 3,072-unit pitch. Renames only names *defined* on more than one deck (261).
- Engine patches `0008` (entities to 4,096; SP bridge tables followed), `0009` (brush-model tables
  4,096 with a bounds check; renderer model table 4,096), `0010` (snapshots cull by visibility and
  keep the nearest).
- The merged ship loads, runs (game frame 3.1 ms avg / 26 ms worst, structure-only build), bakes
  navigation, and draws decks 4 and 11 when the player is placed there (`g_shipTest 4`,
  `g_shipTestPos "x y z"`).
- The ship compiles **with visibility and lighting** in about three minutes (54 MB BSP).

Also done and committed (2026-10-06):
- `module/ship/g_scope.{h,cpp}`: deck-scoped name lookup, on with `+set g_shipDeckPitch 3072`;
  its attach points are in `patches/0005` (regenerated, series verified).
- The lit ship (visibility + lighting) loads with `+set com_hunkMegs 768`, draws deck 4 shaded,
  and runs at 3.0 ms average / 18 ms worst game frame.

To load the ship by hand: `scripts/build-ship.sh --out build/home/baseEF`, then
`scripts/run-engine.sh +set com_hunkMegs 768 +set g_ship 1 +set g_shipDeckPitch 3072 +map voyager`.

Next steps for S3, in dependency order:
1. (done) lit ship loaded and measured.
2. (done) `patches/0005` regenerated with the scope hooks.
3. (done 2026-10-06) Turbolift links: the stitcher rewrites deck-to-deck `target_level_change`
   into `target_teleporter` -> `dNN_arrival`; verified in the engine by firing deck 1's links
   (`g_shipTest 4` with `g_shipTestPos <targetname>` uses that entity). Still to do here: drive the
   real turbolift *menu* (`ui_turbolift.cpp`) on the merged ship, and restore whatever each level
   change's original `target` fired.
4. (done 2026-10-06) Triggers as boxes: `patches/0011` + the stitcher + `inject.py`; brush models
   900 -> 406; a boxed trigger proven to fire (`g_shipTest 4` with `g_shipTestWatch "x y z"`).
5. (done 2026-10-06, as placeholders) Decks 6, 7, 12, 13, 14: `tools/shipmap/gendeck.py` makes a
   sealed lit hall with waypoints for each; the stitcher links every deck to every other (129
   links added). Real interiors for these decks are future authoring, not S3.
6. (mostly done 2026-10-06) Missing textures: four are substituted by the stitcher's `SUBSTITUTE`
   table (66 surfaces) — chosen by name, not by eye, so look at them in the ship. Two of the seven
   (`common/glassportal`, `sickbay/lights`) are defined as shaders and only lack an editor image,
   which is harmless. `common/light_floor` has no obvious stand-in and is still missing.
7. (machine part done 2026-10-06: `scripts/s3-check.sh` tours all fifteen decks, 15 of 15.)
   Someone walks it. S3's exit criterion is every deck reachable on foot. **This is the main thing
   left in S3** and needs either the owner in a session or a harness that walks: e.g. for each
   deck, ride the link, then check the baked navigation connects the arrival point to that deck's
   waypoints. Also still open: drive the real turbolift menu; investigate why `num_entities` at
   init differed between two loads of the same map (3,323 vs 2,543); frame time for fifteen decks.

Known risks in S3: all ten decks' scripts now run at once in one level, which they were never
written for; the worst-case game frame was 26 ms before rendering; crew cannot open doors (found in
G3) and will need to for S5.

## S5, when S3/S4 allow: how the pieces already fit

The crew direction layer (G3) embodies a *declared* roster and walks it to posts; the ship core
(S1) knows where each of the 141 is supposed to be. S5 joins them:

1. (done as a first slice — `SyncShipRoster`) The roster on the player's deck comes from
   `ship::CrewOnDeck`, checked by `scripts/s5-check.sh`. Next for this step: run it on the merged
   ship with `g_shipDeckPitch` (the code path exists, untested); raise the cap toward 20-30 and
   measure frame time; one in ten gave up reaching a place on deck04 — find out why.
2. A crew member's post is their system's station (`ship::Spec(post).deck/station`); stations need
   positions on the merged map — a table from station name to a waypoint, authored per deck.
   Off-duty crew go to the mess hall, the holodeck, their quarters: more positions.
3. Watch change is then just the roster query changing; the layer already handles arrival,
   holding, yielding to scripts, and save/load.
4. **Doors** — "crew cannot open doors" (as recorded in G3) is probably too strong. Read from the
   source and the ship's map on 2026-10-06, not yet tested in the engine: a door's own trigger
   opens for any client, NPCs included (`Touch_DoorTrigger`, `g_mover.cpp`). Of the ship's 192
   doors, 124 are ordinary automatic doors (most with `MUST_FACE`: they open within 32 units, or
   within 72 if you are walking at them), **26 are security doors** (spawnflag 32) and **34 are
   named** — opened only by a script or trigger, which is how the turbolift doors work. The two
   places crew were blocked in G3 were beside the turbolift and at a dead end, i.e. probably named
   or security doors. So the likely truth is "crew cannot pass doors that are meant to be shut".
   Supporting this: in the S5 slice, roster crew spawned across deck04 walked 20-35 s to places
   elsewhere on the deck, which they could not have done without passing doors. Still to do: then
   decide what clearance means for security doors (that is S10's rank model arriving early).
5. Measure as G3 was measured (`tools/crewgen/g3report.py` is reusable): station coverage across a
   watch change, at 20-30 embodied.

## Things the owner should be told (already said, repeat if asked)

- **Saves from before `patches/0008` do not load** — including his own in `build/home`.
- The six stale `longwayhome` processes from before the session were closed on 2026-10-06 after
  the owner said old idle game processes may be closed ("if there's nothing using them"). That
  permission is for orphaned, idle game processes only; check before closing anything.
- Most numbers in the ship simulation are invented; canonical ones in `docs/lore-ledger.md` are
  marked "recalled" where written from memory and need checking against a source.
- `build/tools` (map compiler, cabextract) and `build/gdk` (the GDK content) were fetched into the
  build tree; both are gitignored and reproducible by `scripts/fetch-map-tools.sh` / `fetch-gdk.sh`.

## Mistakes made this session, so they are not repeated

- Read only the "passed" lines of `scripts/test.sh` and missed that it exited non-zero for two
  commits. Check exit status.
- Edited a shell script while it was running; killed and re-ran the measurement.
- Wrote a nested heredoc that terminated early and ran the remainder as shell with empty
  variables. Use the Write tool for any file that contains a heredoc.
- First brush-model count for the merged ship was 2,011; the real figure is 1,275 in source and
  899 after stitching. Count with the tool, not a one-off script.
- A `pkill -f` pattern matched the shell running it.
