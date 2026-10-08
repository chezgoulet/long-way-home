# The character layer: the derivation, three crew to read, and morale from three components

`docs/character-attributes.md`, `docs/crew-roster.md` and `docs/morale.md`, applied on
`feat/the-character-layer` (cut from `testing`), 2026-10-08. **O8**, and the second half of the
walkthrough's **G14** (the manner).

**Observed, and the command that produced it.** Everything below is the output of a command named
beside it, on this tree, after
`cmake --build /home/c/big/git/upstream/efgame/build-linux --target efgame`, which built
`libefgame.so` with only the pre-existing `-Wwrite-strings` warnings, and after the unit tests
(`scripts/test.sh`). The one-place account of the derivation is `docs/character-derivation.md`.

## What was built, and where

One model, in `module/ship/ship_core.{h,cpp}`, beside the record it describes (`CrewMember`): the
four kinds of modifier, the species table, the derivation, the condition record with source, cure
and visibility, the three morale reads, and the manner. It is in `ship_core` rather than a new
translation unit for the reason recorded in `docs/evidence/meeting-brief.md` — the module build
finds sources with a configure-time glob, so a new `.cpp` is not compiled until cmake
re-configures, and the save format lives beside the state. No game header and no engine code was
touched; no new art, no assets, no audio.

The checks are `tests/ship/test_ship_core.cpp` (the new unit tests below) and
`scripts/character-check.sh` (the acceptance the owner judges, printed).

## Task A — the four kinds, and the derivation

The four kinds are records, kept apart: **skills** (seven ratings, `CrewMember.skills`), **traits**
(a bitmask over `Trait`), **conditions** (`CrewMember.conditions[CONDITION_MAX]`), and **state**
(the rest of the record: the three morale reads, fatigue, health, injuries, marks). Drives are in
three parts (desire, need, fear).

**The derivation is one function of the record and the seed** — `void DeriveCharacter(CrewMember&,
uint32_t&)` — called from `BuildRoster`, and again on load (Unpack calls `BuildRoster`). Skills
lead with the department's own; rank lifts all seven; the seed jitters; **species contributes
nothing**, because a species is not a stat bonus. Traits are two, distinct, drawn from the seed;
drives are three, drawn from the seed; the starting state is drawn from the seed. It is a function
of the record, not a table of hand-written people.

The full derivation, in one place, is `docs/character-derivation.md`.

## Task B — species as capabilities and needs, never bonuses

`SpeciesRecord` holds **words and condition ids only** — `capabilities[4]`, `needs[4]`,
`susceptibilities[4]`, `prone[4]`, and the requirement-shaped `restNeedHours` / `needsFood` /
`sleeps`. **There is no multiplier field, because there is no multiplier.** The two rules are
honoured: every capability carries a cost (Vulcan strength comes with meditation and managed
emotion; Betazoid empathy cannot be switched off; Borg strength comes with an alcove and a
reputation), and biology is not culture (Torres rejects Klingon tradition on purpose).

