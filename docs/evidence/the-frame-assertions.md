# The frame assertions — the evidence

The brief (`BRIEF.md`, this pass) closes `docs/audit-whole-project.md` §2.3: **thirty checks that
assert a screenshot exists and not what it shows**. The register of what each check can now assert,
with reasons, is `docs/frame-assertions.md`. **This file is the evidence**: what was observed and the
command that produced it.

Everything below is the output of the command named beside it, on `feat/the-frame-assertions`, cut
from `testing`, on the playtest host (`sasquatch`), with the module rebuilt from this tree
(`cmake --build ../upstream/efgame/build-linux`) before any engine check ran. No frame is committed;
the frames the assertions judge are brought out under `build/frame-assertions/` (gitignored) and
named at the end.

## Task A — the instrument

`tools/frames/frame.py` — one file, one rule, no second copy. It offers the three assertions the
register uses (floor, difference, region) and its limits are in its header: *a floor proves a region
legible, not right; a difference proves a region changed, not the change you meant; the owner judges
"right".* One implementation serves every check: `scripts/room-survey-check.sh` and
`scripts/meeting-place-check.sh` were moved onto it (their legacy rule, a floor on the band
`0.10–0.55` with `median ≥ 35` and `p90 ≥ 80`, is now passed as `--band 0.10,0.55` with the same
defaults, so their verdicts are unchanged), and `tools/meetings/legibility.py` is deleted.

The unit tests prove the instrument *fails* where it must:

```
$ python3 -m unittest tests.tools.test_frames
Ran 12 tests in 0.333s
OK
```

## Task C — the assertions, failing and passing, on real frames

The demonstration below plants the failing cases (a blackened copy of a real frame; a flat fill; a
frame against itself) and runs the tool on the real frames the checks produce. The command is
`bash /tmp/opencode/fa/frame-demo.sh`, which calls `python3 tools/frames/frame.py`:

```
### floor: a real room frame (the security office post) -- LEGIBLE
lwh_roompost_security-office.tga   whole frame   mean  93.0 median  92.0 p90 141.0 spread 153.0  LEGIBLE
exit=0

### floor: the same frame, blackened -- DARK (fails)
black.tga                          whole frame   mean   0.0 median   0.0 p90   0.0 spread   0.0  DARK -- median 0 < 35 (mostly black)
a frame does not carry the floor's claim
exit=1

### floor: a flat fill -- DARK (uniform; fails)
flat.tga                           whole frame   mean  57.0 median  57.0 p90  57.0 spread   0.0  DARK -- p90 57 < 80 (nothing lit)
a frame does not carry the floor's claim
exit=1

### floor: a real frame that is mostly black and carries nothing (the ship log) -- DARK (fails)
lwh_log.tga                        whole frame   mean  15.4 median   0.0 p90   0.0 spread 152.0  DARK -- median 0 < 35 (mostly black)
a frame does not carry the floor's claim
exit=1

### diff: a frame against itself -- SAME (fails; the claim is not carried)
lwh_power.tga                      whole frame   changed   0.0%  SAME -- the claim is not carried by the picture
a frame did not differ from the reference where the claim lives
exit=1

### diff: the allocation console against the world before it opened -- DIFFERS (passes)
lwh_power.tga                      whole frame   changed  97.1% mean|d| 79.2  DIFFERS
exit=0
```

**A region is a region.** The region names the part it means, and choosing it changes the verdict.
The damage check's before/after frames, measured whole and then on the scene band (10%–55%: below the
engine's console text, above the weapon model):

```
$ python3 tools/frames/frame.py diff --min-changed 0.05 lwh_damage_before.tga lwh_damage.tga
lwh_damage.tga   whole frame   changed   6.2% mean|d|   6.1  DIFFERS
$ python3 tools/frames/frame.py diff --band 0.10,0.55 --min-changed 0.05 lwh_damage_before.tga lwh_damage.tga
lwh_damage.tga   band 0.1-0.55 changed   0.3% mean|d|   0.4  SAME -- the claim is not carried by the picture
```

The whole-frame "DIFFERS" is the console text scrolling (see the finding below); the part the claim
owns is unchanged.

## The finding — four "visible in the world" claims are not in the picture

`damage` (sparks), `fire` (smoke and flame), `breach` (a force field) and `emergency` (emergency
lighting) each photograph two frames and claim a thing is visible. **In all four the two frames do
not show it.** The scene band (10%–55%) changes by:

```
  lwh_damage_before   vs lwh_damage     changed 0.3%
  lwh_fire_before     vs lwh_fire       changed 0.3%
  lwh_breach_off      vs lwh_breach     changed 0.7%
  lwh_emergency_off   vs lwh_emergency  changed 0.0%
```

The differences that exist are the engine's console text (≈24% of the top strip) and the weapon's
animated screen. The frames were looked at: `build/frame-assertions/damage_before.tga` and
`damage_after.tga` are the same corridor; the amplified difference is the console text and the HUD.
**The honest assertion for these four is the log line**, which the register names for each. They are
classified behavioural, and no frame assertion was bolted on.

## The conversions that pass, run live

Each converted check was run on the playtest host after rebuilding the module. The difference checks
print the measured fraction; all are far above their threshold.

