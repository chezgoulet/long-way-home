# The meeting overlay — the surface a meeting is attended on (M3)

`docs/staff-meetings.md` (the owner's design), `docs/programme-meetings-and-voice.md` (the phase it
sits in), `docs/handoff-live-crew-and-meetings.md` (M3), and `docs/art/lcars-meeting-overlay.png` (the
storyboard), applied on `feat/the-meeting-overlay` (cut from `testing`), 2026-10-08.

**Observed, and the command that produced it.** Everything below is the output of a command named
beside it, on this tree, after `cmake -S ../upstream/efgame -B ../upstream/efgame/build-linux
-DLWH_MODULE_DIR=$PWD/module && cmake --build ... --target efgame --target efui` (which built both
modules with only the pre-existing `-Wwrite-strings` warnings). This phase is **the screen and the way
in**: no model is called, no audio is opened or rendered, and no asset is added. M4, M5 and M6 are
deliberately not built.

## What was built, and where

| file | what it is |
|---|---|
| `module/ui/ui_lwh_meeting.cpp` | **new**: the queue and the room — the overlay, drawn on the panel path the other screens use. |
| `module/ui/lwh_ui.h` | the one name the patched upstream UI calls (`LWH_UI_MeetingScreens`), beside the other screens. |
| `module/ui/ui_lwh_command.cpp` | the way in: `M` at the command console opens the meetings queue, and the footer says so. |
| `module/ui/ui_lwh_engineering.cpp` | the console dispatch calls the meeting screens as it calls the rest. |
| `module/ship/ship_core.{h,cpp}` | the novelty seam (`ClassifyNovelInput`) and the scope read (`LogScopeForCrew`). |
| `module/ship/g_ship.cpp` | the cvar publication the screen reads, the `ship meeting` verbs (`open`, `close`, `choose`, `say`), and the `g_shipTest 82` demo. |
| `tests/ship/test_ship_core.cpp` | `TestMeetingOverlaySeam`: the surface's semantics without a screen. |
| `scripts/meeting-overlay-check.sh` | **new**: one headless run of the whole path. |

The overlay is a **new `.cpp` under `module/ui/`**, so cmake was re-run to pick it up (the
configure-time glob does not see a new file otherwise), and `nm` confirms the symbol is in
`libefui.so`:

```
$ nm /home/c/big/git/upstream/efgame/build-linux/libefui.so | grep MeetingScreens
000000000004fab0 t _Z21LWH_UI_MeetingScreensPKc
$ nm /home/c/big/git/upstream/efgame/build-linux/libefgame.so | grep -E 'ClassifyNovelInput|LogScopeForCrew'
0000000000151090 t _ZN4ship15LogScopeForCrewERKNS_4ShipEi
0000000000151180 t _ZN4ship18ClassifyNovelInputERKNS_4ShipERKNS_12MeetingBriefERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE
```

**The build is not the proof; the screen is.** The frame below is a render, not a build log.

## Task A — the overlay

The frame is the storyboard's: a **speaker rail** on the left (name, post, watch, mood), the **line
being answered**, the **enumerated options as pills** with a short description (their cost), and a
**free-text pill carrying a `VOICE` affordance** drawn even though voice does not exist. It sits in a
**144px band at the bottom of the 640x480 virtual frame** — 30%, the lower part of the frame, the
storyboard's lower quarter once the scene above is counted — and it **paints nothing above `y=336`**,
so the scene stays visible. The `look` is the owner's to judge; two frames are captured.

```
$ scripts/meeting-overlay-check.sh --map tour/deck04
      screenshots: .../screenshots/lwh_meeting_queue.tga, .../screenshots/lwh_meeting.tga
```

- **the queue** (`lwh_meeting_queue.tga`): `STAFF MEETINGS - THE QUEUE`, the count (`1 WAITING. THE
  ROOM TAKES THE OLDEST.`), `KIND` and `DECIDING` columns, the pending brief selected, and the keys.
