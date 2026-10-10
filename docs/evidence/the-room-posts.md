# The room posts — one legible post per meeting room, and the check that guards them

The owner's ruling (docs/evidence/the-meeting-place.md, 2026-10-10): *"if a room is pitch black it
needs its own light."* The dark-room pass (docs/evidence/the-dark-room.md) found the security office's
fault — the meeting's post stood **inside** the copied armory geometry — and fixed it. It left a
second finding: the room survey stood at each room's own point and turned to four facings, and **ten
of those frames measured dark while the same room read from another facing** — the signature of a
**post** problem. This is the pass that gives every room a post a meeting can use, and makes the
survey's own check **assert** it. Applied on `feat/the-room-posts`, cut from `testing`, 2026-10-10.

Everything below is the output of the command named beside it, on this tree. No model is called, no
audio is rendered, and no asset is added.

## The instrument, and what a "post" is here

A post is a **position and a facing** — where the meeting's camera stands and what it looks at. The
candidate tool is `g_shipTest 96` (the dark-room pass's vantage test): `g_shipTestPos "x y z"` stands
the player at that anchor and photographs four facings there plus one each a pace west and east, so a
post is tried **without rebuilding the map**. A frame is measured by `tools/meetings/legibility.py`:
the middle band of the screen, `median >= 35` and `p90 >= 80` on a pixel's brightest channel.

The premise was re-measured before any post was chosen, because the previous one-frame-per-facing
survey's frames **do not reproduce on a fresh run** at the same coordinates (the harness's view timing
left them showing another frame's view; the dark-room pass had already named that defect for the
meeting frames). The candidate sweep is the measurement this pass acts on. The shape the brief names
held: every room had at least one legible vantage — **except astrometrics, which had none**.

## Task A — one legible post per room, established by measurement

The sweep of all seven rooms, `g_shipTest 96` at each room's own point (index order: 0 north/yaw 90,
1 south/270, 2 east/0, 3 west/180, 4 west-east at x−192/yaw 0, 5 east-west at x+192/yaw 180):

```
$ for room in briefing-room ready-room bridge sickbay astrometrics engineering security-office; do
    xvfb-run -a scripts/run-engine.sh --home-dir build/g3-home ... \
      +set g_shipTest 96 +set g_shipTestPos "<the room's point>" +map voyager
    python3 tools/meetings/legibility.py build/g3-home/baseEF/screenshots/lwh_darkroom_security_*.tga
  done
briefing-room   0 LEGIBLE 55/255   1 DARK 29/57   2 LEGIBLE 39/88   3 LEGIBLE 49/255  4 DARK 37/43  5 DARK 12/51
ready-room      0 LEGIBLE 42/82    1 DARK 37/56   2 LEGIBLE 44/255  3 DARK 45/71      4 DARK  6/52  5 LEGIBLE 49/255
bridge          0 LEGIBLE 72/233   1 DARK 33/193  2 LEGIBLE 52/192  3 LEGIBLE 55/192  4 LEGIBLE 44/192 5 LEGIBLE 50/188
sickbay         0 DARK 48/74       1 LEGIBLE 41/95 2 LEGIBLE 56/82  3 DARK 44/56      4 DARK  0/51  5 LEGIBLE 118/167
astrometrics    0 DARK 18/63       1 DARK 18/49   2 DARK 53/72      3 DARK 55/72      4 DARK  0/0   5 DARK  0/12
engineering     0 LEGIBLE 45/126   1 DARK 62/77   2 LEGIBLE 63/84   3 DARK 59/79      4 DARK  0/39  5 DARK  0/0
security-office 0 LEGIBLE 87/122   1 LEGIBLE 114/139 2 LEGIBLE 119/122 3 LEGIBLE 62/122 4 LEGIBLE 60/226 5 LEGIBLE 89/141
```

Each room's post is one of its **legible** vantages. The post is recorded once, in the harness
(`module/ship/g_ship.cpp`, `g_shipTest 97`), and the survey photographs exactly it:

```
$ scripts/room-survey-check.sh        (excerpt: the survey's own lines)
==> every meeting room, stood at its post (g_shipTest 97)
    room post: briefing-room at (-3080 -3440 -3989) facing 90
    room post: ready-room at (-3950 -3400 -3984) facing 0
    room post: bridge at (-3450 -3435 -3971) facing 90
    room post: sickbay at (-3252 -3136 -16272) facing 270
    room post: astrometrics at (-1672 -3040 -25224) facing 0
    room post: engineering at (-3806 -3996 -34664) facing 90
    room post: security-office at (-4172 -3906 -19308) facing 90
    room post: 7 room(s) photographed
```