```
$ scripts/power-check.sh
lwh_power.tga           whole frame  changed 97.1%  DIFFERS   (the console against the world)
$ scripts/computer-check.sh
lwh_computer.tga        whole frame  changed 96.4%  DIFFERS   (the meeting overlay against the world)
$ scripts/order-check.sh
lwh_order_give.tga      whole frame  changed 96.4%  DIFFERS   (the order overlay against the world)
$ scripts/personal-log-check.sh
lwh_personal.tga        whole frame  changed 96.3%  DIFFERS   (the personal log against the world)
$ scripts/s2-check.sh
lwh_engineering.tga     whole frame  changed 97.0%  DIFFERS   (the Engineering console against the world)
$ scripts/s4-check.sh
lwh_tactical.tga        whole frame  changed 96.6%  DIFFERS
lwh_ops.tga             whole frame  changed 96.5%  DIFFERS
lwh_sickbay.tga         whole frame  changed 16.1%  DIFFERS   (Sickbay replaces the Operations screen)
lwh_nonstation.tga      whole frame  changed 96.5%  DIFFERS
$ scripts/s3-check.sh
lwh_turbolift.tga       whole frame  changed 95.9%  DIFFERS   (the menu against the world)
```

Rooms gained a floor ("lit and structured": `--min-median 0 --min-p90 80 --min-spread 40`), and pass
on the real frames:

```
$ scripts/deck06-check.sh   PASS   (three frames LEGIBLE)
$ scripts/deck07-check.sh   PASS   (two frames LEGIBLE)
$ scripts/deck12-check.sh   PASS
$ scripts/appearance-check.sh  PASS  lwh_appearance_lineup.tga  mean 58.6 median 45.0 p90 122.0  LEGIBLE
$ scripts/deck-check.sh 13  PASS
$ scripts/deck-check.sh 14  PASS
```

Deck 13 (life support) and deck 14 (stasis) render dim — `median 12` and `29` — and are the reason
the room floor is "lit and structured" rather than the measurers' "a room reads". **That is a
judgement call**: it is recorded for the owner's eye, not failed.

## Task B — the classification

The register is `docs/frame-assertions.md`: all 23 places the audit names, each marked
`visual`/`behavioural`/`both`, with the reason and, where behavioural, the log line that carries the
proof. **The count is corrected there**: the audit says "thirty" and names 22 scripts plus the
`screens photographed` block (23); a `grep` finds 33 scripts that write a frame. The number was
never the defect; the shape was.

## Task D — the half that skips

`scripts/appearance-check.sh`, with no game data, no longer prints `PASS (half 1)` and exits 0.
Demonstrated by moving the game data aside (`mv build/baseEF build/baseEF.hold`), running the check,
and restoring it:

```
$ mv build/baseEF build/baseEF.hold && scripts/appearance-check.sh; echo "exit=$?"; mv build/baseEF.hold build/baseEF
...
SKIP  (half 1 only) the derivation is enumerated, canon-free and deterministic;
      half 2 (the rendered lineup) cannot run: no game data at build/baseEF
      exit 3 -- a skipped half must not read as a pass. Full suite green requires game data
exit=3
```

**The choice: exit 3, not 1.** A skipped half must not read as a pass, so it is non-zero; but a
caller should be able to tell "could not run" from "ran and failed", so it is a distinct code. Both
are non-zero and neither is a pass. The header says so.

## The frames I looked at

Brought out of the scratch to `build/frame-assertions/` (gitignored; a copy, not a commit):
`room_security-office.tga` (the floor's passing case), `power_closed_world.tga` and
`power_console.tga` (the difference's passing pair), `damage_before.tga` and `damage_after.tga`
(the finding: the same corridor), and the planted `black.tga` and `flat.tga`. **I looked at the security office room, the
allocation console, the damage pair and the emergency pair.** Whether each reads as the *right* room
or a *good* screen is the owner's judgement — a check that asserts a difference is not the same as a
person seeing the right thing.

## What could not be verified

- **Whether a frame shows the right room, the right screen, or a good one.** The floor proves
  legible and the difference proves changed; both stop short of "right". That is the owner's eye.
- **Whether deck 13 and 14 are lit well enough.** They measure dim; the floor admits dim rooms by
  design. Recorded for the eye, not judged.
- **A visual assertion for `viewscreen-check`.** A before-frame at the panel with no contact was
  added; with it, the cgame's own `LWH: viewscreen` line stopped firing altogether (the engine drew
  three frames in the run, and the panel trace needs the frame the extra `screenshot` consumes). The
  change was reverted rather than shipped with a suppressed behavioural proof. The draw is proved by
  the cgame's log line; the frame is left for the owner's eye.

## Judgement calls, named as calls

1. **The one instrument is a new file, not the old path generalised in place.** The tool moved to
   `tools/frames/frame.py` because a frame instrument is not a meeting tool; `legibility.py` is
   deleted rather than kept as a shim, which is what "one file, no second copy" asks.
2. **The four visible-effect checks are behavioural, not converted.** Their frames do not carry the
   claim, measured above. Bolting on a difference that passes on the console text would be the
   decoration the brief warns against.
3. **The room floor is "lit and structured", not "amply lit", for the deck checks.** Deck 13 and 14
   would otherwise fail on a look that may be intended. The two measurers keep the stricter rule.
4. **The screen-open difference is a difference from the world, not from another screen.** It proves
   the screen opened; it does not prove it is the *asked-for* screen. That limit is the tool's, and
   it is stated in every such check's comment.
5. **`appearance-check` exits 3, not 1,** for a skip, so a caller can distinguish it. Named in the
   header and above.
6. **The audit's "thirty" is 23 named places.** Corrected visibly in the register rather than quietly
   reconciled; the wider class is named there as remaining, not counted as fixed.

## The checks

- `scripts/test.sh` — all checks passed (crew, ship, tools including `test_frames`, hooks, api,
  orders, harness, docs, python, shellcheck, patches).
- `scripts/check.sh` — exit 0 (entity dictionary, validator negatives, compiler corpus).
- The converted engine checks, run on the playtest host: results above.
