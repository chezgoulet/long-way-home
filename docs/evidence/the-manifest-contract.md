# Evidence: the manifest's field contract — the gate content waits on (O15)

`docs/the-derivation.md` measured the crossing as **one-sided**: the manifest validated `name`, `maps`,
`scripts` (plus `inhabited`, `asset_roots`, `crew`) and **had no key for any declaration field** — *"no
declaration reaches any primitive."* `docs/omissions.md` **O15** held the scenario slate and named two
authoring laws: the **post-not-name rule** and **the delivery direction per line**. This pass makes the
declaration layer real on `feat/the-manifest`, cut from `testing`, **2026-10-13**, on the static host (no
engine, no game data). It builds the contract, not the content: the nine scenarios are their own programme,
and the evaluator is the next pass.

**Observed, and the command that produced it.** Every figure and transcript below is the output of the
command named beside it, on this tree. The whole pass is one check, `scripts/scenario-check.sh`, wired into
`scripts/test.sh`.

## 1. The contract — every field the design declares, as a key

`docs/scenario-atlas.md` names the fields (*The authoring contract* `:66-76`, *The post-not-name law*
`:78-87`, *What a scenario must therefore declare* `:137-141`, *A first slate* `:183-184`). `tools/validator/
declarations.py` is the one machine form; the validator (`tools/validator/validate.py`) and the builder
(`tools/scenario/scenario_build.py`) both read it, so they cannot disagree about a field. The full table,
with types, allowed values and the design line for each, is `docs/scenario-manifest.md`, *The declaration
contract*; the eleven keys are:

```sh
python3 tools/scenario/scenario_build.py --fields
```

```
canon_root
breeding_conditions
systems
staff
cost
residue
visibility
warmth
positions
decision
lines
```

The vocabularies the **engine owns** are read from the engine, not copied, so they cannot drift: the 23
system names from `SPECS[]`, canon's six arcs from `ShieldSectionName`, the command seats and their canon
record types from the `SEATS[]` table, the nine departments from `DepartmentByName`, and the delivery
directions from `DeliveryName`. Where the **design is silent** about a shape — the breeding-condition tuple,
the effect objects, `deck:<n>` visibility, the `{state, op, value}` form — the shape is decided once and
recorded as ours in the same section.

## 2. The refusals — the validator's own words

**A manifest that names a crew member is refused** (`E010`, the post-not-name law). The check is
mechanical: it knows the manifest's own `crew.members`, and the canon command crew is read from the
engine's own seat table rather than listed by hand.

```sh
bash scripts/scenario-check.sh   # the "two laws" section
```

```
E010  ERROR  lines[0]: line addresses a person ('tuvok'); a line addresses a post
E010  ERROR  lines[0].text: content names a crew member ('tuvok'): content addresses a post, a system id or a role, and the simulation resolves it to a person (docs/the-entry-point.md, part three)
```

**A manifest with a bad field is refused** (`E009`), and **a line with no declared direction is refused**
(`E011`):

```
E009  ERROR  systems[0]: unknown system 'not_a_system'; a shipped system name, or <system>.<arc> for one of aft, dorsal, forward, port, starboard, ventral
E009  ERROR  scenario.json: unknown manifest key 'bogus_field'
E011  ERROR  lines[0]: line declares no delivery direction; known: order, report, confession, condolence, flat
```

**An unknown top-level key is refused**, deliberately: a key nothing reads is a field an author believes in
that the game ignores, and that is the failure this pass exists to prevent. The refusal is also in the
validator's own negative-test suite (`tools/validator/tests/negative_tests.py`, cases `E009`, `E010`,
`E011`), so `scripts/check.sh` exercises it against a real shipped map fixture.

## 3. The entry points — the 53, named, and what each is now

The derivation's inventory named **53 entry points** `content can call` and nothing reaches. The brutal
honesty, first: **the declaration contract calls two of them directly**, and the other fifty-one are **not
the contract's** — they are the **evaluator's event set** (what a scenario *does when it fires*), the
**voice layer's** (how a line is delivered), the **report layer's** (what a scenario leaves in the record),
or plumbing. Wiring all fifty-three for symmetry would be the mistake the brief names; the fields call what
they need.

