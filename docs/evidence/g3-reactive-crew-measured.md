# Evidence: G3 reactive crew, measured

Date: 2026-10-05. Gate G3's measurable criteria (`docs/g3-reactive-crew.md` §5), taken on the playtest
host from headless runs of the real engine against the retail data — not from reasoning about the code.

Reproduce: `scripts/g3-measure.sh` (about 25 minutes). It needs the game data, the built module
(`scripts/bootstrap-upstream.sh`) and the engine; it writes under `build/g3-home` only.

## The result

Scenario `scenarios/deck04-watch`: six crew, six posts, Virtual Voyager's deck 4 (`tour/deck04`).
Ten-minute observation run, coverage sampled every five seconds.

```
PASS  5-10 crew present on one deck                      6 on tour/deck04
PASS  post coverage at every sample                      minimum 100% over 121 samples (bar 90%)
PASS  observation run long enough                        600 s sampled (bar 600 s)
PASS  every NPC reaches its post within the bound        slowest 6100 ms (bound 90000 ms)
PASS  zero navigation failures                           0 stuck, 0 out of world
PASS  address -> acknowledgement within the bound        6 of 6 acknowledged, slowest 1500 ms (bound 5000 ms)
PASS  no ICARUS script regressions                       0 precedence violations; scripts in flight 4 with the crew, 4 without
PASS  a script takes a post-holder, and gives them back  1 script run on a crew member at post: the layer yielded 1 time(s), and had them back on duty 1 time(s)
PASS  save/load restores posts and schedule cursor       6 crew compared field by field; after the reload coverage is 100%
PASS  save-size increase within budget                   25 bytes per NPC of layer state (budget 256); the whole save grew 53584 bytes with 6 crew added
PASS  frame time holds                                   game frame 343 us with the crew, 239 us without (slack 1000 us); layer 4 us avg, 3407 us max per frame

G3 measured run: PASS
```

From the crewed run's log:

```
CREW: sampling post coverage from t=6100 ms
CREW: script test: running lwh/interrupt on watch1
CREW: script test: yielded, and back on duty 4200 ms after the script began
```

The second scenario, `scenarios/testroom-watch` — five crew in a room the scenario brings as a map
source, so the whole authored chain runs: source → `q3map2` → structural check → pack → load →
navigation baked by the engine → crew — passes the same eleven criteria on a 60-second run.

## How each number was taken

Three engine runs under a virtual display, software Vulkan, sound into SDL's null device:

1. **Baseline** — the deck with the layer off. Records the time spent in each game frame and which
   entities have an ICARUS script in flight when the run ends.
2. **Crewed** — the measured run. Coverage is sampled once every crew member has arrived. At the
   half-way point the harness *uses* each crew member in turn, exactly as the player's use key does,
   and times the reply; then runs a real compiled script on one who is holding a post. It ends by
   saving.
3. **Reload** — loads that save. The crew's persisted state is written out as restored and compared,
   field by field, with what was written at the save; then the run continues for thirty seconds and
   coverage is sampled again.

`tools/crewgen/g3report.py` reads the files the module wrote and prints the table. A criterion whose
evidence is missing is NOT MEASURED, and that fails the run.

## What these numbers do not say

- **Whether the deck feels inhabited.** That is the gate's last criterion and it is the owner's.
  `scripts/run-scenario.sh` is the run for judging it.
- **The six crew are added to the deck, not found on it.** None of deck04's own eight can hold a post
  (`docs/g3-reactive-crew.md` §8). They are existing character types with existing voices, so nothing
  is authored — but "named crew already placed" is not what was measured.
- **Frame time is the game module's, not the renderer's.** The run is software-rendered, so a frame
  rate would be meaningless; what is compared is the time inside the game's own frame, with and
  without the crew. The difference, about 0.1 ms, is six more NPCs thinking; the layer itself
  averages 4 microseconds. The 3.4 ms maximum is a single frame in ten minutes — it did not recur in
  the shorter runs (23–53 µs maximum) and its cause was not identified. Against a 60 fps budget on the
  target desktop this wants one look in a real session.
- **"0 precedence violations" is an assertion, not a measurement** — it is zero by construction. The
  evidence for precedence is the two rows that do not depend on the layer being right: the script
  census matching the baseline, and the script test.
- **Combat was not exercised.** No hostile appears on a Virtual Voyager deck, so level 2 of the
  arbitration is covered by unit tests only.
- **The director was not exercised in this run** (every post stayed staffed). It was seen working in
  an earlier run, when a crew member behind a door failed to reach the most important post and
  another was pulled across the deck to cover it; its rules are unit-tested.
- **The save-size figure is the layer's own record.** The whole save grew about 8.9 KB per added
  crew member, which is what any NPC costs in this save format, placed by a map or by us.

## What does not need the game

`scripts/test.sh` — the direction layer's decision logic (arbitration for every combination of
signals, both corollaries, assignment, restaffing, coverage, the save record and its rejection of
bad input, the configuration parser, the verdict), the authoring tools, and the patch series' shape.
CI runs it on every push.
