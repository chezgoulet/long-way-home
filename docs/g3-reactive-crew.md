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
  (unused in G3, present for G4). An explicit per-NPC byte budget is set at implementation and tested.

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

## 7. Risks

- **Waypoint density is not navigability.** 194 waypoints sounds like plenty; whether they are
  mutually reachable from the posts chosen is a different question, and the first implementation step
  answers it by trying. If the deck's graph is too coarse, `deck05`'s 26 navgoals may be the better
  choice despite fewer waypoints.
- **Barks are shared.** Reusing existing lines means several NPCs may say the same thing; acceptable
  at G3 and addressed by voice work later, not by this milestone.
- **The frame-time criterion is the one most likely to bite**, because the direction layer runs on top
  of an interpreter-based VM. It is measured explicitly rather than assumed.
