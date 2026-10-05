# G3 — Reactive crew: specification

Status: **specification, no code written.** The charter requires the arbitration rules below to exist
*before* any Track C implementation, and requires navigation coverage on the candidate space to be
measured before crew work begins. Both are done here; the measurement is real, taken from the
published map sources.

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
- **Reused machinery, explicitly.** Posts are `waypoint_navgoal` entities already in the map — the
  space's own navigation furniture, no new authoring required. Acknowledgement uses the `say.h` bark
  vocabulary. Facing uses the existing head/torso tracking states. Steering uses the navigator, and
  **navigation data bakes itself**: the engine writes `maps/<map>.nav` on a map's first load whenever
  the file is missing (see `docs/evidence/trackb-map-toolchain.md`).
- **No new animation.** `BS_SAY` already provides the talk animation timed to a sound's length, and
  `BS_WAIT`/`BS_LOOK`/`BS_AIM` cover standing, turning and facing.
- **Persistence follows a precedent that already works in this engine family.** RPG-X keeps character
  and ship state in SQLite bundled *inside the gamecode* (`g_sql.c`, `sqlite3.c`, `ui_sql.c`) — no
  external daemon, state stored where it is owned. See `docs/prior-art-rpg-x.md`. G3 keeps its per-NPC
  state in the SP module's own save path for the same reason.
- **Persistence is bounded from the start.** Per NPC: post id, current goal, and the schedule cursor
  (unused in G3, present for G4). G3's target is **256 bytes per NPC** -- a number to fail against
  rather than a hope, and it is measured at five crew before anything is designed for thirty.

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
  ("while valid, overrides other behavior"). `BS_SAY` already demonstrates the full cycle -- turn to
  the target, play the bark, **revert when the sound finishes** -- which is exactly "an interruption
  is a pause, not a reassignment". G3 does not need a new arbitration system; it needs to use this one.
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
| no ICARUS script regressions | the deck's existing `target_scriptrunner` scripts all fire and complete |
| save/load restores posts and schedule cursor | save, load, assert per-NPC state equality |
| save-size increase within budget | byte delta of the save before and after the crew is added |
| frame time holds | the existing performance harness, with and without the crew |

Every one of these is a number, and every one can fail loudly. That is the point: the milestone is
judged by measurements, and the qualitative judgement — does the deck feel inhabited — comes after.

## 6. Implementation order

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

## 7a. Open decisions (owner's, pending)

Five, and only the first blocks implementation. Recommendations are mine and are reversible.

1. **Which deck.** Recommendation: **`deck04`** -- densest navigation of any deck, eight crew already
   placed, nineteen interactive objects to hold posts at. `deck05` is the alternate if the goal
   entities prove too coarse in practice.
2. **Crew count for the first pass.** Recommendation: **five, not ten.** The save path is twenty
   years old, and the per-NPC cost should be measured at five before anything is designed for thirty.
3. **Anchored posts in scope?** Whether a post may forbid its holder from leaving (bridge, security),
   answering with a refusal bark instead of following the player. Recommendation: **in** -- it is a
   data flag, and it is the difference between a staffed deck and a room of people who all abandon
   their stations when addressed.
4. **Who authors the posts.** Recommendation: **generated as a small data table** for G3; the
   alternative is hand-marking in an editor, which pulls in the editor question. That question belongs
   to Track B, not to this milestone.
5. **The reject condition for the subjective item.** Decide in advance what would make the deck read
   as mannequins rather than as inhabited, so the judgement is not made after the fact.
   Recommendation: **if it reads as furniture, G3 fails and is fixed before G4 starts.**

One boundary that is not a fork: G3 passing on one deck unlocks G4's *specification*, not its
implementation. The mechanism should hold in a second space before it is generalised. That is the
wedge principle, and skipping it is the most likely way for this program to become expensive.

## 7b. Decisions locked (2026-10-05)

All five accepted as recommended: **deck04**; **five crew** for the first pass; **anchored posts in
scope**; posts **generated as data** rather than hand-marked; and the rejection criterion below. These
are binding for G3, and are not revisited mid-implementation.

## 7c. The rejection criterion, decomposed

The owner's criterion, verbatim: **"unrealistic and unnatural actions for the characters in their
environment."** That is the bar the whole milestone serves. It is a judgement rather than a number, so
it is decomposed here into the impressions that produce it -- and note what it is a bar *about*: fit to
the environment, not intelligence. Nobody will be disappointed that a crew member is not clever. They
will be disappointed the moment one stands in a wall.

In rough order of how fast a player notices:

1. **Position.** Standing inside furniture or geometry, in a doorway, or on top of another crew member.
2. **Facing.** Facing a blank wall, or not facing the thing they are ostensibly standing at.
3. **Spacing.** Posts clustered so crew overlap -- or spaced so evenly the deck reads as a chessboard.
4. **Attention.** Answering someone they cannot see, or through a wall; ignoring someone standing
   directly in front of them.
5. **Motion.** Routes a person would not take, walking through objects, stopping mid-stride, snapping
   turns.
6. **Sameness.** Everyone reacting on the same beat, with the same animation, at the same pacing. This is
   the clearest single tell that a deck is machine-driven.
7. **Idleness.** Perfect rigidity at a post. A person shifts weight, glances, adjusts. Absolute stillness
   is more unnatural than motion, not less.

What follows is binding on how the code is written, rather than being polished in afterwards:

- **Posts are placed clear of geometry and spaced to the room**, and the post set is reviewed as a *set*,
  not one post at a time.
- **Facing is derived from the object the post belongs to** -- a console, a station, a doorway -- never
  from an arbitrary angle.
- **Attention is gated by the NPC's existing perception** (sight, earshot, line of sight), so that "they
  noticed me" is always truthful about what the NPC could actually perceive.
- **Movement uses the deck's own navigation**, so routes follow the space instead of cutting through it.
- **Idle variation is a requirement, not decoration.** No two crew members share a cycle.
- The machine checks in §5 are necessary and not sufficient. This list is what decides, and it is judged
  by the owner, by eye, on the deck.

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
