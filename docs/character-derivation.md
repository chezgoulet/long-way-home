# The character layer: the four kinds, the species, and the derivation

**O8.** The design is `docs/character-attributes.md` (the four kinds of modifier, the derivation,
and species as capabilities rather than bonuses), `docs/crew-roster.md` (the record, the traits and
the drives) and `docs/morale.md` (morale as three derived reads). **This document is the one place
the derivation is written down**, as O8's closing requirement asks: *"a condition record with
source, cure and visibility, and the derivation written down."*

**Why it is a derivation and not a roster.** The owner's ruling (`docs/the-entry-point.md`, Part
three) is that **the player may field a crew that is entirely fictitious** — none of the show
characters. A hand-authored roster is therefore not merely wasteful, it is impossible: the game has
to invent people nobody wrote. The crew is already generated, *"the same crew for the same seed"*,
and **this layer is a pure function on top of that generator** — a function of the record and the
seed, never a table of hand-written individuals.

The code is `module/ship/ship_core.{h,cpp}`, beside the record it describes (`CrewMember`). It
lives there rather than in a new translation unit for the reason recorded in
`docs/evidence/meeting-brief.md`: the module build finds sources with a configure-time glob, so a
new `.cpp` is not compiled until cmake re-configures, and the save format lives beside the state.

## 1. The four kinds, kept apart

| kind | what it is | stored? | where |
|---|---|---|---|
| **Skills** | seven ratings, 0–5: what a person *can* do | static (seed) | `CrewMember.skills[SKILL_COUNT]` |
| **Traits** | few, permanent, behavioural: how they *tend* to do it | static (seed) | `CrewMember.traits` (a bitmask over `Trait`) |
| **Conditions** | temporary, sourced, legible: buffs and debuffs | **save** | `CrewMember.conditions[CONDITION_MAX]` |
| **State** | the transient: deficit, outlook, holdings, fatigue, health, injuries, assimilation, marks | **save** | the rest of `CrewMember` |

The split matters because of the affordability argument in `docs/crew-roster.md`: **static
character data is content; dynamic state is save.** Skills, traits, species and drives are derived
from the seed and *are not in the save*; conditions and the three morale reads are the save. That
is what keeps a hundred and forty-one heads out of the save blob and keeps characters editable.

**And the refusal is honoured: there is no attribute block.** No strength, agility, constitution,
perception or charisma. If endurance is needed it is a *trait* plus a fatigue curve, not a stat.
`CrewMember` carries none.

### Skills

The department's own skill leads, rank lifts all seven, and the seed jitters the rest. The lead
skill is the department's: engineering → engineering, medical → medical, sciences → science,
security → security, command → command.

```
base = 1 + (rank >= 4 ? 2 : rank >= 2 ? 1 : 0)
if skill == the department's lead skill: base += 2
base += (seed draw) % 3
skill = min(SKILL_MAX, base)          # SKILL_MAX = 5
```

Species contributes **nothing** here. That is deliberate: `docs/character-attributes.md` is
emphatic that a species is not a stat bonus, so a Vulcan is not `+N` to science.

### Traits

Two, distinct, drawn from the seed: `steady under fire`, `needs less sleep`, `good with people`,
`claustrophobic`, `poor with authority`, `first-contact trained`, `adaptable`, `quick healer`,
`scrounger`. Traits are behavioural and legible, never a percentage.

### Drives

Three, drawn from the seed: **desire** (promotion, home, a particular person, to prove something,
to be left alone), **need** (sleep, food, company, purpose, medical care), **fear** (dying alone,
decompression, the Borg, being useless, being seen as a coward).

### State

A fresh crew begins rested and reasonably hopeful. The seed colours the outlook and the holdings so
that two people do not start identical; fatigue and deficit begin at zero. Conditions arrive in
play.

### Appearance

**The face is part of the person, so it comes from the same derivation.** `DeriveAppearance(c, rng)`
sits beside `DeriveCharacter` and draws from **the same stream**, so the traits and the face are one
person, a save replays identically face and all, and a face can never drift from its record
(`docs/character-attributes.md`, *Appearance*). It fills an `Appearance { head, build, colour }`:
the **head** is an index into the shipped pool (the shipped player models, enumerated in
`docs/evidence/the-appearance.md`); the **build** is the record's own (`crewthin`/`crewfemale`, as
the game's type carries it); the **colour** is the department's. **It is not stored** — static
content derived from the seed, exactly as species, skills, traits and drives are, so a hundred and
forty-one faces cost the save nothing.

Three rules bind it, and each is enforced in code rather than in prose:

- **Species constrains the pool.** `SpeciesHeadCount`/`SpeciesHeadAt` return a species's own part of
  the pool; the parts are disjoint by construction, so a Vulcan and a human do not share a face.
  Where the game ships no non-canon face for a species (Vulcan, Ocampa, Talaxian), the pool is a
  named placeholder carve, and the shortfall is in the evidence rather than hidden.
