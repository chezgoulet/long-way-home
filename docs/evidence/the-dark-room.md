# The dark room — the meeting's own spawn stood inside geometry

The owner's ruling (recorded at the end of `docs/evidence/the-meeting-place.md`, 2026-10-10):
**"if a room is pitch black it needs its own light."** So a meeting room that renders black is a
**defect**, not the look, and it was queued. This is the pass that establishes which defect.

The premise was measured before it was continued, and it holds: the composed deck 6 carries 26 light
entities within 900 units of the meeting's stand position, the merged BSP's light pass ran
(`VKDIAG: … biggest 8192 KB (*mergedLightmap0)`, so lightmaps exist), and the run records no alert or
emergency-lighting line. A `.map` full of lights rendered dark, so the question was which of three it
was — the vantage, the bake, or the light — and the vantage was to be settled first, by frames.

**The finding: it was the vantage.** The player's own stand position — the `lwh_armory_post` the
composition places — lay **inside the copied armory geometry**, so the camera was inside a solid and
saw only its unlit interior faces. Moving the post into the open armory floor, in the composition
script, makes the room read. No light was added, and none was needed.

Applied on `feat/the-dark-room`, cut from `testing`, 2026-10-10. Everything below is the output of the
command named beside it, on the rebuilt ship. No model is called, no audio is rendered, and no asset
is added.

## Task A — the room from somewhere else, before changing anything

The vantage test is `g_shipTest 96`: the player is stood at the room's own anchor and at a pace each
side of it, and turned to all four facings, one frame each. On the **old** composition (post at
`-4336 -3904`):

```
$ python3 - <<'EOF'   # is the post inside the composition's own geometry?
    ... reads build/ship/generated/deck06.map's world brushes ...
EOF
post (-4336,-3904) at z_local -3972 inside 4 brushes
    lwh/detail06/engineering/enggrey x -4357..-4331 y -4042..-3888 z -3983..-3904
    lwh/detail06/engineering/basictom x -4444..-4272 y -3940..-3770 z -3996..-3908
    ... (and two more enggrey panels in the same row)
```

The post stood in the locker bank the composition copied from `_brig`. The two frames the
meeting-place pass had delivered (`lwh_meeting_place_a`, `_b`) were both the same view — the
harness's "facing" was written into `client->ps.viewangles`, which does **not** turn the client (the
client owns its viewangles; only the game's `SetClientViewAngle`, which `TeleportPlayer` calls, sets
`delta_angles` and turns it). So both "angles" were the leftover facing, and both were the dark wall.
Measured, the delivered room frame was `mean 58.0 median 55.0 p90 62.0` and the overlay frame
`mean 22.4` — the near-black picture the owner ruled on.

On the **rebuilt** ship, with the post in the open and the turn applied through `SetClientViewAngle`,
all six vantages read:

```
$ SDL_AUDIODRIVER=dummy xvfb-run -a scripts/run-engine.sh --home-dir build/g3-home \
    +set g_ship 1 +set g_shipMode 1 +set g_shipDeckPitch 3072 +set g_shipTest 96 +map voyager
SHIP: dark room: the security office anchor at (-4172 -3906 -19332)
SHIP: dark room: security post-north at (-4172 -3906 -19308) facing 90
SHIP: dark room: security post-south at (-4172 -3906 -19308) facing 270
SHIP: dark room: security post-east at (-4172 -3906 -19308) facing 0
SHIP: dark room: security post-west at (-4172 -3906 -19308) facing 180
SHIP: dark room: security west-east at (-4364 -3906 -19308) facing 0
SHIP: dark room: security east-west at (-3980 -3906 -19308) facing 180
SHIP: dark room: 6 vantage(s) photographed

$ python3 tools/meetings/legibility.py build/g3-home/baseEF/screenshots/lwh_darkroom_security_*.tga
lwh_darkroom_security_0.tga                    mean  92.0 median  87.0 p90 122.0  LEGIBLE
lwh_darkroom_security_1.tga                    mean  86.9 median 114.0 p90 139.0  LEGIBLE
lwh_darkroom_security_2.tga                    mean 108.6 median 119.0 p90 122.0  LEGIBLE
lwh_darkroom_security_3.tga                    mean  75.3 median  62.0 p90 127.0  LEGIBLE
lwh_darkroom_security_4.tga                    mean  81.2 median  60.0 p90 226.0  LEGIBLE
lwh_darkroom_security_5.tga                    mean  88.8 median  89.0 p90 141.0  LEGIBLE
all 6 frame(s) legible
```