Species is read as a source of **conditions**: a species difference shows up as a mark on a person,
not a hidden number. The Vulcan meditation cycle is applied by the simulation as a condition
(`TestSpeciesAreCapabilitiesNotBonuses`: a tired Vulcan draws `meditation cycle due`, source *"the
meditation cycle, unkept"*). Talaxian and Bolian are marked `[ours]` in the table.

**The galley and the watch bill read species and adapt**, as `docs/character-attributes.md` asks:
a species that does not eat takes nothing from the galley and never goes hungry, and a species that
does not rest takes no fatigue at all (a hologram, whose failure set is different rather than
absent). `TestSpeciesAreCapabilitiesNotBonuses` measures both: a hologram's fatigue stays at zero
across a full watch, and a day at table costs rations where the same person made to eat costs more.
**What is not built is named in the judgement calls: the sickbay does not yet adapt to copper-based
blood, and the species-specific shape of the rest cycle is data the watch bill does not yet read.**

The one thing that could look like a bonus — `restNeedHours` — is a **need**, not a gift: the
Vulcan needs *less* rest and must meditate for it, the Borg needs the alcove, the hologram needs
none and has a wholly different failure set. And the human check: Vulcans and humans have the same
average skill total in the generated crew (`TestTheDerivation`), so no species reads as "the good
one".

## Task C — morale as three derived reads, not a counter

`Morale(c)` is a weighted mean of **deficit**, **outlook** and **holdings** (`docs/morale.md`),
with the weights `0.40 / 0.35 / 0.25` — **ours**, recorded so they can be overruled. Nothing writes
a scalar. Every driver in the tick now eases one of the three: the day's activity, hunger, the deck
without air, the alert, losses still felt, the Maquis split, crowding, quarters quality, trauma
(which lowers *holdings*) and the assimilation scar (which lowers *outlook*). `MoraleReason(c)`
names the component most responsible, in words, and the six-hourly mood line writes it:

```
the crew's mood: morale 59% (does not believe the course is worth the cost), fatigue 40%
```

**No hidden counter remains.** The field is gone — `float morale` is removed from `CrewMember`,
replaced by `deficit`, `outlook` and `holdings`:

```
$ grep -rn '\.morale\b\|->morale\b' module/
NO morale field references remain
```

The test that matters asserts the explanation directly (`TestMoraleIsReadFromThree`): with
`deficit 0.2 / outlook 0.6 / holdings 0.6` the reading is exactly the weighted mean; set one
component to zero and the reading moves and `MoraleReason` names it; and the log then carries the
reason for the person who carries the most of it.

## The acceptance: three generated crew members, in full

`scripts/character-check.sh` (`test_ship_core --crew`). One from each of three departments, after a
day and a half in the normal course and then eight hours at battle stations with an empty galley —
so the conditions are the ship's own, not posed on the people:

```
== three generated crew members, in full (O8; docs/character-derivation.md)
  [19] Crewman 020 -- Human, crewman, command department, post communications
      skills: engineering 1 medical 2 science 1 security 2 operations 1 command 5 flight 1   (effective engineering 0.3)
      traits: steady under fire; needs less sleep;
      drives: wants to go home; short of food; afraid of the Borg
      species: Human
        capability: none in particular: adaptability is a trait, not a species gift
        need: sleep
        need: food
        need: air
        need: company
        susceptibility: decompression
        susceptibility: radiation
        susceptibility: infection
        susceptibility: a long war
      conditions (2):
        afraid (debuff, slight) from "the ship is at battle stations"; clears with the end of the watch; visible to player crew (not logged)
        hungry (debuff, clear) from "the galley is empty"; clears with a meal; visible to player crew (not logged)
      morale 0.65 (worn) = deficit 0.26 (heart 0.74) / outlook 0.48 / holdings 0.72 -- does not believe the course is worth the cost
      says: "I'll do it. I'd just rather not be the one who meets them first."
  [34] Crewman 035 -- Betazoid, crewman, engineering department, post structural integrity
      skills: engineering 5 medical 2 science 2 security 2 operations 3 command 2 flight 1   (effective engineering 4.4)
      traits: needs less sleep; quick healer;
      drives: wants promotion; short of medical care; afraid of being useless
      species: Betazoid
        capability: empathy: reads feeling, not thought
        need: quiet: the faculty cannot be switched off
        susceptibility: others' pain arrives uninvited, which is a debuff as often as a buff
      conditions (2):
        afraid (debuff, slight) from "the ship is at battle stations"; clears with the end of the watch; visible to player crew (not logged)
        hungry (debuff, clear) from "the galley is empty"; clears with a meal; visible to player crew (not logged)
      morale 0.71 (fit) = deficit 0.16 (heart 0.84) / outlook 0.56 / holdings 0.72 -- carrying on
      says: "Ready when you are. Let's do it properly."
  [81] Crewman 082 -- Human, crewman, security department, post shields
      skills: engineering 2 medical 3 science 3 security 4 operations 3 command 2 flight 1   (effective engineering 1.3)
      traits: poor with authority; quick healer;
      drives: wants a particular person; short of sleep; afraid of decompression
      species: Human
        capability: none in particular: adaptability is a trait, not a species gift
        need: sleep
        need: food
        need: air
        need: company
        susceptibility: decompression
        susceptibility: radiation
        susceptibility: infection
        susceptibility: a long war
      conditions (2):
        afraid (debuff, slight) from "the ship is at battle stations"; clears with the end of the watch; visible to player crew (not logged)
        hungry (debuff, clear) from "the galley is empty"; clears with a meal; visible to player crew (not logged)
      morale 0.67 (worn) = deficit 0.21 (heart 0.79) / outlook 0.50 / holdings 0.72 -- does not believe the course is worth the cost
      says: "Another watch. It goes on. It always goes on."
```

Three people, three departments, three sciences of person: a command-crewman who wants to go home;
an engineer who wants promotion and is afraid of being useless; a security crewman who wants a
particular person and is afraid of decompression. Their conditions, their sources and their cures
are on the face of the print.

## The same seed produces the same person

`TestTheDerivation`: two `NewShip()` builds are identical field for field in the static layer
(species, traits, desire, need, fear, all seven skills), which is what *saves must replay
identically* requires; a different seed produces a different crew (more than 20 of 141 differ in
species, traits and desire). `TestCharacterSaveRoundTrip` packs, unpacks, and checks the three
morale reads and a condition's source and cure survive, and that the static half came back
identical from the seed.

## A condition arrives, is cured, and is recorded

`TestConditionLifecycle`: `Sicken(..., COND_CONCUSSED, "concussed in the coolant bay", ..., CLEAR_SICKBAY, ...)`
adds the condition and writes the log on arrival; the condition carries its source and its cure;
the log can see it; `Cure` clears it and writes the log again; a condition with `visible == 0` is
refused (a hidden penalty is not allowed); and a fourth condition evicts the oldest rather than
stacking (`CONDITION_MAX = 3`).

## The manner, at three morale levels

`scripts/character-check.sh` (`test_ship_core --manner`) — one person, the same person, at three
levels. **A demonstration, not a verdict:**

```
== the manner at three morale levels (G14, second half)
  one crew member: Crewman 020, Human; wants to go home; afraid of the Borg
  fit              morale 0.88 (fit): "We'll get there. One more day's work, one more mile."
  worn             morale 0.53 (worn): "I'll do it. I'd just rather not be the one who meets them first."
  at breaking point morale 0.15 (at breaking point): "I've done my part. Find someone else. I can't do that again."
```

## The save version, and what it costs

**Save format 53.** The static half (species, skills, traits, drives) is *derived from the seed* and
is **not stored**, per `docs/crew-roster.md`'s "static character data is content". The dynamic half
is stored: the three morale reads (three floats, replacing the removed single `float morale`), a
condition count, and each condition (id, valence, magnitude, onset, clear rule, visibility, and its
name-capped source). Measured on this tree:

```
$ /tmp/size
save, fresh ship: 9137 bytes
save, after 48h at red alert with an empty galley: 32849 bytes; 141 of 141 carry conditions
```

- **Baseline: +9 bytes per head** (+3 floats, +1 count, −1 float morale), i.e. **+1,269 bytes** for
  141 crew when nobody carries a condition.
- **Worst case measured: +168 bytes per head** (~24 KB) when all 141 carry the full set of three
  conditions, each with a named source.

That is comfortably inside the "roughly four hundred bytes per head, on the order of sixty
kilobytes" the design budgeted (`docs/character-attributes.md`). **Older saves do not load** — the
version bump invalidates them, which is the cost named here and in the header comment.

## The checks, and what they returned

```
scripts/test.sh
  -> crew_core: all checks passed
     ship_core: all checks passed, including TestTheDerivation, TestConditionLifecycle,
       TestMoraleIsReadFromThree, TestSpeciesAreCapabilitiesNotBonuses, TestManner,
       TestCharacterSaveRoundTrip
     tools OK; python syntax ok; shellcheck ok; 20 patches

scripts/check.sh --source-map build/gdk/maps/eliteforce_voyager_maps/voy1.map \
                 --script-corpus build/gdk/scripts
  -> entity dictionary 318 classes; validator negative tests ALL PASS;
     2,024 files: 2,016 compiled and read back, 8 rejected — exactly the known, documented set

scripts/character-check.sh
  -> PASS  three generated crew printed in full; morale broken into three components;
     every condition with source, cure and visibility; species as capabilities and needs;
     the manner across three levels
```

## What could not be verified

- **Whether a crew member reads as a *person* rather than as a plausible set of numbers.** That is
  the owner's, and printing three of them in full is how he judges it. This lane only proves the
  layer is derived, deterministic, legible and saved.
- **Whether the manner is good.** Three lines at three levels are a demonstration; the owner judges
  whether they read.
- **Everything a session would show** — conditions accumulating over a long run, a therapy session
  fading what a character carries, a species need interacting with the watch bill. The unit tests
  exercise the mechanisms; the felt run is the playtest.

## Judgement calls, named as calls

1. **The layer lives in `module/ship/ship_core.{h,cpp}`**, not a new file under `module/crew/`. The
   brief points at `module/crew/` for the generated crew, but the generation is `BuildRoster` in
   `ship_core.cpp`, the record is `CrewMember`, and the save format is beside it; a new `.cpp` would
   also fall to the configure-time-glob trap. So it went beside the record. **This is a call, and it
   can be moved.**
2. **The morale weights (0.40 / 0.35 / 0.25) are ours.** `docs/morale.md` names the three components
   and never weights them; these are a small, documented choice, recorded so they can be overruled.
3. **The condition sources and clear rules are ours.** The design names the sources (sleep loss,
   hunger, the air, fear, grief, a species need) and the clear rules (rest, sickbay, a meal, the end
   of the watch, salience); the exact pairings and the thresholds (fatigue > 0.6 to arrive, ≤ 0.15
   to clear) are an implementation choice with hysteresis, so a value resting on a threshold does
   not flicker the log.
4. **Automatic conditions are visible but not logged per head.** Visibility is per condition, and
   the tick's conditions carry `CVIS_PLAYER | CVIS_CREW` without `CVIS_LOG`: a red alert touches
   every one of 141 people, and logging each arrival would bury the log (it did, on the first run —
   `TestLog` caught it). A condition applied deliberately (scenario, console, `Sicken`) carries
   `CVIS_LOG` and is recorded on arrival and cure.
5. **`EffectiveSkill` is computed but not yet read by the failure roll.** The conditions layer is
   honest and visible and the derivation is written down, but wiring a named condition into
   `UseSystem`'s odds is left to the failure-content work; the acceptance that matters here
   ("a tired engineer is measurably slower") is already served by fatigue and the morale read. This
   is named so it is not mistaken for done.
   **Corrected in the follow-up lane:** the wiring is done in `docs/character-attributes.md`, "The
   wiring: how each kind reaches the work" (`feat/the-work-bites`, evidence
   `docs/evidence/the-work-bites.md`). This call stands as what was true of *this* pass.
6. **The species table's Talaxian and Bolian entries are `[ours]`**, because canon gives thin or no
   biology and `docs/character-attributes.md` says to be honest about it. The generated crew can
   draw both; holograms are never generated (that is the Doctor, and a ship does not crew many of
   them).
7. **Who is aboard did not change.** The named crew keep their names, ranks, posts and watches;
   only their species, skills, traits and drives are now derived onto them. No post was added or
   removed; the configurator, the vacancies and the roster's occupancy are untouched, as the brief
   directed.