- **The generated pool excludes the canon faces in code.** `IsCanonFace` names the show command crew
  in one place; the derivation refuses a canon face at selection, and `CanonFaceInPool` walks the
  whole pool — a canon head planted in it is caught by name. A comment is not a mechanism.
- **Named crew are never re-rolled and never generated.** `DeriveAppearance` leaves a named record
  unset (`head 0xFFFF`); the one named list (`NamedHeadModel`) keeps the model its type ships.

The whole layer is a pure function of the record and the seed, as the rest of this document is. The
evidence, tests and the photographed lineup are `docs/evidence/the-appearance.md`.

## 2. Conditions — the missing layer

A condition is `id · source · valence · magnitude · onset · clears-when · visible`
(`docs/character-attributes.md`). In the code:

```c++
struct Condition {
    uint8_t id;            // ConditionId: exhausted, hungry, hypoxic, grieving, ... or a buff
    std::string source;    // the named cause: "concussed in the coolant bay"
    int8_t  valence;       // CVAL_DEBUFF | CVAL_BUFF
    uint8_t magnitude;     // CMAG_SLIGHT | CMAG_CLEAR | CMAG_SHARP: slower, unreliable, sharper
    float   onset;         // ship seconds when it arrived
    uint8_t clears;        // rest | sickbay | a meal | the end of the watch | the passage of salience
    uint8_t visible;       // bits: CVIS_PLAYER | CVIS_CREW | CVIS_LOG
};
```

Three rules bind it, and each is enforced in code rather than in prose:

- **Every condition names its cause and its cure, or it does not exist.** `AddCondition` refuses an
  empty source; the clear rule is a field, not an assumption.
- **A modifier nobody can see is a hidden penalty, and hidden penalties feel like bugs.**
  `AddCondition` refuses `visible == 0`. Visibility is per condition: some are written in the log,
  some are visible to the player and the crew only. The automatic conditions the tick applies are
  *not* logged per head, because a red alert would otherwise bury the log under one line per
  person; conditions applied deliberately (a scenario, a console, `Sicken`) carry `CVIS_LOG` and are
  recorded on arrival and on cure.
- **Two or three at a time, never a soup.** `CONDITION_MAX = 3`; a fourth evicts the oldest debuff
  (or the oldest condition if all are buffs).

The sources the ship itself applies, and what clears them:

| condition | source (as the simulation names it) | clears with |
|---|---|---|
| `exhausted` | "hasn't slept enough" | rest (fatigue ≤ 0.15 or asleep) |
| `hungry` | "the galley is empty" | a meal (rations or replicators) |
| `hypoxic` | "the deck has no air" | sickbay (the air returns) |
| `afraid` | "the ship is at battle stations" | the end of the watch (alert drops) |
| `grieving` | "lost someone and still carries it" | the passage of salience (trauma < 0.4) |
| `meditation cycle due` | "the meditation cycle, unkept" | rest (a species need — see below) |

The buffs the design lists (a hot meal, coffee, shore leave, a promotion, a service held properly,
being told the truth by someone in authority) are in the vocabulary and applied by scenario and
console through the same `Sicken`/`Cure` seam.

## 3. Species: capabilities, needs and susceptibilities — never bonuses

A species record is static content and holds **words and condition ids only**:

```c++
struct SpeciesRecord {
    const char *name;
    const char *capabilities[4];      // what they can do that another cannot (no number attached)
    const char *needs[4];             // what they require differently: a diet, a cycle, a charge
    const char *susceptibilities[4];  // what can go wrong for them specifically
    uint8_t      prone[4];            // condition ids the simulation may derive for them
    float        restNeedHours;       // the shape of rest, a requirement, not a bonus
    bool         needsFood;
    bool         sleeps;              // false: the rest is a cycle, not sleep
};
```

There is no multiplier field, because there is no multiplier. Species does exactly the three real
things `docs/character-attributes.md` names:

1. **Capability** — something they can do that another cannot, no percentage attached. Vulcan
   touch-telepathy, the nerve pinch; Betazoid empathy; Borg-recovered strength and resistance;
   Ocampan limited telepathy and a botanical gift.
2. **Need** — a difference in what they require. Vulcan meditation in place of sleep; the Borg
   alcove cycle; a Klingon high-protein diet; an Ocampan lifespan measured in single-digit years;
   a Hologram's emitter charge and program integrity. `restNeedHours`, `needsFood` and `sleeps` are
   the machine-readable half of this: the **shape** of rest for the watch bill and the galley, not a
   better one.
3. **Susceptibility** — what can go wrong for them specifically. Pon farr, Bendii syndrome and
   copper-based blood for a Vulcan; others' pain arriving uninvited for a Betazoid; the Collective
   and the crew's suspicion for the Borg-recovered; a wholly different failure set for a Hologram.

