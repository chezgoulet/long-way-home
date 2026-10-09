# Rising to the occasion: the act, its cost, and the memory it leaves

`docs/rising-to-the-occasion.md` (the design), `docs/character-attributes.md` (the four kinds and the
wiring), `docs/damage-and-budgets.md` (condition sets the odds, stress sets the severity) and
`docs/memory-and-consequence.md` (the marks), on `feat/rising`, cut from `testing`, 2026-10-08.

**The design in one line: the choice is the heroism, not the success.** A hero is someone who does the
thing nobody could ask them to do, and pays for it. This pass builds that as a **rule over the
character layer**, not beside it: the drive realisation, the failure roll, the severity ladder and the
mark system are all the ones the work-bites pass already put in place.

**Observed, and the command that produced it.** Everything below is the output of a command named
beside it, on this tree, after
`cmake --build /home/c/big/git/upstream/efgame/build-linux --target efgame` (which built
`libefgame.so` with only the pre-existing `-Wwrite-strings` warnings) and after the unit tests
(`scripts/test.sh`). No new translation unit was added, so the configure-time-glob trap does not
apply.

## What was built, and where

One mechanism, in `module/ship/ship_core.{h,cpp}`, beside the work factors it extends. No game
header, no engine code, no new art, no assets, no audio.

- `RisingDriveAtStake(c, skill, context, stress)` -- **the drive the work realises**, read from the
  `WORK_DRIVE` factor `WorkFactors` already computes. It is the same reading, not a second one: a
  fear realised by the task in front of them (`FEAR_DECOMPRESSION` on a hull job, `FEAR_THE_BORG` on
  a reclaim, `FEAR_DYING_ALONE` on a hazard job), and the load-based desire answers when no fear does.
- `OfferRising(s, post)` / `OfferRisingTo(s, post, crew)` -- **the offer**: the post that must be
  held, the person in front of it, whether they are beyond their effective skill, the drive at
  stake, and the decision that answers it. A pure read.
- `RisingWouldGo(c, o, reason)` -- **the decision**, from the drives and the morale, with the reason
  in the log's own words. Not a dice roll.
- `RisingOdds(o, c, f, n)` -- the odds of the attempt, the person's own factors **made worse by how
  far it is beyond their effective skill**, and never impossible.
- `AttemptRising(s, post, crew, roll, out)` -- one attempt end to end: the decision (a refusal is
  logged), the roll, the cost (a condition, a wound, and at the extreme their life), and the
  witnesses' memory. `NextRisingRoll(s)` is the saved deterministic draw it takes.
- `RetellRising(s, hero)` -- the crew retell it, which reinforces the mark.
- `RisingOutcomeName` -- none / refused / held / failed / died.

The console is `ship rise <system> [crew]` in `module/ship/g_ship.cpp`, and the engine transcript
under `g_shipTest 73` is the one the owner judges. **Corrected 2026-10-09: it was `g_shipTest 60`,
which the deck 14 re-dress had already claimed on 2026-10-06** — a duplicate case number meant the
earlier block ran and quit the run before this one, and deck 14's own check failed silently until the
game-data sweep found it. The model transcript is `test_ship_core --rising`.
The check is `scripts/rising-check.sh`.

## Task A -- the offer, and the drive is the lever

A post must be held and nobody who can hold it is available: **the hand in front of the post is
beyond their effective skill.** It is effective skill, not the raw rating, so a rated hand whom
conditions or fatigue have taken below the post is still beyond it:

```
  a rising is offered only when the hand in front of the post is beyond their skill
    (TestRisingOfferRequiresBeyondSkill: offered at skill 0; not offered at engineering 5; offered
     again when a rated hand is dragged below the post by what they carry)
```

**The lever is the drive, not the difficulty.** The same crisis is a hero's moment for one person and
a Tuesday for another, and the difference is the character layer doing its job. The offer names the
drive the work realises -- in the engine's own words, on the real crew:

```
SHIP: RISING why them: afraid of being useless, and being useful is all they have: goes anyway
```

**`FEAR_COWARDICE`.** It is the axis a refusal runs on, and it is used as one: a person afraid of
being seen a coward goes rather than be thought one while their morale holds (`m >= 0.25`), and is
**frozen** -- refuses -- only when it has gone (`m < 0.25`). That is the cowardice axis spent on
the *refusal*, and it is named in the reason either way.

## The post, not the name (`docs/the-entry-point.md`)

**The act addresses the post; the simulation resolves it to a person.** `OfferRising(s, post)` and
`AttemptRising(s, post, ...)` take a system id, and `RisingOperatorAt`/`FirstFitOfDept` resolve the
person -- the same resolution `UseSystemAt` makes. Nothing in the mechanism names a crew member, so
**the same scenario produces a different heroism with a different crew**, which is what the
post-not-name law exists to allow. The engine transcript names a person only because the roster
generated one; on an all-fictitious crew the same line names someone else.