**Six rooms were a post.** Choosing the legible facing — and, for sickbay and engineering, choosing a
lit vantage over a bulkhead — is the whole fix; no light was added for them and none was needed.

**One room was a lighting defect, not a post: astrometrics.** Every vantage at its point, and at four
further points on the deck (`t1000`, `d08_t104`, `seven_con1`, `seven_con2`), measured dark — `median
18..55, p90 49..76`, thirty frames with nothing above `p90 76`. The room's own fixtures read `light`
30 in the published source. A lighting fix belongs in `tools/shipmap/` so a rebuild keeps it, so the
fix is `tools/shipmap/lightdeck08.py` (light entities only, no geometry, no art):

```
$ python3 tools/shipmap/lightdeck08.py --deck build/gdk/maps/.../deck08.map --out build/ship/generated \
      --report build/ship/deck08-light.json
deck08 <- build/gdk/maps/.../deck08.map (astrometrics lighting fix)
  astrometrics room, point (-1672, -3040): 5 working light(s) added
  wrote build/ship/generated/deck08.map

$ scripts/build-ship.sh
==> q3map2 -meta
==> q3map2 -vis
==> q3map2 -light
/tmp/.../maps/voyager.bsp  OK   (1/17 lumps empty)
wrote /home/c/big/git/long-way-home/build/ship/out/longway_voyager.pk3
```

The room, re-measured from the same six vantages (the viewer frame is still dark — it is a starfield —
so the post faces a console bank, which is now lit):

```
astrometrics    0 DARK 30/128  1 LEGIBLE 133/138  2 LEGIBLE 131/138  3 LEGIBLE 132/138  4 DARK 0/0  5 DARK 0/27
```

The frames: the **before** frames are kept beside the ship as
`build/g3-home/baseEF/screenshots/lwh_astrometrics_dark_point_<n>.tga` (the point) and
`lwh_astrometrics_dark_{t1000,t104,con1,con2}_<n>.tga` (the four further points); the **after** post is
`lwh_roompost_astrometrics.tga`.

## Task B — the check guards the posts, and can fail

`scripts/room-survey-check.sh` now measures every room's post frame and **fails, naming the room**,
when one is dark — a survey that prints a dark frame and passes is the defect this whole line of work
came out of. It passes on the tree:

```
$ scripts/room-survey-check.sh
lwh_roompost_briefing-room.tga                 mean  72.3 median  55.0 p90 255.0  LEGIBLE
lwh_roompost_ready-room.tga                    mean 113.2 median  44.0 p90 255.0  LEGIBLE
lwh_roompost_bridge.tga                        mean  97.6 median  70.0 p90 234.0  LEGIBLE
lwh_roompost_sickbay.tga                       mean  62.8 median  41.0 p90 108.0  LEGIBLE
lwh_roompost_astrometrics.tga                  mean 109.0 median 130.0 p90 139.0  LEGIBLE
lwh_roompost_engineering.tga                   mean  59.1 median  45.0 p90 127.0  LEGIBLE
lwh_roompost_security-office.tga               mean  93.3 median  88.0 p90 123.0  LEGIBLE
PASS  every meeting room was stood at one post and photographed; every post measures legible;
      the frame and its numbers are above, one line per room
```

**Planted, and caught.** `LWH_ROOM_POST_ANCHOR="x y z"` re-anchors the meeting's own room through
`g_shipTestPos`, the candidate tool, so a post can be put inside a wall without rebuilding the map.
The locker bank is at x −4357..−4258, y −4042..−3888, z −3983..−3904 in the deck's own frame; the
anchor stands the camera among its slats:

```
$ LWH_ROOM_POST_ANCHOR="-4344 -3965 -19332" scripts/room-survey-check.sh     (excerpt)
==> planted: the security office's post anchored at -4344 -3965 -19332 (g_shipTestPos)
    room post: the security office anchored at (-4344 -3965 -19332) by g_shipTestPos
    room post: security-office at (-4344 -3965 -19308) facing 90 (anchored)
lwh_roompost_security-office.tga               mean  64.8 median  62.0 p90  68.0  DARK
FAIL  the security-office post renders dark -- the room a meeting would use has no legible post
$ echo $?
1
```

