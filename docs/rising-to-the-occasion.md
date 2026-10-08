# Rising to the occasion

**The owner's question, 2026-10-08:** *does that mean we will also have characters that really rise to the
occasion and are heroes?*

**Answer: yes, and not by the same mechanism as the buffs.** The distinction is the whole document.

## Two different things, and only one of them is the one that matters

**A buff is cheap.** `docs/character-attributes.md` carries traits and conditions with a sign, and
*"steady under fire"* is already a trait. A positive condition lifting performance — *inspired*, *steadied*,
*protecting someone they love* — is the **same wiring with the opposite sign**, and it should exist. It costs
almost nothing to build, and this document is not about it.

**Rising to the occasion is not a buff, and building it as one would waste it.** A hero in *Long Way Home*
should not be someone with a +2. **It should be someone who does the thing nobody could ask them to do, and
pays for it.** The game's subject is attrition with no reset; the corpus already says the captain's failure
mode is **losing the crew's confidence**, and that a chair you cannot be removed from would not be a game. **A
stat bonus touches none of that.**

**So: the choice is the heroism, not the success.**

## The shape

A crew member, under pressure, may **exceed their own numbers** — and it costs them.

**When.** A situation in which a post must be held and nobody who can hold it is available, or in which the
*drive* of the person in front of it is at stake: someone *afraid of decompression* at a breach; someone
*worried they are useless* with a system failing; someone *wanting promotion* with the captain watching. **The
drive is the lever, not the difficulty** — the same crisis is a hero's moment for one person and a Tuesday for
another, and that difference is the character layer doing its job.

**What happens.** The attempt is made **beyond the person's effective skill** — that is the rising. The odds
are worse than the task would be for someone properly qualified, and they are *not zero*.

**What it costs, and this is not optional.** At least one of: a wound, a new condition (*exhausted*, *burned*),
a mark in the record, the loss of something they had. **And at the extreme, their life** — this is a game where
people die and stay dead, and a heroism that cannot kill is a cutscene.

**What it leaves.** **A memory in the witnesses.** The corpus already has marks with valence and provenance, and
`docs/affinities-and-allegiance.md` has allegiance. So an act like this **writes into the people who saw it**:
the crew's regard for that person rises, morale moves, and the record carries it. **That is how a hero is made
rather than granted — the act is remembered, and the remembering is the reward.**

**And both refusals must be possible.**

- **The person may refuse.** Not as a dice roll — as a decision from their drives and morale. A crew member who
  refuses is not broken; they are *legible*, and the log says why.
- **The attempt may fail.** A heroism that always works is not heroism, it is a cutscene with the ending
  spoiled.

**Neither of those is a courtesy. They are the mechanic.** Remove the refusal and the act is compulsory, which
makes it obedience rather than courage. Remove the failure and there is no stake, which makes it theatre.

## The fork, which is the owner's

**What does *low* morale do to this?** A person whose morale reads *"does not believe the course is worth the
cost"* is either:

- **the one who refuses** — they have stopped believing, so they will not spend themselves; or
- **the one who goes anyway** — because there is nothing left to lose, and a person with no future is the one
  who will trade it.

**Both are true of people, and the design should not pick one.** The recommendation is that **the drive
decides**: *wants to go home* and *wants promotion* spend themselves for different things, and *afraid of being
useless* is a person who will do something reckless precisely because being useful is all they have. **The
mechanic should read the drive and the morale together, and the fork belongs to the owner to rule on.**

## Why this matters more than it looks

**Heroes are how attrition becomes meaning.**

This is a game with no reset, where the dead stay dead and the ship only gets worse. In that design, **the only
thing that makes a loss bearable is that someone did something with their time.** A run where people are lost
and nothing was ever done is not tragic — it is merely dull. So rising to the occasion is not a nice-to-have
bolted onto a survival game; **it is the answer to why any of it should feel like anything.**

**And it is the positive half of the same mechanic now being built.** The pass in flight makes conditions reach
the work — *afraid* and *short of sleep* make a person worse. **This document is the other direction**, and the
two together are what make a crew a cast rather than a table.