| entry point | what the contract does with it |
|---|---|
| `LogEvent` | **wired here.** A declared line reaches the official log, signed by the post's resolved holder (`scenario_core.cpp`, `Apply`). |
| `ScanCompartment` | **wired here.** A `deck:<n>` visibility is a compartment read — what the player sees where they are. |
| `UseSystemBy`, `RecommendAllocation`, `RepairDeck`, `MakeBreach`, `TakeMind` | **left — not the contract's.** A use attributed to an operator, a recommendation, a deck repair, a breach, a taken mind: the evaluator's acts, not a declaration field's. |
| `Scan`, `FireTorpedo`, `ShuttleByClass`, `RecallShuttle`, `StrandShuttle`, `LoseShuttle`, `ShuttleBayHit`, `RebuildShuttle`, `Recomposite`, `LocateDilithium`, `TractorWreck`, `TractorHold`, `RevealPhenomenon`, `ActivateEMH`, `DamagePylon`, `Hail`, `Trade`, `AnswerDistress`, `Disengage`, `TransportAway`, `TransportBack`, `SetCourse`, `CatchUp`, `Suspend`, `Sleep`, `SetPlayerDeck`, `WoundPlayer`, `AttendIncapacitatedPlayer` | **left — the evaluator's event set.** These are what a scenario *fires* once its breeding conditions are met; the evaluator is the next pass. |
| `NextRisingRoll`, `RetellRising` | **left — the rising offer.** A post's decision to rise is a scenario the evaluator may recruit; `OfferRising`/`AttemptRising` (already named by a field row) are the post-addressed verbs. |
| `VoiceWrite`, `VoiceRetire`, `EmitCue`, `QueueRender`, `CacheRendered`, `PruneVoiceCache`, `PlanMeetingAudio`, `WarmVoice`, `SubmitNovelAnswer` | **left — the voice layer.** A declared line must be *spoken* through this seam, but that is the meeting/voice programme's (M5), not the contract's. |
| `DraftReport`, `StrikeReportLine`, `SoftenReportLine`, `EditReportLine`, `AddReportLine` | **left — the report layer.** `LogEvent` is the record reach wired here; the month report and its editor are their own surface. |
| `CreateCharacter`, `Unpack` | **left — the layer beneath.** A position resolves to a *person the roster already has*; creating one is the configurator's, and `Unpack` is the save seam. |

And the fields' **own** call paths, beyond the 53, are wired rather than named: the systems via
`SetEnabled` / `DamageSystem` / `ShieldHit`; the staff via `PostsNeeded` / `CoverWithCrew`; the warmth via
`RunHolodeck`; the post via `SeatHolder` / `DepartmentHead` / `NameOf` — **the two primitives the derivation
found reached by nothing but the tests are now reached by a `staff` field** (`docs/hook-register.md`'s reach
column reads `content` for `PostsNeeded` and `CoverWithCrew`; `docs/hook-triage.md` §5 marks them wired).

## 4. The worked example, end to end

`scenarios/deck11-gel-pack/` — the slate's first scenario, the gel pack fever, smallest real manifest the
contract accepts, **every field used once**. It validates, compiles, and the simulation does what it
declares:

```sh
python3 tools/validator/validate.py scenarios/deck11-gel-pack
```

```
          info   declaration: 11 field(s) declared and checked; 11 in the contract
0 error(s), 4 warning(s)
```

```sh
python3 tools/scenario/scenario_build.py scenarios/deck11-gel-pack
test_ship_core --scenario scenarios/deck11-gel-pack/build/maps/deck11-gel-pack.scenario
```

```
  scenario "deck11-gel-pack": 11 declaration field(s) carried by the loader
  fields: canon_root breeding_conditions systems staff cost residue visibility warmth positions decision lines
  canon root: the gel pack fever (the forward bio-neural grid; slate 1, docs/scenario-atlas.md)
  systems, as declared (before -> after):
    system replicators: condition 1.00 -> 0.70 (nominal)
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
  the post-not-name law, at the two rosters:
    post the engineering seat   canon: B'Elanna Torres fictitious: Crewman 022
    post the medical seat       canon: The Doctor   fictitious: Crewman 098
    order to the engineering seat (Crewman 022): "Isolate the forward grid. Nothing draws from it until I say so."
    condolence to the medical seat (Crewman 098): "Treat the carriers first. We will hold the deck."
  carried for the evaluator (not applied here): 3 condition(s), 2 option(s)
```

**The law, demonstrated rather than asserted.** The same manifest, applied to the canon roster and to an
all-fictitious one, resolves each post to a different person **and the content is unchanged** — the exact
property the post-not-name law exists to buy. **What the loader instantiates:** the systems it stresses
(`holodecks` off; `replicators` and `life support` under load), the staff coverage (`PostsNeeded` /
`CoverWithCrew`), the warmth beat (`RunHolodeck`), and every position and line resolved from its post to a
person. **What it carries:** the breeding conditions, the costs, the residue and the decision — the
evaluator reads those, and half-building it here is what the brief forbids.

## 5. The falsification — a field the loader ignores is caught

The loader's field list and the schema's must agree in both directions, or a field on one side and not the
other is a field an author writes and the game ignores. The check compares them, and is demonstrated
failing on a plant:

```sh
python3 tools/scenario/scenario_build.py --fields | sort   # vs
test_ship_core --scenario-fields | sort
```

```
PASS  the loader carries every contract field, and no field the schema does not name
PASS  the planted field fails the comparison, so the check has teeth
```

The plant adds `bogus_field` to `FIELDS[]` in a copy of `module/ship/scenario_core.cpp`, rebuilds, and the
comparison must fail — a guard that cannot fail for the reason we care about is decoration. A second
falsification is in the two laws themselves: the refusals of §2 are the validator failing on a planted
name, a planted missing direction, and a planted bad field.

## 6. What the engine still lacks — named as the next pass

