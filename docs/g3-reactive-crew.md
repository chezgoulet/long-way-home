# G3 — Reactive crew: specification

Status: **implemented and measured; awaiting the owner's judgement.** The direction layer is in
`module/crew/`, attached by `patches/0005`, authored through the scenario manifest's `crew` section,
and measured headless by `scripts/g3-measure.sh` — results in
`docs/evidence/g3-reactive-crew-measured.md`. Every criterion in §5 is a number and has been taken;
the one thing left is the one thing a log cannot decide: whether the deck feels inhabited.

The specification below was written before any code, as the charter requires. Where implementation
found the deck or the engine to be other than the specification assumed, the text is corrected in
place and the finding is recorded in §8, so that this document describes what exists.

---

## 1. Scope

The charter's first Track C milestone: **5–10 NPCs, on one deck, with no new animations and reused
existing barks, performing simple posts and acknowledgement.**

What that means in practice, and what it deliberately excludes:

- **In:** named crew already placed in the space hold assigned posts; move between them using existing
  navigation; turn to face the player; acknowledge being addressed; persist across save and load.
- **Out:** new animation (none is authored for this milestone), new recorded voice (existing barks
  only), new props or geometry, schedules and duties (those are G4), NPC-to-NPC conversation (G4).

## 2. Arbitration precedence — written first, as required

When two systems want the same NPC, the higher number wins:

1. **Scripted sequence (ICARUS).** An authored moment is intent. Nothing displaces it.
2. **Direct combat or reaction.** Being shot at, or a hostile in perception, overrides routine.
3. **Director override.** The ship-level scheduler may pull an NPC off routine for coherence —
   restaffing an empty post, clearing a corridor.
4. **Duty / routine.** The post and its schedule. What the NPC should be doing.
5. **Idle / social.** What it does when nothing above applies.

Two corollaries, and they are what make the rule usable rather than decorative:

- **An NPC interrupted above level 4 returns to its routine when the interrupt clears.** The
  direction layer remembers the post, so an interruption is a pause and not a reassignment.
- **Nothing in levels 3–5 may interrupt a level-1 script mid-sequence.** If the director wants an NPC
  that is mid-script, it waits.

## 3. The measured candidate space

Charter task 0 is answered with counts from the published map sources, not impressions. Coverage per
candidate deck (`waypoint` family entities, `waypoint_navgoal`, crew already placed, interactive
`func_usable` props):

| deck | waypoints | navgoals | NPCs placed | usable |
|---|---|---|---|---|
| **deck04** | **194** | 8 | **8** | **19** |
| deck02 | 145 | 25 | 4 | 14 |
| deck08 | 141 | 19 | 2 | 18 |
| deck05 | 106 | 26 | 8 | 9 |
| deck09 | 53 | 7 | 7 | 22 |

**Recommendation: `maps/tour/deck04`** — the densest navigation of any deck, eight crew already
placed, and nineteen interactive objects to hold posts at. `deck05` is the alternate if more navgoal
granularity turns out to matter than raw waypoint count.

Those counts are from the published map *sources*. The deck as shipped in the expansion pak differs:
`maps/tour/deck04.bsp` carries **93 waypoints, 8 navgoals, 8 placed NPCs and 10 spawners**. The
recommendation stands — it is still the best-furnished deck — but §8 records what those eight NPCs
turned out to be.

Two consequences worth stating. The decks are Virtual Voyager's, so **G2 and G3 share a space** —
which is efficient, and means G2's acceptance walk doubles as the reconnaissance for G3. And the
campaign maps carry far more of everything (forge3 has 641 waypoints and 109 NPCs), so nothing here is
limited by the data; it is limited only by what a first milestone should attempt.

## 4. Design

A **direction layer above the existing behaviour states**, not a replacement for them:

```
perception (existing: visrange, earshot, vigilance, LOS)
      |
post state        -- who holds which post, and whether it is filled
      |
duty selection    -- this NPC's post, its current goal, returning after interruption
      |
behaviour states  -- the existing bstate machine, extended with life-shaped states
      |
navigator + anims + say  -- existing steering, animation, dialogue
```

