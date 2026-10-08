# Character attributes: does the record carry everything?

An audit of the crew record against every system that reads it, prompted by the owner's question. The short
answer: **the record is mostly sufficient, two things are missing, and one thing should be refused.**

## What each system needs from a person

| system | reads from the record |
|---|---|
| crew work (`docs/crew-work.md`) | skills -- who can do which job, how fast, how safely |
| morale (`docs/morale.md`) | deficit, outlook, holdings |
| access and authority (`docs/access-and-authority.md`) | rank, post, credentials, delegations |
| watch and bills (`docs/crew-manifest.md`) | department, watch, post, bill assignments, qualifications |
| failure (`docs/failure-is-content.md`) | injuries, permanent marks, assimilation, death |
| memory (`docs/memory-and-consequence.md`) | marks with provenance, valence and salience |
| damage and budgets (`docs/damage-and-budgets.md`) | who is fit, who is spent, who is at a post |
| start states (`docs/start-states.md`) | vacancies, seniority, promotion order |
| exploration (`docs/exploration-and-science.md`) | kit qualification, away-team fitness |

Everything above is already in the record except two things.

## The four kinds of modifier, kept apart

The gap the question exposes is that we specced **skills** (ratings), **traits** (dispositions) and **state**
(needs, morale, health) -- and never said what a *temporary* modifier is. So:

1. **Skills** -- seven, rated 0-5. What they *can* do.
2. **Traits** -- few, permanent, behavioural: steady under fire, needs less sleep, good with people,
   claustrophobic, poor with authority. How they *tend* to do it.
3. **Conditions** -- **the missing layer**: temporary, sourced, legible. *Buffs and debuffs live here.*
4. **State** -- deficit, morale, fatigue, health, injuries, duty, assimilation progress, memory marks.

A condition is:

```
id · source · valence · magnitude · onset · clears-when · visible
```

- **source** -- what caused it, named: *concussed in the coolant bay*, *hasn't slept in 31 hours*, *Neelix's
  coffee*, *promoted this morning*, *attended the service*.
- **clears-when** -- rest, sickbay, a meal, the end of the watch, or simply the passage of salience.
- **magnitude** -- small and legible. Not "+7%"; *slower*, *unreliable*, *sharper*.
- **visible** -- because a modifier nobody can see is a hidden penalty, and hidden penalties feel like bugs.

### Buffs and debuffs, from sources the ship already has

**Debuffs, by source:** sleep loss; hunger and rations; injury and blood loss; decompression and hypoxia;
radiation (canon: chroniton exposure); infection (canon: the gel-pack plague); phaser burns; assimilation
progress; grief; fear; overwork; and *being the person who has been awake longest*.

**Buffs, by source:** a hot meal; coffee (canon gives us Neelix's); a night on the holodeck; stimulants from
sickbay, with a cost afterwards; shore leave; a promotion; a service held properly; a success; and -- the
cheapest in the whole design -- **being told the truth by someone in authority.**

**The rule that keeps it sane: two or three conditions at a time, never a soup.** A person is *exhausted* and
*grieving*, not carrying fourteen stacked percentages. Every condition must name its cause and its cure, or it
does not exist.

## The derivation, which was also missing

Nothing yet said how the parts combine. So, explicitly:

- **effective skill** = base skill + aptitude traits, then reduced by conditions, deficits and haste. This is
  what decides whether a repair is quick, slow, or *dangerous* -- the failure risk that
  `docs/failure-is-content.md` needs.
- **reliability** = f(outlook, fatigue, fear, conditions). This gates compliance: carried out, delayed, done
  badly, or refused.
- **capacity** = f(deficit, health, conditions). Whether they can stand a watch at all.
- **holding** = bonds and allegiances. Who they will confide in, and whose orders they will quietly not carry.

And **the refusal**: no attribute block. No strength, agility, constitution, perception, charisma. Skills,
traits, conditions and drives are enough, and an attribute layer on top is how a design like this dies -- it
multiplies the authoring surface and adds nothing the conditions layer does not already do. If endurance is
needed, it is a *trait* plus a fatigue curve, not a stat.

## The wiring: how each kind reaches the work

A layer that only describes a person is decoration. So **the four kinds and morale reach the failure
roll's odds** -- the roll that decides whether a use is quick, slow, or dangerous
(`docs/failure-is-content.md`) -- each by its own route, and each named. This section owns that
wiring: the four kinds, and how each reaches the work.

| kind | how it reaches the work | named in the record as |
|---|---|---|
| **skill** | what they can do: the department's skill leads, and the one aptitude trait adds; a competent hand lowers the odds, an unskilled one raises them | `skill engineering skill 5.0 -0.30` |
| **condition** | temporary and situational: a debuff raises the odds while it holds, a buff lowers them, by its magnitude word | `condition afraid (slight) +0.15` |
| **trait** | durable and behavioural, by **interacting with the moment** rather than as a flat shift: steady under fire helps under load, claustrophobia bites on a tight or hull task, adaptable softens the state drag | `trait steady under fire (under load) -0.30` |
| **drive** | a motive, **conditionally on the work**: a fear is realised by the task in front of them (decompression on a hull seal), a desire leans in or holds back under load | `drive afraid of decompression (sealing the hull) +0.25` |
| **morale** | the read from deficit, outlook and holdings; the component most responsible is named | `morale (worn): does not believe the course is worth the cost +0.08` |
| **capability** | a species capability, **two-sided**: a Betazoid's empathy reads a patient's feeling for good and takes on others' pain for bad; the same faculty is the cost, so it is never a bonus | `capability empathy: reads the feeling, not the thought -0.15` |

The system's condition still sets the base odds and the load still sets the severity (the owner's
ruling, `docs/failure-is-content.md`); the operator's factors **move the odds multiplicatively**, and
the top tenth of capability stays nominal whatever the operator, so a well-kept system still does not
mangle anyone. A factor that does not move the odds is not listed, so **the list is exactly what
touched the outcome** -- and the log carries it, so a worse outcome names what did it instead of
reading as a random failure. The mechanism is `WorkFactors` / `WorkOdds` / `RollWork` /
`WorkFactorLine`, and `WorkReading` is the console's read of one person at one system. The weights are
ours, small, and marked `[inv]`, recorded so they can be overruled.

