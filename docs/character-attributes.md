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
- The Doctor has conditions and no fatigue; a clone diverges; a merge carries two memory sets with their
  provenance intact.
- And no attribute block is introduced -- if one is ever proposed, the proposal has to argue against the
  refusal above.
