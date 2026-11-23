# Evidence: the transwarp route

**Session 2026-11-08.** Stage E's deferred item, `docs/endings.md`, "Two arrivals, one clip" and
"Transwarp": *"The clip is the constant; the route is the variable."* *"It requires the enemy's
knowledge."* *"The design says the shortcut costs a principle, a crew member, or a year. Transwarp
costs all three."* The acceptance line is: *"The arrival clip is identical for every destination and
for every route, including the conduit."*

## What was built

- **The route is an enum, not an arrival.** `EndingRoute { ROUTE_GRIND, ROUTE_CONDUIT }` and
  `RouteName` live in `module/ship/ship_core.{h,cpp}`. `ARRIVAL_MS` remains 3000 and the engine path is
  untouched: the conduit takes the same camera (`CGCam_Enable`/`CGCam_Move`/`CGCam_Fade`), the same
  fade and the same timer. What changes is the record: the route is logged, published in
  `lwh_ship_ending_route`, and printed in the arrival line as "by the conduit".
- **The gate is the enemy's knowledge.** `BorgKnowledgeAboard` is true when a living record is
  Borg-recovered (Seven, a recovered drone, a Borg-recovered allegiance), or a deck is still more than
  half assimilated — "a section of the ship never got back". The canon roster carries Seven, so the
  default run has the road; an all-fictitious crew or a run that lost her may not. `TranswarpCostOf`
  says whether it can be taken and who would carry it.
- **All three costs are paid, and they are state changes.** `TakeTranswarp` advances the clock by a
  year, kills the crew member who carried the knowledge (`KillCrew` with the cause "the transwarp
  conduit"), and records the principle — "the ship takes the Collective's road" — in the official log.
  It refuses, changing nothing, when there is no knowledge aboard or no one fit to take it.
- **Where it is reached.** `ship ending <destination> [nofile] [conduit]` and
  `ship wildcard <destination> [conduit]` in `module/ship/g_ship.cpp`, and the endgame screen's
  approach row (`module/ui/ui_lwh_ending.cpp`). The route is taken once; `endingRouteTaken` is host
  state for the one-shot endgame.
- **Neutrality.** The route name, the cost line and the principle are produced from a fixture inside
  `EndingToneFindings` and scanned by the same assertion as every other ending string.

## The command

```sh
scripts/ending-check.sh
```

It compiles the ship tests on the host, runs the suite, prints `test_ship_core --endings`, then
falsifies five ways. The excerpt below is this session's run.

```text
== the route
  BORG KNOWLEDGE: 0 aboard; the conduit is not available
  CONDUIT: available; the conduit: Crewman 141 takes the hub's knowledge and does not come back; the ship takes the Collective's road; a year passes in three seconds
  TAKEN: 1; the clock moved 365 days; crew fit 141 -> 140
  CONDUIT NEUTRAL: 0
  THE CLIP: 3000 ms on the grind and on the conduit (the route is the variable)
```

The module was also built against the engine in a separate tree (the shared checkout's `build-linux`
was not disturbed), so the UI and the game module compile and link with the route:

```sh
cmake -S /home/c/big/git/upstream/efgame -B /home/c/big/git/upstream/efgame/build-lwh-endings2 \
  -DLWH_MODULE_DIR=/home/c/big/work/lwh-endings/module
cmake --build /home/c/big/git/upstream/efgame/build-lwh-endings2 -j4
# [100%] Built target efgame
cd /home/c/big/git/upstream/efgame
nm -C build-lwh-endings2/libefui.so | grep LWH_UI_EndingScreens
# 000000000004c780 t LWH_UI_EndingScreens(char const*)
```

## PASS lines, and what each is evidence of

- **PASS — the gate exists.** `BORG KNOWLEDGE: 0 aboard; the conduit is not available` is a ship with
  the recovered records stripped and every deck below the Borg threshold; `TestEndingRoute` asserts
  `!BorgKnowledgeAboard` and `!TranswarpCostOf(s).available`, and `TakeTranswarp` refuses with a
  reason and changes nothing.
- **PASS — the route can be taken when the knowledge is aboard, and the cost is all three.**
  `CONDUIT: available; the conduit: Crewman 141 takes the hub's knowledge and does not come back; the
  ship takes the Collective's road; a year passes in three seconds`. The line names the crew member,
  the principle and the year.
- **PASS — the costs are state changes.** `TAKEN: 1; the clock moved 365 days; crew fit 141 -> 140`.
  `TestEndingRoute` asserts the clock advanced at least a year, `CrewFit` fell by one, and the last log
  entry is the cost line.
- **PASS — the clip is the constant.** `THE CLIP: 3000 ms on the grind and on the conduit (the route
  is the variable)`. `ARRIVAL_MS` is the one constant `StartArrival` uses for both routes; the
  `TestEndingRoute` assertion is `ARRIVAL_MS == 3000` after the route is taken.
- **PASS — the conduit's own words are neutral.** `CONDUIT NEUTRAL: 0` scans the cost line and the
  principle with `EndingToneFindings`.

## Decisions taken where the document leaves a choice

- **The cost's three parts are: the year on the clock, the crew member who carried the knowledge, and
  the principle recorded as the fact of taking the Collective's road.** The first two are saved state;
  the third is the log. The design names the three, not their units.
- **The carrier is a recovered record first, then the best science hand aboard.** That is the person
  the knowledge belongs to; it is deterministic, not a volunteer roll.
- **The conduit is available on the default canon run because Seven is aboard.** That is the design's
  own first example ("Seven"), not an oversight; the gate bites on an all-fictitious crew or after a
  loss.
- **The route is host state, one-shot.** Nothing after the log is replayed and no destination is
  saved, so the route is not written into the record. `SAVE_VERSION` remains **66**.

## What could not be run here, and is not claimed

- **The rendered arrival was not watched.** The route deliberately does not change the picture; the
  camera, the fade and the duration are the engine's own and are proven to compile and link, not to
  look like the show's shot. The picture is the owner's open decision and is out of scope for this
  build.
- **No graphical session ran the conduit.** The evidence is the model's state changes, the one shared
  clip constant, and the module link; the frame sequence is not claimed.
