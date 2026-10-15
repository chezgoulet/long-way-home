# Evidence: the evaluator — the thing that recruits a scenario (G4)

**The finding this closes.** `docs/walkthrough.md`, **G4**: *"There is no scenario content at all, and
nothing recruits one… To close: the scenario manifest and a breeding-condition evaluator, plus at
least one scenario end to end."* The manifest landed on `feat/the-manifest` (#149); this is the second
half — **nothing recruits one.** Built on `feat/the-evaluator`, cut from `testing`, **2026-10-13**, on a
static host (no display; the module is built, but the engine is not run). One scenario — the worked
example — recruited and instantiated end to end; **not** the nine.

**Observed, and the command that produced it.** Every figure and transcript below is the output of the
command named beside it, on this tree. The whole pass is one check, `scripts/evaluator-check.sh`,
wired into `scripts/test.sh` after `scripts/scenario-check.sh`, plus the unit suite's
`TestScenarioEvaluator`.

## 1. What the evaluator is

`module/ship/scenario_core.cpp` (the loader the manifest pass built) gained the **evaluator**: a read
of the ship's actual state that decides, per breeding condition, whether it holds, and per scenario,
whether it fires. Its shape is `scenario::Evaluation`:

- **`fires`** — every declared condition is met (a tautology is met; see §5), none is unreadable and
  none is a contradiction;
- **`fired_by`** — the condition the premise leans on hardest, **named**, not inferred;
- **`nearest_miss`** — when it does not fire, the unmet condition closest to being met, **named with
  the ship's own number**;
- **`unreadable`** — the states named that no reader carries;
- **`degenerate_conditions`** — the tautologies and contradictions, named;
- **`reason`** — the whole decision in one line an author can read.

The seam is named rather than blurred: **the evaluator consumes the breeding conditions and the
decision; the simulation applies the premise** (`Recruit` → `Apply`: the systems it stresses, the
staff it needs, the warmth beat, every position and line resolved from its post to a person) **and the
cost and residue** (`Resolve` → `ApplyEffect`: the 13 effect kinds mapped onto the primitives the
design names). `scenario_core.h` says which side owns which.

**The states it reads.** All 18 the contract names (`docs/scenario-manifest.md`, *The readable
state*). Fifteen are single numbers the ship carries (`deuterium` … `intruders`); three are **scoped**
— `system_condition`, `deck_life_support`, `minutes_of_air` — and the `{state, op, value}` tuple
carries no subject, so they are read as the ship's **worst** (see §6).

## 2. The decision, both ways — quoted, not asserted

```sh
python3 tools/scenario/scenario_build.py scenarios/deck11-gel-pack
test_ship_core --recruit scenarios/deck11-gel-pack/build/maps/deck11-gel-pack.scenario
```

```
  scenario "deck11-gel-pack": the evaluator decides against the ship's actual state
    fresh ship: does not fire: nearest miss system_condition < 0.7 -- now 1 (short by 0.3)
      deuterium          < 0.35  now 1         unmet
      patients           >= 1    now 0         unmet
      system_condition   < 0.7  now 1         unmet
    the state it asks for: deuterium 0.20, 1 patient(s), worst system 0.60
    it decides: fires: patients >= 1 -- now 1
```

**One way:** on a fresh ship it **does not fire**, and the nearest miss is `system_condition < 0.7 --
now 1 (short by 0.3)` — the ship's own number, and every unmet condition listed with its reading.
**The other way:** with the state its own conditions ask for (deuterium 0.20, a patient in the ward, a
system below 0.7), it **fires**, and names the condition that fired it: `patients >= 1 -- now 1`. Both
lines are asserted by `scripts/evaluator-check.sh`, so a silent change in either fails the check.

## 3. The worked example, end to end

The same command, continued — the scenario instantiated, its decision resolved:

