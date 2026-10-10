# The meeting has a place — the room is on the brief, derived from the kind and the subject

`docs/staff-meetings.md` (the owner's ruling, 2026-10-09), `docs/ship-master-map.md` (which deck hosts
what), `docs/handoff-live-crew-and-meetings.md` (M2/M3) and `docs/evidence/meeting-overlay.md` (the
overlay this phase edits), applied on `feat/the-meeting-place` (cut from `testing`), 2026-10-10.

**The premise, measured.** `MeetingBrief` (`module/ship/ship_core.h`) carried the kind, trigger,
decision, participants, options, instruments, systems and digest — **and no place.** The overlay drew
wherever the player was standing, and the only "room" it knew was the participants list, which it
labelled "IN THE ROOM". The rooms themselves are real and mapped. This phase adds the missing binding:
the place **belongs to the brief, is known before the meeting, and is what makes the asynchronous
render window meaningful.**

**Observed, and the command that produced it.** Everything below is the output of a command named
beside it, on this tree, after `cmake -S ../upstream/efgame -B ../upstream/efgame/build-linux
-DLWH_MODULE_DIR=$PWD/module && cmake --build ../upstream/efgame/build-linux --target efgame --target
efui` (which built both modules with only the pre-existing `-Wwrite-strings` warnings). No model is
called, no audio is opened or rendered, and no asset is added.

## What was built, and where

| file | what it is |
|---|---|
| `module/ship/meeting_rooms.def` | **new**: the one place a program reads the mapping — a row per (kind, subject), naming the room and the deck. |
| `module/ship/ship_core.{h,cpp}` | `struct MeetingRoom` and `MeetingRoomCount/At/For`, `MeetingSubjectDepartment`; `MeetingBrief` gains `subject` and `room`; `BuildBrief` resolves the place at generation; the emit site logs it; `SAVE_VERSION` 54 → 55. |
| `module/ui/ui_lwh_meeting.cpp` | the overlay names the place where the participants line was: `IN THE SECURITY OFFICE:`, not `IN THE ROOM:`. The queue gains a `ROOM` column. |
| `module/ship/g_ship.cpp` | publishes `lwh_ship_meeting_room` / `lwh_ship_meeting_deck`; the `ship meeting open` line names the room; the `g_shipTest 91` demo. |
| `tools/meetings/rooms.py` | **new**: the table check — fails when a kind of meeting has no room. |
| `scripts/meeting-place-check.sh` | **new**: the table check and one headless engine run. |
| `tests/ship/test_ship_core.cpp` | `TestMeetingPlace`: the mapping, the subject, and the save round-trip. |
| `tests/tools/test_meeting_rooms.py` | **new**: the check fails for a homeless kind, and passes on the tree. |

`module/ship/meeting_rooms.def` is included (X-macro) by `ship_core.cpp`, so the module and the check
read the same rows:

```
$ python3 tools/meetings/rooms.py list
MEET_WATCH_CHANGE  DEPT_COMMAND      the briefing room    1
MEET_DEPARTMENTAL  DEPT_COMMAND      the ready room       1
MEET_DEPARTMENTAL  DEPT_ENGINEERING  main engineering     11
MEET_DEPARTMENTAL  DEPT_SECURITY     the security office  6
MEET_DEPARTMENTAL  DEPT_SCIENCES     astrometrics         8
MEET_DEPARTMENTAL  DEPT_MEDICAL      sickbay              5
MEET_ALLOCATION    DEPT_ENGINEERING  main engineering     11
MEET_DILITHIUM     DEPT_ENGINEERING  main engineering     11
MEET_CASUALTIES    DEPT_MEDICAL      sickbay              5
MEET_BORG          DEPT_SECURITY     the security office  6
MEET_DEFERRED      DEPT_COMMAND      the ready room       1
MEET_COMPUTER      DEPT_COMMAND      the bridge           1
```

Every room is one the master map names (`docs/ship-master-map.md`): the briefing room and the ready
room on deck 1, sickbay on deck 5, the security office by the brig on deck 6, the bridge on deck 1,
astrometrics on deck 8, main engineering on deck 11. No room is invented.

## Task A — the place is part of the brief, and every kind has one

