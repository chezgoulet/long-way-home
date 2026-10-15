# The room reads — four posts moved to the fixture their room is named for

> **Corrected 2026-10-15 (docs/evidence/the-two-way-fixes.md).** The frame measure this document names, `tools/meetings/legibility.py`, no longer exists: it was generalised into `tools/frames/frame.py` (`docs/frame-assertions.md`: *"the old `tools/meetings/legibility.py` is deleted: one file, one rule"*). The citations below are corrected to the surviving path; the figures are this pass's own and are unchanged.

The premise was measured, not re-derived (the room-posts pass, `docs/evidence/the-room-posts.md`):
legibility was **done** — all seven posts measured legible and `scripts/room-survey-check.sh` failed
when one did not — but **the frame did not read as the room it was named for**. Three rooms read (the
bridge, sickbay, astrometrics); four did not, and their closing table was this pass's work list.

This pass moved the four: each post now stands **inside its room**, turned at the fixture that names
the room, chosen by the frame (the candidate tool `g_shipTest 96`, re-anchoring without rebuilding).
It also closed the anchor trap (Task B): an anchor the harness cannot honour was **rejected loudly**
instead of being used as if it were absent. Legibility is unchanged and still asserted.

Applied on `feat/the-room-reads`, cut from `testing`, 2026-10-10. Everything below is the output of
the command named beside it, on this tree, with the merged ship already built
(`scripts/build-ship.sh`). No model is called, no audio is rendered, and no asset is added.

## Task A — the four posts, moved and chosen by the frame

The candidate tool is `g_shipTest 96`: stand the player at `g_shipTestPos` and photograph four
facings there plus one each a pace west and east (index: 0 north/yaw 90, 1 south/270, 2 east/0,
3 west/180, 4 west-east at x−192/yaw 0, 5 east-west at x+192/yaw 180). The chosen post is recorded
once, in the harness (`module/ship/g_ship.cpp`, `g_shipTest 97`), and the survey photographs exactly
it. The four candidates, by the frame:

```
$ SDL_AUDIODRIVER=dummy xvfb-run -a scripts/run-engine.sh --home-dir build/g3-home \
    +set s_useOpenAL 0 +set g_ship 1 +set g_shipMode 1 +set g_shipDeckPitch 3072 \
    +set g_shipTest 96 +set g_shipTestPos "<candidate>" +map voyager
$ python3 tools/frames/frame.py build/g3-home/baseEF/screenshots/lwh_darkroom_security_*.tga

briefing-room  anchor (-3082 -3320 -4013): frame 1 (post-south, yaw 270)  median 43  p90 255  LEGIBLE
ready-room     anchor (-3970 -3360 -4013): frame 5 (east-west, yaw 180)   median 40  p90 255  LEGIBLE
engineering    anchor (-3206 -3220 -34688): frame 0 (post-north, yaw 90)  median 58  p90 255  LEGIBLE
security-office anchor (-4210 -3990 -19332): frame 0 (post-north, yaw 90) median 75  p90 124  LEGIBLE
```

The four posts in the harness after the move, and the frame each now produces (`g_shipTest 97`):

```
$ scripts/room-survey-check.sh          (excerpt: the survey's own lines and verdicts)
    room post: briefing-room at (-3082 -3320 -3989) facing 270
    room post: ready-room at (-3778 -3360 -3989) facing 180
    room post: bridge at (-3450 -3435 -3971) facing 90
    room post: sickbay at (-3252 -3136 -16272) facing 270
    room post: astrometrics at (-1672 -3040 -25224) facing 0
    room post: engineering at (-3206 -3220 -34664) facing 90
    room post: security-office at (-4210 -3990 -19308) facing 90
    room post: 7 room(s) photographed

lwh_roompost_briefing-room.tga                 mean  69.2 median  42.0 p90 255.0  LEGIBLE
lwh_roompost_ready-room.tga                    mean  78.8 median  40.0 p90 255.0  LEGIBLE
lwh_roompost_engineering.tga                   mean  97.1 median  58.0 p90 253.0  LEGIBLE
lwh_roompost_security-office.tga               mean  87.5 median  75.0 p90 128.0  LEGIBLE
```