```
    instantiating:
      scenario "deck11-gel-pack": fires: patients >= 1 -- now 1
      system replicators: condition 0.60 -> 0.30 (offline)
      system life support: condition 1.00 -> 0.85 (nominal)
      system holodecks: off
      system sickbay: read and written, no initial change
      staff sickbay held
      staff engineering held
      staff operations held
      staff: 3 post(s) required; the ship needs 22 posts fully manned, its 141 crew cover 22 per watch (critical set holds); 3/3 held
      warmth holodeck: a program runs for B'Elanna Torres
      position the engineering seat -> B'Elanna Torres (3 information, 2 authority, 2 responsible, decision: which systems go dark while the grid is down)
      position the medical seat -> The Doctor (3 information, 2 authority, 2 responsible, decision: who is treated first, and what is turned off to keep the beds lit)
      order to the engineering seat (B'Elanna Torres): "Isolate the forward grid. Nothing draws from it until I say so."
      condolence to the medical seat (The Doctor): "Treat the carriers first. We will hold the deck."
      visibility station:engineering: a surface at the post
      visibility station:sickbay: a surface at the post
      visibility deck:11: deck 11: air 100%  hull 100%  life support nominal
      carried: 3 breeding condition(s), cost-to-answer "isolate the forward grid and treat the carriers", cost-to-ignore "let the fever run and keep the lights on", 2 residue, 2 decision option(s)
      scenario "deck11-gel-pack": instantiated, and recorded in flight
    resolving (answer):
      resolve "deck11-gel-pack": isolate the forward grid and treat the carriers
      effect fabricate_parts 2: spare parts 102
      effect sicken sickbay (The Doctor): exhausted
      effect sicken sickbay (The Doctor): grieving
      residue: one deck's upkeep gone quiet
      scenario "deck11-gel-pack": resolved and recorded
```

So the rest of the contract is honoured, and the seam is visible in the transcript: the **premise**
(systems, staff, warmth, positions, lines) is instantiated by `Recruit`; the **decision's cost** and
the **residue** are applied by `Resolve`. The residue carries both its words (`one deck's upkeep gone
quiet`) and its effect on a person (`sicken sickbay`).

## 4. The post-not-name law, at runtime

Not merely validated at load — the instantiated scenario resolves each line to whoever holds the post,
and the same content recruits under an entirely fictitious crew:

```
    the law at runtime, an all-fictitious crew:
      order to the engineering seat (Crewman 022): "Isolate the forward grid. Nothing draws from it until I say so."
      condolence to the medical seat (Crewman 098): "Treat the carriers first. We will hold the deck."
```

The line is the same; only the person changes. The ledger also records the **scenario's** name, never a
person's (`docs/scenario-manifest.md`, the two laws).

## 5. The falsification — a tautology and a contradiction

A condition that is true for **every** value the state can take recruits nothing (it would fire
regardless), and one true for **no** value can never fire. The evaluator does not treat either as an
ordinary met/unmet condition: it names them **`always`** and **`never`**. A condition naming state **no
reader carries** is likewise not ordinary: it is **`unreadable`**, cannot recruit, and blocks.

```
    degenerate, and named (not ordinary): deuterium >= 0 -> always; deuterium > 1 -> never
    a state no reader carries cannot recruit: nebula_contact >= 1 -> unreadable (nebula_contact)
```

The range per state is the state's own (`deuterium` ∈ [0,1]; the counts ∈ [0,∞)), so `deuterium >= 0`
is a tautology and `deuterium > 1` a contradiction, while `patients >= 1` is ordinary. **The check has
teeth:** `scripts/evaluator-check.sh` copies `scenario_core.cpp`, plants an evaluator that drops the
distinction (`COND_ALWAYS` → `COND_MET`, `COND_NEVER` → `COND_UNMET`), rebuilds, and **must fail** —
demonstrated failing, and passing once restored:

```
PASS  the planted evaluator fails the degeneracy assertion, so the check has teeth (restored)
```

## 6. What no state carries

Two lists, and they are different problems for the next pass.

**Declared, but not answerable as declared — the condition shape carries no subject.** The contract
names three scoped readers, and the `{state, op, value}` tuple has nowhere to say *which* system or
*which* deck:

```
    declared but not answerable as declared (no subject in the condition shape): system_condition deck_life_support minutes_of_air
```