The room is derived from the meeting's **kind** and its **subject** — the department whose business it
is (`MeetingSubjectDepartment`). The subject is fixed by the kind for most meetings (allocation and
dilithium are engineering's, casualties sickbay's, a Borg matter security's) and is the busiest
department for the departmental meeting. `BuildBrief` resolves the room and writes its index on the
brief:

```
$ python3 tools/meetings/rooms.py check
rooms.py: module/ship/ship_core.h names 8 kind(s); module/ship/meeting_rooms.def maps 12 room row(s)
PASS  every meeting kind has a room, and every room row names a known kind, subject and deck
```

**The check fails when a kind has no room.** The Borg row is removed from a copy of the table and the
check is pointed at it (`docs/staff-meetings.md` uses this demonstration, as `scripts/api-check.sh`
does for the console):

```
$ cp module/ship/meeting_rooms.def /tmp/meeting_rooms_missing.def
$ sed -i '/MEETING_ROOM(MEET_BORG/d' /tmp/meeting_rooms_missing.def
$ python3 tools/meetings/rooms.py check --table /tmp/meeting_rooms_missing.def ; echo $?
rooms.py: module/ship/ship_core.h names 8 kind(s); /tmp/meeting_rooms_missing.def maps 11 room row(s)
FAIL  the meeting kind MEET_BORG has no room
1
```

Restoring the row returns exit 0 (the run above). The unit tests prove the same in-process
(`tests/tools/test_meeting_rooms.py::test_fails_when_a_kind_has_no_room`), and `TestMeetingPlace`
proves every kind resolves a room whose deck is one of the fifteen.

## Task B — the meeting happens there, with the people the subject concerns

The room is on the brief, the overlay names it where the participants line was, and the people are
the ones the subject concerns — the allocation meeting's engineering chief, the security matter's
security chief. On the screen, the security matter names **the security office** and the security
chief is present:

```
$ scripts/meeting-place-check.sh        (excerpt)
    meeting place: at the security office: standing at (-4336 -3904 -19308)
    meeting place: open=1
    meeting place: the room is the security office (deck 6); kind Borg
    meeting place: present Crewman 090 (security)
```

The overlay draws `IN THE SECURITY OFFICE:  Reyes (command)  Kathryn Janeway (all scopes)  Crewman 090
(security)  Crewman 036 (engineering)`, in the frame edited, not a new screen (`docs/evidence/
meeting-overlay.md`'s overlay).

## Task C — the render window is the point

Because the room and the people are known **before** the meeting, generation has the window the ruling
describes. The place is on the brief **at generation time**, not at arrival: the emit site logs it when
the brief is enqueued, and the queued brief carries it. The demo, before the player arrives:

```
$ scripts/meeting-place-check.sh        (excerpt)
    meeting place: watch-change (subject command) -> the briefing room, deck 1
    meeting place: Borg (subject security) -> the security office, deck 6
    meeting place: allocation (subject engineering) -> main engineering, deck 11
    meeting place: casualties (subject medical) -> sickbay, deck 5
    meeting place: departmental (subject engineering) -> main engineering, deck 11
    meeting place: 1 brief queued before the player arrives
    meeting place:   Borg in the security office (deck 6), present 4
    meeting place: the log at generation: a Borg meeting is called in the security office (deck 6): how to meet the Borg pressure
```

The room is on the queued brief, and it is written into the ship's own log at the emit site (scope
`command`, authored at `the bridge`); it is not discovered on entry. `TestMeetingPlace` shows the
queued brief's `room` does not move across a later tick, and that a save replays it.

**Two kinds, two rooms — and a third.** The demo shows a command decision names the briefing room
(deck 1), a security matter the security office (deck 6), and an engineering matter main engineering
(deck 11). A medical question names sickbay (deck 5).

**Derived, not hand-set.** The room comes from the kind *and* the subject, computed from the ship
rather than carried in by hand. `test_ship_core --place` prints the mapping; the departmental meeting
is the one kind with more than one subject, so two briefs of the same kind name two rooms:

```
$ tests/ship/test_ship_core --place
== every kind of meeting resolves a room, from the kind and the subject
  watch-change  subject command     -> the briefing room  deck 1
  departmental  subject engineering -> main engineering   deck 11
  allocation    subject engineering -> main engineering   deck 11
  dilithium     subject engineering -> main engineering   deck 11
  casualties    subject medical     -> sickbay            deck 5
  Borg          subject security    -> the security office deck 6
  deferred      subject command     -> the ready room     deck 1
  computer      subject command     -> the bridge         deck 1
== the departmental meeting is the one kind with more than one subject
  MEET_DEPARTMENTAL / DEPT_ENGINEERING -> main engineering (deck 11)
  MEET_DEPARTMENTAL / DEPT_MEDICAL     -> sickbay (deck 5)
```

For the fixed-subject kinds the mapping is single-valued by kind (allocation and dilithium are
engineering's; casualties medical's; Borg security's; watch-change, deferred and computer command's),
and that is said plainly rather than dressed as multi-valued. The departmental row above shows the
subject varying: the same kind, two subjects, two rooms.

## The pictures

The player stands **in the named room**, and the frame names it — photographed, because whether the
room reads as the *right* room is the owner's judgement:

```
$ scripts/meeting-place-check.sh
    screenshots: build/g3-home/baseEF/screenshots/lwh_meeting_place.tga   (the security office, the
                overlay naming IN THE SECURITY OFFICE, the security chief's rail)
                 lwh_meeting_place_a.tga, lwh_meeting_place_b.tga          (the room itself, two angles)
```

`lwh_meeting_place.tga` is the delivered frame: the lower-quarter overlay reads **IN THE SECURITY
OFFICE** with the security chief (Crewman 090, PHASERS) on the rail. The two room-only frames are the
same post without the overlay, so the owner can judge the room apart from the transcript.

## The checks, and what they returned

```
scripts/test.sh
  -> all checks passed (crew, ship, tools, hooks, api, harness, docs, python, shellcheck, patches)

scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
  -> exit 0 (the entity dictionary, the validator negatives, the compiler corpus pass)

scripts/meeting-place-check.sh
  -> PASS  every kind of meeting has a room; a security matter named the security office with the
     chief present; the room was on the brief before the player arrived; the frame is photographed

scripts/meeting-check.sh        -> PASS  a brief is emitted at the watch change; a person set an
                                   allocation end to end; the ship's own set was refused
scripts/meeting-overlay-check.sh-> PASS  the queue is reachable in play; a screen key opened a brief;
                                   a pill applied one outcome with the player's provenance; typed text
                                   reached the novelty seam and changed nothing; the meeting reached
                                   its end
scripts/model-call-check.sh     -> PASS  the script is waiting before the player arrives; one novel
                                   input makes exactly one call and is promoted; a reload replays the
                                   promoted branch with no call; with the model absent the meeting plays
                                   from the skeleton
scripts/audio-check.sh          -> PASS  the delivery direction reaches synthesizer --exaggeration; no
                                   audio is committed; a line's measured duration paces the pill
scripts/api-check.sh            -> PASS  every console verb is in the ship's API, and every API verb is
                                   answered by the console
scripts/voice-in-check.sh       -> PASS  the transcribed string takes the identical path; no microphone
                                   was built
scripts/appearance-check.sh     -> PASS  the face is derived from the record, never stored
```

## What could not be verified

- **Whether the room *feels* like the right room for the subject.** The frames show the security office
  on deck 6 with the overlay naming it; whether that reads as security, and whether the briefing room
  or main engineering would read better for their subjects, is the owner's, and it needs the pictures.
- **The overlays' brightness behind the open room.** The menu path renders the scene dark behind the
  band; the room-only frames (`lwh_meeting_place_a/b`) carry the room's own light and are included for
  that reason. This is the existing overlay path's behaviour, not a change here.

## Judgement calls, named as calls

1. **The room name is stored on the brief as a row index, not a string.** `MeetingBrief.room` is an
   index into the table; `MeetingRoomAt(room)` is the name and deck. This keeps the saved brief small
   and free of pointers, and lets the save reject an index that is out of range. `SAVE_VERSION` is 55,
   with the changelog naming what it carries; the queued brief round-trips byte-for-byte.
2. **The subject is a department.** "The subject" is read as *the department whose business it is*, so
   the mapping is `(kind, department) -> room`. The departmental meeting is the one kind whose subject
   varies with the ship; the rest are single-valued by kind, and that is stated rather than implied.
3. **`MEET_DEFERRED` and `MEET_COMPUTER` are given the captain's rooms** — the ready room and the
   bridge — because both are the command's own business and the map names neither a "promise room" nor
   a "computer room". Ours and overrulable.
4. **`MEET_DEPARTMENTAL` for the sciences meets in astrometrics** (deck 8), the map's science space;
   the map also names a science lab there. Either is the map's, and this is the one chosen.
5. **`scripts/meeting-check.sh` was not executable in the tree** (mode 100644, unlike its siblings),
   so the acceptance "the meeting ... check exits 0" could not be met by running it. Its mode is
   restored to 100755. Named because it is a change beyond the place.
6. **The room check is a separate script** (`meeting-place-check.sh`) rather than folded into
   `api-check.sh`, because the console API and the room table are different enumerations with different
   owners. Both follow the same pattern (a `.def` a program reads, a Python check, a planted failure).
