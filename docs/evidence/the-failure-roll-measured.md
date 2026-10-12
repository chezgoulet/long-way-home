# Does a condition reach the failure roll? — C-23 measured

`docs/designs-against-code.md` C-23 is the fork the owner ordered **measured rather than ruled**: *"the
failure roll uses raw skills through `WorkFactors`, while the document names `EffectiveSkill` — the function
where a crew member's conditions bite. Rather than rule blind, I would have a pass measure whether a condition
reaches the roll today and bring you the number."*

**This pass decides nothing.** It answers one question with evidence and hands the fork back priced. The roll
is not changed and `docs/character-derivation.md` is not changed; the only code touched is the measurement
itself (a unit test, `TestConditionReachesTheFailureRoll`, and a printed section in the `--work` transcript),
and the only documents touched are this evidence and the register's own C-23 row.

**Date:** 2026-10-12, on `feat/the-failure-roll-measured`, cut from `testing`.

## The question, exactly

**When a crew member carries a condition, does the odds calculation they face change?** And if the roll were
made to call `EffectiveSkill` — the function the document names — what would change, in the same units?

## The path, drawn

The failure roll is reached from a use of a system. The call chain, with file and line on this branch:

```
UseSystem            module/ship/ship_core.cpp:5535
  -> UseSystemAt                          :5481
       resolves the operator (the named person, or the station's own hand)  :5490-5501
       -> WorkFactors     :5661            the operator's named factors
            skill term, from the RAW rating:  base = c.skills[skill] (+ the aptitude trait)  :5671-5678
            conditions, each its own factor:  WORK_CONDITION, delta = +mag (debuff) or -~ mag (buff)  :5682-5690
            traits / drives / morale / species capability                                            :5692-5751
       -> RollWork        :5766
            -> WorkOdds    :5756            mult = 1 + sum(deltas); odds = AnomalyOdds(condition) * mult
                 -> AnomalyOdds  :5577      the system's condition sets the base; the top tenth is 0
            the draw is AnomalyRoll  :5608  (the saved counter and the seed)

EffectiveSkill       module/ship/ship_core.cpp:795
  base = c.skills[skill] (+ the aptitude trait)          :798-801
  minus drag = 0.5*deficit + 0.3*fatigue + condition magnitudes (debuffs only)  :803-808
```

`EffectiveSkill` is **not on the roll's path**. It is called only from `WorkReading` (`:5796`), from
`OfferRising`/`RisingOdds` (`:5915, :5933`), from the rising transcript (`:6029`) and from the console
(`module/ship/g_ship.cpp:3706`). The roll reaches a condition — but by a **parallel construction**: the skill
term reads the raw rating, and each condition is added as its own named factor, which is what makes the
outcome attributable (`docs/character-attributes.md`, "The wiring: how each kind reaches the work").

**Answer to the question as asked: yes, a condition reaches the odds today**, through `WorkFactors`. The fork
is not "does it reach" but "which construction reaches it".

## The measurement

Same person, same task, clean and conditioned. One operator, engineering 3, a routine engineering task at 40%
condition under battle stations; morale exactly neutral (`PlainCrew`) so the morale factor is absent and the
condition is isolated. The instrument is `MeasureWorkFork`, a unit test over `WorkFactors` / `WorkOdds` /
`RollWork` (the engine is not needed). Quoted from `test_ship_core --work`:

```
  C-23, measured: does a condition reach the roll, and the fork's size
    one person (engineering 3), engineering routine, 40% under battle stations
      clean                    odds 0.2361   measured 4724 of 20000
      afraid (sharp)           odds 0.3673   measured 7347 of 20000   (+0.1312 odds)
      COND_NONE (counter)      odds 0.2361   measured 4724 of 20000   (+0.0000 odds)
      nominal system (counter) odds 0.0000   (the top tenth: the operator cannot move it)
    if the roll called EffectiveSkill (2.67 clean / 2.08 conditioned; condition subsumed):
      clean                    odds 0.2446   measured 4894 of 20000
      afraid (sharp)           odds 0.2604   measured 5208 of 20000   (+0.0157 odds)
    the condition's own weight, in the odds multiplier: today +0.50 (WORK_COND_SHARP);
    through EffectiveSkill +0.0600 (a sharp debuff drags effective skill 2.67 -> 2.08, slope 0.10/point)
```

