# The character layer bites: the four kinds and morale reach the work

`docs/character-attributes.md` (the four kinds and the derivation), `docs/crew-roster.md` (traits and
drives), `docs/morale.md` (the three components) and `docs/failure-is-content.md` (condition sets the
odds, stress sets the severity), applied on `feat/the-work-bites` (cut from `testing`), 2026-10-08.
**O8's named remainder, and the pass the owner's ruling asks for.**

The owner's ruling:

> I agree with your observation that buffs and debuffs should have impacts on the quality of work and
> the outcomes.

So the layer stops being descriptive. The judgement call that named this pass is call 5 of
`docs/evidence/the-character-layer.md` -- *"`EffectiveSkill` is computed but not yet read by the
failure roll"* -- and that call now carries a visible correction pointing here.

**Observed, and the command that produced it.** Everything below is the output of a command named
beside it, on this tree, after
`cmake --build /home/c/big/git/upstream/efgame/build-linux --target efgame` (which built
`libefgame.so` with only the pre-existing `-Wwrite-strings` warnings) and after the unit tests
(`scripts/test.sh`). The one-place account of the wiring is now `docs/character-attributes.md`, "The
wiring: how each kind reaches the work".

## What was built, and where

One mechanism, in `module/ship/ship_core.{h,cpp}` beside the record and the failure roll it extends.
No game header, no engine code, no new translation unit (so the configure-time-glob trap does not
apply); the module compiles the same file it already did. The only module-side change is one line of
console output in `module/ship/g_ship.cpp`. No new art, no assets, no audio.

- `WorkFactors(c, skill, context, stress, out, max)` -- the named factors a person brings to a task,
  in a fixed order. Each factor is one kind, names itself, and carries a signed weight; **a factor
  that does not move the odds is not returned**, so the list is exactly what touched the outcome.
- `WorkOdds(condition, factors, n)` -- the system's condition sets the base odds (`AnomalyOdds`, the
  owner's ruling of 2026-10-06), the operator's factors move them **multiplicatively**, clamped. If
  the base is zero -- the top tenth of capability -- the result is zero whatever the operator: a
  well-kept system still mangles no one.
- `RollWork(condition, stress, factors, n, roll)` -- one use, the odds first and the severity ladder
  (`AnomalySeverityFor`) unchanged: the condition and the load still set the severity.
- `WorkFactorLine(factors, n)` -- the factors as one line, for the log and the console.
- `WorkReading(s, crew, id)` -- the whole picture for one person at one system, for the console.
- `WorkContext` and `WorkContextOf(id)` -- the moment a trait or drive answers (a hull task, a medical
  task, a hazardous use, a reclaim), so a fear can be *realised by the work in front of them*.
- `DepartmentSkill(dept)` -- one place the department-to-skill mapping lives, used by the derivation
  and by the work.

`UseSystemAt` now resolves an operator -- the named person, or **the station's own hand** (the first
fit, unbrigged crew member posted there) -- computes that person's factors, rolls `RollWork`, and
writes the factors into the anomaly line. `DepartmentSkill` replaced the inline mapping in
`DeriveCharacter`, so the derivation and the work read the same table.

## Task A -- the skills reach the odds