One line per moved room — what the frame shows against what the room is called (Task C: whether a
room *reads* is the owner's eye; this is the honest description he judges from):

| room | post frame | measured | what the frame shows |
|---|---|---|---|
| the briefing room | `lwh_roompost_briefing-room.tga` | mean 69.2, median 42, p90 255 | the conference table with its chairs, the lit ceiling, and the observation windows. **Reads as the briefing room.** |
| the ready room | `lwh_roompost_ready-room.tga` | mean 78.8, median 40, p90 255 | the captain's curved desk with its monitor and chair, guest chairs, the wood-panelled wall. **Reads as the ready room.** |
| main engineering | `lwh_roompost_engineering.tga` | mean 97.1, median 58, p90 253 | the warp core — the lit column between its rails — with the consoles and a figure at the rail. **Reads as main engineering.** |
| the security office | `lwh_roompost_security-office.tga` | mean 87.5, median 75, p90 128 | an armory bay: a locker bank, a workstation counter, grating floor, a door at the bay's end. **Reads as an armory, not as an office** (see the finding below). |

The frames are `build/g3-home/baseEF/screenshots/lwh_roompost_<room>.tga`.

**The `briefing room` post was in the right room and facing the wrong way.** Before the move it stood
at `(-3080 -3440)`, which is the conference table model's own origin (`models/mapobjects/confroom/conftable`,
`deck01.map`), facing yaw 90 — so the camera stood *on* the table looking at the wall behind it, and
the table fell below the view. The fix is a facing: stand north of the table and look south across it
to the windows. It was not a post in a corridor.

**`main engineering`'s post was in the wrong part of its own deck.** The old point (`-3806 -3996`)
measured a louvered plant wall and a closed bulkhead door; the warp core (`func_usable` `warpcore`,
`deck11.map`) stands at about `(-3206 -3085)`. The new post stands south of the core and looks north
at it.

