# The frame assertions — what each check can assert about its picture

`docs/audit-whole-project.md` §2.3 found **thirty checks that assert a screenshot exists and not what
it shows**: `[ -f "$SHOT" ]` proves a file was written, and cannot tell a lit room from a black one,
the right screen from the world behind it, or the feature from the previous frame. This is the
register for the fix: **one instrument** (`tools/frames/frame.py`) and, for each check, **what its
claim can actually be asserted as**, honestly, including where the answer is *not this way*.

The instrument's limits are in its header and are repeated because every row depends on them: **a
floor proves a region is *legible*, not *right*; a difference proves a region *changed*, not *the
change you meant*; a region proves only about the pixels inside it. The owner judges "right".** A
check that gains an assertion it cannot fail is worse than the one it replaced, so a check whose
frame does not carry its claim is classified *behavioural* and says so, rather than being given a
green assertion.

## The count, and a correction to the audit

The audit says *thirty* and names "among them" **22 scripts** — `deck-check`, `deck06/07/12/13/14`
(five), `damage-`, `fire-`, `gravity-`, `breach-`, `emergency-`, `computer-`, `order-`, `power-`,
`s10-`, `s3-`, `s4-`, `s4-panel-`, `viewscreen-`, `personal-log-`, `playerbody-`, `appearance-check`
— **plus "the 'screens photographed' block inside `every-screen-check`"**, which is **23** named
places, not thirty. The word "among them" is doing the work: the class is wider. A `grep` for a
screenshot assertion finds **33 scripts** that reference a written frame. **This register classifies
the 23 the audit names** and records, at the end, the rest of the class it found, so the "thirty" is
not quietly read as the list above. The number was never the defect; the shape was.

## The three assertions

| assertion | what it asks | fails on |
|---|---|---|
| **floor** | the region is legible: not black, not uniform | a pitch-black frame, a frame into a solid, a flat fill |
| **difference** | the region changed from a reference taken at the same vantage | two frames that do not differ |
| **region** | the part of the frame the claim is about, named and measured | the wrong region, or one too small to hold the feature |

`tools/frames/frame.py` is the one implementation; `scripts/room-survey-check.sh` and
`scripts/meeting-place-check.sh` use it, and their verdicts are unchanged (the legacy rule was a
floor on the band `0.10–0.55` with `median ≥ 35` and `p90 ≥ 80`, now passed as
`--band 0.10,0.55`). The old `tools/meetings/legibility.py` is deleted: one file, one rule.

**What the room floor asks, and why the deck checks ask less of it.** The two measurers assert that
a *room reads* (the dark-room ruling): `median ≥ 35` **and** `p90 ≥ 80`. The deck checks cannot ask
that of every room — deck 13 (life support) and deck 14 (stasis) render at median 12 and 29, dim by
design rather than pitch black — so their floor is **"lit and structured"**: `p90 ≥ 80` (something
is lit) **and** `spread ≥ 40` (the frame is not a flat fill), with no median clause. That is the
brief's "not black, not uniform" and no more. **Whether deck 13 and 14 are lit *well* is the
owner's judgement**, named here as a call: the frames measure `median 12, p90 135` and are recorded
for the eye rather than failed.

## The classification

`visual` = the claim is about what is on the screen → assert the frame. `behavioural` = the claim is
a state change or a log read → the log line is the instrument; name it. `both` = assert both.