- **The post is the unit, not the character.** A post exists whether or not anyone holds it. This is
  what makes the multiplayer deficit model possible later, and it is what makes G3 testable now: a
  post is either occupied or it is not.
- **Reused machinery, explicitly.** A post is a position: either authored in the scenario, or
  borrowed from a `waypoint_navgoal` already in the map. Travel to it uses the engine's own
  locational-goal mechanism (`NPC_SetMoveGoal`, the per-NPC `tempGoal`), so the layer sets no
  behaviour state at all. Acknowledgement uses the game's existing response path (`NPC_Use` →
  `NPC_Respond`, the `EV_RESPOND`/`EV_BUSY` lines). Facing uses the existing look-target and
  desired-yaw fields. Steering uses the navigator, and
  **navigation data bakes itself**: the engine writes `maps/<map>.nav` on a map's first load whenever
  the file is missing (see `docs/evidence/trackb-map-toolchain.md`).
- **No new animation.** Standing, walking, turning the head and turning the body are all things the
  default behaviour state already does for an NPC with a goal, a look target and a desired yaw.
- **Persistence follows a precedent that already works in this engine family.** RPG-X keeps character
  and ship state in SQLite bundled *inside the gamecode* (`g_sql.c`, `sqlite3.c`, `ui_sql.c`) — no
  external daemon, state stored where it is owned. See `docs/prior-art-rpg-x.md`. G3 keeps its per-NPC
  state in the SP module's own save path for the same reason: one extra chunk (`CREW`) in the save,
  written only while the layer is on, so a save made with `g_crew 0` is byte-for-byte what it was.
- **Persistence is bounded from the start.** Per NPC: post id, current goal, and the schedule cursor
  (unused in G3, present for G4). G3's target is **256 bytes per NPC** -- a number to fail against
  rather than a hope, and it is measured at five crew before anything is designed for thirty.
  Measured: **20 bytes per NPC plus a 16-byte header** — fixed-width and little-endian, independent
  of the host's struct layout.

## 4a. The substrate, named from the shipped source

Read from `src/game/`, so the design above rests on named mechanisms rather than on expected ones:

- **The state machine is large enough to express a life.** `bstate.h` declares 46 states. `BS_IDLE`,
  `BS_WALK`, `BS_RUN`, `BS_WAIT`, `BS_LOOK`, `BS_AIM`, `BS_FACE`, `BS_STAND_GUARD`, `BS_PATROL`,
  `BS_DEFEND`, `BS_FACE_LEADER` and `BS_SAY` cover everything G3 asks for, and `BS_REMOVE` is worth
  noting for later crowding work.
- **The arbitration rule we specified is the shipped design, not a layer we impose.** The enum's own
  header comment says: *"These take over only if script allows them to be autonomous."* The precedence
  in §2 formalises what the engine already intends.
- **There is an override slot, and the corollaries are its documented behaviour.** The NPC struct
  carries `behaviorState` ("determines what actions he should be doing") beside `tempBehavior`
  ("while valid, overrides other behavior"). The layer reads both and writes neither: a temporary
  behaviour, or any state that does not simply follow a goal, marks the NPC as someone else's.
  *(Corrected: this paragraph originally cited `BS_SAY` as a working example of the cycle. Its
  header comment describes one; `NPC_BSSay` itself is a two-line `FIXME: Implement` stub. Nothing
  in G3 depends on it.)*
- **Goals are already navigator-driven**: `goalEntity`, `captureGoal`, plus leadership fields
  (`lastLeaderPoint`, `leaderTeleportSpot`). Travel to a post is an existing capability.
- **Speech has both ends**: `sayString` and `sayTarg` on the NPC, and the bark vocabulary in `say.h` --
  `SAY_ACKCOMM1-4` (acknowledge), `SAY_REFCOMM1-4`, `SAY_BADCOMM1-4`, `SAY_BADHAIL1-4`. Sixteen lines
  across four classes. `ACKCOMM` is the acknowledgement the bar asks for; `REFCOMM`/`BADCOMM` are the
  refusal vocabulary, which is what a post that cannot leave (security, bridge) should use instead of
  walking off.