`EffectiveSkill` is no longer computed and unused. The failure roll's odds are a function of what the
operator can do: a competent hand lowers them, an unskilled one raises them, at all seven skills (the
department's skill leads; first-contact training adds for science and command). Measured, one task at
40% condition under battle stations (`test_ship_core --work`):

```
  the same task, two skill levels
    engineering 1              odds 0.2568 (base 0.2623)   measured 5138 of 20000
                               reasons: skill engineering skill 1.0 +0.10; morale morale (fit): carrying on -0.12
    engineering 5              odds 0.1519 (base 0.2623)   measured 3039 of 20000
                               reasons: skill engineering skill 5.0 -0.30; morale morale (fit): carrying on -0.12
```

The same person at two skill levels produces different outcomes -- 5,138 of 20,000 against 3,039 --
and the difference is the named skill factor.

**Judgement call.** The skill factor reads the *raw rating* (plus the aptitude trait), and the drag a
condition or a deficit imposes is named separately, below. `EffectiveSkill` remains the summary read
(the console prints both: *"effective engineering 0.0"* beside the decomposed factors). If the roll
had consumed `EffectiveSkill` directly, a condition inside it would be unnamed at the point of the
roll, which is the one thing this pass forbids.

## Task B -- each kind reaches the work, kept distinct

The four kinds are not flattened into one number. Each reaches the odds by its own route, and the
route is testable (the units are in `tests/ship/test_ship_core.cpp`; the demonstration is the
transcript below).

- **A condition** is temporary and situational: a debuff raises the odds while it holds, a buff lowers
  them, by its magnitude word. `afraid (slight) +0.15`, `hungry (clear) +0.30`. The same condition
  costs the same at any load -- it is a condition, not a trait.
- **A trait** is durable and behavioural: it reaches the work by **interacting with the moment**, not
  as a flat shift. `steady under fire` does nothing on a quiet watch (its factor is not even present
  at stress 0) and helps under load; `claustrophobic` bites hardest on a hull or hazardous task;
  `adaptable` softens the state drag. `quick healer` is a trait too, and it never touches the odds --
  it shapes recovery. That difference is the point: not every trait is a work modifier.
- **A drive** is a motive: it **biases what a person does and how they bear up, conditionally on the
  work** -- never a flat subtraction. `afraid of decompression` costs on a hull task and costs nothing
  on a routine one; `afraid of the Borg` costs on a reclamation; `wants promotion` (or `to prove`)
  *helps* under load, while `wants to be left alone` does the minimum and `afraid of being seen a
  coward` freezes. The same person, only the work changed, moves the odds.
- **Morale** is read from its three components and reaches the work: a person who does not believe the
  course is worth the cost (`outlook`, and `holdings` with it) works like one who does not, and the
  component most responsible is named. `morale (worn): does not believe the course is worth the cost
  +0.08`. The deficit (heart) reaches the work through the same read, and its reason names it (*short
  of what they need*, *short on sleep*).
- **A capability** (species) is **never a bonus**, and it is the two-sided test of Task D. It is a new
  factor kind, `WORK_CAPABILITY`, so it stands apart from the four; it is discussed below.

## Task C -- an outcome is explainable, or it is indistinguishable from a bug

Every factor that touched an outcome is named, with its kind and its weight, and **the reason travels
with the work**: the anomaly line the simulation writes carries the full list. This is the line a real
use produced, in the transcript (`test_ship_core --work`):

```
    console: Crewman 035 at the transporters (hazardous work; effective engineering 0.0): skill engineering skill 0.0 +0.20; condition afraid (clear) +0.30; drive wants promotion, and leans in -0.10; drive afraid of being useless, and works harder -0.10; morale morale (at breaking point): short of what they need +0.12
    log    : Crewman 035: transporters: catastrophic anomaly at 40% condition under 100% load -- skill engineering skill 0.0 +0.20; condition afraid (clear) +0.30; drive wants promotion, and leans in -0.10; drive afraid of being useless, and works harder -0.10; morale morale (at breaking point): short of what they need +0.12
```

The console read is `WorkReading`; the log line is `UseSystemAt`'s own line with `WorkFactorLine`
appended. `TestReasonsRecoverableFromOutcome` asserts the requirement directly: it recomputes the
operator's factors and requires every one of their names in the anomaly line the log actually
carries. **A modifier the player cannot see would be worse than none**, which is why nothing here is
hidden: the factor list is complete and the weights are printed.

**And in the engine, on the real crew.** `scripts/condition-check.sh` is the existing check for the
condition-sets-the-odds ruling, and it passes unchanged (exit 0) -- the factors are appended, so its
regex still matches -- and now carries a named crew member's factors:

```
SHIP: day 0 08:17  [engineering] B'Elanna Torres: transporters: catastrophic anomaly at 40% condition under 100% load -- skill engineering skill 4.0 -0.20; trait poor with authority (under orders) +0.10; drive wants promotion, and leans in -0.10; drive afraid of being useless, and works harder -0.10; morale morale (fit): carrying on -0.12
```

That is the requirement met on a named crew member in a real run: the chief engineer's own skill, her
`poor with authority` trait, her `wants promotion` and `afraid of being useless` drives, and her
morale, all named on the outcome.

At the console, `ship operate` now states the operator's own picture before the use
(`module/ship/g_ship.cpp`), so a player who has just had a console go off in their face can see why
they were working badly. **What is drawn**: the console command output. **What is not drawn**: a
personnel-screen panel for another crew member's factors -- the read exists (`WorkReading`) and the
log carries it, but the panel is not built (named below).

## Task D -- the two-sided test

`docs/evidence/the-character-layer.md` records a Betazoid whose species line reads *"empathy: reads
feeling, not thought"*, *"others' pain arrives uninvited, which is a debuff as often as a buff."*
That sentence is the test of the pass, and it is in the factor list as a capability that is
situational and has no net sign. From the transcript:

```
  the Betazoid's empathy, two-sided (one faculty, two situations)
    Human, a medical task      odds 0.2246 (base 0.2623)   measured 4492 of 20000
                               reasons: morale morale (fit): carrying on -0.14
    Betazoid, a medical task   odds 0.1852 (base 0.2623)   measured 3706 of 20000
                               reasons: morale morale (fit): carrying on -0.14; capability empathy: reads the feeling, not the thought -0.15
    Human, a hull task         odds 0.1459 (base 0.2623)   measured 2919 of 20000
                               reasons: skill engineering skill 5.0 -0.30; morale morale (fit): carrying on -0.14
    Betazoid, a hull task      odds 0.1852 (base 0.2623)   measured 3706 of 20000
                               reasons: skill engineering skill 5.0 -0.30; morale morale (fit): carrying on -0.14; capability empathy: others' pain arrives uninvited +0.15
```

**Helping**: on a medical task, the Betazoid's odds are lower than the same person as a human
(0.1852 against 0.2246; 3,706 against 4,492 of 20,000), named `empathy: reads the feeling, not the
thought`. **Costing**: on a hull task, the Betazoid's odds are *higher* than the human's (0.1852
against 0.1459; 3,706 against 2,919), named `empathy: others' pain arrives uninvited`. The same
faculty, two situations, opposite signs. A capability that only ever helped would be decoration; this
one is not.

`TestBetazoidEmpathyTwoSided` asserts both directions, both names, and both measured counts.

## The demonstration the owner judges

`scripts/character-check.sh` prints, in full, the same task done by the same person in two conditions,
with the reasons (`test_ship_core --work`):

```
== the character layer reaches the work (docs/character-attributes.md)

  one person (Crewman 035, engineering 5), one task at 40% under battle stations
    fresh                      odds 0.1459 (base 0.2623)   measured 2919 of 20000
                               reasons: skill engineering skill 5.0 -0.30; morale morale (fit): carrying on -0.14
    afraid, hungry, worn       odds 0.3054 (base 0.2623)   measured 6109 of 20000
                               reasons: skill engineering skill 5.0 -0.30; condition afraid (slight) +0.15; condition hungry (clear) +0.30; morale morale (strained): does not believe the course is worth the cost +0.01
```

The same crewman, the same task, the same system, the same load: fresh, 2,919 anomalies in 20,000;
afraid and hungry and worn, 6,109 -- and the reasons are on the face of the print. The four kinds and
morale, each in one list:

```
  the kinds kept distinct (one person carrying all of them, a hull task)
    under a hull task          odds 0.5163 (base 0.2623)   measured 10328 of 20000
                               reasons: skill engineering skill 1.0 +0.10; condition afraid (clear) +0.30; trait steady under fire (under load) -0.30; trait claustrophobic (under load) +0.45; drive afraid of decompression (sealing the hull) +0.25; drive wants to go home, and hesitates +0.08; morale morale (at breaking point): does not believe the course is worth the cost +0.09
```

**What this transcript cannot decide** is whether it *feels* right -- whether a crew working tired and
afraid reads as an organisation under strain or as an arbitrary penalty. That is the owner's, and this
is the transcript he asked for.

## Morale: what was done, and why not more

Morale reaches the work through one factor read from `Morale(c)` and named by `MoraleReason(c)`. That
was small: no new state, no new save, and the existing test `TestMoraleIsReadFromThree` already proves
the read is explainable from deficit, outlook and holdings. **What was not built** is the rest of what
`docs/morale.md` says morale drives -- **compliance** (the soft authority gate: an order delayed, done
badly, refused), **voluntary risk**, **initiative**, and **breakdown**. Those are behavioural gates,
not odds, and they need the order system (`docs/access-and-authority.md`) and the job queue to carry a
refusal, which is a larger build than this lane. **The cost of not building them is named here rather
than half-built**: morale reaches whether the work is done well, not yet whether it is done at all.
The drives already bias *bearing up* under load (the `WORK_DRIVE` factors above), which is the first
half of the same behaviour; the explicit refusal path remains open.

## The save version, and what it costs

**Save format 53 is unchanged.** Everything the wiring needs is derived from state the save already
carries -- skills and traits from the seed, conditions and the three morale reads from the save -- so
there is no new field, no version bump, and **older saves still load**. The only measurable cost is
that a log line that carries an anomaly is longer (it now names the factors); the log is text, not
versioned, and the anomaly line is written only when an anomaly is drawn. `TestCharacterSaveRoundTrip`
and the whole `scripts/test.sh` suite pass unchanged, and this lane changes no save semantics.

## The checks, and what they returned

```
scripts/test.sh
  -> crew_core: all checks passed
     ship_core: all checks passed, including TestEffectiveSkillReachesTheOdds, TestConditionMovesTheOdds,
       TestTraitShapesPerformance, TestDriveBiasesBehaviour, TestMoraleReachesTheWork,
       TestReasonsRecoverableFromOutcome, TestBetazoidEmpathyTwoSided (and the existing
       TestConditionOdds, TestTransporterAnomaly, TestCharacterSaveRoundTrip)
     tools OK; python syntax ok; shellcheck ok; 20 patches

scripts/character-check.sh
  -> PASS  three generated crew in full; morale broken into three components; every condition with
     source, cure and visibility; species as capabilities and needs; the manner across three levels;
     the four kinds and morale reach the work, each reason printed, the Betazoid's empathy both
     helping and costing

scripts/check.sh --source-map build/gdk/maps/eliteforce_voyager_maps/voy1.map --script-corpus build/gdk/scripts
  -> entity dictionary 318 classes; validator negative tests ALL PASS;
     2,024 files: 2,016 compiled and read back, 8 rejected -- exactly the known, documented set;
     exit 0

scripts/condition-check.sh
  -> PASS  a nominal system is beamed fifty times with no anomaly; the console states the condition
     before the act; the same system degraded and at battle stations lets go, writes the chain to the
     log, and changes or hurts a record -- now with the operator's factors named on the anomaly line;
     exit 0

module build
  -> cmake --build ... --target efgame: [100%] Built target efgame
     (only the pre-existing -Wwrite-strings warnings; nm shows ship::WorkFactors, WorkOdds, RollWork,
      WorkFactorLine, WorkReading and DepartmentSkill in libefgame.so)
```

## What could not be verified

- **Whether the feel is right.** The transcript is the demonstration; whether a tired, afraid crew
  reads as strain rather than as an arbitrary penalty is the owner's judgement, and this lane only
  proves the mechanism is measured and attributable.
- **The weights.** `[inv]` and small, recorded so they can be overruled. Whether a sharp debuff
  *should* be +0.50 and a competent hand -0.30 is a tuning question the felt run answers.
- **A personnel-screen panel** for another crew member's factors. The read (`WorkReading`) and the log
  exist; drawing a second crew member's factors on the personnel screen is not built.
- **Compliance, initiative and refusal** -- the rest of what morale drives (above).
- **A logged-in session** reading the console's `WORK` line: the module builds and the path is
  compiled, but a person at the console has not read it in a logged-in run.

## Judgement calls, named as calls

1. **The factors move the odds multiplicatively, not additively.** An additive term would let an
   unskilled operator make a nominal system (odds exactly zero) fail, which breaks
   `docs/failure-is-content.md`'s promise that the top tenth is nominal. A multiplier leaves a
   well-kept system untouchable and keeps the condition setting the base. **This is a call.**
2. **The operator is the named person, or the station's own hand.** A use with no named operator
   (the ship's own transport, a console nobody is at) reads the first fit, unbrigged crew member
   posted at the system; the injury already landed on that same person, so the operator and the victim
   are now the same hand by construction. **This is a call, and it is why the crew's own work bites
   and not only the player's.**
3. **`WorkContext` is set by the system, not by a general job kind.** A hull task, a medical task and
   a hazardous use are read from the system; `WORK_RECLAIM` exists for reclamation but is not wired to
   a system yet (reclamation is a deck job, `UpdateJobs`). **This is a call.**
4. **The skill factor reads the raw rating, and the drag is decomposed.** If `EffectiveSkill` were
   consumed directly, a condition inside it would be unnamed at the roll; the summary is still printed
   (`WorkReading`). **This is a call**, and it is the one that makes Task C possible.
5. **The capability factor is the species reading, and it is two-sided.** `docs/character-attributes.md`
   forbids a species multiplier, so this is not a bonus: the same faculty has a negative weight in one
   situation and a positive one in another, with no net sign, and it is named. **This is a call**; if
   it is judged a hidden species modifier it should move into the condition vocabulary, where the
   document says species differences live.
6. **The weights and the contexts are ours.** `WORK_SKILL_STANDARD` (2.0), the condition weights
   (0.15 / 0.30 / 0.50), the morale slope (0.40), the multiplier cap (4.0), and the trait/drive
   pairings are `[inv]`, small, and recorded so they can be overruled.
7. **No save version bump.** Nothing new is stored, so bumping would invalidate older saves for
   nothing. **This is a call**, and the cost is exactly zero.