**The evaluator.** Something must read a scenario's `breeding_conditions` and decide that it fires, then
apply its `cost` and write its `residue`. `docs/the-derivation.md` names this the first content-blocker
(reconciliation 7.34 and 8.56, `absent`): *no director recruits an authored event from the ship's state.*
This pass deliberately **does not** build it: the loader parses those fields and carries them, and the
comment in `module/ship/scenario_core.h` says so. It is the next pass, and the 51 entry points above are its
event set. Two smaller pieces wait behind it: the **voice seam** for a line (the meeting/voice programme),
and the **per-deck residue** the derivation found contradicted (reconciliation 7.8), which is what slate 8's
sealed deck needs before a residue can name *the place* it changed.

## 7. What could not be verified

- **Whether a scenario written this way reads as a situation rather than a config.** That is the owner's,
  and it is the judgement every scenario after this one is written against. Nothing on this host decides it.
- **The engine and the rendered surfaces.** Static host: no engine, no game data. The loader was built and
  driven as a unit test (`tests/ship/test_ship_core.cpp`, `TestScenarioContract`, and the `--scenario`
  transcript above), the way the ship simulation's own tests are; that a station screen shows a scenario's
  visibility is the playtest host's, on `scripts/run-lwh.sh` / `scripts/run-scenario.sh` in a logged-in
  session. **The module itself does compile the loader**, verified here by re-configuring and building it
  (`AGENTS.md`'s trap: a new `.cpp` under `module/` is not compiled until cmake re-configures):
  `nm libefgame.so` shows `scenario::Parse`, `scenario::Apply`, `scenario::ResolvePost` and
  `scenario::Fields` in the built library. No in-game verb invokes the loader yet — that is the evaluator's.
- **`scripts/check.sh`.** It needs the GDK: a shipped map source and a script corpus, neither of which is
  on this host (they are on the playtest host). Its new cases were run against the `testroom` map instead,
  with no dictionary and no compiler, and the four declaration cases pass:
  `python3 tools/validator/tests/negative_tests.py --source-map scenarios/testroom-watch/maps/lwhroom.map`
  → `PASS E009 bad declaration field`, `PASS E009 unknown manifest key`, `PASS E010 post-not-name law`,
  `PASS E011 delivery direction missing` (the `E001`/`E006` cases need the dictionary and the compiler and
  are the playtest host's).
- **The nine scenarios.** Not in this pass.

## 8. Judgement calls, named as calls

1. **The field list is the atlas's, taken in full** (the same set `docs/the-derivation.md` used): the
   authoring contract's eight, the post-not-name law's one, *what a scenario must therefore declare*'s five,
   and the slate's two. The eleven manifest keys are the two slate fields (`canon_root`, `decision`) folded
   together with the rest where they shared a shape.
2. **The manifest is closed.** An unknown top-level key is `E009`, not ignored. This is the only way a
   "field the loader ignores" can be caught at the manifest, and it is a deliberate tightening.
3. **The design's silence was filled, not invented.** The `{state, op, value}` condition tuple, the effect
   objects, `deck:<n>` visibility and the `stress`/`off` load are ours, recorded in
   `docs/scenario-manifest.md` under *Where the design is silent*, so they can be overruled.
4. **`content` is a fourth reach state, added to the register's three.** A primitive a *scenario* reaches is
   not a player's reach and not "nothing"; calling it `nothing` after wiring it would be the staleness the
   register exists to prevent.
5. **Only two of the 53 were wired, and the fifty-one are named.** The brief says not to wire for symmetry;
   this is the honest reading of which a declaration calls.
6. **The five checks whose `test_ship_core` compile line needed the new source** were updated to compile
   `scenario_core.cpp` too (`ood`, `damage-control`, `departments`, `shields`, `bills`): the test binary now
   needs it, and a stale compile line would have read as a broken check rather than a missing source.
7. **The engine's seat table, not a hand list, supplies the canon names** the law refuses; the two lists
   (systems, arcs, departments, deliveries) are likewise read from the engine. A vocabulary a document owns
   should not be retyped where a script can read it.
8. **A canon name is refused as a capitalised proper noun; a member identifier is refused without case.**
   The role word `doctor` ("tell the doctor") is not the name `Doctor`, and a mechanical check that refused
   the role would cry wolf on correct content. The manifest's own member names are identifiers (`watch1`),
   never prose, so they are matched case-insensitively. **What the check cannot see**, said plainly: a
   person named in a form neither list carries, and a canonical name used as an ordinary word.

## 9. The checks

```sh
PATH="/home/robot/.cache/lwh-venv/bin:$PATH" bash scripts/test.sh
bash scripts/docs-check.sh
```

Both exit 0; `scripts/scenario-check.sh` is wired into `scripts/test.sh`, and the register rows this pass
moves are marked (`docs/omissions.md` O15 → *partly*; `docs/hook-triage.md` §5 `PostsNeeded` /
`CoverWithCrew` → closed; `docs/hook-register.md`'s reach column → `content`). `scripts/check.sh` needs the
GDK and is the playtest host's; the validator's new negative cases are in
`tools/validator/tests/negative_tests.py` for that run.