## What it needs, and what it does not

**Verified against the code, 2026-10-08 — and this line replaced an earlier one that was wrong.**

**Already there, and better than the first draft claimed:**

- **The witnesses.** `struct Memory` carries `source` (`MEM_SAW` / `MEM_TOLD` / `MEM_RUMOUR` / `MEM_LOG`) — **which
  is witness provenance, first-class** — a `person` field for *who the mark involved*, and a `valence` whose
  positive end is written as **"pride/relief"**. `Remember(Ship&, crew, event, person, source, valence)` writes
  one; `MemoryCount` and `Bond(a, b)` read them, and `Bond` is *how they feel about a person*. **`MEM_RESCUE` is
  already an event in the enum.**
- **Allegiance.** `holdings` — *"0 alone and blaming .. 1 held: bonds, allegiances"* — is one of morale's three
  components, so regard already reaches morale.
- **The cost.** Wounds; the three severities (*degraded / acute / catastrophic*); conditions with source, cure
  and visibility; and death. All present.
- **The log and the record**, and the work queue.

**NOT there, and it is the crux of this design: **drives are derived and stored, and nothing reads them.**
`DeriveCharacter` fills them; no consumer exists. **The lever this document leans on — that the drive decides
who rises — does not exist yet.** It is the same shape of gap as `EffectiveSkill` (computed, unwired), and it is
one step past the pass in flight, not a parallel system.

> **Built 2026-10-08 (`feat/rising`), and this paragraph is corrected rather than left to rot.** The lever now
> exists: `RisingDriveAtStake` reads the `WORK_DRIVE` factor `WorkFactors` already computes, and the whole act
> — offer, exceeding, cost, witness memory and refusal — is built as a rule over the character layer. The
> account is `docs/evidence/rising-to-the-occasion.md`; the console is `ship rise`, the transcript
> `test_ship_core --rising`, and the check `scripts/rising-check.sh`. No save version changed, because nothing
> new is stored.

**One thing the design gets for free, and it was not planned by anyone.** A mark's `salience` **decays unless
reinforced.** So a heroism that nobody talks about *fades*, and one the crew keep telling persists. **Heroism
survives only if it is retold** — which is the behaviour this document asks for, already in the model.

**And the mechanism is not new.** A crisis is a **condition**; a person's state sets the **odds**; what it costs
them is the **severity**. That is `docs/damage-and-budgets.md`'s own shape — *condition sets the odds, stress
sets the severity* — applied to a person instead of a system. **Nothing here needs an engine feature.**

**And the crew are simulated by rules, not by agents** — arbitration, post assignment, the restaffing rule — so
**a heroism and a refusal are authored as rules in the crew layer.** That is consistent with the law that *the
simulation decides*, and it makes this smaller than it sounds: a rule that reads the drive and the morale and
rolls beyond effective skill.

**New:** the *offer* — a situation in which exceeding is possible; the **exceeding** (odds beyond effective
skill); the **cost**; the **witness memory**; and the **refusal**, legible in the log.

**Not needed, and should be refused:** a `heroism` stat, a courage roll, a bar, or anything that makes
heroism a property of a person rather than an act they chose under pressure. **The moment it is a number, it
stops being a story.**

## Acceptance

- **A named crew member exceeds their effective skill** in a specific situation, **the reason they were the one
  is their drive**, and the log says so.
- **It costs them something**, and the cost is in the record.
- **The witnesses remember it**, and their regard changes — allegiance up, morale moved, the mark recorded with
  its provenance.
- **A refusal is possible, chosen from the drives, and legible** — the log says why they would not.
- **A failure is possible**, and a failed attempt is not a wasted one: it still cost, and it is still
  remembered.
- **No heroism stat, no courage roll, no bar.** The check is that the act is *situational* — the same person
  does it here and does not do it there.
- **The same scenario produces a different heroism with a different crew** — which is what the post-not-name
  rule (`docs/the-entry-point.md`) exists to allow.