## Task B -- the exceeding

The attempt is made **beyond the person's effective skill**. The odds are the failure rule's own, with
the shortfall to a qualified hand (`RISING_QUALIFIED = 3.0`) worsening them by `RISING_BEYOND_SLOPE
= 0.35` a point. **Worse than a qualified hand, and never impossible** (the odds of failing are capped
below 1 by `RISING_MIN_SUCCESS = 0.05`):

```
  the exceeding: the same post and crisis, two hands
    effective engineering 0.0   odds of failing 0.564 (holds 44%)
    effective engineering 5.0   odds of failing 0.144 (holds 86%)
```

The same post (structural integrity at 40% under a green watch), the same crisis: the hand beyond
their skill fails more than half the time, the qualified hand about one in seven. **No courage roll,
no heroism stat, no bar** -- the only number is the shortfall to `RISING_QUALIFIED`, derived from the
effective skill that already existed, and every factor that moved the odds is named on the outcome
(the same `WorkFactorLine` the work-bites pass prints).

## Task C -- the cost, which is not optional

The cost is the **severity the condition and the load set** (`AnomalySeverityFor`), applied to a
person: a held post still costs a condition (exhausted, or hypoxic on a hull or hazardous job), a
failure costs a wound, and **at the catastrophic severity it costs their life.** A draw at zero at
40% condition under a red alert is catastrophic and kills; the same draw under a yellow alert is
acute and wounds; under a green watch it is degraded and costs only the condition. Measured in
`TestRisingFailureStillCostsAndIsRemembered` and `TestRisingAtTheExtremeKills`.

**The cost goes in the record**, with its cause, through the same `Sicken` path everything else uses:

```
SHIP: day 0 08:05  [crew] The Doctor: B'Elanna Torres comes down with exhausted (held the structural integrity)
```

## Task D -- the witnesses remember, which is how a hero is made

**Who counts as a witness, and why: everyone on the deck when it happened.** Presence is the
provenance, and a `MEM_SAW` mark means *"saw it myself"* (`docs/memory-and-consequence.md`); telling
the rest is `Brief`'s path (`MEM_TOLD`), and is not this. `NoteDeath` already uses deck presence for
exactly this reason, so the act is consistent with how a death is witnessed. On the real ship that is
Main Engineering:

```
SHIP: day 0 08:05  [crew] Kathryn Janeway: 13 saw it; B'Elanna Torres is remembered for it by name
```

Each witness takes a **positive mark naming the hero** (`Remember(..., MEM_RESCUE, hero, MEM_SAW,
+valence)`), which is what moves `Bond` toward them, and their **`holdings` rises** -- the allegiance
read, so morale moves with it (`docs/affinities-and-allegiance.md`). From `--rising`:

```
    the witnesses now carry Crewman 041: bond 0.70, holdings 0.92, salience 1.00
```

**`MEM_RESCUE` was used, not a new event.** It is the vocabulary's mark for a person who was saved by
another's act, which is what holding the post is from the witnesses' side; a new event would need a
vocabulary decision the design has not asked for, and the brief says to use the existing one.

**And it is legible later, not recorded once.** A mark's `salience` decays unless reinforced
(`MemoryDecay`), so a heroism nobody talks about fades, and one the crew keep retelling persists.
`RetellRising` is the retelling; the measurement is in the same transcript:

```
    60 hours on, untold: salience 0.40
    retold: salience 1.00
```

That behaviour is the model's own (`docs/memory-and-consequence.md`); this pass made sure it applies
to the act rather than defeating it.

## Task E -- both refusals are possible

**The person may refuse, as a decision from their drives and morale, and the log says why.** From
`--rising`, the same person and the same post as one who goes:

```
    wants home, afraid of decompression: wants to go home: will not spend themselves on this; afraid of decompression (sealing the hull)
    -- and the offer is refused
    the record: Crewman 031 will not hold the structural integrity: wants to go home: will not spend themselves on this; afraid of decompression (sealing the hull)
```

A refusal changes no state; it is legible, and the drive that decided it is named. **The attempt may
fail, and a failed attempt is not a wasted one:** it still costs, and it is still remembered. The
engine run above held `failed` -- the post let go, the hero took the condition, and 13 witnesses still
carry the name.

## The fork, applied by default and named so it can be overruled

**The design recommends that the drive decides what low morale does, and this pass applies that as a
recommendation rather than a ruling.** At or above `worn` morale the desire carries them; below it the
drive decides: *afraid of being useless* goes anyway (being useful is all they have), *wants to go
home* refuses (nothing left worth spending), *wants promotion* or *to prove* takes it while there is
anything left to lose and not when there is nothing, and *afraid of being seen a coward* goes rather
than be thought one and is frozen only at the end. Both readings of a person with no future are in the
model; the fork is the owner's to rule on, and moving it is one function (`RisingWouldGo`).

