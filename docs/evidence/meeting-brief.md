# The meeting — phase one: the brief, the skeleton, and the seams

`docs/staff-meetings.md` (the owner's design), `docs/programme-meetings-and-voice.md` (the order and the
Hermes Vox lessons), and `docs/evidence/voice-review.md` (delivery direction per line), applied on
`feat/the-meeting-brief` (cut from `testing`), 2026-10-08.

**Observed, and the command that produced it.** Everything below is the output of a command named beside
it, on this tree, after `cmake --build /home/c/big/git/upstream/efgame/build-linux --target efgame` (which
built `libefgame.so` with only the pre-existing `-Wwrite-strings` warnings). This phase is **text and
state**: no audio player, no cue track, no synthesis call, no model call. The seams for those are named and
left. Save format is **52**: the queued meeting briefs and the schedule they fall on append to the blob, so
older saves are invalid — the cost named in the brief.

## What was built, and where

One model, in `module/ship/ship_core.{h,cpp}` (the same file the power assignment lives in): the brief
generator, the authored skeletons, the delivery vocabulary and the synthesis seam, and the allocation seam.
The engine host in `module/ship/g_ship.cpp` adds the `ship meeting` console command and the `g_shipTest 80`
harness. The unit tests are in `tests/ship/test_ship_core.cpp`; the engine check is
`scripts/meeting-check.sh`.

## Task A — the brief, and it is a read

`BuildBrief(const Ship &, kind)` is a pure function: it builds a `MeetingBrief` and writes nothing. The
acceptance that matters most is asserted directly — the ship's own serialised blob is byte-identical before
and after building every kind of brief:

```
== a brief is a read: the ship's state is unchanged
  7 briefs built; the ship's blob is unchanged
```

(`test_ship_core --meeting`; `TestMeetingBriefIsARead` also asserts `Pack(s) == before`.)

**The brief as it is built, with one example printed.** The watch-change brief, from
`/tmp/.../test_ship_core --meeting`:

```
== a brief for each kind the design names
  watch-change  present 5, decision: the watch handover, and what the incoming watch carries
               trigger: the watch changes: day 0, alpha watch; plant 1350 of 1350 (0 short), crystal 100%, casualties 0/4
               option 1: Carry on as briefed  [cost: nothing]  (record)
               option 2: Watch the reserve  [cost: Engineering's attention this watch]  (record)
               option 3: Bring her to yellow  [cost: the watch runs hot and the plant draws harder]  (set the alert)
               option 4: Note what the department raised  [cost: nothing yet]  (record)
```

A `MeetingBrief` carries, as the design asks:

- **who is present** — `BriefParticipant present[]`, resolved from the roster by post and department (the
  officer posts put in the room), each with name, post, watch and mood, and each with **their own** view:
  the marks they hold, the open promises they carry, and the log they can read;
- **what is being decided** — `decision`, and the **enumerated options**, each with a label, an **intent
  descriptor** (shared with the pills and novelty matching), and **what it costs** (`cost`);
- **the ship and the people as they are right now** — the plant (`powerCommitted` against
  `powerAvailable`, and the shortfall), **every system's commitment and what it is delivering**
  (`BriefSystem systems[]`: the numbers the allocation argument is made of), the alert, the crystal, the
  casualties against the beds, the Borg pressure, and each participant's marks and open promises. A
  `stateDigest` is carried so a stale script can be detected before the meeting.

A brief for **each kind the design names** exists: the watch change, the ordinary departmental meeting, the
allocation meeting, the dilithium threshold, casualties over beds, Borg pressure, and a promise come due
(`TestMeetingBriefIsARead` builds all seven and checks each carries participants, a decision, and options
with costs).

**A brief is built per participant, from marks and the log** (`docs/the-record-and-the-log.md`, so the room
does not all know the same thing). A post reads its own log scope; command reads all; a participant carries
their own marks and the promises they are party to. `TestMeetingBriefPerParticipant` proves an engineering
participant holds the engineering entry and not the sickbay one, and a beneficiary holds the promise made to
them. The log is **read as documents, never as a decision input**: `BuildBrief` is const, nothing in `Tick`
consults a log, and the month report is still drafted from marks.

## The every-meeting trigger, and the emit site

**Proved by the emit site, not a counter.** The brief is emitted in exactly one place, `EmitDueMeetings` in
`module/ship/ship_core.cpp`, called by the simulation's own clock at the end of `Advance` whenever time has
actually passed (a zero-length tick — a load, a console read — emits nothing). It logs the emission, so it
can be grepped:

```
LogEvent(s, "the bridge", "command",
    std::string("a ") + MeetingKindName(kind) + " meeting is called: " + decision);
```

**The dead-layer fix (lesson 6): a trigger in the normal case.** Two kinds fall on the ordinary clock, so a
brief is generated even when nothing dramatic has happened:

- **the watch change** — every watch, `WatchToken(s) != lastWatchMeeting`;
- **the ordinary departmental meeting** — daily, and it resolves nothing and still produces one.

`TestEveryMeetingEmitsABrief` reads the *emission* — the queued `MeetingBrief` and the log line — not a
tally:

```
== every meeting emits a brief, in the normal case (the emit site, not a count)
  at the watch change: 1 brief(s) queued
    watch-change  the watch changes: day 0, beta watch
```

The threshold kinds (allocation, dilithium, casualties, Borg, deferred) are **latched**: one episode calls
one meeting, and the latch releases when the fact that called it passes, so the next episode calls its own.
`TestEveryMeetingEmitsABrief` also advances a day and sees the departmental brief.

## Task B — the authored skeleton, which is the floor

For every kind there is a `MeetingSkeleton`: the enumerated outcomes, their costs, and minimal dialogue for
each. It is static content in the module, **the floor the generated flesh sits on**, and the meeting plays
with nothing generated at all. `TestSkeletonPlaysWithoutAModel` walks all seven skeletons and asserts every
outcome has dialogue and every line carries a delivery direction; `test_ship_core --meeting` prints them.

**No model is referenced anywhere in this phase.** A grep for a model call (or a synthesis call) in
`module/ship` finds none. The demonstration that the skeleton plays with the model absent is that the
skeleton is the only thing that plays:

```
== the skeleton plays with no model present
  watch-change  the watch handover, and what the incoming watch carries
    Carry on as briefed  [cost: nothing]
      (order) You have the watch. Carry on.
      (report) Engineering holds at the allocation we set.
    ...
```

## Task C — the delivery direction, per line

The voice review recorded that `exaggeration` defaults to **0.5** whatever the text says, so `exaggeration`
is the missing control. Every `MeetingLine` carries a `delivery`, and the seam `LineToSynthesis` carries it
with the text:

```c++
struct SynthesisRequest { std::string text; float exaggeration; uint8_t delivery; };
bool LineToSynthesis(const MeetingLine &line, SynthesisRequest &out);
```