The evaluator answers them by a decided **worst-case default** — the most damaged system, the
least-supported deck, the deck soonest out of air — so a scenario can still recruit, but it cannot pin
the subject. `system_condition` is read as the worst **health**, not `SystemCondition`: `SystemCondition`
is health capped by *power*, and the standby weapons (shields, phasers, the launchers) draw no power
until used, so a ship-wide worst `SystemCondition` is **0 on a completely healthy ship** (measured:
`shields`, `phasers`, `torpedo launchers` each `health 1.00 out 0.00 cond 0.00` on a fresh `NewShip`).
Reading the damage axis is what makes the worked example's `system_condition < 0.7` mean "a system is
ailing" rather than "the weapons are on standby".

**The design implies, and no manifest key reaches at all.** `docs/scenario-atlas.md` names breeding
facts the vocabulary has no entry for, so they cannot even be declared:

```
    the design implies, and no manifest key reaches at all: contact in_battle beacon breach structural_integrity clock
```

— a **contact** ("a mass reading, no transponder"), a **battle** ("a wounded crew list plus a battle";
`InCombat` exists but is not a readable state), a **beacon** ("one source available", a world, a live
link, a derelict), a **breach** on a named deck, **structural integrity**, and the **clock** ("nine
days out at warp 6.2"). Each is a state the next pass adds to the contract, and a primitive it already
has.

## 7. The save — what an in-flight or fired scenario costs

A scenario fires **once**: the evaluator's verdict is a read, but the firing is recorded, so a scenario
does not fire twice across ticks or a load, and an **in-flight** one (fired, its decision not yet
taken) survives a save. The record is a bounded ledger of `{name, branch}`, at most
`SCENARIO_FIRED_MAX` 32, saved as **format 65**:

```
    the ledger: deck11-gel-pack(answer)
    save: 10398 bytes; round-trips the ledger: yes; an older save (v64) loads: no (refused)
```

The cost is named, as the house rule requires: **an older save (64) does not load** — the version the
maintenance pass wrote. The 10,398-byte blob above is a whole ship save, not the ledger's own cost; the
ledger itself is a name (≤63 bytes) and a `uint16` branch per fired scenario, bounded at 32. A save
with no scenario fired carries one byte (`0` entries). `TestScenarioEvaluator` proves the round-trip,
the no-double-fire across a load, and the refusal of a version-64 blob.

## 8. The checks

```sh
PATH="/home/robot/.cache/lwh-venv/bin:$PATH" bash scripts/test.sh      # includes scripts/evaluator-check.sh
bash scripts/docs-check.sh
```

Both exit 0. `scripts/evaluator-check.sh` asserts the two-sided decision, the instantiation, the
post-not-name law at runtime, the unreadable and design-implied lists, and the save — and demonstrates
its own falsification.

`scripts/check.sh` also exits 0 here. It needs the GDK (a shipped map source and a script corpus), which
is not vendored; run against the host's extracted GDK — `--source-map …/maps/eliteforce_borg_maps/borg1.map
--script-corpus …/gdk/scripts` — it runs `scripts/test.sh` (including `scripts/evaluator-check.sh`), the
entity dictionary, the validator negative tests (the declaration cases `E009`/`E010`/`E011` pass), and
the corpus pass: **2408 files, 2394 compiled and read back, 14 rejected by the compiler** (exactly the
documented set), 0 read-back failures, and the two unresolved map script references are the recorded
boundary. The command and its exit status are the evidence; the GDK itself is not committed.

## 9. What could not be verified

- **Whether a scenario is a *situation* rather than a config.** That is the owner's, and the judgement
  every scenario after this one rests on. Nothing on this host decides it.
- **The engine and the rendered surfaces.** No display on this host: the evaluator was built and driven
  as the unit suite and the `--recruit` transcript, the way the ship simulation's own tests are; that a
  station screen shows a scenario's visibility is the playtest host's. The module itself compiles the
  evaluator and the artifact was asserted (see §10); the *rendered* frame was not.
- **The in-game reach.** No console verb and no director loop loads a `*.scenario` and recruits it yet:
  the loader reads a string and the evaluator decides on the ship in hand, and the wiring of a file from
  the game's filesystem into that string is the playtest host's next step. This is the loader's own
  seam, carried forward rather than claimed: the evaluator is the mechanism the manifest pass named as
  missing, and it is built and demonstrated here.
- **The nine scenarios.** Not in this pass; one — the worked example — end to end.

## 10. The module

The evaluator lives in `module/ship/scenario_core.cpp`, which the module build globs. It was verified
the way `AGENTS.md` requires — **assert the artifact, not the tree**: the module was re-configured
(`cmake -S ../upstream/efgame -B ../upstream/efgame/build-linux -DLWH_MODULE_DIR=$PWD/module`) and
built, and `nm` shows the evaluator's symbols in the built library:

```sh
nm -C ../upstream/efgame/build-linux/libefgame.so | grep 'scenario::'
```

## 11. Judgement calls, named as calls

1. **`system_condition` is the worst health, not the worst `SystemCondition`.** The reason is measured
   (§6): the power-capped condition reads 0 for every powered-down-but-intact system. This is the
   decided reading; it is what makes the worked example's own condition mean what its author meant.
2. **The three scoped states are answered by a worst-case default**, and named as under-specified
   rather than refused, so the worked example fires. Refusing them would have made the worked example —
   the manifest pass's own example — unable to recruit, which is the failure this pass exists to end.
   The next pass gives the shape a subject.
3. **A tautology does not block firing; a contradiction does.** A condition met for every value is
   *satisfied* (the scenario can still fire), but it is flagged, because a scenario recruited by a
   tautology recruits itself. A condition met for no value can never fire, so it blocks. Both are named.
4. **The decisive condition is the one met by the narrowest slack** (normalised by its threshold): the
   latest condition to become the reason, the one the premise leans on hardest. When every condition is
   a tautology the first is named and the degeneracy carries the meaning.
5. **The nearest miss is the unmet condition with the smallest relative shortfall** — the fractional
   gap to its threshold — so counts and fractions compare. `!=` is never a near miss (a value is either
   equal or not; there is no near unequal).
6. **The effect vocabulary maps onto the primitives the design names**, and where the design is silent
   it is decided once: `sicken`'s `wounded` maps to `COND_CONCUSSED` (the injury status `CREW_INJURED`
   is not a condition), a debuff clears at `CLEAR_SICKBAY`, and `write_off` reads a system name or a
   deck. All 13 kinds are reached; the mapping is one function (`ApplyEffect`).
7. **`Resolve` writes the residue** when the decision is taken, not when the scenario fires: the residue
   is what is *permanently different afterwards*, and "afterwards" is the answer.
8. **The ledger is bounded and saved (format 65), and the cost is named in the header's version line**
   and in `docs/designs-against-code.md` C-2's table. The alternative — hold no state, and let a fired
   scenario re-fire on every load — is a defect, not a saving.

## 12. The register rows this pass moves

- **`docs/omissions.md` O15** → *partly* stays; its text and marker now carry the evaluator as built
  (`scripts/evaluator-check.sh`); the nine scenarios remain the content.
- **`docs/walkthrough.md` G4** → *partly closed*: both halves of its own "To close" are built — the
  manifest and the breeding-condition evaluator, with one scenario end to end; the nine and the
  in-game reach remain. New marker `check:scripts/evaluator-check.sh`.
- **`docs/gates.md`** — a new subsection, *The evaluator — the thing that recruits a scenario*, follows
  the manifest section; O15 stays *partly*.
- **`docs/hook-register.md`** — three hooks added (`FiredScenarios`, `ScenarioFired`, `RecordScenario`),
  each reached by content and owned by `docs/scenario-manifest.md`; the register now lists all 496
  public functions. The two content-reached rows `docs/hook-triage.md` §5 already carries
  (`PostsNeeded`, `CoverWithCrew`) are unchanged.
- **Save format 65** is propagated where the current value is asserted as a number:
  `module/ship/ship_core.h` (the owner), `tools/docs/owned_claims.json`,
  `docs/designs-against-code.md` (C-2, three sites) and `docs/morale.md`.
