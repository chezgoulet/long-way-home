# The hook register — what the simulation offers, and who can reach it

`docs/scenario-atlas.md` (what the scenarios will need), `docs/scenario-manifest.md` (the manifest's
vocabulary), `docs/staff-meetings.md` and `docs/evidence/meeting-brief.md` (the meeting seam),
`docs/power-assignment.md` (the allocation seam), `docs/rising-to-the-occasion.md` and
`docs/evidence/rising-to-the-occasion.md` (the counter-example), `docs/walkthrough.md` finding **G5**
(the same defect), and `docs/the-entry-point.md` (the post-not-name law). Applied on `feat/the-hooks`,
cut from `testing`, 2026-10-08.

**Observed, and the command that produced it.** Everything below is the output of a command named
beside it, on this tree. No engine was rebuilt: no translation unit changed, so the
configure-time-glob trap does not apply, and the module build was already current (`scripts/test.sh`
passed). This pass adds **no game logic, no hook, no new path** — it is the index and the check.

## What was built, and where

- `docs/hook-register.md` — the register: 348 hooks, grouped, each with what it takes, what it does,
  **who can reach it**, the post-not-name law where it binds, and the owning document.
- `tools/hooks/surface.py` — the derivation that reads the public surface out of
  `module/ship/ship_core.h`.
- `tools/hooks/reach_report.py` — the reachability *evidence*: call sites by entry surface, a
  transitive closure, not a verdict.
- `tools/hooks/check_register.py` and `scripts/hooks-check.sh` — the freshness check.
- `scripts/test.sh` — the check runs as part of the ordinary checks.

## Task A — the hooks, derived rather than recalled

The authority is the public surface of `module/ship/ship_core.h`. `surface.py` derives it —
comments stripped, declarations matched by a type run then a name then `(` — and finds **348 public
functions** (351 declarations; `MayOperate`, `MayCallAlert` and `ShuttleByClass` are each declared
twice, so 348 names). The derivation is not recalled and not recalled-by-grep; it is re-run:

```
$ python3 tools/hooks/surface.py module/ship/ship_core.h | wc -l
348
$ python3 tools/hooks/surface.py --self-test
surface.py self-test ok
```

The self-test proves the extractor is narrow: it takes a declaration and refuses a definition, a
call and a cast. The groups are a reading of what each hook is, and the rule is written in the
register: the **entry points** content calls, the **situations** the simulation raises on its own,
the **outcomes** content asks for, and the **readers** content consults. The counts:

| group | hooks |
|---|---|
| entry points | 73 |
| situations the simulation generates | 55 |
| outcomes | 54 |
| readers | 166 |

## Task B — reachability, per hook, and the two that hid

`reach_report.py` classifies each hook by where a call can start, then closes transitively through
the model's own functions (including its `static` helpers) so that a hook only the console reaches
is not reported as reached in play because a play function calls it. Its entry surfaces are the
in-game screens (`module/ui`), the console `Publish` (the readouts the screens draw), the crew /
environment / player integration, the simulation's own `Advance`, the developer console
(`Svcmd_Ship_f`) and the `g_shipTest` harness.

```
$ python3 tools/hooks/reach_report.py --detail   (summarised)
  reached in play                          184
  reached only from the developer console  129
  reached by nothing yet                    35
    — of those, reached only by the tests   29
    — of those, no caller anywhere           6
  unknown                                    0
```

**The six nothing calls at all:** `SpeciesCapability`, `SpeciesNeed`, `SpeciesSusceptibility`,
`PromiseKindName`, `LogVisibilityName`, `BandGrantCount`. (The species capability/need/
susceptibility readers are built and no surface uses them; the walkthrough's `docs/walkthrough.md`
G5 shape, one layer down.)

**The two hooks the brief names as the proof, and what is actually true:**

- **`OfferRising`** — the brief calls it *reached by nothing yet*. **The code says otherwise:** it is
  reached from the developer console (`ship rise`, `module/ship/g_ship.cpp`), so it is *reached only
  from the developer console*. **No play path reaches it**, which is the substance of the brief's
  point and stands. The same is true of the rest of the rising act (`AttemptRising`).
- **The systems under stress** — `Board`, `BoardAs`, `BoardBorg`, `IgniteDeck`, `BreachDeck`,
  `DamageSystem`, `DamageSource`. Their only *deliberate* reach is the developer console; the
  simulation also raises them in a fight, but no play surface starts the fight. Marked `console`,
  flagged, and named as a call.

**And a fourth case the brief's three states did not separate, named as a call:** a hook the
simulation raises on its own in play with no deliberate control. The seven above are the members;
they are counted under `console` because the question a scenario author asks is *can a player reach
it on purpose*. The register says so in its own words.

The full unreachable-in-play list is the register's own page, kept there because it is the page a
person reads; this evidence does not copy it.

## Task C — the register is an index, not a source of truth