**This is the diagnosis, said with the frames: the room is lit from another vantage, so the defect
is the spawn, and no light needs adding.** The bake is not the cause and the light is not the cause;
both were checked and cleared before anything changed. The office's own copied surfaces carry
lightmaps averaging 169 of 255 (sampled from `build/ship/out/longway_voyager.pk3`'s BSP, the armory's
`lwh/detail06/engineering/enggrey` faces), *brighter* than the deck corridor's own `voyager/basic`
faces at 145 — the lights reach it. Nothing was stripped from the bake.

## Task B — the cause fixed where a rebuild keeps it

A hand edit to `build/ship/generated/deck06.map` is regenerated away, so the fix lives in the thing
that writes it: `tools/shipmap/dressdeck06.py`, in `_armory_post()`. The post moved from the middle of
the copied locker bank to the open armory floor, with the reason recorded beside it:

```
        # ... The lockers (the copied `_brig` room) are a furniture bank at x -4444..-4272,
        # y -3940..-3770; the post at its own centre stood *inside* that solid, so the meeting's own
        # frame was a blank wall (the dark room, 2026-10-10). ...
        ax, ay = -4172, -3906
```

The meeting's facing was fixed in the same place it was broken — `module/ship/g_ship.cpp`,
`g_shipTest 91`, which now calls `SetClientViewAngle` after `TeleportPlayerTo`, so the player is
actually turned north into the room. Both changes survive `scripts/build-ship.sh`.

```
$ scripts/build-ship.sh
==> q3map2 -meta
==> q3map2 -vis
==> q3map2 -light
wrote /home/c/big/git/long-way-home/build/ship/out/longway_voyager.pk3
```

The rebuilt ship's own meeting frame, **the room legible**:

```
$ scripts/meeting-place-check.sh         (excerpt)
    meeting place: at the security office: standing at (-4172 -3906 -19308)
==> the room is legible (measured, not counted)
lwh_meeting_place_a.tga                        mean  91.7 median  87.0 p90 122.0  LEGIBLE
PASS  every kind of meeting has a room; a command decision named the briefing room and a security
      matter the security office with the chief present; the room was on the brief before the
      player arrived; the player stood in the named room and the frame is photographed
```

## Task C — every room a meeting can happen in

`scripts/room-survey-check.sh` runs `g_shipTest 97`, which stands the player in each room the meeting
table (`module/ship/meeting_rooms.def`) names and photographs four facings of each, then measures the
frames with `tools/meetings/legibility.py`. (The room positions are chosen from the maps' own
floor-standing nav goals; they are recorded in the harness.)

| room | deck | verdict | frame(s) that read |
|---|---|---|---|
| the briefing room | 1 | **legible** | `lwh_room_briefing-room_1.tga` (median 55, p90 255), `_0`, `_2` |
| the ready room | 1 | **legible** | `lwh_room_ready-room_0.tga` (median 44, p90 255), `_1` |
| the bridge | 1 | **legible** | `lwh_room_bridge_1.tga` (median 57, p90 233), `_0`, `_2` |
| sickbay | 5 | **legible** | `lwh_room_sickbay_3.tga` (median 41, p90 127), `_0` |
| astrometrics | 8 | **legible** | `lwh_room_astrometrics_1.tga` (median 37, p90 209) |
| main engineering | 11 | **legible** | `lwh_room_engineering_0.tga` (median 64, p90 86), `_1`, `_2` |
| the security office | 6 | **legible** | all four (`_1` median 89, p90 123) |

**One room needed the fix; none of the others is pitch black.** The survey did find that a *facing*
can be dark in almost every room — a corner, a bulkhead, the back of a console — and that is expected:
the ruling is about the room, not about every direction a camera can point in it. The meeting's own
room, the security office, reads from every one of its four faces.