**The vocabulary, small and named** (the values are the review's own knob, `exaggeration` / `emotion_adv`):

| delivery | what it means | `exaggeration` |
|---|---|---|
| `DELIVERY_ORDER` | an order or a warning: urgency | **0.8** (the review's fix-pass value) |
| `DELIVERY_REPORT` | a factual report or an answer: neutral | 0.5 |
| `DELIVERY_CONFESSION` | something admitted against interest: low and close | 0.35 |
| `DELIVERY_CONDOLENCE` | grief said to someone: low and slow | 0.3 |
| `DELIVERY_FLAT` | procedural, the log read aloud: flat | 0.2 |
| `DELIVERY_UNMARKED` | **the brief cannot know it: marked, not guessed** | — (the seam refuses it) |

**Where the brief cannot know a line's delivery, it is marked rather than guessed.** One authored line does
this — the sciences line *"The reading is not something I can call yet."* — and `LineToSynthesis` refuses it
rather than defaulting it to flat:

```
== the delivery vocabulary (docs/evidence/voice-review.md, finding three)
  order       exaggeration 0.80
  report      exaggeration 0.50
  confession  exaggeration 0.35
  condolence  exaggeration 0.30
  flat        exaggeration 0.20
  unmarked    unset (the seam refuses it)
  seam: "Make it so." -> exaggeration 0.80, delivery order
```

## Task D — the seam to the allocation model

`docs/power-assignment.md` left the calls open: `SetAllocation`, `RecommendAllocation`, the delegation.
`ApplyMeetingOutcome(s, brief, outcome, decidedBy, automatic)` applies exactly one enumerated outcome, and
the allocation options call `SetAllocation` / `SetAllocationBy` / `AcceptRecommendation` /
`RefuseRecommendation` / `SetPowerAuto`. Demonstrated end to end in the engine:

```
    SHIP: meeting test: allocation outcome applied=1; holodecks 100%, shields 0% (provenance the player)
    SHIP: meeting test: set as the ship (automatic) applied=0 (0 = refused, as designed)
```

**The ruling it implements, and how it was handled:** *the player and the crew decide; an outcome set by the
ship itself is automatic mode; one set by a person is theirs, and the meeting does not get to override a
person unless a person in the room does.*

- A **person in the room** decides (`decidedBy`): the outcome is applied with that person's provenance —
  the player's hand uses `SetAllocation` (provenance `the player`); an officer uses `SetAllocationBy`, which
  succeeds only if they hold the band, so an officer without the authority cannot set it at all
  (`TestMeetingAllocationSeam`).
- **The ship's own answer is automatic mode**: the "run the ship's own ladder" option calls `SetPowerAuto`,
  and the ladder then applies only to systems nobody has set, exactly as `docs/power-assignment.md` requires.
- **The meeting may not set an allocation *as the ship***: an allocation outcome marked `automatic` is
  refused, and the log says why (`the meeting will not set an allocation as the ship; a person must decide
  it, or automatic mode must be granted`). This is asserted in the unit test and observed in the engine.

## The commands, and what they returned

```
scripts/test.sh
  -> all checks passed (crew, ship: ship_core: all checks passed including TestMeetingBriefIsARead,
     TestEveryMeetingEmitsABrief, TestSkeletonPlaysWithoutAModel, TestMeetingAllocationSeam,
     TestMeetingBriefPerParticipant, TestMeetingSaveRoundTrip; tools, python, shellcheck, 19 patches)

scripts/check.sh --source-map build/gdk/maps/eliteforce_voyager_maps/voy1.map \
                 --script-corpus build/gdk/scripts
  -> entity dictionary 318 classes; validator negative tests ALL PASS;
     2,024 files: 2,016 compiled and read back, 8 rejected — exactly the known, documented set

scripts/meeting-check.sh --map tour/deck04
  -> PASS  a brief is emitted at the watch change; a person set an allocation end to end; the ship's own set was refused

scripts/power-check.sh --map tour/deck04
  -> PASS the Engineering console set an allocation key by key and showed committed against available

scripts/s2-check.sh --map tour/deck04
  -> PASS the reloaded ship matches the saved one, system by system;
     PASS alert, damage and the reordered priority all took effect and persisted;
     PASS the Engineering console, operated key by key with the game running, changed the ship
```

## What could not be verified

- **Whether a meeting is any good to sit in** — whether the officers' competing asks read as a room rather
  than a menu. The brief reserves that for the owner, at the walkthrough. This lane only proves the brief
  exists, the skeleton plays, the lines carry direction, and the decisions apply.
- **Nothing else**, for phase one. The rendering of the overlay, the async worker, novelty detection, and
  the audio are phase two and three and were deliberately not begun.

## Judgement calls, named as calls

1. **The meeting model lives in `ship_core.{h,cpp}`**, not a new file, because the module build discovers
   our sources with a configure-time glob (`file(GLOB_RECURSE LWH_MODULE_SRC ...)`): a new `.cpp` would not
   be compiled without re-running cmake, and the save format lives beside the state. This is the same file
   the power assignment used, and the same "one model" shape.
2. **The delivery vocabulary and its values are ours.** The review names the knob (`exaggeration`) and one
   value (0.8 for urgency); the other four values and the names of the directions are a small, documented
   choice, recorded here so they can be overruled.
3. **`DELIVERY_UNMARKED` is a value, not a missing field.** A line always carries a direction; where the
   brief cannot know it, the direction says so and the seam refuses the line, rather than guessing flat.
   This is the review's own point (the default 0.5 is the defect) taken to its conclusion.
4. **The participants are chosen per kind, deterministically** — the player and whoever commands, plus the
   department heads the meeting needs (engineering for allocation, sciences for dilithium, medical for
   casualties, security and engineering for Borg), the parties to a promise for a deferred decision, and
   the most-manned department's officers for the routine departmental meeting. The design says "the officer
   posts the roster puts in the room"; this is the mapping, and it is ours.
5. **The threshold kinds are latched, and the latch releases.** One episode, one meeting; when the fact that
   called it passes, the next episode can call another. Without the release, a single long shortage would
   either never call a second meeting or call one every tick.
6. **A brief carries the log a participant can read, but no decision consults it.** The design and
   `docs/the-record-and-the-log.md` ask for a brief built per participant from marks *and the log*; the law
   that the simulation writes the logs and never reads them governs *decisions*, and no decision path
   consults either store. `BuildBrief` is const and the ship's blob is unchanged by it.
7. **The queued brief is persisted in full.** The design says the simulation enqueues a brief before the
   player arrives; a save taken between scheduling and the meeting must not lose it, so the brief and its
   schedule are in the save. The cost is the version bump to 52, named above.
8. **One allocation effect per option, up to two systems.** The proving option sets two (the holodecks full,
   the shields dark). Decreases are applied before increases so an increase is measured against the power
   freed.
