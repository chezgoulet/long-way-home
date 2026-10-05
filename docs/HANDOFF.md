# Hand-off — where the work stands and how to continue

Written 2026-10-06 at the owner's request, for whoever picks this up next (a new session has no
memory of this one). Read this, then `docs/ship-programme.md` (the plan and the owner's decisions),
then `docs/gates.md` (the ledger). Keep this file current: update it at the end of every session.

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

All engine runs are headless (`xvfb-run`, software Vulkan, `SDL_AUDIODRIVER=dummy`) and write under
`build/g3-home`, never the owner's `build/home`. A stale `*.pid` file in the home's `baseEF` makes
the engine wait at a dialog forever; the scripts delete it first.

**Run the two regressions after any engine or module change**: `scripts/s2-check.sh` and
`scripts/g3-measure.sh --seconds 30`. Do not edit `scripts/` or rebuild the module while a
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
| S4–S10 | not started |

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
5. The five decks with no source (6, 7, 12, 13, 14): generated; everything about them is invention
   and goes in `docs/lore-ledger.md`.
6. (mostly done 2026-10-06) Missing textures: four are substituted by the stitcher's `SUBSTITUTE`
   table (66 surfaces) — chosen by name, not by eye, so look at them in the ship. Two of the seven
   (`common/glassportal`, `sickbay/lights`) are defined as shaders and only lack an editor image,
   which is harmless. `common/light_floor` has no obvious stand-in and is still missing.
7. Someone walks it. S3's exit criterion is every deck reachable on foot.

Known risks in S3: all ten decks' scripts now run at once in one level, which they were never
written for; the worst-case game frame was 26 ms before rendering; crew cannot open doors (found in
G3) and will need to for S5.

## Things the owner should be told (already said, repeat if asked)

- **Saves from before `patches/0008` do not load** — including his own in `build/home`.
- Six stale `longwayhome` processes from before the session were left running
  (PIDs 63223, 67616, 98050, 186058, 309455, 398990). They are not ours to kill.
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