The kinds are kept distinct on purpose, and the distinctions are testable:

- **A trait is not a condition.** `steady under fire` does nothing on a quiet watch and helps at
  battle stations; `afraid` costs the same whether the load is high or low.
- **A drive is not a flat penalty.** `afraid of decompression` costs on a hull task and costs nothing
  on a routine one; `wants promotion` *helps* under load; `quick healer` never touches the odds at all
  -- it shapes recovery.
- **A capability is not a bonus.** The Betazoid's empathy is the same faculty in both directions:
  -0.15 reading a patient, +0.15 on a hull full of the hurt.

**No hidden modifiers.** Every factor that moved an outcome is nameable in the log and at the
console, because a modifier the player cannot see is worse than no modifier at all: it turns command
into guessing.

`scripts/character-check.sh` prints the transcript -- the same task and the same person in two
conditions, with the reasons -- and the full evidence is `docs/evidence/the-work-bites.md`.

## Species: capabilities, needs and susceptibilities -- never bonuses

The obvious move is a species modifier table: Vulcans +strength, Betazoids +empathy, Klingons +toughness. **Do not
do that.** It is the point at which the game stops being Star Trek and becomes an MMO, and it contradicts the
thing the setting actually believes: individuals vary far more than species do, and diversity is the point
rather than a stat block.

Species does three real things, and all three are *situations* rather than numbers:

1. **Capability** -- something a person *can do* that another cannot. No percentage attached.
2. **Need** -- a difference in what they *require*: a diet, a cycle, a charge, a lifetime.
3. **Susceptibility** -- what can go wrong for them specifically, including things that cannot go wrong for
   anyone else.

### What that looks like for the species we can actually field

- **Vulcan** (Tuvok, Vorik). Capability: touch-telepathy, the nerve pinch, strength and endurance above the human
  norm. *Need*: meditation in place of sleep, which is a different shape of rest for the watch bill rather than
  a bonus to it. Susceptibility: pon farr, Bendii syndrome, and copper-based blood, which is a sickbay problem
  with a supply implication. Cost: emotions are managed rather than absent, and managing them takes practice.
- **Betazoid** (Jurot). Capability: empathy -- reads feeling, not thought. **The cost is the same faculty**: it
  cannot be switched off, so others' pain arrives uninvited, which is a debuff as often as a buff.