- **the room** (`lwh_meeting.tga`): the rail (`Crewman 039 / STRUCTURAL INTEGRITY`, `WATCH BETA`,
  `MOOD FIT`), the line (`The plan holds every system the plant can feed.`), four pills each with its
  label and cost, the free-text pill with `VOICE`, and `IN THE ROOM: Reyes (command) Kathryn Janeway
  (all scopes) Crewman 039 (engineering)`.

The delivery direction is **read into the line struct and never drawn** (Task C). It is not shown on
the frame; the seam to synthesis owns it.

## Task B — the way in, and it is three hooks

`PendingMeetings`, `TakeBrief` and `ApplyMeetingOutcome` were reachable only from the developer
console. They now have a station surface: **`M` at the command console opens the queue** (and the
command console's own footer names it). From there the keys are the surface's own:

```
UP/DOWN meeting   ENTER open it   ESC leave                       (the queue)
UP/DOWN  number  ENTER speak  T type an answer  R replay  ESC queue   (the room)
```

`ENTER` on the queue calls `TakeBrief` (the room takes the oldest brief); the room publishes
`PendingMeetings` for the queue; a pill calls `ApplyMeetingOutcome`. The register moves all three from
**console** to **play**.

**A post sees its own scope, command sees all.** Each participant is shown with the scope their own
brief was built from (`LogScopeForCrew`): `Reyes (command)` and `Crewman 039 (engineering)` above
versus `Kathryn Janeway (all scopes)`. The unit test proves the contents behind it
(`TestMeetingBriefPerParticipant`): an engineering participant holds the engineering log entry and not
the sickbay one.

## Task C — the meeting plays, on screen

**The screen never writes ship state.** It reads the ship's published cvars (`lwh_ship_meeting_*`) and
sends `ship meeting` commands; the simulation takes the brief and applies the outcome. The room's
lines are the authored skeleton's, published per outcome and per line with the resolved speaker and the
delivery; a pill plays its outcome's lines and then sends `ship meeting choose`.

Drive the allocation seam end to end, from a pill:

```
$ scripts/meeting-overlay-check.sh --map tour/deck04
    overlay test: the queue is reachable in play and lists 1 pending meeting(s)
    overlay test:   allocation: where the ship's power goes
    overlay test: meeting open=1, kind allocation
    overlay test: speaker Crewman 039; line "The plan holds every system the plant can feed."; delivery report (carried, not rendered)
    overlay test: pill 1: The chief's plan [adopted as set]
    overlay test: pill 2: Refuse the chief [he notes who refused, and the argument stands]
    overlay test: pill 3: Run the ship's own ladder [automatic mode: the ship decides the unset systems]
    overlay test: pill 4: The holodecks full, the shields dark [no shields while the holodecks run]
    overlay test: after the pill, holodecks 100% shields 0% provenance the player
    meeting open: the room takes the allocation brief: where the ship's power goes (present 3, options 4)
    meeting choose: allocation outcome 3 applied (decided by Reyes)
```

And the ship's own report shows the change with **a person's provenance, not automatic mode**
(`build/g3-home/baseEF/ship/meeting-overlay.txt`):

```
  holodecks                deck  6  alloc 100%  power  60/ 60  output 100%  manned 0/0  health 100%  the player
  shields                  deck  1  alloc   0%  power   0/200  output   0%  manned 1/1  health 100%  the player
```

**Typed text routes to the novelty seam and cannot become a branch by accident.** The free-text pill
collects characters and submits them with `ship meeting say`; the ship calls `ClassifyNovelInput`,
which — with the embedding classifier absent — reports **novel** every time and applies nothing. The
same input twice, through the screen, is the demonstration:

```
    meeting say: "Make it so": no branch matched (novel)
    meeting say: novel: no branch descriptor matched (the embedding classifier is M4); one live call would be made and its answer written to the log. The simulation applies nothing.
    meeting say: "Make it so": no branch matched (novel)
    meeting say: novel: no branch descriptor matched (the embedding classifier is M4); one live call would be made and its answer written to the log. The simulation applies nothing.
    overlay test: allocation unchanged by the typed text: 100 (was 100)
```

**What the novelty seam would need.** An embedding classifier: a small always-resident local embedding
model that maps the typed text and each option's `intent` descriptor into one space, and a tuned
threshold. Above it the branch plays; below it, exactly one live call is made, its answer written to
the log and promoted back into the script (M4). Until then the seam's answer is always *novel*, which
is the safe and honest result: reporting a match would let a typed string pick a branch.

## No model, and arriving calls nothing

There is no model call anywhere in the module: the meeting plays from `AuthoredSkeleton`, which is
static data. `g_shipTest 82` sets no model cvar and reaches the meeting's end (the decision and the
report file are written). The check greps the run for the decision and the record, and the run calls
no synthesis and opens no device. "How it is disabled" is that **it does not exist in this pass** —
there is nothing to disable.