**Every capability carries a cost**, so nothing is an upgrade and nothing is a choice anyone makes
for power: Vulcan strength comes with meditation and managed emotion; Betazoid empathy cannot be
switched off; Borg strength comes with an alcove and a reputation. **Biology is not culture**:
what a species *can* do is biology, what it is *expected* to do is culture, learned and rejectable
(Torres rejects Klingon tradition; that is the point).

Species is read as a source of **conditions**, so a difference shows up as a mark on a person — a
Vulcan's *meditation cycle due*, a Hologram's *emitter charge low* — rather than as a hidden
multiplier. The record is in the `SPECIES[]` table in `module/ship/ship_core.cpp`. Talaxian and
Bolian are marked `[ours]`, because canon gives thin or no biology and the document says to be
honest about it. A generated member draws a species from the seed; the named crew carry theirs.

**The galley and the watch bill read the record and adapt**, which is the logistics half of
"capabilities, needs and susceptibilities": a species that does not eat takes nothing from the
galley and never goes hungry (`needsFood`), and a species that does not rest takes no fatigue at
all (`restNeedHours == 0`, the hologram). Heterogeneity is a logistics cost, not a stat.

## 4. Morale: three derived reads, never a counter

`docs/morale.md`: morale is per person, derived from three components, and **never stored as a
scalar**.

- **Deficit** — what they are short of: sleep, food, medical care, comfort, company. Objective.
- **Outlook** — what they believe: are we getting home, is command competent, was the last loss
  worth it. Subjective, and it moves when the facts do.
- **Holdings** — who they hold with: bonds, grudges, allegiances.

The reading is a weighted mean, **the weights ours** and recorded so they can be overruled:

```
heart    = 1 - deficit
morale   = 0.40 * heart + 0.35 * outlook + 0.25 * holdings      # clamped to 0..1
```

`Morale(c)` is a function; it is never a field. Every driver in the tick eases one of the three
components — the day's activity, hunger, the air on their deck, the alert, losses, the Maquis
split, crowding, quarters quality, what they carry (trauma, which lowers *holdings*) and the
assimilation scar (which lowers *outlook*). `MoraleReason(c)` names the component most responsible,
in words, so the log can say **which one moved and why** — *short on sleep*, *does not believe the
course is worth the cost*, *alone, or at odds with the crew*. The personnel screen's bands are the
document's own: **fit, worn, strained, at breaking point**.

The acceptance is that a person's morale can be *explained* from the three: change one component and
the reading moves, and the reason names it. That test exists
(`TestMoraleIsReadFromThree`), and the log line carries it.

## 5. Effective skill and the other derived quantities

`docs/character-attributes.md` names four derived quantities. They are:

- **effective skill** = base skill + aptitude traits, reduced by conditions, deficits and haste.
  This is `EffectiveSkill(c, skill)`: an aptitude trait (`first-contact trained`) adds; behavioural
  traits do not; conditions, the deficit and fatigue subtract, down to zero. This is what decides
  whether a repair is quick, slow, or dangerous.
- **reliability** = f(outlook, fatigue, fear, conditions) — gates compliance.
- **capacity** = f(deficit, health, conditions) — whether they can stand a watch at all.
- **holding** = bonds and allegiances — who they will confide in, and whose orders they will quietly
  not carry.

`Morale` is the shipped read of the three components today; `Effectiveness`, compliance and capacity
read the same state. **The failure roll's odds are now wired**: `UseSystem` (via `WorkFactors`,
`WorkOdds`, `RollWork`) reads the operator's four kinds and morale, each by its own named route, so a
worse outcome says what did it. That wiring and the derivation of each kind's reach are written in
`docs/character-attributes.md`, "The wiring: how each kind reaches the work", with evidence in
`docs/evidence/the-work-bites.md`.

## 6. The special cases

- **The Doctor** is a Hologram: no fatigue, hunger or injury, and a different failure set
  (emitter charge, program integrity). The record handles him with the same structure and the
  species condition vocabulary.
- **Seven** is Borg-recovered: strength, resistance, and a regeneration requirement in place of
  sleep.
- **Civilians** have no rank; reliability derives from a different source (loyalty, affection, the
  fact that they can walk away).
- **A clone** starts with a copy of the record and diverges from that moment.
- **A merge** inherits two memory sets with contradictory provenance; provenance is preserved per
  mark rather than reconciled.

(These last are the record's own asymmetries; the clone and merge are not built here and belong to
the memory layer, `docs/memory-and-consequence.md`.)

## 7. The manner

G14's second half: barks wired to morale, so the crew *sound* like what they feel. `MannerLine(c)`
is a deterministic function of the band and of the person's desire and fear — one line each for the
fit, the worn and the broken. It is a **demonstration, not a verdict**: `scripts/character-check.sh`
prints one crew member at three morale levels and lets the owner judge whether the manner reads.