- **Klingon** (crew, and Jurot's circle). Capability: redundant physiology, so injuries that would kill a human
  are survivable. Culture -- honour, ritual -- is **learned, not biological**, and individuals vary: Torres
  rejects Klingon tradition entirely, and that is the point.
- **Ocampa** (Kes). Capability: limited telepathy and a botanical gift. Need: a lifespan measured in single-digit
  years. That is not a modifier, it is a *tragedy with a clock*, and it is the best attrition hook available.
- **Talaxian** (Neelix). Canon gives thin biology; his value is temperament, cooking and scrounging -- traits and
  skills, not species. **Marked as ours.**
- **Bolian** (Chell). Canon gives us a distinct biology and little else. Ours to fill, and honest about it.
- **Borg-recovered** (Seven, and anyone taken and reclaimed). Capability: strength, resistance, regeneration in
  place of sleep. Need: the alcove cycle. Susceptibility: the Collective, and the crew's suspicion, which is
  social and every bit as mechanical.
- **Hologram** (the Doctor). No fatigue, hunger or injury at all -- and a different failure set: emitter charge,
  program integrity, and an authority that is situational by definition.
- **Human**. No special capability. Which is the moment to avoid the trap of treating humans as the baseline
  other species deviate from: better to give humans *adaptability* as a trait and read everyone else as
  equally normal.

### The two rules

- **Every species capability carries a cost.** Vulcan strength comes with meditation and managed emotion; Betazoid
  empathy cannot be shut off; Borg strength comes with an alcove and a reputation; Ocampan gifts come with a
  lifespan. Nothing is an upgrade, so nothing is a choice anyone makes for power -- which is also why a crew of
  mixed species is interesting rather than optimal.
- **Biology is not culture.** What a species *can* do is biology. What a species is *expected* to do is culture,
  it is learned, and characters may reject it. Keeping those apart is what stops the design turning sentient
  beings into stereotypes with mechanics.

### Why species belongs in the data, not the code

A species record is static content: capabilities, needs, susceptibilities, and the conditions it is prone to.
The simulation then reads species the same way it reads anything else -- as a source of **conditions** -- which
means a species difference shows up as *a mark on a person* ("hypoxia-resistant", "meditation cycle due",
"cannot be treated with the standard agent") rather than as a hidden multiplier. Heterogeneous biology then
becomes what it is in canon: a logistics problem for the galley, the sickbay and the watch bill, and a story
generator -- a treatment that works for a human and not for a Bolian, a telepath who hears what nobody said, a
decompression that one person walks away from.

## Acceptance

- No species carries a percentage bonus, and no proposal for one should survive review.
- Capabilities, needs and susceptibilities live in species data, produce conditions, and every condition names its
  source and its clearing rule.
- The galley, the sickbay and the watch bill all read species and adapt -- heterogeneity is a logistics cost, not
  a stat.
- Culture is modelled separately from biology, and a character may reject their species' expectations.
- And the human check: no species reads as "the good one".

## The special cases the record must handle

- **The Doctor** is a hologram: no fatigue, no hunger, no injuries -- and a different failure set entirely.
  His conditions are *emitters damaged*, *program degraded*, *mobile emitter charge*. Model him with the same
  structure and a different condition vocabulary.
- **Seven** carries Borg physiology: a condition set that resists some debuffs and a *regeneration*
  requirement in place of sleep.
- **Civilians** (the morale officer, the botanical aide) have no rank, so reliability derives from a different
  source: loyalty, affection, or the simple fact that they can walk away.
- **A clone** starts with a copy of the record and diverges from that moment -- the same memories, then not.
- **A merge** is the hard one: one record inheriting *two* memory sets, with **contradictory provenance** --
  both "saw it myself" and "was told by the other one". The design's answer is that provenance is preserved
  per mark rather than reconciled, because the resulting person genuinely has two sets of first-hand memories,
  and that is a story, not a bug.

## What the record costs now

Roughly four hundred bytes per head, of which about two hundred are memory marks (eight per person). Across
141 crew that is on the order of **sixty kilobytes** -- comfortably fine, and roughly sixteen times the G3
direction-layer budget. Which is exactly why the split in `docs/crew-roster.md` matters: **identity, skills,
traits and drives are static content; conditions, marks and state are the save.**

## Acceptance

- Every condition names its source and its cure, is visible on the personnel screen, and expires or clears.
- No character carries more than a handful at once, and the derivation is written down rather than implied.
- A tired engineer is measurably slower and measurably more likely to fail; a rested one is not.
- **The four kinds and morale reach the failure roll's odds, each by its own route and each named**; a
  condition moves the odds while it holds, a trait shapes performance without being a flat penalty, a drive
  biases behaviour conditionally on the work, and a person who does not believe the course is worth the
  cost works like one who does not. **Every factor that touched an outcome is recoverable from the
  outcome** -- the log and the console name it (the wiring above; evidence
  `docs/evidence/the-work-bites.md`).
- The Doctor has conditions and no fatigue; a clone diverges; a merge carries two memory sets with their
  provenance intact.
- And no attribute block is introduced -- if one is ever proposed, the proposal has to argue against the
  refusal above.

---

## Appearance: the face is part of the person, so it comes from the same seed

**The owner's question, 2026-10-08:** *how do we approach the visuals for each character in the crew? Does
changing the seed remix the character attributes and assets based on how they were rolled?*

**Checked against the code. The answer splits in two, and the halves differ.**

### Attributes: already true, and stronger than the question assumes

`DeriveCharacter(CrewMember&, uint32_t &rng)` is *"a function of the record, not a table of hand-written
people"* — **pure and deterministic**, called from `BuildRoster` **and again on load**, so **the static half is
never in the save.** The field comment says it plainly: *"the same seed produces the same person."*

**So changing the seed remixes the attributes, and the seed is not a flavour on top of the character — it is
the character.** And `DeriveSpecies` already draws a species from the seed for a *generated* member while
**named crew carry theirs.** So the two-tier split is not a proposal here; it is built, for species.

### Assets: not yet, and that is the gap

**A crew record carries `std::string type` — "the game's NPC type used to embody them (an existing character)."**
That is an **assigned** string, not a derived one. And there is exactly one record-to-model path in the engine
host: `ApplyPlayerBody`, *"the player's model set from the crew record this department"* — **one person, for the
player.**

**So today the seed does not touch a face.** The body is a field somebody set.

### What it should be, in one line

**The face is part of the person, so it should come from the same derivation.**

Add `DeriveAppearance(record, seed)` beside `DeriveCharacter`, filling **which parts were chosen** — and **not
stored**, exactly as the character layer is not stored, recomputed on load. Then the seed remixes the whole
person together: traits and face from one stream, and **a save replays identically, face and all.** Two
derivations over one seed is the whole principle; anything else would let a person's face drift from their
record.

### How the parts are chosen — and this is already the doctrine

`docs/asset-doctrine.md` rules on it, and the ruling is decisive:

- ***"Characters are parts."*** Mixing and matching the shipped character assets *"produces faces and bodies we
  do not have"*, and the crew are drawn from the game's own bots *"because those bots have models, skins and a
  place in the fiction."*
- ***"Whole-cloth generation is in scope for our own content."*** A mesh-and-skin generator is *"a legitimate way
  to make the anonymous crew particular"* — **and it is a later step, not this one.**
- **And the boundary, which is sharp:** *"an original face for an original ensign is ours; **a generated likeness
  of a lore character is not**, and lore characters use the shipped assets they already have."*

**So: composition from named, shipped parts — a cast list per slot (head, hair, tone, build, uniform,
department colour), and the seed selects.** Hundreds of distinct people out of the existing assets, no new art,
every one of them reproducible from a seed. **It is the same trick the decks used: re-dress from named
sources, and say which.**

### Three rules the derivation must obey

1. **Species constrains the pool.** A Vulcan draws from Vulcan parts; a Betazoid from Betazoid ones. **This is
   the visual corollary of *"species are capabilities and needs, never bonuses"*** — the species is *visible*
   and it is *not a stat*. Nonsense results otherwise, and nonsense is what tells the player the roster is a
   slot machine.
2. **The generated pool excludes the canon faces — in code, not in a note.** The boundary above is the voice
   rule in another medium: **we may not generate a likeness of a named character, so the derivation must be
   unable to produce one.** A comment saying it does not is not a mechanism.
3. **Named crew are never re-rolled and never generated.** They use their shipped model, as they do now. **The
   derivation applies to the crew the game invented**, which is what the owner's start-state ruling made
   possible in the first place.

### And the judgement, which is the owner's

**The failure mode of slot-rolling is a crew that reads as a selection of parts rather than as people** — the
same failure as *"a plausible table of numbers"*, one level up. So the acceptance is **his eye on a group, not
on one body**: four or five generated crew, on a deck, at their posts.

**And this is not decoration.** *"Does the crew read as inhabited"* is the gate that has been open since the
crew was first measured — and faces are a large part of that answer.

### And what it costs, which must be measured rather than assumed

Skins and models are a finite allowance, and the performance survey measured the *ship*, not a derived crew.
**A full crew of derived bodies must be counted against the ceiling before it is promised** — the survey's own
lesson is that an allowance nobody measured is an allowance already spent.