- **The reaction layer G3 must not fight is a known file**: `NPC_reactions.cpp`, alongside
  `NPC_behavior`, `NPC_goal`, `NPC_move`, `NPC_senses`, `NPC_sounds`, `NPC_formation`, `NPC_spawn`.

## 5. Acceptance — measurable, and how each is measured

The charter's bar, with the measurement named for each:

| criterion | measurement |
|---|---|
| 5–10 named crew present on one deck | entity count in the loaded map |
| ≥90% of assigned posts occupied at any sampled moment | sample post occupancy every 5s over a 10-minute run; report the minimum |
| every NPC reaches its post within a bounded time | timestamp from level start to first arrival per NPC; bound set at implementation |
| zero navigation failures | count of stuck NPCs (no movement for N seconds) and out-of-world events |
| player address → acknowledgement within a bounded time | trigger an address, timestamp the bark |
| no ICARUS script regressions | the entities with a script in flight, compared against a run with the layer off; and a script run on a post-holder must take them and give them back |
| save/load restores posts and schedule cursor | save, load, assert per-NPC state equality |
| save-size increase within budget | byte delta of the save before and after the crew is added |
| frame time holds | the existing performance harness, with and without the crew |

Every one of these is a number, and every one can fail loudly. That is the point: the milestone is
judged by measurements, and the qualitative judgement — does the deck feel inhabited — comes after.

"At post" is the engine's own arrival test (`NAV_HitNavGoal`, the one `UpdateGoal` uses), not a
fixed distance: the layer and the navigator must agree about when a walk is over, or the NPC is
re-sent to a post it has already reached. Coverage sampling starts when every crew member has
arrived or failed, or when the reach bound expires, whichever is first — the walk to the post is
judged by the reach criterion, not counted against occupancy.

How each is taken, concretely (`scripts/g3-measure.sh`, three engine runs):

- a **baseline** run with the layer off records the game-frame time and which entities have a
  script in flight;
- the **crewed** run samples coverage, addresses every crew member as the player would and times
  the reply, counts stuck events and precedence violations, takes the same script census, and ends
  by saving;
- a **reload** run loads that save and compares every persisted field of every crew member against
  what was saved, then checks the posts are still held.

`tools/crewgen/g3report.py` prints one line per criterion. Evidence that is absent is reported as
NOT MEASURED and fails the run.

## 6. Implementation order

All six steps are done; they are kept as the record of the order the work took.

1. **Direction layer skeleton**, no behaviour change: posts loaded from the map's navgoals, occupancy
   tracked, logged. Verifies the substrate without moving anything.
2. **Post assignment and travel**: each of the 5–10 crew gets a post and walks to it, using existing
   states, with arrival detection.
3. **Facing and acknowledgement**: `BS_LOOK`/`BS_AIM` on the player's presence; a bark on address.
4. **Arbitration**: implement the precedence above, with the two corollaries, and test each level
   against the others (script mid-sequence must not be displaced by the director, and so on).
5. **Persistence**: save/load per-NPC state, with the budget asserted.
6. **Measurement harness**: the acceptance table above, automated where it can be, so the milestone is
   reported as numbers rather than impressions.

## 7. Risks, with probabilities

Each with the mitigation that makes it testable rather than merely worrying.

- **Waypoint density is not navigability** (~30%). 194 waypoints sounds like plenty; whether they are
  mutually reachable from the chosen posts is a different question. First implementation step answers
  it by trying; if the deck's graph is too coarse, `deck05`'s 26 navgoals are the fallback.
- **The direction layer races the existing reaction layer** (~35%). `NPC_reactions.cpp` already
  responds to sight and sound; a layer that also sets state can fight it for control. Mitigation: the
  driver asserts only when no hostile is in the NPC's perception, and never overrides a state it did
  not itself set. Both are one predicate each, and both are logged so a violation is countable.