I could not correct two of the chosen stand points to stand exactly where a person would: main
engineering's first chosen point sat on a `target_level_change` and transitioned the run to `_brig`,
so it was moved to the deck's own player start; and the "ready room" point is my pick from the deck's
geometry, not a name the map gives it (see below).

## The remaining factor, measured and not fixed

The **delivered** frame is the room with the meeting band over it, and the band dims the scene: at the
same position and facing, a mid band of the room reads `mean 75.4` without the band and `mean 19.4`
with it — a factor of about **0.26**. So even now the room behind the band is dim, because the band is
nearly opaque, not because the room is unlit. This is the overlay path the meeting-place pass already
recorded ("the menu path renders the scene dark behind the band"), and it is a change to a screen, not
to the room; it is reported here rather than folded into this fix, because the brief for this pass is
the room. If the owner wants the room to read *through* the band, that is the next change and it is a
brightness change in `module/ui/ui_lwh_meeting.cpp`, not a light in the map.

## The checks

```
scripts/test.sh                 -> all checks passed (crew, ship, tools, hooks, api, orders,
                                   harness, docs, python, shellcheck, patches)
scripts/check.sh --source-map build/gdk/maps/brig-map/_brig.map --script-corpus build/gdk/scripts
                                -> exit 0 (the entity dictionary, the validator negatives, the
                                   compiler corpus pass)
scripts/meeting-place-check.sh  -> PASS, and the room frame measures LEGIBLE
scripts/room-survey-check.sh    -> PASS; every room photographed; the security office reads
```

The legibility measure is the new `tools/meetings/legibility.py`. It reads the engine's own screenshot
(an uncompressed TGA) and calls a frame **legible** when its typical pixel is not near black
(`median ≥ 35`) *and* it carries something clearly lit (`p90 ≥ 80`), the measure being a pixel's
**brightest channel** so a red console or a blue lamp counts as light. It measures the middle band of
the frame, because above it is the console text and below it the weapon and the band. Its unit tests
(`tests/tools/test_legibility.py`) prove it *fails* for a pitch-black frame and for a flat wall with
nothing lit, which are the two ways the old frame failed.

## What could not be verified

- **Whether a room is *pleasing*.** That is the owner's eye. All this settles is legibility, and the
  frames are there to be looked at.
- **That each survey stand point is the *named* room.** For decks 5, 8 and 11 the deck *is* the room,
  so the point is the room's own; for deck 1 I chose the points from the geometry (`bridge/confchair`
  and the conference walls for the briefing room; the small room west of the bridge for the ready
  room) and the frames look like those rooms, but the map gives the rooms no names, so which is the
  "ready room" is my read of the map, not the map's word.
- **Whether the astrometrics consoles are bright enough for the owner.** They read in the frame but
  measure `DARK` on three of four facings; the metric is conservative about dim red consoles.

## Judgement calls, named as calls

1. **The cause is called the vantage, not the light.** The room reads from a position in the open and
   fails from a position inside a solid; that is the vantage by the brief's own definition, and no
   light is added. The post moved, the turn was fixed, and both live where a rebuild keeps them.
2. **`_armory_post()` is a measured constant, not a derived search.** `_arrival()` searches for the
   deck door; this post is placed at a constant like the other re-dress posts, and the constant is
   now one measured clear of every brush at standing height. A derived search would be more robust and
   is not done here.
3. **The legibility thresholds are ours and overrulable.** `median ≥ 35` and `p90 ≥ 80`, on the
   brightest channel, in the middle band. They separate the frames this pass produced — a room at
   median 40-120 with a lit feature at p90 120-255, versus a wall inside geometry at median 55 p90
   61 — and they are stated so they can be overruled.
4. **The overlay's dimming is reported, not changed.** It is the dominant reason the *delivered*
   frame is dim (×0.26), but it is a screen, and this pass is the room. Named as the next thing the
   owner may want.
5. **`g_shipTest` 96 and 97 are new case numbers** (the vantage test and the room survey); 96 also
   accepts a `"x y z"` anchor in `g_shipTestPos`, which is how the candidate post was tried without
   rebuilding the map each time.