## Who counts as a witness -- the call, restated

- **Everyone on the deck, and no one else.** Not the whole crew, not the department, not "everyone who
  heard". Presence is the only provenance a `MEM_SAW` mark can honestly carry, and it matches
  `NoteDeath`. A person elsewhere only learns of it if the crew tell them, which is `Brief`'s job.
- That makes the witnesses a small-to-medium set that the simulation can name, and it makes the act's
  reach depend on where people were -- which is the point.

## The save version, and what it costs

**Save format 53 is unchanged, and no version bump is needed. The cost is exactly zero.** Everything
the act writes is already in the save: conditions and their source, the three morale reads (one of
which, `holdings`, the regard moves), each person's marks with their provenance and salience, and a
death. There is no new field and no new stored state; `TestRisingSaveRoundTrip` packs a rising,
unpacks it, and finds the condition, the marks and the bond intact. The draw is taken from
`s.riskRolls`, the counter `UseSystem` already draws on and the save already carries, so the act
replays identically across a save.

## The checks, and what they returned

```
scripts/test.sh
  -> crew_core: all checks passed
     ship_core: all checks passed, including TestRisingOfferRequiresBeyondSkill,
       TestRisingDriveDecides, TestRisingCostAndWitnesses,
       TestRisingFailureStillCostsAndIsRemembered, TestRisingAtTheExtremeKills,
       TestRisingOddsBeyondSkill, TestRisingSaveRoundTrip (and every existing check)
     tools OK; python syntax ok; shellcheck ok; 20 patches

scripts/character-check.sh
  -> PASS  unchanged (the four kinds and morale reach the work)

scripts/rising-check.sh
  -> PASS  a post is held beyond the hand's skill; the record says why them and what it cost;
     the witnesses remember the name
     (engine output: "SHIP: RISING instrument: B'Elanna Torres, engineering 0, effective 0.0, at the
      structural integrity"; "why them: afraid of being useless ..."; "failed ... odds it holds 25%";
      "13 saw it; B'Elanna Torres is remembered for it by name"; "comes down with exhausted")

scripts/condition-check.sh
  -> PASS  unchanged (condition sets the odds, stress sets the severity)

module build
  -> cmake --build ... --target efgame: [100%] Built target efgame
     (only the pre-existing -Wwrite-strings warnings; nm shows ship::OfferRising, OfferRisingTo,
      RisingDriveAtStake, RisingOdds, AttemptRising, NextRisingRoll, RetellRising and
      RisingOutcomeName in libefgame.so)
```

## What could not be verified

- **Whether a heroism *lands*** -- whether it reads as someone spending themselves rather than as a
  lucky roll. That is the owner's, and the engine transcript above is the demonstration he judges.
- **The weights.** `RISING_QUALIFIED` (3.0), `RISING_BEYOND_SLOPE` (0.35), `RISING_MIN_SUCCESS`
  (0.05), the witness valence and the holdings step are `[inv]`, small, and recorded so they can be
  overruled.
- **A scenario that generates the offer.** This pass builds the rule; who and when a scenario offers
  it to (and the configurator that authors the situation) are the next work, and the brief forbids
  building them here.
- **A console panel** for the offer: the console command exists and the record carries it, but no
  screen draws it.

## Judgement calls, named as calls

1. **Availability is the post's own hand.** "Nobody who can hold it is available" is read as the hand
   in front of the post being beyond their effective skill, not as a scan of the whole roster: on a
   ship always short of crew, a qualified relief somewhere else is a reassignment decision, not an
   availability fact, and the offer must be able to fire. **This is a call.**
2. **The drive realisation is read from `WorkFactors`.** `RisingDriveAtStake` takes the first
   `WORK_DRIVE` factor the work-bites pass computes; it does not reread the drives. **This is a
   call**, made because the brief forbids a second reading.
3. **The cost is a condition always, a wound on a failure, and death at the catastrophic severity.**
   The "loss of something they had" alternative is not built; the condition, wound and death cover the
   four kinds the design lists. **This is a call.**
4. **The witnesses are the deck, and the regard step is `holdings`.** Bond is derived from the mark;
   `holdings` is the one stored read that carries allegiance, and moving it moves morale, which is what
   the design asks for. **This is a call.**
5. **`MEM_RESCUE` rather than a new event.** **This is a call**, and it is the brief's own preference.
6. **The fork is applied as the design recommends, not ruled.** `RisingWouldGo` is one function, so
   the owner can overturn it in one place. **This is a call.**
7. **No save bump.** Nothing new is stored, so bumping would invalidate older saves for nothing.
   **This is a call**, and its cost is exactly zero.