**The security office is a finding, not a fix (Task A's "no fixture" case).** Deck 6 *composes* its
geometry from three source maps (`tools/shipmap/dressdeck06.py`); the security zone is a slice of
`_brig`, and the slice carries `_brig`'s **locker bank and grating**, not the security office that
sits beside the brig in that map (the only `models/mapobjects/cargo/control_stand` is at `(26 10)` in
`_brig`'s own frame — outside the copied slice, `x -320..192, y -460..-124`). So the
room has **no desk and no console fixture** to face, and no post can make it read as an office. The
post was moved off the locker bank at the door to the bay's own floor (`-4210 -3990`), looking down
the bay past the lockers to the workstation counter: **this reads as an armory, which is the security
space on this deck, but it is not an office.** Report, not a licence: no art was added and no room
was re-dressed.

## Task B — a bad or out-of-bounds anchor now reports itself and fails

The trap (named in `docs/evidence/the-room-posts.md`): an anchor the harness did not honour was
**used as if it were absent** — the run quietly stood at the table's own position and printed it, so
a planted, wrong anchor looked like a passing check. That is now closed in `RoomPostAnchor`
(`module/ship/g_ship.cpp`), used by both `g_shipTest 96` and `97`: an anchor that is not three
numbers, or whose point has no world geometry beneath it (a downward trace finds no floor and does
not start in a solid — the point is in the void), is **rejected loudly** and the survey does not run.
An anchor *inside a solid* is still honoured, because standing in a wall is a **frame** defect for the
legibility assertion to catch, not a format error.

A malformed anchor, and its output:

```
$ LWH_ROOM_POST_ANCHOR="not a coordinate" scripts/room-survey-check.sh         (excerpt)
==> planted: the security office's post anchored at not a coordinate (g_shipTestPos)
==> every meeting room, stood at its post (g_shipTest 97)
    room post: BAD ANCHOR "not a coordinate" -- not three numbers; refusing to stand anywhere
    room post: 0 room(s) photographed -- the anchor was rejected
FAIL  the planted anchor was rejected by the harness (above); the survey did not run
$ echo $?
1
```

An out-of-bounds anchor, and its output:

```
$ LWH_ROOM_POST_ANCHOR="100000 100000 100000" scripts/room-survey-check.sh       (excerpt)
==> planted: the security office's post anchored at 100000 100000 100000 (g_shipTestPos)
    room post: ANCHOR OUT OF BOUNDS "100000 100000 100000" -- no world geometry beneath it; refusing to stand there
    room post: 0 room(s) photographed -- the anchor was rejected
FAIL  the planted anchor was rejected by the harness (above); the survey did not run
$ echo $?
1
```

`scripts/room-survey-check.sh` also fails if an anchor was given but the harness neither rejected it
nor printed that it honoured it — so a hook that stops taking effect cannot pass as a no-op.

## Task C — the legibility assertion is intact, and still fails on a bad post

Legibility thresholds are unchanged (`median >= 35`, `p90 >= 80`, on the brightest channel, in the
middle band). A planted post that is *honoured* but stands in the composition's own geometry still
renders dark and the survey still fails, naming the room:

```
$ LWH_ROOM_POST_ANCHOR="-4344 -3965 -19332" scripts/room-survey-check.sh         (excerpt)
    room post: the security office anchored at (-4344 -3965 -19332) by g_shipTestPos
    room post: security-office at (-4344 -3965 -19308) facing 90 (anchored)
    room post: 7 room(s) photographed
...
lwh_roompost_security-office.tga               mean  64.8 median  62.0 p90  68.0  DARK
1 frame(s) dark: lwh_roompost_security-office.tga
==> every meeting room's post must read (the assertion this pass adds)
FAIL  the security-office post renders dark -- the room a meeting would use has no legible post
$ echo $?
1
```

The frame is above; the `median 62, p90 68` is the same "camera inside the composition's geometry"
signature the dark-room pass measured.

## The checks

```
scripts/room-survey-check.sh    -> PASS; one frame per room, every post legible
scripts/meeting-place-check.sh  -> PASS; the security office reads (lwh_meeting_place_a median 87 p90 122)
scripts/deck06-check.sh         -> PASS; the deck 6 composition loads, is reachable on foot, photographed
scripts/harness-check.sh        -> PASS; 81 harness case numbers, all distinct
scripts/test.sh                 -> all checks passed (crew, ship, tools, hooks, api, orders,
                                   harness, docs, python, shellcheck, patches)
scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
                                -> exit 0 (the entity dictionary, the validator negatives, the
                                   compiler corpus pass)
```

## What could not be verified

- **Whether each room *reads* as its name.** That is the owner's eye, and the one-line table above is
  what he judges it from. The three that read are named; the fourth is reported, not dressed.
- **The security office has no fixture to fix.** The finding above is that the composition has no
  office desk; if the owner wants one, that is new art or a re-dress of the copied slice — both out of
  this pass by its own terms.
- **The meeting's own walk-through still stands at the composition's `lwh_armory_post`**
  (`-4172 -3906`, the east aisle facing the lockers). The room *post* is what this pass owns and has
  moved; the map entity, and `g_shipTest 91`'s meeting frame, are the composition's and the meeting
  system's business. The meeting frame stays legible (`median 87`, `p90 122`).

## Judgement calls, named as calls

1. **The four posts live in the harness, and only there.** Where a post stands is the frame the
   survey photographs, so it stays beside the room list in `g_shipTest 97` rather than in
   `meeting_rooms.def`, which owns *which room*, not *where in it* (the previous pass's call, kept).
2. **A post is chosen by the frame, and the frame is the deliverable.** No metric for "reads as its
   room" is invented; the numbers reported are legibility, and the reading is the one-line table.
3. **The security office is reported, not fixed.** The frame is moved to the room's own floor, which
   is the one part of Task A a post can do; the missing fixture is named as a finding.
4. **"Out of bounds" is a point the player cannot stand on**, not a point inside a solid. The test is
   a short downward trace (`RoomPostAnchor`); a point in a wall still starts inside the trace and is
   honoured so the legibility assertion — not the format check — is what catches a bad *post*.
5. **The candidate tool `g_shipTest 96` gained the same rejection** as the survey, so the instrument
   used to choose a post cannot itself be fooled by a silently-dropped anchor.
6. **The three rooms that already read were left alone** (the bridge, sickbay, astrometrics): their
   posts and frames are unchanged, and the survey reports them beside the four.