The anchor is not passed, and the run passes again (the "restored" transcript above) — exit 0. The
`median 62, p90 68` is the same signature the dark-room pass measured for a camera inside the
composition's own geometry.

## Task C — the room reads as its name, or it is reported

Legibility is settled; whether each room **reads** as the room it is named for is the owner's, and
this pass only reports it. One line per room, from the post frame:

| room | what the frame shows |
|---|---|
| the briefing room | a curved wood-panelled wall and a lit ceiling, with a workstation readout — no conference table in view |
| the ready room | a dim panelled antechamber, two doorways and a wall console — no lounge, no couch, no window |
| the bridge | the bridge: the viewscreen bank, the helm console and a figure at the rail |
| sickbay | a medical room: a bio-bed, a wall console and a supply cabinet |
| astrometrics | the astrometrics console bank and its starfield displays, with stools (after the light fix) |
| main engineering | a louvered plant wall and a closed bulkhead door — not the warp core |
| the security office | the armory's louvered locker bank and a handrail |

The frames are `build/g3-home/baseEF/screenshots/lwh_roompost_<room>.tga`. **The briefing room, the
ready room and main engineering are the ones the owner is likely to ask about**: their posts read as
a panelled wall, a dim antechamber and a corridor door, not as the table, the lounge and the core.

**Corrected by the next pass.** That table became the work list for `docs/evidence/the-room-reads.md`
(2026-10-10): the briefing room, the ready room and main engineering were re-aimed at the fixture that
names each room, and the security office's post was moved within its room (and is reported there as an
armory, not an office — the composed deck 6 has no office fixture). The four descriptions above are
this pass's own frames, which the next pass overwrote; they are the *before*, kept as the record of
what the open was.

## The checks

```
scripts/room-survey-check.sh    -> PASS; one frame per room, every post legible
scripts/meeting-place-check.sh  -> PASS; the security office reads (lwh_meeting_place_a median 87 p90 122)
scripts/test.sh                 -> all checks passed (crew, ship, tools, hooks, api, orders,
                                   harness, docs, python, shellcheck, patches)
scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
                                -> exit 0 (the entity dictionary, the validator negatives, the
                                   compiler corpus pass)
```

## What could not be verified

- **Whether a room reads as the room it is named for.** That is the owner's eye, and the one-line
  table above is what he will judge it from.
- **`main engineering`'s post is a corridor door, not the plant.** The deck's own arrival nav goal is
  a corridor; a post at the plant (the warp core) would need the plant's coordinates chosen by eye
  from the `.map`, which is the owner's call, not a measurement. The post is the corridor because
  that is where a legible vantage exists at the room's own point.
- **That the added astrometrics lights suit the room's look.** The bar was legibility, and the frames
  are there to be looked at; the five added lights are a judgement, recorded in the tool.

## Judgement calls, named as calls

1. **The post is recorded in the harness, not in `meeting_rooms.def`.** The meeting renders wherever
   the player stands; the post is the place the harness is told the meeting *would* stand, so it lives
   with `g_shipTest 97` beside the room list rather than in the room table, which owns *which room*,
   not *where in it*.
2. **The check's assertion is the frame, measured — not the light count.** Each room is asserted by
   the frame its post produces, so the check fails for the reason the ruling is about.
3. **Astrometrics is called a lighting defect, and fixed in `tools/shipmap/`.** It was the one room no
   post could save; a lighting fix belongs there so `scripts/build-ship.sh` keeps it, and it is light
   entities only. The pre-fix frames are kept as the proof it was the light, not the post.
4. **The planted-failure hook re-anchors the meeting's own room.** `LWH_ROOM_POST_ANCHOR` overrides
   the security office — the ruling's room, the one `g_shipTest 96`'s anchor was built for; the
   assertion itself covers all seven rooms.
5. **Legibility thresholds are unchanged** (`median >= 35`, `p90 >= 80`); they were not moved to make
   a room pass, because a promise repaired by weakening it is worse than none.
6. **`g_shipTest 97` changed from a four-facing survey to one post per room.** The frame names change
   from `lwh_room_<room>_<n>.tga` to `lwh_roompost_<room>.tga`; the old four-facing survey's frames
   did not reproduce on a fresh run and are not cited.