## The register

`docs/hook-register.md`: the three hooks move from *console* to *play*, and two hooks are added
(`LogScopeForCrew`, `ClassifyNovelInput`). The counts are re-derived from the register's own rows, not
carried over: **73 + 57 + 54 + 166 + 18 = 368**, and **211 + 122 + 35 = 368**. `docs/hook-triage.md`
records the same move (three situations leave the genuine-gap list, 62 → 59). `scripts/hooks-check.sh`
passes: every public function in `ship_core.h` is registered and every registered hook exists.

## The commands, and what they returned

```
scripts/meeting-overlay-check.sh --map tour/deck04
  -> PASS  the queue is reachable in play; a screen key opened a brief; a pill applied one outcome
     with the player's provenance; typed text reached the novelty seam and changed nothing; the
     meeting reached its end

test_ship_core            (the meeting overlay seam)
  -> ship_core: all checks passed   (including TestMeetingOverlaySeam)

scripts/test.sh           -> all checks passed
scripts/check.sh          -> entity dictionary and the script corpus as before
```

*(The `scripts/test.sh` and `scripts/check.sh` lines are re-run on the finished tree; their full
output is the house's usual check sweep.)*

## What could not be verified

- **Whether the overlay reads as a room rather than a menu**, and whether the pills feel like a
  conversation. That is the owner's, and it needs the picture above. The screen draws; whether it is
  *good* is not a thing this lane can decide.
- **Nothing else in this pass.** M4 (the async generator and the classifier), M5 (voice out) and M6
  (voice in) are named and left; the casting map is untouched.

## Judgement calls, named as calls

1. **The overlay is 144px (30%) of the frame, not the storyboard's exact 26.7%.** At 640x480 four
   pills, an answer line, a free-text pill and the rail do not fit legibly in 128px. The band is the
   lower part of the frame and the scene is untouched above `y=336`; this is ours and is overrulable.
2. **The line being answered is the highlighted option's opening line**, so the rail shows who is
   advocating the option under the cursor; picking an option is what plays the full branch. The
   skeleton has no explicit "question" line, so this is the honest reading of the authored data.
3. **The decision is applied after the branch plays**, on the frame clock (650ms a line), not at the
   key press. A pill is a choice, and the lines are the outcome's dialogue; the check waits for the
   apply. `R` replays the branch, which does not re-apply.
4. **Typed text is collected by the screen itself** (the engine delivers character keys with
   `K_CHAR_FLAG`), not by a `menufield_s`; the test types through the same buffer
   (`lwh_meet_type`), so what a test drives is what a hand drives.
5. **The held brief is host-local, not saved.** `open` takes the brief off the persisted queue
   (`TakeBrief`) and holds the copy in `g_ship`; a save taken *while the room is open* would reload
   without the room. The ship's state is unaffected (nothing is applied until a pill), so saves still
   replay identically; a room lost to a reload is a display matter, not a determinism one. Named here
   so it can be overruled.
6. **The pill labels are clipped** (30 characters, description 46) so the label cannot run into its
   description in the fixed columns. The authored labels are whole sentences; the clipping is the
   frame's, and the full text stays in the brief.