**The difference, today.** The same clean person at the same task draws 4,724 anomalies in 20,000; carrying a
sharp debuff, 7,347. The condition is worth **+0.1312 in odds** (a 55.6% increase over the clean case), and
`WorkFactorLine` names it — *condition afraid (sharp) +0.50*. The measurement moves.

## The counter-case

A measurement that moves for every input measures nothing, so two inputs that **must not** move the odds are
shown not moving them.

1. **A condition slot carrying `COND_NONE`** — a record present in `conditionCount` whose id is the sentinel.
   `WorkFactors` skips it (`ship_core.cpp:5684`), so the odds and the draws are identical to the clean case:
   0.2361 / 4,724 of 20,000, `+0.0000`.
2. **A nominal system** (condition 0.95, the top tenth). The design promises the operator cannot move it —
   `AnomalyOdds` returns 0 above `NOMINAL_CONDITION`, so `WorkOdds` is 0 whatever the factors. The same
   conditioned person draws nothing: odds 0.0000. The condition is present and the odds do not move.

Both are asserted in the test. Because the instrument reports a difference for the debuff and no difference
for the sentinel and for the nominal system, the difference it reports for the debuff is the condition and not
the act of measuring.

## What would change if the roll called `EffectiveSkill`

The hypothetical, stated so it can be disagreed with: the skill term is rebuilt from `EffectiveSkill(c,
skill)` instead of the raw rating, and the **separate condition factor is subsumed** — the condition is already
inside `EffectiveSkill`'s drag, so keeping both would count it twice. The weights are `WORK_SKILL_STANDARD`
2.0 and `WORK_SKILL_SLOPE` 0.10 (read from `module/ship/ship_core.cpp:5622-5623`; not exported through the
header). From the same run:

- **The conditioned person's odds fall from 0.3673 to 0.2604** (7,347 → 5,208 of 20,000).
- **The condition's own weight in the odds multiplier falls from +0.50 to +0.0600** — **8.3× smaller**. A
  sharp debuff drags effective skill by 0.60, and the skill slope is 0.10 per point, so a sharp condition
  becomes a 0.06 nudge to the multiplier rather than a 0.50 one.
- **The clean baseline also moves, 0.2361 → 0.2446 (+0.0085, +3.6%)**, because the deficit (0.65 in this
  person) now enters the skill term. For anyone whose morale is *not* neutral, the deficit would then be
  counted twice — once inside `EffectiveSkill` and once through the morale factor — which is a further cost of
  the fork, not measured here.

**And the fork was run for real, to price what it breaks.** On a throwaway worktree at `HEAD` with the change
applied to `WorkFactors` (skill term ← `EffectiveSkill`; condition factor dropped) and then removed, the whole
suite was re-run: **exactly one existing test moves — `TestConditionMovesTheOdds`, 5 of its checks fail**
(`an > bn`; the names `afraid`, `sharp`, `condition`; the log-naming check), because a condition that is
inside `EffectiveSkill` is no longer a separately named factor. Every other test in `tests/ship` passes
unchanged. The `--work` transcript's own demonstration changes shape with it:

```
    afraid, hungry, worn       odds 0.2658 (base 0.2623)   measured 5316 of 20000   [fork A]
                               reasons: skill engineering skill 2.0 -0.00; morale morale (strained): does not believe the course is worth the cost +0.01
    afraid, hungry, worn       odds 0.3578 (base 0.2623)   measured 7158 of 20000   [as shipped]
                               reasons: skill engineering skill 3.0 -0.10; condition afraid (slight) +0.15; condition hungry (clear) +0.30; morale morale (strained): does not believe the course is worth the cost +0.01
```

The fork keeps the condition *biting* (the odds still move) and removes it from *what did it* — which is the
property `docs/character-attributes.md` calls "No hidden modifiers", and the reason the parallel construction
exists (`docs/evidence/the-work-bites.md`, judgement call 4). Fork A has a second design decision inside it:
either the conditions stay listed as well (and are counted twice), or they are not (and are hidden).