| # | check | class | the assertion added (or why not) | the log line that carries the behavioural half |
|---|---|---|---|---|
| 1 | `deck-check` | **both** | floor: the deck frame must read (`--min-median 0 --min-p90 80 --min-spread 40`: lit and structured) | `SHIP: deck N blockout: standing at …` / `in solid` |
| 2 | `deck06-check` | **both** | floor on the three frames | `SHIP: deck 6 room: …` |
| 3 | `deck07-check` | **both** | floor on both frames | `SHIP: deck 7 room: …` |
| 4 | `deck12-check` | **both** | floor on the frame | `SHIP: deck 12 room: …` |
| 5 | `deck13-check` | **both** | floor on both frames | `SHIP: deck 13 room: …` |
| 6 | `deck14-check` | **both** | floor on both frames | `SHIP: deck 14 room: …` |
| 7 | `damage-check` | **both** | difference: the camera now stands where the life-support station is and takes a before and an after frame at that one vantage; the spark changes **0.6%** of the subject region (`--region subject:615,525,665,585 --min-changed 0.005`). On the old (wall) pair that region changes **0.0%**, so the assertion fails where it must. | `CREW: life support is damaged; sparks at … on deck 12` |
| 8 | `fire-check` | **behavioural** | **still not this way**: the camera now stands where the deck's first fire appears, but the smoke renders against a dark deck — the subject region changes **0.2–0.9%**, which is the crew walking through it, not the fire. The log line is the proof. | `CREW: fire on deck 12 (60%); N effects in the world` |
| 9 | `gravity-check` | **behavioural** | the claim is a value, not a picture | `SHIP: gravity test: floating: ps.gravity 0, custom 1` … |
| 10 | `breach-check` | **behavioural** | **still not this way**: the camera now stands at the field, and the breach compartment renders near-black: the field-off and field-on frames differ by **0.0–0.1%** of the subject region. The log lines are the proof. | `ENV: deck 12 is breached…` and `SHIP: breach test: field on: …published field 1` |
| 11 | `emergency-check` | **both** | difference: the camera now stands at the nearest authored strip to the room's watch post and holds that aim for both frames; the red and stood-down states differ **25.3%** of the strip's own region (`--region strip:480,510,820,640 --min-changed 0.05`). The old vantage changed 0.0% of the scene band. The difference proves the lighting changed where the strip is; not that every strip is the named one. | `ENV: emergency lighting on on deck 12 …` |
| 12 | `computer-check` | **both** | difference: the meeting overlay must differ from the world before it opened | `SHIP: computer test: session: N exchange(s)…` |
| 13 | `order-check` | **both** | difference: the order overlay must differ from the world | `SHIP: order overlay: given=1, job N` |
| 14 | `power-check` | **both** | difference: the console must differ from the world (measured 97.1% of the frame) | `SHIP: allocation test: life support 80%…` |
| 15 | `s10-check` | **behavioural** | there is **no screenshot at all** — the claim is ironman saves and permission, proved by the log | `Ironman: save 'byhand' refused` / `SHIP: order refused…` |
| 16 | `s3-check` | **both** | difference: the turbolift menu must differ from the world. The fifteen-deck tour is behavioural. | `LWH: turbolift deck list …` / `SHIP: turbolift tour: 15 of 15 decks…` |
| 17 | `s4-check` | **both** | difference: each console must differ from the world before it opened | `SHIP: off refused at …` / `LWH: the … panel opens …` |
| 18 | `s4-panel-check` | **both** | floor: the frame reads as the room with its screen (`--min-median 0 --min-p90 80 --min-spread 40`) | `SHIP: panel test: standing on deck 6…` |
| 19 | `viewscreen-check` | **behavioural** | **not this way**: a before-frame at the same vantage (the panel with no contact) was added and the cgame's own draw stopped reaching its evidence line — the panel trace is timing-fragile and an extra `screenshot` before it breaks it. The draw is proved by the cgame's own log line; the frame is left for the owner's eye. | `LWH: viewscreen its weapons hull 62% shields 35%` |
| 20 | `personal-log-check` | **both** | difference: the personal screen must differ from the world before it opened | `personal test: the private screen reads "…"` |
| 21 | `playerbody-check` | **behavioural** | the body is not visible in the first-person frame; the transcript names it. A floor on the session frame is kept but proves only legibility. | `SHIP: Tuvok Test walks as head tuvok/default…` |
| 22 | `appearance-check` | **both** | floor: the lineup frame must read as a row of people (`--min-median 0 --min-p90 80 --min-spread 40`). Half 2 now exits **3 (SKIPPED)**, not 0, with no game data. | `appearance lineup: … walks as head …` |
| 23 | `every-screen-check` | **both** | difference between the paired frames: the report locked vs open (the two-lock) and each screen against the last (report → jobs → chart → survey → Operations). The contrast and type height are already measured. | `SHIP: report test: …` / `SHIP: chart test: …` |

## The finding the difference assertion produced

Four checks claim a thing is *visible in the world* — sparks (`damage`), smoke and flame (`fire`), a
force field (`breach`), emergency lighting (`emergency`) — and the two frames the harness first took
**did not show it**. Measured on the scene band (10%–55% of the height: below the engine's console
text, above the weapon model, the same band the two measurers use), the two frames changed by
**0.3%** (damage), **0.3%** (fire), **0.7%** (breach) and **0.0%** (emergency). The whole-frame
difference a naive assertion would have scored as a pass was the console text scrolling (≈24% of the
top strip) and the weapon's animated screen. **This was exactly the defect §2.3 is about**: a check
that asserted "a file exists" was green, and a check that asserted "the frame changed" would also
have been green, for a reason that has nothing to do with the claim.

**The follow-up pass (`docs/evidence/the-posts-and-the-floor.md`) removed the reason: the camera was
photographing a wall.** The harness now stands where each claim's subject is — the damaged system's
station, the deck's first fire, the authored field, the nearest emergency strip — and takes a
*before* frame at that one vantage where the claim needs a difference. Re-measured in the region the
camera aims at: **damage 0.6%** (the spark; **0.0%** on the old wall pair, so the assertion fails
where it must) and **emergency 25.3%** of the strip's own region (the lighting state). **Fire and breach
still cannot be carried by a frame** — the smoke renders dark against a dark deck, and the breach
compartment renders near-black (0.0–0.1% of the subject region) — so those two stay behavioural, and
say so.

**Where the difference *does* work**: a screen that opens replaces the world, and the frame carries
it — measured 96–97% of the frame changes when the Engineering console (`power`), the computer's
room, the order overlay and the personal log open. Those checks gained a difference assertion and
pass. The difference is the right instrument exactly where the picture changes into the thing the
claim means, and it is not where it does not.

## What is not converted, and why

- The four above are behavioural: the picture does not carry the claim.
- `gravity` and `s10` and `playerbody`'s body are values and reads, not pictures.
- The **wider class** the audit's "among them" leaves open was listed but not classified row by row
  in this pass: `borgfx-`, `gaps-`, `s2-`, `s9-`, `s4-glance-`, `screens-`, `reachability-`,
  `meeting-overlay-` and `meeting-place-check` all write frames. `s2-check` gained the same
  difference assertion as its siblings, and `meeting-place` and `room-survey` were moved onto the
  instrument; the rest are named here as remaining, not silently counted as fixed.

## The register

The instrument and its tests: `tools/frames/frame.py`, `tests/tools/test_frames.py`. The evidence
that the assertions fail and pass, on the real frames and on planted ones, is
`docs/evidence/the-frame-assertions.md`.