Each row carries only its signature, the header's own one-line description (or the name read out),
the reachability, the law marker and **the owning document**. Where a design belongs to
`docs/power-assignment.md`, `docs/staff-meetings.md`, `docs/the-record-and-the-log.md` or another,
the register points and does not restate. `—` in the owner column means the header names no owner;
that is a document to write, not a design detail invented here.

## Task D — the freshness check, demonstrated failing and passing

`scripts/hooks-check.sh` re-derives the surface and compares it to the hook names in the register's
tables. **It does not judge reachability.** Demonstrated by adding a function to `ship_core.h`,
watching it fail, and removing it:

```
$ scripts/hooks-check.sh
hooks-check: module/ship/ship_core.h declares 348 public functions; docs/hook-register.md registers 348 hooks
PASS  every public function is registered, and every registered hook exists

# a declaration added to module/ship/ship_core.h:
bool HookProbeFunction(int x);

$ scripts/hooks-check.sh
hooks-check: module/ship/ship_core.h declares 349 public functions; docs/hook-register.md registers 348 hooks
FAIL  in the code and not in the register (1): HookProbeFunction
$ echo $?
1

# the declaration removed again, and the header is clean (`git diff --stat` empty):
$ scripts/hooks-check.sh
... PASS  every public function is registered, and every registered hook exists
```

The check also fails the other way — a register row whose function the code no longer has — because
a stale row is the same drift pointing the other way.

## The checks, and what they returned

```
scripts/test.sh
  -> crew_core: all checks passed
     ship_core: all checks passed
     tools: 54 tests OK
     hooks-check: PASS  every public function is registered
     python syntax ok; shellcheck ok; 20 patches; all checks passed

scripts/check.sh --source-map build/gdk/maps/eliteforce_voyager_maps/voy1.map \
                 --script-corpus build/gdk/scripts
  -> entity dictionary; validator negative tests ALL PASS;
     2,024 files: 2,016 compiled and read back, 8 rejected — exactly the known, documented set
     exit 0

scripts/hooks-check.sh
  -> PASS (and demonstrated failing on a probe function, above)
```

## What could not be verified

- **Whether the register is complete in *intent*** — whether a scenario will want a hook nobody
  provided. The check proves the public surface is *present*, not that it is *enough*. The honest way
  to find the missing ones is to ask what each of `docs/scenario-atlas.md`'s nine scenarios needs and
  see whether the list answers; that is the scenario authors' and the owner's, and it is said in the
  register.
- **A scenario that calls any of it.** No breeding-condition director exists (`docs/walkthrough.md`
  G4/G5); the register says what *can* be called, not what does.
- **Whether a reached hook is good to use.** A console drawing a number is not a decision being
  legible; only the owner's sitting can say.
- **The grouping of a few hooks between "entry point" and "outcome".** The brief's four groups
  overlap; the rule used is stated in the register.

## Judgement calls, named as calls

1. **The reachability is derived by transitive closure, not by counting direct calls.** This is what
   keeps `OfferRising` out of "reached in play": `OfferRisingTo` and `AttemptRising` call it, and both
   are console-only, so a direct-call count would have reported a false green. **This is a call**, and
   it is the one the brief warned about.
2. **"Developer console" includes the `g_shipTest` harness.** The harness is developer-only and
   cvar-started, not console-typed; they share the property that a player cannot reach them, so they
   are one state. **This is a call.**
3. **The automatic-only case is counted under "console".** The simulation raises boarders, fire and
   damage itself; no play surface does. Counting on *deliberate* reach is what a scenario author
   needs. **This is a call**, and the seven members are named.
4. **Test-only is "reached by nothing yet", with the distinction named.** A hook only the unit tests
   call has no path in the game; the register says which of the 35 are test-only (29) and which have
   no caller at all (6). **This is a call.**
5. **The `does` column is the header's own line, or the name read out.** The register invents no
   design detail; where 219 functions share a section comment the name is read out instead of
   attributing the section's words to the wrong function. **This is a call.**
6. **The owner column is empty where the header names no document.** A wrong pointer is worse than
   none. **This is a call.**
7. **The check runs in `scripts/test.sh`.** The brief asked for the check, not for it to be wired
   into CI; it needs no game data and belongs with the checks that run everywhere, because a check
   that never runs cannot be the deliverable's half-life. **This is a call.**
8. **No hook, no path, no save format was touched.** Save format 53 and every existing behaviour are
   unchanged; `scripts/test.sh` and `scripts/check.sh` are green. **This is a call** the brief
   required.

## Files

- `docs/hook-register.md` (the register)
- `docs/evidence/hook-register.md` (this file)
- `tools/hooks/surface.py`, `tools/hooks/reach_report.py`, `tools/hooks/check_register.py`
- `scripts/hooks-check.sh`
- `scripts/test.sh` (runs the check)