- **Save size grows faster than budgeted** (~40%). Entity, NPC-info and script state are all
  serialised per NPC, and this engine's save path is twenty years old. Mitigation: measure at five
  crew before designing for thirty; if the growth is bad, store posts as a small table indexed by NPC
  slot rather than as per-entity fields.
- **Crew stand at their posts looking wrong** (~50% on first run). Facing, idle animation and
  furniture collision read badly before they read well. Mitigation: the criterion is at-post within
  1 unit, which is machine-checkable; appearance is the single subjective item and is judged last.
- **Barks are shared** (certain, ~4 acknowledgement lines). Several NPCs will say the same thing.
  Acceptable at G3, and addressed by voice work later rather than by this milestone.
- **Script precedence is subtler than one predicate** (~30%). Sequences can suspend and resume, so the
  driver may re-issue a post mid-sequence. Mitigation: log every driver assertion and count violations
  during the G3 run instead of trusting inspection -- criterion 3 of §5 is that count.
- **The frame-time criterion is the one most likely to bite** (~15%), because the direction layer runs
  on top of an interpreter-based VM. It is measured explicitly rather than assumed.

## 8. What implementation found

Recorded because each of these was an assumption in the text above until the deck was loaded.

- **None of deck04's eight placed crew can be given a post.** Four — Tuvok, Chell, Cuervo and the
  transporter chief — run permanent ICARUS scripts from the moment they spawn; under the precedence
  in §2 they are level 1 for the whole level, and the layer correctly never touches them. The other
  four are the lounge party, spawned `STARTINSOLID` in their chairs: they have no ground under them
  and cannot walk. The charter named this risk ("named-crew entities may have hard-coded
  behaviour"); it turned out to be the whole deck.
- **The deck's navgoals are already spoken for.** `cuer1–3` are Cuervo's own patrol route and
  `transporternav1–4` belong to the transporter-room script. "Every navgoal is a post" remains the
  default for a map with no authored crew section, but it is the wrong default for a shipped deck.
- **So the crew are declared by the scenario.** `scenarios/deck04-watch` names six crew — existing
  character types from the game's own NPC table, with their own models and voices — and six posts
  on the deck's waypoints. The layer spawns them through the map's own `NPC_starfleet` spawner.
  Nothing new is authored, which keeps the milestone's promise; but they are *added* to the deck
  rather than found on it, and that is a departure from "named crew already placed" that the owner
  should weigh when judging the gate.
- **Waypoints do not see doors.** Two of the six suggested starting positions were one waypoint
  away from their posts and still unreachable: a door between them does not open for crew. The
  measured run reported both as failures to reach; the positions were moved. This is risk one of §7
  (density is not navigability) arriving exactly as predicted, and it is why the suggestion tool is
  described as a starting point.
- **The director works, and was seen to.** When one crew member failed to reach the most
  important of two posts, the director pulled the holder of a less important one across the deck to
  cover it — unprompted, in a real run — and the hole moved to the post that mattered less.
- **Script precedence needed four predicates, not one.** A running sequencer, a pending task, a
  temporary behaviour or foreign behaviour state, and a goal the layer did not set. Risk six of §7
  expected this.
- **"Zero violations" proves less than it sounds, so precedence is also tested by doing it.** The
  violation counter is an assertion at every place the layer writes to an NPC; the decision above
  each write has already excluded scripted NPCs, so the counter is zero by construction and only
  moves if the construction is wrong. Two measurements stand beside it that do not depend on the
  layer's own logic being right: the script census compared against a run with the layer off, and
  a live test in which the harness runs a real ICARUS script (compiled by our own compiler) on a
  crew member who is holding a post, then checks that the layer yielded on its next turn and had
  the member back on duty when the script ended.
- **Decisions are taken every frame, after every entity has thought.** At a slower cadence a script
  that took an NPC mid-walk would find the layer's goal still on it for up to a tenth of a second.
  Evaluated at the end of each frame, the goal is gone before the NPC next thinks. The layer costs
  a few microseconds a frame, so there was no reason to ration it.
- **Acknowledgement was already in the game.** Using a friendly NPC makes Munro greet them by name
  or rank and the NPC reply. G3 times it and adds the collision case; it did not need to build it.