## The price of each fork

Two numbers, in the same units.

- **Fork A — make the roll call `EffectiveSkill`.** The condition's weight in the odds multiplier falls from
  **+0.50 to +0.0600 (8.3× smaller)**; the conditioned person's odds fall by **0.1069** (from 0.3673 to
  0.2604; 2,139 fewer anomalies per 20,000 uses), of which the condition's own marginal share falls from
  **+0.1312 to +0.0157**; the clean baseline moves **+0.0085**. **1 test moves (`TestConditionMovesTheOdds`,
  5 checks)**, and the named weights in `docs/character-attributes.md` (the factor table) and
  `docs/evidence/the-work-bites.md` (Task B, calls 4 and 6) become wrong, and the `scripts/character-check.sh`
  transcript changes. Fork A also needs a sub-decision: keep the condition named (double count) or let it
  hide.
- **Fork B — change the document to name `WorkFactors`.** **0 code changes, 0 tests move, 0 save changes.**
  Two sentences in two documents stop claiming `EffectiveSkill` "is what decides whether a repair is quick,
  slow, or dangerous" — `docs/character-derivation.md:234-235` and `docs/character-attributes.md:64-65` — and
  name the mechanism the roll actually calls, as `docs/character-attributes.md:97` and the code header
  (`module/ship/ship_core.h:684-708`) already do. The register's C-23 row then closes.

In one line: **fork A buys nothing the roll does not already have and costs an 8.3× change in every
condition's weight plus a test and two documents; fork B costs two sentences.**

## What is left (the fork, still open)

The register's C-23 row records this measurement and **the fork remains open for the owner**: whether the roll
should call `EffectiveSkill` (and the condition weight drop 8.3×, with the naming sub-decision above), or
whether the document should name `WorkFactors` (and nothing in the game change). Nothing is ruled here.

## What could not be established

- **Whether the weight is right.** The weights (0.15 / 0.30 / 0.50 for conditions, 0.10 per skill point) are
  the code's own, marked `[inv]`. This measures what the code does, not whether a sharp debuff *should* be
  worth 0.50 or 0.06.
- **The feel.** Whether a condition that moves the odds by 55% reads as strain rather than as an arbitrary
  penalty is the owner's judgement; this is the number, not the verdict.
- **The deficit double-count under fork A, measured.** It is named and reasoned above but not run: this
  person's morale is neutral, so the double count does not appear in the numbers. A non-neutral person would
  show it.
- **The engine.** The module was not rebuilt and the console was not read; the measurement is at the unit
  level, which the brief names as the right instrument.
- **Whether fork A should keep the conditions named.** Priced as a sub-decision, not decided.

## Judgement calls, named as calls

1. **"The roll calls `EffectiveSkill`" is modelled as: the skill term reads `EffectiveSkill`, and the separate
   condition factor is subsumed.** A condition is already inside `EffectiveSkill`'s drag, so keeping both would
   count it twice. **This is a call.** The alternative (keep both) makes conditions bite *harder*, not softer,
   and is the other half of the sub-decision named above.
2. **Two counter-cases, not one.** The brief asks for "a condition that should not move the odds"; two
   qualify — the `COND_NONE` sentinel, and a nominal system (where the operator is promised no reach).
   **This is a call**, made because the second is the design's own promise and is stronger.
3. **The test asserts reach and the counter-cases, and asserts neither magnitude.** A condition moves the
   odds, and moves them through `EffectiveSkill` too; which magnitude is the design is the owner's, so the
   test does not encode the answer. **This is a call.**
4. **"Which tests move" was measured on a throwaway worktree, at `HEAD`, then removed.** No change to the roll
   is committed. **This is a call** — and it is the only way the number is not an opinion.
5. **The isolated person is `PlainCrew`** (morale exactly 0.5, no traits, no drives) so the condition is the
   only difference. A real crew member carries a morale factor too, which would interact with the fork's
   deficit handling. **This is a call.**
