# The reconciliation register: the design corpus against the implementation

**What this is.** The sibling of `docs/omissions.md`, and the other question. The omissions register asks
*is this commitment registered?* — it sweeps the corpus against the roadmap and finds what no gate carries.
This register asks **does the game do what the document says?** A claim can be on the roadmap, carried by a
gate, marked ✅ — and still be wrong in the code. The register is a reconciliation, not a build: **nothing was
fixed, in a document or in a line of code.**

**Date:** 2026-10-11, on `feat/the-designs-vs-code`, cut from `testing`.

**The rule that decides a verdict.** The code is the witness. A claim is `implemented` when the code, a unit
test or a check script says so — never when a second document agrees with the first. A claim whose only
witness is another document is `unverified`, and that is a verdict, not a pass.

**The verdicts.**

| verdict | meaning |
|---|---|
| **implemented** | a named symbol, test or check proves it |
| **partial** | half of it is built; the missing half is named in the claim's own terms |
| **absent** | nothing implements it |
| **contradicted** | **the code does something else** — the document and the game disagree |
| **superseded** | a later ruling changed it; the ruling is named |
| **unverified** | no code witness was reachable either way |

---

## The contradictions

This is the section the pass exists for. Each entry quotes the document and the code, and says **which side
is wrong** where the evidence says so. Where it does not, it gives the **fork** rather than choosing quietly.
A `contradicted` row also appears in its domain table below; the entries here are the ones with both sides
worth quoting.

### C-1 — The morale scalar the document still describes is gone

- **Document:** `docs/story-and-semantics.md:132-133` — *"`CrewMember` in `ship_core.h` carries a single
  `float morale` ("0 broken, 0.5 going through the motions, 1 heart in it")."*
- **Code:** `module/ship/ship_core.h:511-517` — *"Morale is a read of three components, never a stored
  counter … deficit … outlook … holdings"*; `SAVE_VERSION` 53 at `module/ship/ship_core.h:2988` — *"The old
  single `float morale` is gone."* `Morale()` is `module/ship/ship_core.cpp:768`, read from the three.
- **Which side is wrong: the document.** Conflict 2 was never annotated, unlike conflict 1 beside it
  (`docs/story-and-semantics.md:123`), so a reader meets a false description of the record. The walkthrough's
  **G14** already calls conflict 2 closed; the conflict itself was not corrected.
- **Disposition 2026-10-11:** corrected — `docs/story-and-semantics.md` conflict **2** is now marked RESOLVED 2026-10-11 (save 53): the single `float morale` is gone and `Morale(c)` reads the three components. <!-- register-row id=C-1 state=closed marker=symbol:module/ship/ship_core.h:Morale -->

### C-2 — The save format is quoted at a version it left two bumps ago

- **Document:** `docs/morale.md:142` — *"the save format is **57** and an older save does not load"*.
- **Code:** `module/ship/ship_core.h:2988` — `const uint16_t SAVE_VERSION = 60;` (58: the Collective's held
  minds; 59: the bills).
- **Which side is wrong: the document.** The moral claim it makes — the per-person `float` left the save — is
  true; the number is stale. `tools/docs/check_claims.py` polices only the claims registered in
  `tools/docs/owned_claims.json`, and this one is not among them.
- **Disposition 2026-10-11:** corrected — `docs/morale.md` now says the save format is **59**. <!-- register-row id=C-2 state=closed marker=symbol:module/ship/ship_core.h:SAVE_VERSION -->

### C-3 — Sickbay's bed count is six in the ledger and four in the code

- **Document:** `docs/lore-ledger.md:209` — *"Sickbay | 6 under treatment at once; 12 hours each at full
  output"*.
- **Code:** `module/ship/ship_core.h:2276` — `const int SICKBAY_BEDS = 4; // three standard and one surgical
  [lore]; the worst get them`. `docs/ship-systems.md:120` says *"three biobeds plus a surgical bed"*, and
  `docs/evidence/gap-triage-and-sickbay.md:34` tests *"exactly `SICKBAY_BEDS` under care"*.
- **Which side is wrong: the ledger.** Four is the built and better-sourced number (the surgical bay is a
  separate constant); the ledger row is the outlier.
- **Disposition 2026-10-11:** corrected — `docs/lore-ledger.md` now says sickbay is **4** (`SICKBAY_BEDS`). <!-- register-row id=C-3 state=closed marker=symbol:module/ship/ship_core.h:SICKBAY_BEDS -->

### C-4 — The sector's composition cannot be produced by the sector's own roll

- **Document:** `docs/lore-ledger.md:242` — *"A sector | 12 beacons: 40% empty, 30% hostile, 20% derelict,
  10% Borg"*.
- **Code:** `module/ship/ship_core.cpp:2686-2689` — `const uint32_t roll = next() % 15;` then
  `roll < 4 ? EMPTY : roll < 7 ? HOSTILE : roll < 9 ? DERELICT : roll < 10 ? BORG : …`, i.e. among the
  interior beacons **4/15 empty, 3/15 hostile, 2/15 derelict, 1/15 Borg**, plus four kinds
  (trader, distress, belt, pre-warp) the ledger does not list. Beacon 0 is forced empty and beacon 11 is the
  goal.
- **Which side is wrong: the ledger**, as a description of the code. Either the divisor moves to `% 10` to
  honour the stated split, or the row is corrected to the roll. The evidence does not say which was intended,
  so the *fix* is a fork — but the number as written is not what the game does.
- **Disposition 2026-10-11:** corrected to what the game does — `docs/lore-ledger.md` now states the `% 15` roll and the four extra beacon kinds; **the design fork is handed to the code list** (move the divisor, or keep the roll). <!-- register-row id=C-4 state=closed marker=symbol:module/ship/ship_core.cpp:BEACON_PREWARP -->
- **Disposition 2026-10-12 (code pass):** **handed back** — the row's document half is closed, and its design fork is not decidable here: moving `next() % 15` to `% 10` to honour the stated 40/30/20/10 split, or keeping the roll, are two designs with the same evidence. The owner rules; `module/ship/ship_core.cpp` is left alone.

### C-5 — "Each recomposition costs her a fifteenth" — it costs 15 per cent, not a fifteenth

- **Document:** `docs/budget-squaring.md:328` — *"each recomposition costs her **a fifteenth** of her plant
  forever"*.
- **Code:** `module/ship/ship_core.h:906` — `const float RECOMPOSITE_CEILING_DROP = 0.15f;`, applied at
  `module/ship/ship_core.cpp:6580`. A fifteenth is 6.7 per cent; 0.15 is 15 per cent. The document's own
  Part five states the constant as `0.15` (`docs/budget-squaring.md:298`).
- **Which side is wrong: the document's prose** (a "15 per cent" slip), and it contradicts the same document's
  table twenty lines above.
- **Disposition 2026-10-11:** corrected — `docs/budget-squaring.md` now says **15 per cent**, not a fifteenth. <!-- register-row id=C-5 state=closed marker=symbol:module/ship/ship_core.h:RECOMPOSITE_CEILING_DROP -->

### C-6 — "Eighteen systems" is the shape before the five the owner added

- **Document:** `docs/walkthrough.md:390` — *"all eighteen systems in power order"*; `:554` — *"four sources
  against eighteen systems"*.
- **Code:** `module/ship/ship_core.h:31-58` — `SYS_COUNT` is **23**: the original eighteen plus
  `SYS_ASTROMETRICS`, `SYS_SCIENCE_LABS`, `SYS_GRAVITY_PLATING`, `SYS_LIGHTING`, `SYS_CARGO_HANDLING`
  (`SPECS[]` in `module/ship/ship_core.cpp:36-64`).
- **Which side is wrong: the walkthrough's route stops.** The gap list's **G12** records the addition; the
  body was not brought with it.
- **Disposition 2026-10-11:** corrected — `docs/walkthrough.md` now says **23** systems. <!-- register-row id=C-6 state=closed marker=symbol:module/ship/ship_core.h:SYS_COUNT -->

### C-7 — "There is no nominal-against-current power display" — there is

- **Document:** `docs/walkthrough.md:554-556` — *"**There is no nominal-against-current power display** …
  it is **not implemented** (**G12**)"*.
- **Code:** `module/ship/g_ship.cpp:722-735` — *"The power budget twice (owner ruling, 2026-10-07): what the
  plant delivers fresh, and what it delivers now"*, drawing `vessel.PowerCapacityFresh(),
  vessel.PowerCapacityNow()`; `module/ship/ship_core.h:2123-2124` declares both.
- **Which side is wrong: the walkthrough's route stop.** Its own **G12** row is marked CLOSED and names this
  exact proof.
- **Disposition 2026-10-11:** corrected — `docs/walkthrough.md` now records the fresh-against-now display as built. <!-- register-row id=C-7 state=closed marker=symbol:module/ship/ship_core.h:PowerCapacityFresh -->

### C-8 — "There is no way to start Long Way Home from the game's menus" — there is

- **Document:** `docs/walkthrough.md:25` — *"**There is no way to start Long Way Home from the game's
  menus.** `g_ship` defaults to `0` …"* (and the "cvars you must remember" table at `:44-58`).
- **Code:** `module/ui/ui_lwh_start.cpp:289` (`LWH_BUTTON_TEXT`), attached by
  `patches/0020-main-menu-long-way-home.patch`; the menu line and `scripts/run-lwh.sh` both execute
  `configs/lwh-start.cfg` (`module/ui/ui_lwh_start.cpp:136`). The gate is `docs/gates.md`, *the way in* ✅ built
  2026-10-08.
- **Which side is wrong: the walkthrough's route stop.** Its own **G1** row is CLOSED and names
  `configs/lwh-start.cfg` as the proof.
- **Disposition 2026-10-11:** corrected — `docs/walkthrough.md` §1 now records the menu line (G1 CLOSED). <!-- register-row id=C-8 state=closed marker=file:configs/lwh-start.cfg -->

### C-9 — "No labs, no astrometrics, no lighting, no cargo loads" — all four are loads now

- **Document:** `docs/walkthrough.md:264-270` — *"there are **no labs, no astrometrics, no lighting and no
  cargo loads in the power model** … life support and the holodecks are *not* independent sources as canon and
  `docs/ship-systems.md` require"* (and `:783-789`, inside G12's own finding text).
- **Code:** `module/ship/ship_core.cpp:36-64` — `SPECS[]` carries `astrometrics`, `science labs`,
  `non-essential lighting`, `cargo handling`, `gravity plating`; the independent air feed is
  `SetAirSource` (`module/ship/ship_core.cpp:2027`) with `AIR_GRID`/`AIR_PLANT_PER_DECK`
  (`module/ship/ship_core.h:183-184`), proved by `scripts/air-check.sh`.
- **Which side is wrong: the walkthrough's route stop** (its G12 row already corrects it). The one thing the
  code does *not* do is per-*compartment* atmosphere, which is the separate, ruled **C4** question.
- **Disposition 2026-10-11:** corrected — `docs/walkthrough.md`'s deck-11 stop and G12's finding text now record the five loads and the independent air feed. <!-- register-row id=C-9 state=closed marker=check:scripts/air-check.sh -->

### C-10 — "The crew's manner … still open" — the manner is built

- **Document:** `docs/walkthrough.md:368-369` — *"the crew's **manner** … is the one item of the morale
  contract still open"*; and the completeness verdict at `:1062`, *"the crew's manner unwired"*.
- **Code:** `MannerFor`/`EmitManner` (`module/ship/ship_core.cpp:990,1052`), `MannerChoice`
  (`module/ship/ship_core.h:1166`), save-safe (`TestMannerDeterminismAndSave`), and `scripts/manner-check.sh`
  passes and fails on a planted unwired state. The gate is `docs/gates.md` G14, ✅ built 2026-10-11.
- **Which side is wrong: the walkthrough's body.** Its G14 row is CLOSED.
- **Disposition 2026-10-11:** corrected — `docs/walkthrough.md` §6 and the completeness verdict now record the manner as built (G14 CLOSED). <!-- register-row id=C-10 state=closed marker=check:scripts/manner-check.sh -->

### C-11 — The officer of the deck's authority is documented, unit-tested, and wired to nothing

- **Document:** `docs/orders.md:28-30` — *"The authority exists … Who may order whom is already
  answerable"*; `docs/crew-manifest.md:225-228` — the OOD may order senior officers who are not the captain.
- **Code:** `OodMayOrder`/`OodRefusal` exist (`module/ship/ship_core.cpp:6154,6167`) and are **called only
  from the unit test** (`tests/ship/test_ship_core.cpp:8537-8556`). The order path uses `MayOrder`
  (`module/ship/ship_core.cpp:9930-9936`), which is `MayCommand(giver) || giver.rank > recipient.rank`.
- **Which side is wrong: fork.** The predicate and its test are real; no decision consumes it, so the deck's
  documented authority cannot actually dispatch an order at a rank-3 lieutenant over a rank-4 commander. The
  code has the rule and does not use it — an `implemented` predicate that is `absent` as authority.
- **Disposition 2026-10-11:** **handed to the code list** — a fork: `OodMayOrder`/`OodRefusal` exist and are unit-tested but no decision consumes them, so the deck's documented authority cannot dispatch an order. Decide the code (`DispatchOrder` should consult the OOD rule) or the document.
- **Disposition 2026-10-12 (code pass):** **fixed** — the register's own code option is taken: the predicate keeps its place and gains its consumer. `MayOrder` (`module/ship/ship_core.cpp:9948`) now returns true when the giver is the officer of the deck (`OfficerOfTheDeck`) and `OodMayOrder` allows it, so `DeriveOrders`, `OrderPermitted`, `OrderRefusalRule` and `DispatchOrder` all carry the deck's authority while the rank rule alone still refuses it. Proven by `TestOodDispatch`; falsified by reverting the branch, which fails it (see `docs/evidence/the-code-contradictions.md`). <!-- register-row id=C-11 state=closed marker=symbol:tests/ship/test_ship_core.cpp:TestOodDispatch -->

### C-12 — "Assigning a repair empties a station" — the repair party is stationless

- **Document:** `docs/ship-systems.md:139` — *"crew time comes from the same people who hold posts —
  assigning a repair empties a station. That single link is what makes the crew model and the damage model one
  system instead of two."* (`docs/crew-work.md:12` repeats it.)
- **Code:** `module/ship/ship_core.cpp:1744-1746` — *"An engineer on duty with no station of their own joins
  the damage-control party"*; a post-holder is counted at their station first (`:1735-1742`), and
  `ORDER_SEE_TO` only sets a queue priority (`:10140`).
- **Which side is wrong: fork.** The stationless party is the deliberate, commented design; two documents
  promise a cost the code does not charge.
- **Disposition 2026-10-11:** **handed to the code list** — a fork: the stationless repair party is the deliberate, commented design, and two documents promise a cost the code does not charge.
- **Disposition 2026-10-12 (code pass):** **handed back** — this is a design decision, not a defect. The stationless party is the code's own commented intent (`ship_core.cpp:1745-1746`: *"An engineer on duty with no station of their own joins the damage-control party"*), and charging the cost the documents promise (a repair that empties a station) would change how the crew and damage models meet. The fork is stated for the owner; the code is left as it is.

### C-13 — Rumour is a provenance the enum declares and nothing writes

- **Document:** `docs/memory-and-consequence.md:47` — sources *"saw it myself / was told by X / heard it as
  rumour / read it in the log"*; `:55` — *"rumours arrive distorted, and the distortion is attributable"*.
- **Code:** `enum MemorySource { MEM_SAW, MEM_TOLD, MEM_RUMOUR, MEM_LOG }` (`module/ship/ship_core.h:381`)
  declares all four; a repository-wide search finds no writer of `MEM_RUMOUR` anywhere in `module/` or
  `tests/`.
- **Which side is wrong: the code.** Half the promise is unimplemented and the enum is decoration for it.
- **Disposition 2026-10-11:** **handed to the code list** — the code is the wrong side: `MEM_RUMOUR` is declared and nothing writes it.
- **Disposition 2026-10-12 (code pass):** **fixed** — the writer is built. `WriteOff` (`module/ship/ship_core.cpp`) now marks the crew when something is given up: those on the written-off deck saw it (`MEM_SAW`), everyone else hears it as a rumour (`MEM_RUMOUR`), and the two versions differ in valence — the deck-write-off case the document names (`docs/memory-and-consequence.md:74`). `MEM_RUMOUR` is no longer decoration. What is left: the full distortion/attribution model (a rumour's teller, and how its telling drifts) — the register's row 4.22, still `partial`/`absent`, and named in the evidence. Proven by `TestWrittenOffList`; falsified by reverting the writer, which fails it (see `docs/evidence/the-code-contradictions.md`). <!-- register-row id=C-13 state=closed marker=symbol:module/ship/ship_core.cpp:MEM_RUMOUR -->

### C-14 — Six shield sections and fourteen grids are one scalar

- **Document:** `docs/ship-systems.md:39,113` — *"shields | fourteen external grids; six sections
  (fwd/port/stbd/aft/dorsal/ventral)"*; *"percentages that buckle"*.
- **Code:** `module/ship/ship_core.h:1956` — `float shieldStrength = 1.0f;` — one number, used by the combat
  path (`module/ship/ship_core.cpp:3596-3597`). No section, no grid, no per-arc state.
- **Which side is wrong: the code** (incomplete). The document is ahead of the build.
- **Disposition 2026-10-11:** **handed to the code list** — the code is incomplete: one `shieldStrength` scalar, no sections or grids.
- **Disposition 2026-10-12 (code pass):** **disputed** — the register names the code, but **I disagree that this is a defect to fix here.** A scalar where the design implies a set is the brief's own named example of a design decision ("a scalar where the design implies a set … guessing there manufactures a defect out of a decision nobody made"), and building it is the feature the brief forbids: six sections, fourteen grids, per-arc state and bypass conditions (`docs/ship-systems.md:39,113`). The code is not doing something else with no ruling; it is doing less than the design. The fork — the one `shieldStrength` (`module/ship/ship_core.h:1959`) against the shield model — is the owner's to rule, and the code is left alone. Reasoning in `docs/evidence/the-code-contradictions.md`.

### C-15 — "At most twelve shots penetrate" vs a continuous adaptation ramp

- **Document:** `docs/ship-systems.md:149-150` — *"Adaptation is a counter per weapon: canon's phaser adapter
  lets at most twelve shots penetrate before the Borg adapt"*.
- **Code:** `module/ship/ship_core.cpp:3572` — `s.enemy.adaptation = std::min(1.0f, s.enemy.adaptation +
  ours * (2.0f + s.borgAwareness));` — a continuous accumulator, reduced by `Remodulate`/`RaidVinculum`, with
  no shot counter and no twelve-shot cap.
- **Which side is wrong: fork.** If the twelve-shot number is the design it is unbuilt; if the ramp is, the
  document is stale.
- **Disposition 2026-10-11:** **handed to the code list** — a fork: the canon twelve-shot counter against the continuous adaptation ramp.
- **Disposition 2026-10-12 (code pass):** **handed back** — a rate where the document implies a threshold, the other shape the brief routes to the owner. The ramp is the code (`module/ship/ship_core.cpp:3572`), the twelve-shot adapter is canon; which one is the design is not decidable from the evidence, and building a counter would replace a working mechanism with a guess.

### C-16 — "Ship-level failure is the Borg reaching the core" — the only loss is the core breach

- **Document:** `docs/ship-systems.md:164-165` — *"**Ship-level failure** is the Borg reaching the core:
  capture, or self-destruct. That is the loss condition."*
- **Code:** `s.lost = true` is set only in `UpdateCore` when the breach countdown expires
  (`module/ship/ship_core.cpp:6522`). There is no Borg-capture or self-destruct loss.
- **Which side is wrong: superseded by `docs/endings.md:73`** — *"exactly one unwinnable end state, the core
  breach"* — which the code matches. The ship-systems sentence is the out-of-date side.
- **Disposition 2026-10-11:** corrected — `docs/ship-systems.md` now records the core breach as the one loss, matching `docs/endings.md`. <!-- register-row id=C-16 state=closed marker=check:scripts/core-check.sh -->

### C-17 — "Crew cannot open doors" — they can, and the check proves it

- **Document:** `docs/ship-programme.md:76` — *"**Crew cannot open doors** (found in G3). A crew that walks
  the whole ship needs door handling before S5."*
- **Code:** `module/crew/g_crew.cpp:258` `OpenDoorFor`, gated by `LayerMayOpen` (`:248`), with
  `g_crewDoorOpen` defaulting to `1` (`:1954`), and `scripts/crew-door-check.sh` as the regression.
- **Which side is wrong: the document** (a stale residual; the doors pass closed it).
- **Disposition 2026-10-11:** corrected — `docs/ship-programme.md` now records the crew door handling as built. <!-- register-row id=C-17 state=closed marker=check:scripts/crew-door-check.sh -->

### C-18 — "Every engine extension is cvar-gated … saves must round-trip" — the ceiling patches do neither

- **Document:** `docs/engine-extension-policy.md:33-37` — *"**Every engine extension is cvar-gated and off by
  default.** … **Saves must round-trip across the extension.** Raising a ceiling or changing entity layout
  must not make a retail save unloadable. Where a change would, it is gated and that is stated in the
  patch."* `AGENTS.md` restates the same promise.
- **Code:** `patches/0008-entities-to-4096.patch` and `patches/0018-entities-to-8192.patch` raise
  `GENTITYNUM_BITS` in a compile-time edit with no cvar, and each states in its own header: *"Saves written
  before this patch do not load after it: entity numbers are persisted and their width moved."* Neither is
  gated.
- **Which side is wrong: the policy's absolute wording** (and its restatement in `AGENTS.md`). The patches
  are honest about the break; the rules say "every" and "must". The honest rule is the one the hook patches
  follow — *behaviour-changing hooks* are gated; a limit raise is not, and its save cost is stated.
- **Disposition 2026-10-11:** corrected — `docs/engine-extension-policy.md` rules 3–4 are narrowed (a limit raise is not gated; the save cost is stated), and `AGENTS.md` is narrowed with them. <!-- register-row id=C-18 state=closed marker=file:patches/0018-entities-to-8192.patch -->

### C-19 — The module is thirty thousand lines, not three

- **Document:** `docs/engine-extension-policy.md:12-13` — *"currently some 3,000 lines of ours in `module/`,
  compiled in through a 51-line attach-point patch"*; `CONTRIBUTING.md:80` — *"the direction layer is some
  1,600 lines of ours and 51 added lines of delta."*
- **Code:** `module/` is **27,883** lines of `.cpp`/`.h`; `patches/0005` is 325 lines of file with ~103 added
  lines. The two documents also disagree with each other by a factor of two.
- **Which side is wrong: both documents** (the ratio argument still holds; the counts are stale).
- **Disposition 2026-10-11:** corrected — `docs/engine-extension-policy.md` and `CONTRIBUTING.md` now carry the measured counts. <!-- register-row id=C-19 state=closed marker=file:patches/0005-direction-layer-hooks.patch -->

### C-20 — "Nothing generated … enters the repository" — a generated image is tracked

- **Document:** `docs/asset-doctrine.md:49-51` — *"**Nothing generated and nothing extracted enters the
  repository.** No game assets, no third-party images, no generated meshes or skins committed as content."*
- **Repository:** `git ls-files docs/art/` returns `docs/art/lcars-meeting-overlay.html` and
  `docs/art/lcars-meeting-overlay.png`, the storyboard referenced at `docs/staff-meetings.md:143`.
- **Which side is wrong: the document's absolute sentence.** The practice is sound — the image is our own
  rendering, not a game asset or a third-party image — but the sentence as written is false; it should be
  narrowed to game and third-party content.
- **Disposition 2026-10-11:** corrected — `docs/asset-doctrine.md`'s absolute is narrowed to game and third-party content. <!-- register-row id=C-20 state=closed marker=file:docs/art/lcars-meeting-overlay.png -->

### C-21 — "Standing distance (24-64 units)" — the tool the document tells you to run uses 24-96

- **Document:** `docs/authoring-a-location.md:30` — *"work objects with navigation furniture at *standing
  distance* (24-64 units)"*; repeated at `docs/location-brief-template.md:65`. Both tell the author to run the
  post generator (`docs/authoring-a-location.md:112`).
- **Code:** `tools/posts/gen-posts.py:37` — `STANDING_MIN, STANDING_MAX = 24.0, 96.0`, applied at `:82`.
- **Which side is wrong: fork.** The document and the instrument it mandates disagree on the threshold.
- **Disposition 2026-10-11:** corrected to the tool — `docs/authoring-a-location.md` and `docs/location-brief-template.md` now say **24-96**, and name `tools/posts/gen-posts.py`. <!-- register-row id=C-21 state=closed marker=symbol:tools/posts/gen-posts.py:STANDING_MAX -->

### C-22 — "Not built: each modelled system's three designed failure states" — built two sections below

- **Document:** `docs/failure-is-content.md:144-145` — *"Not built: each modelled system's three designed
  failure states (this covers the core; the other systems still fail one way)…"*
- **Code:** the same document's next subsection says the opposite and the code agrees with it:
  `FailureStateOf`/`UpdateSystemStates` (`module/ship/ship_core.cpp:5424,5947`), `SystemState`
  (`module/ship/ship_core.h:93`), save format 40, `TestSystemStates`.
- **Which side is wrong: the stale sentence**, inside a section whose own dated subsection supersedes it. A
  reader who stops at line 145 is misled by the document's own history.
- **Disposition 2026-10-11:** corrected — `docs/failure-is-content.md`'s stale 'not built' sentence is marked superseded by its own next subsection. <!-- register-row id=C-22 state=closed marker=symbol:module/ship/ship_core.h:SystemState -->

### C-23 — The failure roll uses the factors, not `EffectiveSkill`

- **Document:** `docs/character-derivation.md:232-246` — *"This is `EffectiveSkill(c, skill)` … This is what
  decides whether a repair is quick, slow, or dangerous … The failure roll's odds are now wired."*
- **Code:** `WorkFactors` builds the skill term from **raw** `c.skills[skill]` plus the aptitude trait
  (`module/ship/ship_core.cpp:5542-5548`) and adds conditions/deficit as separate factors; `EffectiveSkill`
  (`:795`) is called only from `WorkReading`, `OfferRising`/`RisingOdds` and the console. The odds are wired —
  through a parallel construction, not through `EffectiveSkill`.
- **Which side is wrong: fork.** Both encode the same design; the document names a function the roll does not
  call, so the two can drift without either failing a test.
- **Disposition 2026-10-11:** **handed to the code list** — a fork: the failure roll uses raw skills via `WorkFactors`, not `EffectiveSkill`; the document names a function the roll does not call.
- **Disposition 2026-10-12 (code pass):** **handed back** — a fork. Both encode the same design; making the roll call `EffectiveSkill` is a change to the odds the work system produces, and the document could instead name `WorkFactors`. Which side moves is the owner's rule; the code is left alone.

### The code pass, 2026-10-12 — what this pass did with the twelve it was handed

The doc pass corrected the document side and **handed twelve rows over** whose closure is a code change
or a ruling. This pass took them, and also the design fork the register's own C-4 row hands here though
the doc pass counts C-4 among its corrections. **One of the twelve, 3.20, was closed on the base before
this pass rebased onto it** (`docs/crew-roster.md`, the owner's instruction, merged as PR #133), so it is
not re-dispositioned here; the remaining twelve rows are:

- **Fixed (3):** **C-11** — the OOD's predicate gains its consumer (`MayOrder` consults `OodMayOrder`);
  **C-13** — `MEM_RUMOUR` gains its writer (`WriteOff` marks the crew, those aboard `MEM_SAW` and the
  rest `MEM_RUMOUR`); **3.2** — the Doctor's rank. Each carries a closed marker above, and each fix's
  falsification is quoted in `docs/evidence/the-code-contradictions.md`.
- **Handed back (8):** **C-4** (its design fork), **C-12**, **C-15**, **C-23**, **3.3**, **3.23**,
  **8.52**, **8.53** — forks the evidence will not decide. The code is left alone and each fork is
  stated for the owner in the evidence.
- **Disputed (1):** **C-14** — the register pins it on the code, but "a scalar where the design implies a
  set" is a design decision, not a defect, and building six sections and fourteen grids is the feature
  the brief forbids. Its reasoning is in the evidence.

**No fix moves the save format; `SAVE_VERSION` stays 59.** Nothing here touches a document except the
dispositions on these rows, and `docs/evidence/the-code-contradictions.md` is the record.

---

## The counts

**400 claims** were extracted and carried a verdict; the table is **per class and per domain**. The
**23 contradictions** are the entries in the section above; a `contradicted` row in a domain table is the
same finding as a numbered entry where the two are the same claim, and a handful of further contradicted rows
are recorded only in the tables.

| domain | claims | implemented | partial | absent | contradicted | superseded | unverified |
|---|---|---|---|---|---|---|---|
| 1. The ship and its systems | 61 | 41 | 8 | 0 | 5 | 2 | 5 |
| 2. Power, damage and the two budgets | 42 | 28 | 4 | 1 | 4 | 4 | 1 |
| 3. The crew and their work | 57 | 27 | 13 | 5 | 7 | 2 | 3 |
| 4. The record, the log and the memory | 32 | 23 | 5 | 3 | 0 | 0 | 1 |
| 5. Orders, access and authority | 26 | 22 | 2 | 1 | 0 | 1 | 0 |
| 6. The rooms, the screens and the meetings | 39 | 24 | 3 | 1 | 8 | 1 | 2 |
| 7. Encounters and the outside | 42 | 24 | 11 | 7 | 0 | 0 | 0 |
| 8. The start, the endings, the modes | 56 | 32 | 8 | 10 | 3 | 1 | 2 |
| 9. The engine, the policy and the pipeline | 45 | 19 | 9 | 1 | 6 | 7 | 3 |
| **total** | **400** | **240** | **63** | **29** | **33** | **18** | **17** |

Two things that table will not say by itself, and should:

- **`implemented` witnesses a symbol, a test or a check — not the whole sentence.** `Hours` on the clock and
  a console that prints a number can both be `implemented` while the *judgement* in the claim is owed to the
  owner. Where a claim is a judgement — "does it read as a place", "does it feel like a month" — it is not
  extractable here and is not counted.
- **`absent` and `unverified` are different answers.** *Absent* means nothing implements it. *Unverified*
  means a witness could not be reached on this host — most often because the shipped map sources, the paks or
  the pinned upstream are not in the repository. A claim is never promoted to `implemented` to make the
  register look better.

---


## The register, by domain

### 1. The ship and its systems

`docs/ship-model.md`, `docs/ship-systems.md`, `docs/failure-is-content.md`,
`docs/navigation-counter.md`, `docs/ship-programme.md`.

| # | claim (doc:line) | verdict | witness | note |
|---|---|---|---|---|
| 1 | "a general keyed store inside the save … four-character tags" (`ship-model.md:14`) | implemented | `module/ship/g_ship.cpp:5478` `AppendToSaveGame('SHIP')`; `g_crew.cpp:2325` | the blob crosses the deck change |
| 2 | "deck links exist … `target_level_change` carries a `mapname`" (`ship-model.md:19`) | implemented | `scripts/s3-check.sh`; `docs/evidence/s3-merged-map-measured.md:274` | engine/map behaviour, verified by the tour |
| 3 | "controls exist … `func_usable` fires `G_UseTargets`" (`ship-model.md:21`) | implemented | `module/ship/g_ship.cpp:1313-1324`; `patches/0005` | |
| 4 | "the code's compartment *is* a deck" (`ship-model.md:39`) | implemented | `module/ship/ship_core.h:1940,201-231` | the C4 ruling is the code |
| 5 | "threats … as pressure per compartment, not a spawn list" (`ship-model.md:41`) | implemented | `module/ship/ship_core.h:205-222`; `ship_core.cpp:2474` | |
| 6 | "a clock … advance the model by elapsed time on every load" (`ship-model.md:42`) | implemented | `module/ship/ship_core.cpp:3763` `CatchUp`; `ship_core.h:2589` | wall-clock mode |
| 7 | "on load: read the blob, then make the deck match it" (`ship-model.md:49`) | implemented | `module/crew/g_crew.cpp:2187`, `:78` | the adapter |
| 8 | "crew crossing decks … a change to a `deck` field" (`ship-model.md:55`) | implemented | `module/ship/ship_core.h:552` | |
| 9 | "the three clocks S10 already implements" (`ship-model.md:62`) | implemented | `module/ship/ship_core.h:1784`; `g_ship.cpp:5377` | |
| 10 | "the choice is `g_shipQuit` (0 by default)" (`ship-model.md:71`) | implemented | `module/ship/g_ship.cpp:5315,5580`; `scripts/exits-check.sh` | O14 |
| 11 | "convergence … cutting long steps up" (`ship-model.md:78`) | implemented | `module/ship/ship_core.cpp:5991-5996`; `TestDeterminismAndStepSize` | |
| 12 | "standing orders are what the ship does while the player sleeps (save version 7)" (`ship-model.md:84`) | implemented | `module/ship/ship_core.h:2005-2009` | |
| 13 | "presence grows while unchallenged; the clock advances it" (`ship-model.md:102`) | implemented | `module/ship/ship_core.h:194-196`; `ship_core.cpp:2605-2608` | value contested (C-16 family) |
| 14 | "141 crew at launch … fifteen decks" (`ship-systems.md:9`) | implemented | `module/ship/ship_core.h:22-23`; `TestRoster` | other canon figures unaudited here |
| 15 | "fourteen external grids; six sections" (`ship-systems.md:39`) | contradicted | `module/ship/ship_core.h:1956` one scalar | **C-14** |
| 16 | "force fields rated 1-10; level 10 can cut a drone from the Collective" (`ship-systems.md:42`) | implemented | `module/ship/ship_core.h:197-198`; `TestForceFieldKit` | |
| 17 | "transporters … deck 4, rooms 1 and 2, six pads; 40,000 km" (`ship-systems.md:43`) | partial | `module/ship/ship_core.cpp:53` | pads and range absent |
| 18 | "the holodeck reactors … matrix incompatible" (`ship-systems.md:45`) | implemented | `module/ship/ship_core.cpp:5275` `JumpStartFromHolodeck` | |
| 19 | "the cheapest thing is shed first and the warp drive last of all" (`ship-systems.md:55`) | implemented | `module/ship/ship_core.cpp:36-64`; `TestBudgetShedSequence` | |
| 20 | "the five rungs exist in the code" (`ship-systems.md:69`) | implemented | `module/ship/ship_core.h:53-57` | |
| 21 | system model fields (`warp_core{…}`, `eps{sector→…}`, `deuterium{…}`) (`ship-systems.md:72`) | partial | `module/ship/ship_core.h:1973-1979` | fields renamed; no EPS-sector grid, no `deuterium.integrity` |
| 22 | "a deck's life support can be fed from one of the sources that survive the main grid" (`ship-systems.md:82`) | implemented | `module/ship/ship_core.cpp:2027`; `scripts/air-check.sh` | O2 |
| 23 | "losing the core is not losing the ship … the survival set" (`ship-systems.md:90`) | implemented | `module/ship/ship_core.cpp:2063,2087` | |
| 24 | "per-compartment `sealed`, `breached`, `force_field_level`" (`ship-systems.md:97`) | partial | `module/ship/ship_core.h:204,210-211` | no per-deck `sealed`/`breached` field |
| 25 | "a crew member walking to a post opens the door in front of it" (`ship-systems.md:99`) | implemented | `module/crew/g_crew.cpp:576`; `scripts/crew-door-check.sh` | |
| 26 | "per-deck atmosphere, gravity and temperature are ours to track" (`ship-systems.md:103`) | partial | `module/ship/ship_core.h:202-203` | no temperature field |
| 27 | "model it as the deck graph's edges, powered" (`ship-systems.md:107`) | partial | `module/ship/ship_core.cpp:55` | nothing gates the lift's links |
| 28 | "fourteen arrays with a power setting from stun to vaporize" (`ship-systems.md:114`) | partial | `module/ship/ship_core.h:237`; `ship_core.cpp:2758` | ladder built; the 14 arrays / 4 tubes are not |
| 29 | "sickbay on deck 5, three biobeds plus a surgical bed" (`ship-systems.md:120`) | implemented | `module/ship/ship_core.h:2276,2280` | |
| 30 | "per-deck `hull_integrity`, `pressure`, `eps`, `life_support`, plus ship-level `sif%`" (`ship-systems.md:133`) | partial | `module/ship/ship_core.h:202,204` | per-deck `eps`/`life_support` absent |
| 31 | "assigning a repair empties a station" (`ship-systems.md:139`) | contradicted | `module/ship/ship_core.cpp:1744-1746` | **C-12** |
| 32 | "at most twelve shots penetrate before the Borg adapt" (`ship-systems.md:149`) | contradicted | `module/ship/ship_core.cpp:3572` | **C-15** |
| 33 | "assimilation … five stages … a short reversal window" (`ship-systems.md:152`) | partial | `module/ship/ship_core.cpp:2608-2626,6357` | window built; no five *named* stages |
| 34 | "around twenty minutes of ship time" to organise (`ship-systems.md:157`) | superseded | `module/ship/ship_core.h:194-196` (120/240/360 s) | `docs/borg-incursion.md:235-243` |
| 35 | "ship-level failure is the Borg reaching the core" (`ship-systems.md:164`) | superseded | `module/ship/ship_core.cpp:6522` | **C-16**, `docs/endings.md:73` |
| 36 | "torpedo complement … 38, depleting" (`ship-systems.md:172`) | implemented | `module/ship/ship_core.h:244` | |
| 37 | "design each system with three failure states" (`failure-is-content.md:37`) | implemented | `module/ship/ship_core.h:93`; `ship_core.cpp:5424`; `TestSystemStates` | **C-22** is the stale line-144 sentence |
| 38 | "a system's chance of failing is a function of its condition, and this is general" (`failure-is-content.md:52`) | implemented | `module/ship/ship_core.cpp:5440,5448` | |
| 39 | "degradation sets the odds; stress sets the severity" (`failure-is-content.md:68`) | implemented | `module/ship/ship_core.cpp:5455`; `TestConditionOdds` | |
| 40 | "the pattern buffer is at sixty-two percent, and below forty I would not send anyone" (`failure-is-content.md:79`) | implemented | `module/ship/ship_core.cpp:2842`; `scripts/condition-check.sh` | |
| 41 | "it lets go visibly — the console arcs, sparks and burns" (`failure-is-content.md:59`) | implemented | `module/ship/ship_core.cpp:5393-5402`; `scripts/damage-check.sh` | |
| 42 | "mangled / cloned / merged" transporter outcomes (`failure-is-content.md:90`) | implemented | `module/ship/ship_core.cpp:2864-2920`; `TestTransporterAnomaly` | |
| 43 | "the warp core breach … reachable only by a chain of decisions" (`failure-is-content.md:113`) | implemented | `module/ship/ship_core.cpp:6493`; `TestCoreCascade` | |
| 44 | "it is a projection from *current capability*" (`navigation-counter.md:13`) | implemented | `module/ship/ship_core.cpp:4884` | |
| 45 | "the change since the last log entry" (`navigation-counter.md:16`) | implemented | `module/ship/ship_core.cpp:4910` | |
| 46 | "show both the *nominal* figure and the *current-capability* figure" (`navigation-counter.md:20`) | implemented | `module/ship/g_ship.cpp:6758`; `scripts/nav-check.sh` | |
| 47 | "any crew console, by query" (`navigation-counter.md:42`) | implemented | `module/ship/g_ship.cpp:6755`; `ui_lwh_engineering.cpp:443` | |
| 48 | "the counter is *recorded* periodically" (`navigation-counter.md:43`) | implemented | `module/ship/ship_core.cpp:6013-6017` | |
| 49 | "command sees the *forecasts*" (`navigation-counter.md:51`) | implemented | `module/ship/ship_core.cpp:4914`; `TestNavigation` | |
| 50 | "one merged map of the whole ship, joined by working turbolifts and Jefferies tubes" (`ship-programme.md:23`) | implemented | `scripts/build-ship.sh`; `scripts/s3-check.sh` | |
| 51 | "all ~141 are individuals … only those where the player is are embodied" (`ship-programme.md:25`) | implemented | `module/ship/ship_core.h:23`; `g_crew.cpp:2187` | |
| 52 | "every style: any crew member … in command … or Munro" (`ship-programme.md:29`) | implemented | `module/ship/ship_core.h:1787`; `g_ship.cpp:5408` | |
| 53 | "ironman is the game: one autosaved ship" (`ship-programme.md:30`) | implemented | `module/ship/g_ship.cpp:5334,5458`; `patches/0012` | |
| 54 | "an engine-independent library a multiplayer server can host unchanged" (`ship-programme.md:32`) | implemented | `module/ship/ship_core.h:6-7` (std headers only) | |
| 55 | "crew cannot open doors (found in G3)" (`ship-programme.md:76`) | contradicted | `module/crew/g_crew.cpp:258,1954` | **C-17** |
| 56 | "Borg asset replacement … an engine capability that does not exist yet" (`ship-programme.md:79`) | contradicted | `module/ship/g_ship.cpp:5154` `BorgAssets`; `scripts/borg-deck-check.sh` | stale; the swap is built **Disposition 2026-10-11:** corrected — `docs/ship-programme.md` now records the Borg asset swap as built. <!-- register-row id=D1-56 state=closed marker=symbol:module/ship/g_ship.cpp:BorgAssets --> |
| 57 | "four thousand teradynes per second" (`ship-systems.md:34`) | unverified | power is an invented EPS scale (`ship_core.h:10`) | the canon figure cannot appear |
| 58 | "343 m, 700,000 t, 257 rooms" (`ship-systems.md:38`) | unverified | not in `ship_core`; no check | |
| 59 | "47 million data channels … 47 spares" (`ship-systems.md:46`) | unverified | no code witness | |
| 60 | "astrometrics … as a data-quality modifier with a real cost when offline" (`ship-systems.md:117`) | unverified | `SYS_ASTROMETRICS` is a demand/band only (`ship_core.cpp:59,88`) | nothing reads it as a modifier |
| 61 | "the 256-byte target in the crew spec is a budget for exactly this record" (`ship-model.md:36`) | unverified | bounded by `MEMORY_MAX=8`, `CONDITION_MAX=3` | no 256-byte cap enforced |

### 2. Power, damage and the two budgets

`docs/power-assignment.md`, `docs/budget-squaring.md`, `docs/damage-and-budgets.md`,
`docs/lore-ledger.md`.

| # | claim (doc:line) | verdict | witness | note |
|---|---|---|---|---|
| 1 | `share`/`allocBy`, `SetAllocation`/`SetAllocationBy`, `PowerCommitted`/`PowerShortfall` (`power-assignment.md:24-26`) | implemented | `module/ship/ship_core.h:154-155`; `ship_core.cpp:2289,2304,2246,2253` | |
| 2 | `GrantBand`/`RevokeBand`, one holder per band, revoke immediate (`power-assignment.md:26-27`) | implemented | `module/ship/ship_core.cpp:2425,2444`; `TestRecommendationAndDelegation` | |
| 3 | the console shows every item; `scripts/power-check.sh` (`power-assignment.md:27-28`) | implemented | `module/ui/ui_lwh_engineering.cpp:409-441,318-322`; `scripts/power-check.sh` | console half; the check needs game data to run the rendered half |
| 4 | "the plant supplies a finite output, which the crystal's ceiling sets" (`power-assignment.md:36-43`) | implemented | `module/ship/ship_core.cpp:1907-1911,2183-2191`; `TestPartialAllocation` | |
| 5 | "nothing sheds itself … the ship tells the player by how much and waits" (`power-assignment.md:44-45`) | implemented | `module/ship/ship_core.cpp:2128-2129,2168-2178`; `TestOversubscriptionReported` | |
| 6 | "a physical failure removes capability. a policy decision never does" (`power-assignment.md:49-52`) | implemented | `module/ship/ship_core.cpp:2116-2119,2147-2162` | |
| 7 | "an automatic mode the player switches on … off by default" (`power-assignment.md:59`) | implemented | `module/ship/ship_core.h:2045` | |
| 8 | "refusing is recorded, and the officer remembers it" (`power-assignment.md:60-63`) | implemented | `module/ship/ship_core.cpp:2408-2422`; `MEM_OVERRULED` `ship_core.h:383` | |
| 9 | "never apply the ladder when a person has decided" (`power-assignment.md:72-81`) | implemented | `module/ship/ship_core.cpp:2134,2141`; `TestLadderOnlyInAutoMode` | |
| 10 | a meeting is where allocations are argued and set (`power-assignment.md:65-68`) | implemented | `module/ship/ship_core.h:1062,1226`; `ship_core.cpp:8278` | state level; the rendered meeting not observed |
| 11 | "total committed against total available … who set each commitment" (`power-assignment.md:87-94`) | implemented | `module/ship/g_ship.cpp:731-737`; `ship_core.cpp:2340` | BUYS is a number |
| 12 | "save format 51" (`power-assignment.md:29`) | superseded | `SAVE_VERSION = 60` | historical **Corrected 2026-10-11:** the header is the owner and it reads 60 on this ref; this statement asserted the value at the time it was written. |
| 13 | "the five added systems and a demand of 1,730" (`budget-squaring.md:9`) | implemented | `module/ship/ship_core.cpp:53-63`; `SPECS` sums to 1730 | |
| 14 | "warp core 1,400 … impulse 250, auxiliary 90, batteries 60, fresh 1,800" (`budget-squaring.md:9-11`) | implemented | `module/ship/ship_core.cpp:226-231`; `PowerCapacityFresh()==1800` | |
| 15 | "the ceiling-scaled warp core; … the torpedo complement at 38" (`budget-squaring.md:11-12`) | implemented | `module/ship/ship_core.cpp:1910,3712,3719`; `ship_core.h:244` | |
| 16 | "the number set below is the one in the code" (`budget-squaring.md:3`) | contradicted | Part two (`:106,136`) and Part 4a (`:207-211`) still carry 1,000/1,500/1,540 | **C-23** in the top section **Disposition 2026-10-11:** corrected — `docs/budget-squaring.md`'s header, Parts two, three and four are now marked, so the implemented set is unambiguous. <!-- register-row id=D2-16 state=closed marker=symbol:module/ship/ship_core.h:SYS_CARGO_HANDLING --> |
| 17 | "covered with sixty to spare" (`budget-squaring.md:147`) | superseded | `SOURCES` now 60/90/250; ten spare | Part five |
| 18 | "lose the warp core and 500 EPS remain" (`budget-squaring.md:149-151`) | superseded | `module/ship/ship_core.cpp:1914-1921`; coreless slope 400 | |
| 19 | "at priority 9 … the warp drive is the largest single load" (`budget-squaring.md:153-157`) | superseded | `module/ship/ship_core.cpp:47` warp drive `priority 4` | shed last |
| 20 | "each recomposition costs her a fifteenth of her plant" (`budget-squaring.md:328`) | contradicted | `module/ship/ship_core.h:906` `0.15f` | **C-5** |
| 21 | "start at 38 … build the two canon moments: eleven, and six" (`budget-squaring.md:267`) | partial | `module/ship/ship_core.h:244` | start built; the two moments have no witness |
| 22 | "the ship's 22 posts must be coverable by a hundred people" (`budget-squaring.md:271`) | implemented | `module/ship/ship_core.cpp:109,119`; `TestHundredCrew` | |
| 23 | "the holodeck matrix is a trap" (`budget-squaring.md:275-276`) | implemented | `module/ship/ship_core.cpp:5272-5298`; `TestHolodeckTrap` | |
| 24 | `RECOMPOSITE_CEILING_DROP`, `JUMP_DILITHIUM`, `MIN_WARP_DILITHIUM` (`budget-squaring.md:297-299`) | implemented | `module/ship/ship_core.h:903-906` | |
| 25 | "`crystalCeiling` should also scale the warp core's output" (`budget-squaring.md:309-313`) | implemented | `module/ship/ship_core.cpp:1910` | |
| 26 | Part six shed table (nothing / four / nine / fourteen) (`budget-squaring.md:372-378`) | implemented | `TestBudgetShedSequence` | arithmetic exact |
| 27 | "the players and the crew decide … nothing sheds itself" (`damage-and-budgets.md:12-18`) | implemented | `module/ship/ship_core.cpp:2074-2076,2352` | |
| 28 | "functionality … integrity per system per compartment" (`damage-and-budgets.md:19-20`) | implemented | `module/ship/ship_core.h:138,204` | |
| 29 | "a destroyed array is a firing arc lost for the next three engagements" (`damage-and-budgets.md:28-30`) | absent | `SYS_PHASERS` is one `health` | no per-array/arc model |
| 30 | core cascade "coolant … injector assembly stressed … containment … BREACH IMMINENT" (`damage-and-budgets.md:39-47`) | partial | `module/ship/ship_core.cpp:6493-6524` | the injector rung is missing |
| 31 | "every arrow is an interrupt point … four chances" (`damage-and-budgets.md:49-53`) | partial | `ShutDownCore`/`EjectCore`/`RestoreCoolant` | no injector bypass |
| 32 | "the ship carries a written list of what she has given up" (`damage-and-budgets.md:63-68`) | implemented | `module/ship/ship_core.cpp:7057,7086`; `LOSS_MAX=24`; `TestWrittenOffList` | |
| 33 | "damage from that engagement persists across a deck change, a save, and a reload" (`damage-and-budgets.md:98-99`) | implemented | `Pack`/`Unpack`; `TestSave` | |
| 34 | "photon torpedoes 38, not replaceable" (`lore-ledger.md:19`) | implemented | `module/ship/ship_core.h:244`; `ship_core.cpp:3180-3183` | |
| 35 | "sickbay 6 under treatment at once; 12 hours each" (`lore-ledger.md:209`) | contradicted | `SICKBAY_BEDS = 4` (`ship_core.h:2276`) | **C-3** |
| 36 | "a sector: 40% empty, 30% hostile, 20% derelict, 10% Borg" (`lore-ledger.md:242`) | contradicted | `ship_core.cpp:2686-2689` (`roll % 15`) | **C-4** |
| 37 | shedding order list "…torpedoes, phasers, then sensors, impulse, and the warp drive last" (`lore-ledger.md:183`) | partial | `module/ship/ship_core.cpp:42` shields `priority 7` | shields omitted from the list |
| 38 | "with the warp core gone the ship runs only … 390 of the 400 remaining" (`lore-ledger.md:184`) | implemented | `module/ship/ship_core.cpp:2063,1914-1921` | |
| 39 | burn rates, batteries 60/3 h, atmosphere 5 min/12 h/1 h, gravity 6 h/1 h (`lore-ledger.md:190-194`) | implemented | `module/ship/ship_core.h:233,235-239`; `ship_core.cpp:226-231` | |
| 40 | job queue "bounded at 24"; "an undermanned station fails after 10 days, losing 40%" (`lore-ledger.md:332`) | implemented | `module/ship/ship_core.h:1691,2270`; `ship_core.cpp:2205` | |
| 41 | dilithium constants, ~3,000 ly, 40 jumps, recomposite 0.35, ceiling −0.15 (`lore-ledger.md:384-390`) | implemented | `module/ship/ship_core.h:902-909` | |
| 42 | "nothing completes while paused; repair orders … execute on resume" (`damage-and-budgets.md:91`) | unverified | host/pause clock is outside `module/ship` | no witness reachable |

### 3. The crew and their work

`docs/crew-manifest.md`, `docs/crew-roster.md`, `docs/crew-work.md`,
`docs/character-attributes.md`, `docs/character-derivation.md`, `docs/morale.md`,
`docs/affinities-and-allegiance.md`, `docs/g3-reactive-crew.md`.

| # | claim (doc:line) | verdict | witness | note |
|---|---|---|---|---|
| 1 | rank / post / department are three separate fields (`crew-manifest.md:23-32`) | implemented | `module/ship/ship_core.h:499-502` | |
| 2 | "the Doctor is a post without a rank … holds no Starfleet commission" (`crew-manifest.md:29-32`) | contradicted | `module/ship/ship_core.cpp:1093` gives rank 3 | **C-3** family; the code is the likely wrong side **Disposition 2026-10-11:** **handed to the code list** — the register's own reading: the code is the likely wrong side (the Doctor is given rank 3). **Disposition 2026-10-12 (code pass): fixed** — the Doctor now carries rank 0, the record's no-commission rank (as Neelix the civilian and Seven the consultant already do), so rank says no commission and the department says sickbay, not the conn. What is left: the *situational* authority the document also gives him (sickbay, quarantine) — row 3.14's `absent` field, named in the evidence. Proven by `TestDoctorNoCommission`; falsified by restoring rank 3, which fails it. <!-- register-row id=D3-2 state=closed marker=symbol:tests/ship/test_ship_core.cpp:TestDoctorNoCommission --> |
| 3 | nine departments whose sizes add to 141 (`crew-manifest.md:52-71`) | contradicted | `DEPT_SIZE[5] = {20,50,25,30,16}` (`ship_core.cpp:241`) | fork: the nine-way split is not in code **Disposition 2026-10-11:** **handed to the code list** — a fork: the nine-way department split is a proposal, not the code's five-way `DEPT_SIZE`. **Disposition 2026-10-12 (code pass): handed back** — a fork. The document itself calls the nine numbers *"ours -- a proposal, to be tuned"*; adopting them is a content decision (and moves five departments to nine across the roster, bills and deck map), not a contradiction to repair. The code's five-way `DEPT_SIZE` is left alone. |
| 4 | three 8-hour watches Alpha/Beta/Gamma (`crew-manifest.md:75-76`) | implemented | `module/ship/ship_core.h:25,501`; `ScheduledActivity` | hours exact |
| 5 | "one post and a set of bill assignments" (`crew-manifest.md:105-107`) | implemented | `module/ship/ship_core.h:506-507`; `DeriveBills` `ship_core.cpp:6095` | |
| 6 | battle stations; "almost nobody's battle station is their normal post" (`crew-manifest.md:109-112`) | implemented | `BattleRoleFor` `ship_core.cpp:6083`; `UpdateCrew:1558-1566` | |
| 7 | emergency bills fire/decompression/abandon/intruder (`crew-manifest.md:113-114`) | implemented | `module/ship/ship_core.h:485-492`; `CallEmergency:6198` | |
| 8 | damage control "reports to the engineer of the watch, not the senior officer present" (`crew-manifest.md:115-116`) | partial | `EngineerOfTheWatch:6111` | computable; no reporting rule wired |
| 9 | "the holes are the game" (`crew-manifest.md:118-121`) | implemented | `BillHoles:6278`; `scripts/bills-check.sh` | |
| 10 | ~80 stand watches (27/watch) / 61 day (`crew-manifest.md:126-134`) | unverified | no code asserts these counts | design projection |
| 11 | the Hazard Team is a standing unit, fourteen names, two squads (`crew-manifest.md:160-218`) | partial | `NAMED[]` `ship_core.cpp:1086-1106` | no unit construct; 9 of 14 present |
| 12 | admin chain CO→XO→heads; boarding chain (`crew-manifest.md:221-254`) | partial | `MayOrder:9930` (rank/command only) | no XO/head/boarding chain |
| 13 | OOD authority "against senior officers, with the exception of the captain" (`crew-manifest.md:225-228`) | contradicted | `OodMayOrder:6167` not used by `DispatchOrder` | **C-11** |
| 14 | field commissions, civilians with posts (`crew-manifest.md:237-244`) | absent | `CrewMember` has no provisional-rank flag | |
| 15 | seven skills rated 0-5 (`crew-roster.md:30`) | implemented | `module/ship/ship_core.h:280-285`; `ship_core.cpp:455-459` | |
| 16 | traits "few, legible, behavioural rather than percentile" (`crew-roster.md:37-39`) | implemented | `module/ship/ship_core.h:291-295` | |
| 17 | drives in three parts (`crew-roster.md:41-46`) | implemented | `module/ship/ship_core.h:299-310` | |
| 18 | "bonds … stored bidirectionally" (`crew-roster.md:51-52`) | superseded | `Holdings`/`Bond` fold (`ship_core.cpp:759,4772`) | the C2 ruling |
| 19 | "the static record … lives in data files in the repository" (`crew-roster.md:88-92`) | superseded | `DeriveCharacter` `ship_core.cpp:1188,1210,9216` | seed-derived, not stored (`character-derivation.md:30-35`) |
| 20 | "25 bytes per crew member" (`crew-roster.md:94`) | contradicted | `SAVE_MEMBER_BYTES=20` (`crew_core.h:220`); `g3-reactive-crew.md:157` says 20 | **C-4** family; also `docs/gates.md:197` **Disposition 2026-10-11:** **the evidence will not decide; handed to the code list** — 25 bytes is a *measurement* (`docs/evidence/g3-reactive-crew-measured.md`) and 20 is the packed `SAVE_MEMBER_BYTES`; `docs/evidence/the-number-owners.md` already recorded this as the owner's to reconcile. **Disagreement with this row's `contradicted` verdict**: the document matches its measurement, so correcting it to 20 would replace a measured number with a different one. **RESOLVED 2026-10-11 by the steward, on the owner's instruction:** **all three figures are right and measure different scopes.** The **crew blob** is `20 bytes per member plus a 16-byte header` against the configured `saveBytesPerNpc = 256` (`crew_core.h:144,219-220`) — exact, by construction, and validated on load, and already stated correctly in `docs/g3-reactive-crew.md`, which owns the format. The **layer** measures **25 bytes per member** — the blob's 20 plus about five bytes held elsewhere in the layer. The **save as a whole** grew **53,584 bytes for six crew** (≈ 9 KB per member) because almost everything that scales with a person is not the blob. `docs/crew-roster.md` is corrected to carry all three, labelled, with each figure's owner named — the error was one dropped qualifier: *of layer state*, which turned a layer measurement into a claim about the save. The paragraph's own argument survives and is stronger on the honest figure. <!-- register-row id=3.20 state=closed marker=file:docs/crew-roster.md --> |
| 21 | a collision check on desire/allegiance/trait, failing the build (`crew-roster.md:105-107`) | absent | no such check | |
| 22 | per-character `provenance` field (`crew-roster.md:139-141`) | absent | `CrewMember` has none | |
| 23 | job kind "maintenance, repair, build, reclaim" (`crew-work.md:20`) | contradicted | `JobKind` enum `ship_core.h:1689` | no maintenance; seal/secure/evacuate added **Disposition 2026-10-11:** **handed to the code list** — a fork: the document's four job kinds against the code's six. **Disposition 2026-10-12 (code pass): handed back** — a fork. The code's six (`ship_core.h:1692`: `JOB_REPAIR, JOB_SEAL, JOB_RECLAIM, JOB_BUILD, JOB_SECURE, JOB_EVACUATE`) are the four the document names plus two the orders pass appended; the document has no *maintenance* kind, and adding or removing kinds changes the queue's contract. The owner rules the vocabulary; the code is left alone. |
| 24 | job carries required skill, crew-hours, state, blocking (`crew-work.md:22-29`) | partial | `Job{kind,target,progress,priority}` `ship_core.h:1693-1698` | the named fields are absent |
| 25 | "department heads queue their department's work" (`crew-work.md:69-75`) | partial | `SetJobPriority:4148` command-only | |
| 26 | measure backlog-by-age, crew-hours/kind/watch, post-vacancy hours (`crew-work.md:77-81`) | unverified | `UpdateJobs:7101` exists; none produced | |
| 27 | four kinds: skills / traits / conditions / state (`character-attributes.md:27-31`) | implemented | `module/ship/ship_core.h:280-347,537-548` | kept apart |
| 28 | condition = id, source, valence, magnitude, onset, clears, visibility (`character-attributes.md:35-43`) | implemented | `module/ship/ship_core.h:339-347`; `AddCondition:709` | |
| 29 | "two or three conditions at a time, never a soup" (`character-attributes.md:55-57`) | implemented | `CONDITION_MAX=3`; eviction `ship_core.cpp:718-728` | |
| 30 | effective skill / reliability / capacity / holding derivation (`character-attributes.md:63-69`) | partial | `EffectiveSkill:795`, `Holdings:759` | reliability/capacity not functions |
| 31 | the four kinds + morale reach the failure roll's odds, each by its own route (`character-attributes.md:78-90`) | implemented | `WorkFactors:5532`; `scripts/character-check.sh` | |
| 32 | capability is two-sided (Betazoid) (`character-attributes.md:86,108-109`) | implemented | `module/ship/ship_core.cpp:5615-5622` | |
| 33 | species are capabilities/needs/susceptibilities, never bonuses (`character-attributes.md:127-137`) | implemented | `module/ship/ship_core.h:359-368`; `TestSpeciesAreCapabilitiesNotBonuses` | |
| 34 | species susceptibility conditions (pon farr, emitter charge) (`character-attributes.md:141-159`) | partial | `SpeciesProne` used only for `COND_MEDITATION_DUE` | others are vocabulary only |
| 35 | "≈400 B/head ≈ 60 KB, ~16× the G3 budget" (`character-attributes.md:211-214`) | unverified | no measurement | rests on the stale 25 B figure |
| 36 | `DeriveCharacter` pure, called from `BuildRoster` and again on load (`character-attributes.md:243-245`) | implemented | `module/ship/ship_core.cpp:1188,1210,9216` | static half not saved |
| 37 | holdings is a fold, not a field (`character-derivation.md:30-35`) | implemented | `Holdings:759`; `scripts/holdings-check.sh` | falsified independently below |
| 38 | skills = 1 + rank lift + dept lead + seed%3, capped 5 (`character-derivation.md:47-52`) | implemented | `module/ship/ship_core.cpp:455-459` | |
| 39 | "`DeriveAppearance` leaves a named record unset (head 0xFFFF)" (`character-derivation.md:97-98`) | partial | `module/ship/ship_core.cpp:676-707` | guards on `type.empty()`, not "named" |
| 40 | condition sources/clears table (`character-derivation.md:137-142`) | implemented | `module/ship/ship_core.cpp:1343-1368` | source strings match |
| 41 | `SpeciesRecord` words + prone ids + restNeedHours/needsFood/sleeps (`character-derivation.md:150-163`) | implemented | `module/ship/ship_core.h:359-368` | `sleeps` declared, never read |
| 42 | morale = 0.40·heart + 0.35·outlook + 0.25·holdings; bands; reason names the component (`character-derivation.md:197-226`) | implemented | `module/ship/ship_core.cpp:768-793`; `TestMoraleIsReadFromThree` | weights exact |
| 43 | `EffectiveSkill` "is what decides whether a repair is quick, slow, or dangerous" (`character-derivation.md:232-246`) | contradicted | `WorkFactors:5542-5548` uses raw skills | **C-23** |
| 44 | manner is a read; precedence `hurt > breathless > … > band`, not stored (`character-derivation.md:264-291`) | implemented | `MannerFor:990`, `EmitManner:1052`; `scripts/manner-check.sh` | |
| 45 | no morale meter; three things derived, never stored (`morale.md:8-20`) | implemented | `Morale:768` | holdings folded; deficit/outlook stored state |
| 46 | department/watch morale aggregates exist and are derived (`morale.md:19-20`) | absent | `Morale` is per-person only | |
| 47 | performance at post measurably degrades (`morale.md:41-43`) | partial | `ship_core.cpp:5606-5610` | ack latency not tied to morale |
| 48 | compliance: delayed / done badly / refused on low outlook (`morale.md:44-46,88`) | partial | `r.resented:10224` | refusal is authority-based |
| 49 | personnel screen: bands + top factor + "what would help" (`morale.md:60-63`) | partial | `MoraleReason:775-793` | "what would help" not produced |
| 50 | "the save format is 57" (`morale.md:141-143`) | contradicted | `SAVE_VERSION = 60` | **C-2** **Corrected 2026-10-11:** the header is the owner and it reads 60 on this ref; this statement asserted the value at the time it was written. |
| 51 | holdings fold over marks, orphaned marks weigh 0 (`morale.md:113-126`) | implemented | `module/ship/ship_core.cpp:759-766`; `TestHoldingsIsAFold`, `TestPurgeMovesRegard` | |
| 52 | `likes[]`/`dislikes[]`/`pursuits[]`, one store (`affinities-and-allegiance.md:18-41`) | absent | `CrewMember` has only `faction`, `resentment` | |
| 53 | allegiance weights how an event lands and whether an order is carried (`affinities-and-allegiance.md:66-76`) | partial | `ship_core.cpp:5154` | faction is the only axis |
| 54 | preference operates at idle choosing a duty post (`affinities-and-allegiance.md:78-83`) | partial | `AssignPosts` `crew_core.cpp:233` | nearest-pair, not preference |
| 55 | arbitration precedence 1-5; interruption is a pause (`g3-reactive-crew.md:27-41`) | implemented | `module/crew/crew_core.h:28-34`; `TestArbitrationOrder`, `TestInterruptionIsAPause` | |
| 56 | 256 B/NPC budget, measured 20 B + 16-byte header (`g3-reactive-crew.md:153-157`) | implemented | `module/crew/crew_core.h:144,219-220`; `TestSaveBudget` | |
| 57 | the G3 acceptance measured (`g3-reactive-crew.md:221-235`) | implemented | `Judge` `crew_core.cpp:367`; `scripts/g3-measure.sh` | needs game data to re-run |

### 4. The record, the log and the memory

`docs/the-record-and-the-log.md`, `docs/memory-and-consequence.md`, `docs/memory-boundaries.md`.

| # | claim (doc:line) | verdict | witness | note |
|---|---|---|---|---|
| 1 | the record / the log / the memory are three different stores (`the-record-and-the-log.md:11`) | implemented | `LogEntry:924`, `personalLog:2070`, `memories:385`; `TestTheTwoLogs` | |
| 2 | "nothing in the ship model may make a decision by consulting the log" (`the-record-and-the-log.md:45`) | implemented | `DraftReport` reads `c.memories` `ship_core.cpp:5041-5046`; log readers are display/save only | |
| 3 | "the log is write-only from the simulation's point of view" (`the-record-and-the-log.md:47`) | implemented | `LogEvent:6920`; no reader feeds a branch | |
| 4 | "a meeting brief is built per participant, from that person's marks and the log, and from nothing else" (`the-record-and-the-log.md:56`) | implemented | `AddParticipant:8014-8053`; `TestMeetingBriefPerParticipant` | also carries instrument state |
| 5 | "a post reads its own scope, command reads all, the player can search" (`the-record-and-the-log.md:64`) | implemented | `ReadOfficialLog:6979`; `LogForParticipant:8006` | |
| 6 | the personal log is private (`the-record-and-the-log.md:65`) | implemented | `PersonalVisibleTo:6964`; `scripts/personal-log-check.sh` | |
| 7 | the month report is drafted honestly from the record (`the-record-and-the-log.md:77`) | implemented | `DraftReport:5005`; `TestTheTwoLogs` | |
| 8 | "edited by the player. strike a line, soften a number, add a claim" (`the-record-and-the-log.md:78`) | implemented | `StrikeReportLine:5076`, `SoftenReportLine:5094`, `AddReportLine:5112` | |
| 9 | "the record keeps the diff … the crew can only ever see the published version" (`the-record-and-the-log.md:80`) | implemented | `ReportLine.draft:424`; `ReportDiff:5190`; `SignReport:5138-5139` | |
| 10 | "each department head signs their own section, the captain signs the whole" (`the-record-and-the-log.md:87`) | partial | `MonthReport.department:441`; only draft site is `DEPT_COUNT` | no per-department signing surface |
| 11 | provenance table: saw / told / rumour / log (`the-record-and-the-log.md:95-103`) | partial | `HoldsSaw` accepts only `MEM_SAW:4970` | told/rumour/log never used as witnesses |
| 12 | "the mark goes in privately, trust falls, and the public challenge costs the challenger" (`the-record-and-the-log.md:107`) | partial | `MEM_LIE` `ship_core.cpp:5157-5163` | no "public challenge" mechanism |
| 13 | "the toll is paid downward" (`the-record-and-the-log.md:113`) | implemented | `if (w.rank >= sig.rank) continue` `ship_core.cpp:5150`; test:2002 | |
| 14 | "divergent allegiance … amplify … alignment … suppress" (`the-record-and-the-log.md:117`) | implemented | factor `ship_core.cpp:5154-5155` | magnitudes invented [inv] |
| 15 | "it orphans marks" (the purge) (`the-record-and-the-log.md:133`) | implemented | `PurgeLogs:5205-5213`; test:2013 | |
| 16 | "it costs the ending. the log is the last frame" (`the-record-and-the-log.md:139`) | absent | no ending mechanism in `ship_core` | owned by `docs/endings.md` |
| 17 | assimilation gains "the personal log … and their access levels" (`the-record-and-the-log.md:150`) | implemented | `CollectiveLog:7015`; `CollectiveOperates:7033`; `TestTheAssimilation` | |
| 18 | "the log is taken from the mind, not from the disk" (`the-record-and-the-log.md:157`) | implemented | `CollectiveLog:7018-7019`; purge cannot end it | |
| 19 | "speak in their voice, using their own words" (`the-record-and-the-log.md:173`) | partial | `CollectiveVoice:8654`; `LineVoice:8733` | no code builds a line from `CollectiveLog` |
| 20 | the fiction-level memory store "must never be implemented as a real memory-store read" (`the-record-and-the-log.md:198-205`) | unverified | no operator-store/vault code exists | true by absence, which is not evidence |
| 21 | mark = event id · source · time · valence · salience (`memory-and-consequence.md:44`) | implemented | `Memory:385-393` | |
| 22 | sources "saw it myself / told / rumour / read it in the log" (`memory-and-consequence.md:47`) | partial | `MemorySource:381` | `MEM_RUMOUR` declared, never produced |
| 23 | "rumours arrive distorted, and the distortion is attributable" (`memory-and-consequence.md:55`) | absent | no `MEM_RUMOUR` writer | **C-13** |
| 24 | "bonds are remembered valence with a person attached" (`memory-and-consequence.md:57`) | implemented | `Bond:4772`; test:1894 | |
| 25 | "salience decays unless reinforced" (`memory-and-consequence.md:60`) | implemented | `Remember:4734-4735`; test:1906 | |
| 26 | storage bounded, evicting the least salient; eight marks (`memory-and-consequence.md:62`) | implemented | `MEMORY_MAX=8:384`; eviction:4737-4743 | "27 KB" not measured |
| 27 | "'what does she know?' is a query" (`memory-and-consequence.md:65`) | implemented | `Recall:4754`; `ship recall:6509` | |
| 28 | "the log can be searched — the crew's memory cannot, except by asking them" (`memory-and-consequence.md:76`) | implemented | `ReadOfficialLog:6979`; `Recall` | |
| 29 | character memory gets a provenance; a person not told must not know (`memory-boundaries.md:44`) | implemented | test:1884-1886 | |
| 30 | "crew memory is records, not retrieval … belongs in the save" (`memory-boundaries.md:47`) | implemented | serialized `ship_core.cpp:8955-8961` | |
| 31 | a test that no game-side path can write to an operator store (`memory-boundaries.md:59`) | absent | no such test; no vault | |
| 32 | a replay check: same save and inputs produce the same memories (`memory-boundaries.md:62`) | implemented | `Pack`/`Unpack` byte-equal; test:1922 | |

### 5. Orders, access and authority

`docs/orders.md`, `docs/access-and-authority.md`.

| # | claim (doc:line) | verdict | witness | note |
|---|---|---|---|---|
| 1 | the order verbs exist as hooks (`orders.md:24`) | implemented | `module/ship/ship_core.h:1704,2831-2834`; `g_ship.cpp:6244-6247` | |
| 2 | "an order's effect is a job" (`orders.md:26`) | implemented | `Job:1693`; `ApplyOrder:10137`; test:3783 | |
| 3 | "who may order whom is already answerable" (`orders.md:28`) | implemented | `MayOrder:9930`; `OrderPermitted:9962`; test:3773 | the OOD exception is not wired (row 13 above, **C-11**) |
| 4 | "a person takes an order according to their traits, conditions, drives, morale and the marks" (`orders.md:31`) | implemented | `ship_core.cpp:10224-10238` | |
| 5 | "the enumerated-API discipline … guarded in both directions" (`orders.md:37`) | implemented | `console_api.def`; `scripts/api-check.sh`; `order_set.def` | |
| 6 | "a one-recipient brief … choosing one pushes a job" (`orders.md:42`) | implemented | `BuildOrderBrief:10099`; `DispatchOrder:10178` | |
| 7 | "the common set is derived per recipient, not written out" (`orders.md:46`) | implemented | `DeriveOrders:9969`; `scripts/order-check.sh` | |
| 8 | "a bespoke order takes the free-text path … refused in character" (`orders.md:51`) | implemented | `BespokeOrder:10315`; `ClassifyNovelOrder:10295` | |
| 9 | "an order that costs them something may write a mark" (`orders.md:54`) | implemented | `MEM_ORDER` `ship_core.cpp:10238` | |
| 10 | "the simulation decides; the model speaks … a save must replay identically" (`orders.md:59`) | implemented | `DispatchOrder`; `scripts/order-check.sh` | |
| 11 | Q3 "where it lands is not yet decided" (`orders.md:71`) | superseded | `MEM_ORDER` written `ship_core.cpp:10238` | the code answered it |
| 12 | "access is technical … authority is social" (`access-and-authority.md:12`) | implemented | `MayOperate:3824`; test:3657-3660 | |
| 13 | scope ladder ship-wide / department / post / read-only / none (`access-and-authority.md:24-30`) | partial | rank≥4 ship-wide `:3818`; `DepartmentOperates:3801` | no read-only level |
| 14 | four sources: post, credentials, delegation, override (`access-and-authority.md:40`) | implemented | `RecordOperates:3816`; `Delegate:3866`; `BeginOverride:3930` | |
| 15 | dead crew: field promotion, override, EMH (`access-and-authority.md:48`) | partial | `AssumeCommand:2719`; `BeginOverride:3930` | EMH holding "the keys" not built |
| 16 | "a locked control is visible and named" (`access-and-authority.md:60`) | implemented | `AccessRefusal:3992`; `scripts/clearance-check.sh` | |
| 17 | "a compromised system does not obey rank" (`access-and-authority.md:74`) | implemented | `Hijacked`; test:3744-3750 | |
| 18 | dead credentials retained but using them reported, not built (`access-and-authority.md:76`) | implemented | `KillCrew:4306-4316` keeps them; no use path | matches the withdrawal **W1** |
| 19 | "a relieved captain's credentials stop working" (`access-and-authority.md:83-84`) | absent | no relieve/demote path | |
| 20 | remote call-up and the lock-out as a first-class act (`access-and-authority.md:96-109`) | implemented | `MayCallUp:4006`; `LockOut:4018`; `MEM_LOCKOUT:4032`; test:3730 | |
| 21 | "remote command is access, not authority" (`access-and-authority.md:110`) | implemented | `MayCallUp`; `Hijacked` guard `g_ship.cpp:5703` | |
| 22 | "reading and operating are different privileges" (`access-and-authority.md:123`) | implemented | `OperatedFrom:190`; `StationReads:205`; `scripts/screens-check.sh` | |
| 23 | "the station shapes the console; the person filters it" (`access-and-authority.md:141`) | implemented | `StationReads:205`; `MayCallUp:4006`; `g_shipTest 68` | |
| 24 | delegation default eight hours, revoked, log names who turned it off (`access-and-authority.md:173`) | implemented | `DELEGATION_SHIFT_HOURS:2627`; `RevokeDelegation:3893`; test:3665 | |
| 25 | override "45 minutes against 15 … twenty minutes", logged, small morale cost (`access-and-authority.md:177`) | implemented | `ship_core.h:2641-2643`; `UpdateAccess:5963-5988`; test:3691 | |
| 26 | "a sliced system refuses everyone, including the captain and an override" (`access-and-authority.md:189`) | implemented | `Hijacked`; test:3744-3750 | |

### 6. The rooms, the screens and the meetings

`docs/staff-meetings.md`, `docs/programme-meetings-and-voice.md`, `docs/ship-master-map.md`,
`docs/hook-register.md`, `docs/walkthrough.md`.

| # | claim (doc:line) | verdict | witness | note |
|---|---|---|---|---|
| 1 | "a security matter meets with the security chief in security" (`staff-meetings.md:14`) | implemented | `module/ship/meeting_rooms.def:32,41,44`; `MeetingRoomFor:7649` | keyed by kind + subject |
| 2 | "a meeting is not bound to one today … and no place" (`staff-meetings.md:20-24`) | contradicted | `MeetingBrief.room` `ship_core.h:1302`; `BuildBrief:8147-8148` | the correction paragraph is stale (place landed 2026-10-10) **Disposition 2026-10-11:** corrected — `docs/staff-meetings.md` now records the room as landed 2026-10-10 (save 55). <!-- register-row id=D6-2 state=closed marker=symbol:module/ship/ship_core.h:MeetingBrief --> |
| 3 | the brief carries kind, trigger, decision, participants, options, instruments, systems, digest (`staff-meetings.md:22`) | implemented | `module/ship/ship_core.h:1299-1319` | plus subject and room |
| 4 | "a background worker … hands that brief to a local model and gets back a meeting script" (`staff-meetings.md:32-34`) | implemented | `tools/meetings/worker.py` `generate`, `build_prompt` | external, off the render loop |
| 5 | "no model is called and no latency exists, because every branch was written before" (`staff-meetings.md:37`) | implemented | `module/ui/ui_lwh_meeting.cpp:357,395-416` | |
| 6 | novel answer "written into the log as text … replay reads the log" (`staff-meetings.md:51-53`) | partial | `ship_core.cpp:8700,8713` | fast path does not store the generated line |
| 7 | "a pre-flight validator … rejects and regenerates" (`staff-meetings.md:56-58`) | implemented | `tools/meetings/worker.py:135,351` | |
| 8 | "embedding similarity against a small always-resident local embedding model" (`staff-meetings.md:84`) | implemented | `tools/meetings/worker.py:61,75` | model is external (ollama) |
| 9 | "the model cannot alter state" — it selects from the enumerated set (`staff-meetings.md:97-99`) | implemented | `ClassifyNovelInput` `const Ship&` `ship_core.h:1366`; `ApplyMeetingOutcome:1675` | |
| 10 | "a `VOICE` affordance is drawn on that pill from the start" (`staff-meetings.md:136`) | implemented | `module/ui/ui_lwh_meeting.cpp:229` | |
| 11 | overlay "bottom-anchored, roughly the lower quarter of the frame" (`staff-meetings.md:133-135`) | implemented | `module/ui/ui_lwh_meeting.cpp:32,372` | measured 30% |
| 12 | "every NPC line is rendered to audio ahead of time" (`staff-meetings.md:148-151`) | partial | `PlanMeetingAudio:8756` queues; no player | `programme-meetings-and-voice.md:29` says the player remains |
| 13 | "voices are cast once per character and held for the campaign"; durations pace the pills (`staff-meetings.md:155-158`) | implemented | `CastVoice:1620`; `MeetingLineSeconds:1632` | |
| 14 | audio "cached beside the save and pruned with it" (`staff-meetings.md:160-162`) | implemented | `PruneVoiceCache:8574`; `VoiceRender:1568-1589` | |
| 15 | cues: "one plays while the model works", slow model → deferral (`staff-meetings.md:166-176`) | implemented | `CueId:1527-1538`; `SubmitNovelAnswer:8682,8705-8713` | |
| 16 | "no microphone is opened by the engine" (`staff-meetings.md:192-196`) | implemented | `console_api.def:56`; `scripts/voice-in-check.sh` | boundary is an audio file |
| 17 | "an unrecognised input gets a diegetic refusal" (`staff-meetings.md:207-210`) | implemented | `ComputerRefusal:1394,1404` | |
| 18 | the computer "could even be an MCP server" (`staff-meetings.md:219-233`) | absent | no MCP server in `module/` or `tools/` | |
| 19 | "the model never calls a tool" (`staff-meetings.md:242-248`) | implemented | `GrantBand`/`RevokeBand`/`SetPowerAuto` declared and dispatched | |
| 20 | "the repository contains no game assets and no generated voice audio" (`staff-meetings.md:314-324`) | implemented | `.gitignore:40-41`; no `*.wav` tracked | |
| 21 | "one player per source, a named owner per track, one retirement rule per reply" (`programme-meetings-and-voice.md:43-45`) | implemented | `TrackOwner:1494`; `VoiceWrite:1516`; `VoiceRetire:1520` | |
| 22 | "every meeting generates a brief, not just the dramatic ones" (`programme-meetings-and-voice.md:63-68`) | implemented | `EmitDueMeetings:8205`, called at `:6024` | |
| 23 | "ten of fifteen decks ship as maps … all five absent decks are built" (`ship-master-map.md:29-36`) | implemented | `tools/shipmap/dressdeck{06,07,12,13,14}.py`; `scripts/deck*-check.sh` | map sources not in repo |
| 24 | systems map decks: power 11, life support 12/13, computer 10/7, sensors 8, medical 5, transporters 4, shuttles 10 (`ship-master-map.md:50-57`) | implemented | `SPECS` deck column `ship_core.cpp:36-64` | matches incl. fore tubes 10 / aft 4 |
| 25 | "139 edges"; "`deck04` reaches all eight holodeck programmes" (`ship-master-map.md:41-44`) | unverified | map sources absent | needs the `.map` sources |
| 26 | `hooks-check.sh` "re-derives the surface and compares it" (`hook-register.md:31-34`) | implemented | `tools/hooks/check_register.py` → 479 functions, 479 hooks, PASS | |
| 27 | "the tables above are now 449 hooks" (`hook-register.md:673`) | contradicted | header has 479; register rows 479 | under-count by 30 **Disposition 2026-10-11:** corrected — `docs/hook-register.md`'s count is re-derived to **479**. <!-- register-row id=D6-27 state=closed marker=check:scripts/hooks-check.sh --> |
| 28 | section headings "Situations … 74 hooks", "Readers … 180 hooks", counts total 398 (`hook-register.md:159,371,583-590`) | contradicted | actual rows 87 / 203 / 479 | not re-derived **Disposition 2026-10-11:** corrected — the section headings are re-derived (87, 56, 203) and the counts paragraph carries the 479 derivation. <!-- register-row id=D6-28 state=closed marker=check:scripts/hooks-check.sh --> |
| 29 | "the six with no caller anywhere" (`hook-register.md:603`) | implemented | grep: each declaration-only outside `ship_core.cpp` | confirmed |
| 30 | `AtEnd` opens the next sector; the third crossed is home (`hook-register.md:243`) | implemented | `AdvanceSector:3529`; `SECTORS_TO_CROSS=3` | |
| 31 | there is no way to start Long Way Home from the menus (`walkthrough.md:25-28`) | contradicted | `module/ui/ui_lwh_start.cpp:289`; `configs/lwh-start.cfg` | **C-8** |
| 32 | "all eighteen systems in power order" (`walkthrough.md:390`) | contradicted | `SYS_COUNT = 23` | **C-6** |
| 33 | "there is no nominal-against-current power display" (`walkthrough.md:554-556`) | contradicted | `module/ship/g_ship.cpp:722-735` | **C-7** |
| 34 | "the crew's manner … still open" (`walkthrough.md:368-369`) | contradicted | `MannerFor`/`EmitManner`; `scripts/manner-check.sh` | **C-10** |
| 35 | opening situation "none of that is implemented" (`walkthrough.md:89-94`) | superseded | `ApplyStartState:4701`; `StartStateAt` | corrected by its own **G2** |
| 36 | "no labs, no astrometrics, no lighting and no cargo loads in the power model" (`walkthrough.md:264-270`) | contradicted | `SPECS[]` `ship_core.cpp:36-64`; `SetAirSource:2027` | **C-9** |
| 37 | "the shuttle load-screen is not built" (`walkthrough.md:253-255`) | implemented | no launch menu; `docs/omissions.md` O9 | accurate |
| 38 | "ends … none of it is implemented" (`walkthrough.md:596-602`) | partial | `won`/`lost` exist; no arrival | |
| 39 | `tools/mastermap.py` diagram labels decks 6/7/12/13/14 "gone / no map" (`tools/mastermap.py:17-25`) | unverified | the doc's table frames itself as "the retail game" | regenerated diagram and prose will disagree |

### 7. Encounters and the outside

`docs/borg-incursion.md`, `docs/outside-the-ship.md`, `docs/shuttles.md`,
`docs/exploration-and-science.md`, `docs/scenario-atlas.md`, `docs/scenario-manifest.md`.

| # | claim (doc:line) | verdict | witness | note |
|---|---|---|---|---|
| 1 | per-compartment `controller` with five values (`borg-incursion.md:23-26`) | implemented | `DeckController:190`; `Deck.controller:219` | |
| 2 | "every write requires dwell time. below the threshold, nothing is written" (`borg-incursion.md:59-62`) | implemented | `DWELL_COMPROMISE:194`; `ship_core.cpp:2560,2605` | |
| 3 | three thresholds: compromise / contested / seized (`borg-incursion.md:77-82`) | implemented | `ship_core.cpp:2575,2661`; `ship_core.h:195-196` | |
| 4 | security console: "contact, deck seven, no systems touched" (`borg-incursion.md:85-86`) | absent | no detection/alarm model | only a post-hoc clean-intercept log |
| 5 | "if the intruders are never detected, nobody is coming" (`borg-incursion.md:90-95`) | absent | `ship_core.cpp:2538` dwell accrues with no detection gate | |
| 6 | clean intercept recorded, names in it (`borg-incursion.md:99-102`) | implemented | `ship_core.cpp:2485-2495`; `cleanIntercepts:1964` | |
| 7 | Borg "grow from what they hold"; retaking permanent, no spawn (`borg-incursion.md:112-115`) | implemented | `ship_core.cpp:2602-2627,4189-4192` | |
| 8 | "you can reclaim a deck, and the deck is not the same afterwards" (`borg-incursion.md:127-131`) | partial | crew `assimScar:522`; **no deck residue field** | the place is fully restored |
| 9 | force fields "rated 1-10; a level-10 field can cut a drone from the Collective" (`borg-incursion.md:135-139`) | implemented | `FIELD_MAX:198`; `ship_core.cpp:2528` | |
| 10 | adaptation "(a) per weapon type" (`borg-incursion.md:143-147`) | partial | single `Enemy::adaptation:860` | not per weapon type |
| 11 | `DWELL_COMPROMISE` 120 / `DWELL_HOLD` 240 / `DWELL_SEIZE` 360 (`borg-incursion.md:235-243`) | implemented | `ship_core.h:194-196`; `TestIncursion:2975` | |
| 12 | `Remodulate` "subtracting 0.5 … 120-second cooldown"; `RaidVinculum` zeroes adaptation for 300 s (`borg-incursion.md:245-250`) | implemented | `ship_core.h:2404-2406`; `ship_core.cpp:3200,3214` | |
| 13 | `RecoverCaptive` window, permanent `assimScar`, `MEM_VIOLATION` (`borg-incursion.md:252-257`) | implemented | `RECOVERY_LIMIT:2339`; `ship_core.cpp:6357-6375`; test:2815 | |
| 14 | `OrderAdvance` musters security and advances a deck in 3 minutes (`borg-incursion.md:266-270`) | implemented | `ship_core.cpp:4169,4181`; `ship_core.h:1711-1712` | |
| 15 | "still to come: the crew layer embodying the squad as bodies" (`borg-incursion.md:272-275`) | absent | no crew-layer squad code | O13 |
| 16 | chart row carries `type`, `position`, `confidence`, `hazards`, `resources`, `first_contact`, `site` (`outside-the-ship.md:13-18`) | partial | `Beacon:828-841` | only kind/visited/surveyed/links/phenomenon |
| 17 | chart displayed on the astrometrics wall, the bridge, and by query (`outside-the-ship.md:20-23`) | partial | `ui_lwh_chart`; `console_api.def:43` | no astrometrics-wall display witness |
| 18 | transporter range 40,000 km and a block list (`outside-the-ship.md:60-62`) | partial | `TransportAway:2923-2929` (shields only) | no range/storm/mineral gate |
| 19 | per-shuttle fields including manifest and cargo (`shuttles.md:11-21`) | implemented | `Shuttle:870-881` | |
| 20 | classes "Class 2, Type 6, Type 8, the Aeroshuttle, the Delta Flyer" (`shuttles.md:12`) | partial | `ship_core.h:866` (4 classes) | Flyer deliberately absent |
| 21 | "the load screen — and it should make you decide something" (`shuttles.md:44-47`) | absent | no menu; `docs/evidence/every-screen.md` row 18 | O9 |
| 22 | return three ways (`shuttles.md:52-54`) | implemented | `RecallShuttle`/`StrandShuttle`/`LoseShuttle` `ship_core.h:2396-2398` | |
| 23 | replacement is a build job (`shuttles.md:58`) | implemented | `RebuildShuttle:2400`; `TestShuttles` | |
| 24 | "shuttlebay 1 is on deck 10" (`shuttles.md:62`) | implemented | `SHUTTLE_BAY_DECK=10:884` | |
| 25 | survey "writes: chart rows with confidence" (`exploration-and-science.md:43-44`) | partial | `Survey` sets `b.surveyed:831` | no `confidence` field |
| 26 | probe consumable, charts the target, sometimes lost (`exploration-and-science.md:46-49`) | implemented | `LaunchProbe`; `scripts/probe-check.sh` | |
| 27 | phenomenon: hidden attributes revealed one scan at a time (`exploration-and-science.md:51-57`) | implemented | `RevealPhenomenon:3012`; `RespondPhenomenon:3027`; test:2300 | |
| 28 | first contact: technology level and awareness; reputation per faction (`exploration-and-science.md:65-69`) | partial | `ObservePreWarp`/`InterferePreWarp:2485-2486` | no tech level, no per-faction reputation |
| 29 | "the lab … research progress, which unlocks a treatment, a material, or a sensor modification" (`exploration-and-science.md:71-74`) | partial | `SYS_SCIENCE_LABS:54`; research only for dilithium | |
| 30 | "the board … the single-player module persists the mission objective set" (`exploration-and-science.md:76-79`) | absent | no board in `ship_core` | |
| 31 | dilithium spent per jump, recomposite, five ways, `DilithiumRange` ×3000 (`exploration-and-science.md:147-156`) | implemented | `ship_core.h:902-909`; test:2399 | |
| 32 | "every sector holds one anomaly, on a beacon fixed by the seed" (`exploration-and-science.md:178-186`) | implemented | `ship_core.cpp:2696-2707`; `scripts/phenomenon-check.sh` | |
| 33 | phenomena probe classes with different sensor packages (`exploration-and-science.md:175`) | absent | `Stores.probes` is one kind | |
| 34 | "scenarios have breeding conditions, not timers" (`scenario-atlas.md:20`) | absent | no scenario/breeding system in `module/` | G4/O15 |
| 35 | "no scenario, meeting brief, log entry or voice line may name a crew member" (`scenario-atlas.md:78`) | partial | law recorded in `docs/hook-register.md:43` | no validator enforces it |
| 36 | "some things need two people" (`scenario-atlas.md:103`) | implemented | `Override` `ship_core.h:1893-1901` | |
| 37 | "the ship's roster promotes to fill its own gaps" (`scenario-atlas.md:119`) | implemented | `FillVacancies:4511-4535` | |
| 38 | "the Borg … the only faction that can take the ship outright" (`scenario-atlas.md:150`) | partial | Borg controller/compromise in core | the variety budget is largely unbuilt |
| 39 | manifest name/maps/scripts required; optional sections (`scenario-manifest.md:19`) | implemented | `tools/validator/validate.py:123-127` | |
| 40 | the crew section is the contract between authoring and the direction layer (`scenario-manifest.md:49`) | implemented | `tools/crewgen/crewgen.py`; `crew_core.h:65` | |
| 41 | validator checks E001..E008, W001..W007 (`scenario-manifest.md:118`) | implemented | `tools/validator/validate.py:10-24` | codes match |
| 42 | "exit status is 0 when there are no errors" (`scenario-manifest.md:127`) | implemented | `validate.py:352` | |

### 8. The start, the endings, the modes

`docs/start-states.md`, `docs/the-entry-point.md`, `docs/endings.md`, `docs/client-modes.md`,
`docs/story-and-semantics.md`, `docs/rising-to-the-occasion.md`, `docs/design-creed.md`,
`docs/design-north-star.md`.

| # | claim (doc:line) | verdict | witness | note |
|---|---|---|---|---|
| 1 | "a start state is simply the initial condition of that engine, expressed as data" (`start-states.md:9`) | implemented | `StartState` `ship_core.h:1833`; `ApplyStartState:4701` | |
| 2 | "vacancies[] — derived from the casualties by the same rules that run during play" (`start-states.md:13`) | implemented | `FillVacancies:4511`; `TestVacancySameRule` | one rule for both |
| 3 | "twelve or more dead (canon), microfractures in the core, no array, the Maquis aboard" (`start-states.md:15`) | partial | `shipDead = 12` `ship_core.h:1850` declared but never read | the log states condition, not the 12 dead |
| 4 | "log_seed — the first entry in the ship's log" (`start-states.md:17`) | implemented | `WriteLogSeed:4689`; `LogSeedText:4666` | |
| 5 | "the default stays canon … the player is a junior officer" (`start-states.md:25`) | implemented | `ship_core.cpp:4585`; `TestCanonStartUntouched` | |
| 6 | "there is no post the player is not allowed to start in" (`start-states.md:30`) | partial | `playerSeat:1846`; `ui_lwh_start.cpp:205-213` | only three presets exist |
| 7 | "a captain start state … comes with a casualty that created the vacancy" (`start-states.md:51`) | implemented | `ship_core.cpp:4594-4598,4670-4679`; test:7276 | |
| 8 | "multiplayer is simply a start state with more players in it" (`start-states.md:57`) | absent | no server/player code | Track D |
| 9 | "command must be losable: relieved by the ship's own people" (`start-states.md:77`) | partial | `AssumeCommand:4461` | succession only on death/assimilation |
| 10 | "the same decision issued by a junior is refused, and by the captain is carried out" (`start-states.md:100`) | implemented | `scripts/s10-check.sh:47-49`; `PlayerMayCommand:4091` | |
| 11 | "a launcher — one thing that sets the cvars and loads the map" (`the-entry-point.md:29`) | implemented | `scripts/run-lwh.sh:61-64` | |
| 12 | "a 'Long Way Home' line on the main menu" (`the-entry-point.md:30`) | implemented | `ui_lwh_start.cpp:289`; `patches/0020:57` | |
| 13 | "the cvars stay off by default. the menu is the switch" (`the-entry-point.md:38`) | implemented | `g_ship.cpp:5310`; `configs/lwh-start.cfg:10` | |
| 14 | "it is set once, in `configs/lwh-start.cfg`, which both paths execute" (`the-entry-point.md:57`) | implemented | `configs/lwh-start.cfg:18`; `ui_lwh_start.cpp:136` | one copy |
| 15 | "the ship loads from the menu, with no console and no cvars typed" (`the-entry-point.md:65`) | unverified | `ui_lwh_start.cpp:123-137` | evidence-only; no rendered-menu check |
| 16 | "what ships is the whole canonical default only … none of it is implemented" (`the-entry-point.md:77`) | superseded | `ui_lwh_start.cpp`; `g_ship.cpp:5925`; save version 54 | historical status; configurator built |
| 17 | "vacancies must be produced by the same rules that run during play — never hand-written" (`the-entry-point.md:88`) | implemented | `ship_core.cpp:4511`; `TestVacancySameRule` | |
| 18 | "the start state chooses the situation; character creation chooses the person" (`the-entry-point.md:97`) | implemented | `CreateCharacter:4098`; `TestCharacterCreationComposes` | |
| 19 | "the entry point is a configurator, and it has four dimensions" (`the-entry-point.md:159`) | implemented | `ship_core.h:1833-1850`; `ui_lwh_start.cpp:87-97,205-213` | |
| 20 | per command-crew member survivor or casualty, any/all/none (`the-entry-point.md:164`) | implemented | `casualty[SEAT_COUNT]:1838`; `g_ship.cpp:5943` | |
| 21 | the roster may be fictitious entirely (`the-entry-point.md:165`) | implemented | `ship_core.cpp:4604`; `TestFictitiousNoCanon` | |
| 22 | "Starfleet junior, lower decks, Maquis …, and the chair" (`the-entry-point.md:166`) | implemented | `CareerPath:1798-1804`; `ship_core.cpp:1140-1154` | |
| 23 | "every authored line … must address people by post and role — never by name" (`the-entry-point.md:174`) | partial | `OfferRisingTo:775-778` | resolution exists; no scenario content; validator does not enforce |
| 24 | "three proving cases … canon default, the captain, an all-fictitious crew" (`the-entry-point.md:214`) | implemented | `ship_core.cpp:4581-4610`; `scripts/configurator-check.sh` | |
| 25 | "the game can end in arrival at Earth, and it will be a joke ending" (`endings.md:9`) | absent | `won` only logs "the ship is home" `ship_core.cpp:3535-3536` | no arrival clip |
| 26 | "the game hands back the run: the whole record, searchable" (`endings.md:34`) | absent | no end-of-run handback | |
| 27 | "same clip … three seconds … whether seventy years or seventy seconds" (`endings.md:45`) | absent | no clip exists | |
| 28 | "a destination declares which part of the record it reads" (`endings.md:58`) | absent | no destination table | |
| 29 | "the set is accrued, not selected" (reputation) (`endings.md:63`) | absent | no reputation in `module/` | |
| 30 | "exactly one unwinnable end state, the core breach" (`endings.md:73`) | implemented | `lost` `ship_core.h:1979`; `ship_core.cpp:6513-6522` | |
| 31 | "Earth: always reachable; not always welcoming" (`endings.md:80`) | absent | Earth not modelled | |
| 32 | "the crew get a vote … the veto fires once, at the destination" (`endings.md:187`) | absent | no vote/destination code | |
| 33 | "mode 1 … the SP path runs the campaign, saves and reloads" (`client-modes.md:9`) | partial | `scripts/exits-check.sh:42` retail quit proven | full campaign/save not automated here |
| 34 | "mode 2 … retail Holomatch, over a LAN or a VPN" (`client-modes.md:10`) | partial | `scripts/run-cmod.sh:23` | launcher staged; no match test |
| 35 | "mode 2 is cMod as shipped — zero delta from us" (`client-modes.md:73`) | implemented | `scripts/run-cmod.sh:4-5` | |
| 36 | "'full original' means compatibility … not a byte-identical binary" (`client-modes.md:109`) | unverified | policy statement; no check | |
| 37 | morale "never stored as a hidden counter"; the conflict claims a single stored float (`story-and-semantics.md:133`) | contradicted | `ship_core.h:511-517`; save 53 | **C-1** |
| 38 | "the ship carries a written list of what she has given up … it is built" (`story-and-semantics.md:65`) | implemented | `losses:2076`; `WriteOff:7057`; `ui_lwh_command.cpp:113` | |
| 39 | the counter "is now built … so the console prints it" (`story-and-semantics.md:94`) | implemented | `NavigationCounter:2776` | |
| 40 | "a post must be held and nobody who can hold it is available … the drive is the lever" (`rising-to-the-occasion.md:27`) | implemented | `OfferRisingTo`/`OfferRising` `ship_core.cpp:5768,5792`; `RisingDriveAtStake:5688` | |
| 41 | "the attempt is made beyond the person's effective skill … they are not zero" (`rising-to-the-occasion.md:33`) | implemented | `RisingOdds:5800-5808`; `RISING_MIN_SUCCESS=0.05` | |
| 42 | "what it costs, and this is not optional" (`rising-to-the-occasion.md:36`) | implemented | `ship_core.cpp:5864-5871,5914` | |
| 43 | "a memory in the witnesses" (`rising-to-the-occasion.md:40`) | implemented | `RisingWitnesses:5824`; `RetellRising:5928` | |
| 44 | "the person may refuse … as a decision from their drives and morale" (`rising-to-the-occasion.md:47`) | implemented | `RisingWouldGo:5725-5766`; `RISING_REFUSED:5851` | |
| 45 | "a mark's salience decays unless reinforced" (`rising-to-the-occasion.md:111`) | implemented | `Remember:4733-4735`; `RetellRising:5928` | |
| 46 | a heroism stat, a courage roll, a bar should be refused (`rising-to-the-occasion.md:127`) | implemented | no such field or roll | |
| 47 | "the core breach … reachable only by a chain, never by a roll" (`design-creed.md:22`) | implemented | `ship_core.cpp:6495-6522` | |
| 48 | "the spotlight is inherited … whoever is promoted into that post inherits an arc" (`design-creed.md:61`) | partial | `FillVacancies:4511-4535` | no authored arcs exist to inherit |
| 49 | "persistent ship state. damage is stored per compartment and per system" (`design-north-star.md:29`) | implemented | `Ship` `ship_core.h:1934` | |
| 50 | "the crew as a load-bearing system … deaths that leave a post empty" (`design-north-star.md:36`) | implemented | `ship_core.h:509-533`; `FillVacancies` | |
| 51 | "the ship's state carries between missions; the save is the ship, not the level" (`design-north-star.md:40`) | implemented | `Pack`/`Unpack`; `g_ship.cpp:5361-5371` | |
| 52 | "the ceiling is 64 played characters — the engine's `MAX_CLIENTS`" (`design-north-star.md:175`) | contradicted | `patches/0018:21` (MP 64) vs `:32` (SP 1) | fork: the shipped SP path is 1 **Disposition 2026-10-11:** **handed to the code list** — a fork: the document's 64-client ceiling against the shipped single-player path's 1. **Disposition 2026-10-12 (code pass): handed back** — a fork. The 64 is `MAX_CLIENTS` in the multiplayer engine header; the Retail single-player build defines it as 1 (with 128 commented). Both are the shipped engine's data, and which ceiling this project's multiplayer premise rests on is Track D's ruling, not a defect. `patches/0018` is untouched. |
| 53 | "the complement is hull data, not a constant" (`design-north-star.md:192`) | contradicted | `const int COMPLEMENT = 141` `ship_core.h:23` | fork: one hard-coded hull **Disposition 2026-10-11:** **handed to the code list** — a fork: the document's 'the complement is hull data' against the hard-coded `COMPLEMENT`. **Disposition 2026-10-12 (code pass): handed back** — a fork. The code carries one hull, Voyager, and `COMPLEMENT = 141` is its canon figure (`ship_core.h:23`); making the complement hull data is the multi-hull system Track D names, a feature, not a contradiction to repair. The owner rules; `COMPLEMENT` is untouched. |
| 54 | "the unit is the post, not the character … a player who takes a post displaces the NPC" (`design-north-star.md:196`) | partial | `seatHolder[]:2065`; seat model | no player/NPC handover across a server |
| 55 | "one ship equals one server" (`design-north-star.md:142`) | absent | Track D unbuilt | |
| 56 | "scenarios have breeding conditions" is the content layer; no director (`walkthrough.md:494-498`) | absent | only the crew restaffing rule | G4 |

### 9. The engine, the policy and the pipeline

`docs/asset-doctrine.md`, `docs/engine-extension-policy.md`, `docs/authoring-a-location.md`,
`docs/location-brief-template.md`, `docs/frame-assertions.md`, `docs/confidence-and-verification.md`,
`docs/multiplayer-premise.md`, `docs/path-to-playtest.md`, `docs/gap-analysis.md`.

| # | claim (doc:line) | verdict | witness | note |
|---|---|---|---|---|
| 1 | the panel-painting path is the mechanism (`asset-doctrine.md:16`) | implemented | `module/ship/lwh_panel.cpp`; `tools/shipmap/panelart.py`; `patches/0014` | |
| 2 | "we do not re-author named characters at all" (`asset-doctrine.md:25-30`) | implemented | `IsCanonFace:581`; `CanonFaceInPool:592`; `scripts/appearance-check.sh` | whole-pool check |
| 3 | "novel voice is code we ship, never audio" (`asset-doctrine.md:39-45`) | implemented | `tools/voice/*.py`; `.gitignore:41` | |
| 4 | "nothing generated and nothing extracted enters the repository" (`asset-doctrine.md:49-51`) | contradicted | `docs/art/lcars-meeting-overlay.png` tracked | **C-20** |
| 5 | "some 3,000 lines of ours in `module/`, compiled in through a 51-line attach-point patch" (`engine-extension-policy.md:12-13`) | contradicted | `module/` = 27,883 lines; `CONTRIBUTING.md:80` says 1,600 | **C-19** |
| 6 | "a patch carries calls into our files; it does not contain our logic" (`engine-extension-policy.md:29-30`) | partial | `patches/0012:22-31` save rule; `0007:22-28` pause rule; `0010:55-64`, `0011:16-21` | rules reach the engine bridge |
| 7 | "prefer a seam to a rewrite … one concern per patch" (`engine-extension-policy.md:31-32`) | implemented | `CONTRIBUTING.md:44-69` (24 rows) | |
| 8 | "every engine extension is cvar-gated and off by default" (`engine-extension-policy.md:33-35`) | contradicted | `patches/0004,0008,0009,0010,0011,0018` have no cvar guard | **C-18** |
| 9 | "saves must round-trip … where a change would, it is gated and stated" (`engine-extension-policy.md:36-37`) | contradicted | `patches/0008:12`, `0018:15` state the break, neither gates it | **C-18** |
| 10 | "no game content or rules in the engine" (`engine-extension-policy.md:40-41`) | partial | `patches/0012` save-policy rule; `0007` pause rule | |
| 11 | "every engine patch records its rebase cost in `CONTRIBUTING.md`" (`engine-extension-policy.md:42-43`) | implemented | `CONTRIBUTING.md:44-69` lists 0001–0024 | |
| 12 | "patches remain a series against a pinned commit" (`engine-extension-policy.md:38-39`) | implemented | `scripts/bootstrap-upstream.sh:30-31` | |
| 13 | "147 map sources … four retail paks … eight holodeck programmes" (`authoring-a-location.md:8-9`) | unverified | shipped data absent | conflicts with `gates.md:28` (106) and asset-doctrine's six |
| 14 | "navigation furniture at standing distance (24-64 units)" (`authoring-a-location.md:30`) | contradicted | `tools/posts/gen-posts.py:37` (24–96) | **C-21** |
| 15 | "`check-bsp.py` … wire it into whatever script builds the map" (`authoring-a-location.md:36-37`) | implemented | `scripts/build-map.sh:94`; `tools/mapgen/check-bsp.py` | |
| 16 | "`tour/deck04` is the model to copy" (`authoring-a-location.md:39`) | unverified | map sources absent | |
| 17 | "the engine writes `maps/<map>.nav` itself on first load" (`authoring-a-location.md:46-47`) | partial | `scripts/build-map.sh:20` repeats it | no check asserts the file; engine absent |
| 18 | "`-fs_basepath` must point at a directory whose child is exactly `baseEF`" (`authoring-a-location.md:57-59`) | implemented | `scripts/run-engine.sh:9-12`; `.gitignore:14` | the AGENTS.md trap |
| 19 | "`map` is the single-player route; `spmap` … Holomatch" (`authoring-a-location.md:60-61`) | implemented | `configs/lwh-start.cfg`; `scripts/build-map.sh:22` | |
| 20 | "copy this file to `docs/locations/<deck>-<room>.brief.md`" (`location-brief-template.md:8`) | implemented | six briefs present in `docs/locations/` | |
| 21 | standing distance 24-64 (`location-brief-template.md:65`) | contradicted | `tools/posts/gen-posts.py:37` (24–96) | **C-21** |
| 22 | "one instrument (`tools/frames/frame.py`)" (`frame-assertions.md:9-11`) | implemented | `tools/frames/frame.py`; `tests/tools/test_frames.py` | |
| 23 | deck floor "`p90 ≥ 80` and `spread ≥ 40`, no median clause" (`frame-assertions.md:40-47`) | implemented | `scripts/deck-check.sh:42` | |
| 24 | "half 2 now exits 3 (SKIPPED), not 0, with no game data" (`frame-assertions.md:77`) | implemented | `scripts/appearance-check.sh:53` | |
| 25 | "33 scripts reference a written frame, 21 assert on the picture, 12 remain path-only" (`frame-assertions.md:118-124`) | partial | 22 scripts invoke `frame.py` | the doc's own count does not reproduce |
| 26 | "contested canon … decide and record in the brief" (`confidence-and-verification.md:13-14`) | implemented | `tools/shipmap/dressdeck14.py:23-30`; the deck-14 brief | |
| 27 | "factions the game never names … never assert their identity" (`confidence-and-verification.md:15`) | partial | `ship_core.cpp:522-528` folds `imperial*` into `SPECIES_HUMAN` | no "unknown" label |
| 28 | "authority becomes a field on the crew record: who holds the conn, who may order a deck sealed" (`multiplayer-premise.md:62-64`) | partial | `OodMayOrder:1745-1756`; `SEAT_CAPTAIN`; `EjectCore:6547` | authority exists as reads; no per-crew stored field |
| 29 | "the first log entry is the premise" (`multiplayer-premise.md:66`) | implemented | `ship_core.cpp:4668` | |
| 30 | "command can change hands mid-run … a contested chair" (`multiplayer-premise.md:67-68`) | partial | `FillVacancies:1860-1869` | seniority yes; agreement/contest absent |
| 31 | "the premise appears in the ship's own log, and the roster starts with the senior posts vacant" (`multiplayer-premise.md:72`) | implemented | `ApplyStartState:4701`; `StartState` | |
| 32 | Stage A "built 2026-10-06" (`path-to-playtest.md:27`) | implemented | `g_env` `g_crew.cpp:1951`; `scripts/gravity-check.sh` | |
| 33 | "with the cvar off, the ship behaves and saves exactly as before" (`path-to-playtest.md:42`) | implemented | `g_crew.cpp:2321-2323`; `g_ship.cpp:5473-5475` | |
| 34 | Stage B "built 2026-10-06" (`path-to-playtest.md:47`) | implemented | `g_player` `g_crew.cpp:1952`; `scripts/playerworld-check.sh` | |
| 35 | Stage C "all five absent decks are built" (`path-to-playtest.md:63`) | partial | `scripts/deck*-check.sh` | owner walkthrough open |
| 36 | Stage D deliverable (affinities, briefs, two logs, overlay, Collective voice) (`path-to-playtest.md:84`) | partial | overlay; two logs; Collective present | affinities absent |
| 37 | Stage E — "a run can finish, several ways" (`path-to-playtest.md:99`) | absent | no destination/ending symbols in `module/` | |
| 38 | "do not begin a stage until … verified; one stage, one branch, one PR" (`path-to-playtest.md:150-152`) | unverified | process claim; no config/script witness | |
| 39 | SIF "currently a footnote in the systems table" (`gap-analysis.md:18`) | superseded | `SYS_STRUCTURAL_INTEGRITY:33` | now a modelled system |
| 40 | tractor "salvage and towing — the missing door" (`gap-analysis.md:20`) | superseded | materials economy `ship_core.h:2453-2455`; `AcquireDilithium:2415` | built |
| 41 | air "a clock: how long a compartment sustains its crew" (`gap-analysis.md:26`) | superseded | `ship_core.h:2213-2220` | built |
| 42 | morale/fatigue "never defined"; relationships "no bonds"; discipline "no hearing" (`gap-analysis.md:33-41`) | superseded | `Morale:624`; `Bond:2733`; `Hearing:2483` | built |
| 43 | "the Maquis split … never built" (`gap-analysis.md:46-47`) | superseded | `faction:528`; Maquis arc `:2487` | built |
| 44 | "the ship's log … there is no specced log" (`gap-analysis.md:55-57`) | superseded | two logs `ship_core.h:941-972`; save 47 | built |
| 45 | "the player's own body … nothing about the player being wounded" (`gap-analysis.md:72-73`) | superseded | Stage B `g_crew.cpp:1952`; `WoundPlayer:4404` | built |


---

## The method, stated

**How the corpus was enumerated.** Every `*.md` under `docs/` was listed, then divided by what it is *for*,
not by where it sits:

- **Design documents** — prose that states how the game behaves. These were read in full and swept for
  normative claims. **50 of them.**
- **State and route documents** — `docs/gates.md` (the ledger), `docs/walkthrough.md` (the route),
  `docs/path-to-playtest.md` (sequencing), `docs/hook-register.md` and `docs/hook-triage.md` (derived
  indexes). The **walkthrough** makes normative claims about the game and was swept as a design document;
  the **ledger** was read for state but its statuses are not design claims, so none was extracted from it.
- **Dated records** — `docs/evidence/` (100+ files), the handoffs, the playtest report, the audits. These
  are measurements and history, not claims about how the game behaves. They were **swept for rule-shaped
  statements only** — the brief's own carve-out — because a dated evidence file that states *a rule* rather
  than *a measurement* is a claim. The pass for this was a keyword sweep for rule language
  (`must never`, `the invariant`, `the rule is`, `the law`) over `docs/evidence/`, plus the evidence files
  the design documents rest on (the-air, the-bills, the-assimilation, the-configurator, the-manner, the
  deck redresses). The evidence files' *numbers* were not treated as claims: they are the measurements a
  claim is checked against, and a stale one is annotated rather than counted here.
- **Material** — `docs/research/`, `docs/locations/`, `docs/art/`: source material and briefs, not claims
  about the running game. Read where a claim pointed into them; not swept.

**How a normative claim was recognised.** A sentence that says what *is* or what *must be*: a mechanism, an
authority, a number, a consequence, a room, a screen, a rate, a permission, a derivation rule. A rationale, a
plan, a hope or a history was not extracted. Where a document's normative content is dense, the load-bearing
claims were taken — the ones a later pass would act on — rather than every clause of a mechanism the next
sentence repeats.

**How each was classified.** By reading the code, the unit tests (`tests/ship`, `tests/crew`, `tests/tools`)
and the check scripts (`scripts/*-check.sh`), and naming the witness. A second document agreeing with the
first is **not** a witness: a claim whose only support is another document is `unverified`, and that is
recorded as a verdict.

---

## The documents not swept, and why

**Swept for claims (50):** `access-and-authority.md`, `affinities-and-allegiance.md`, `asset-doctrine.md`,
`authoring-a-location.md`, `borg-incursion.md`, `budget-squaring.md`, `character-attributes.md`,
`character-derivation.md`, `client-modes.md`, `confidence-and-verification.md`, `crew-manifest.md`,
`crew-roster.md`, `crew-work.md`, `damage-and-budgets.md`, `design-creed.md`, `design-north-star.md`,
`endings.md`, `engine-extension-policy.md`, `exploration-and-science.md`, `failure-is-content.md`,
`frame-assertions.md`, `g3-reactive-crew.md`, `gap-analysis.md`, `hook-register.md`,
`location-brief-template.md`, `lore-ledger.md`, `memory-and-consequence.md`, `memory-boundaries.md`,
`morale.md`, `multiplayer-premise.md`, `navigation-counter.md`, `orders.md`, `outside-the-ship.md`,
`path-to-playtest.md`, `power-assignment.md`, `programme-meetings-and-voice.md`,
`rising-to-the-occasion.md`, `scenario-atlas.md`, `scenario-manifest.md`, `ship-master-map.md`,
`ship-model.md`, `ship-programme.md`, `ship-systems.md`, `shuttles.md`, `staff-meetings.md`,
`start-states.md`, `story-and-semantics.md`, `the-entry-point.md`, `the-record-and-the-log.md`,
`walkthrough.md`. (The hook triage was used as material; its claims fell under `hook-register.md`.)

**Pure history, dated snapshots and reviews — not swept:**

- `docs/HANDOFF.md` — marked a superseded dated snapshot of 2026-10-06.
- `docs/handoff-full-scope.md`, `docs/handoff-live-crew-and-meetings.md`,
  `docs/handoff-lwh-persistent-ship.md` — dated mandates.
- `docs/playtest.md` — a report of a first run.
- `docs/audit-whole-project.md`, `docs/vv-gap-list-verified.md`, `docs/community-inheritance-audit.md` —
  reviews and surveys of the tree at a date.

**Proposals for another project, or superseded proposals — not swept:** `docs/program-proposal-v2.md`,
`docs/program-charter-v3.md`, `docs/native-linux-client-proposal.md`, `docs/ship-workshop-proposal.md`,
`docs/workshop-suite.md`, `docs/proving-ground-for-reachlock.md`, `docs/prior-art-rpg-x.md`,
`docs/sp-and-rpgx-plan.md`, `docs/ef-community-catalog.md`, `docs/elite-force-content-options.md`. These are
ReachLock's or are superseded by `docs/gates.md`; their deferred items belong to another programme.

**The register that this pass is the sibling of — read, not touched:** `docs/omissions.md`. Its rows are left
exactly as they are; its findings are cited above where the same mechanism is at issue. A **`contradicted`**
row here is a gap the omissions sweep could not see — it asked *is this registered?*, and none of these is.

---

## What could not be established, said plainly

- **Anything that is a judgement, not a claim.** Whether a console is pleasant, whether a re-dressed deck
  reads as a place, whether the red emergency strip reads as red, whether the crew read as inhabited. These
  are the owner's, and the register does not pretend to a verdict on them.
- **Anything that needs the shipped content, the paks or the pinned upstream.** The map sources (so the
  "139 edges" and "`deck04` reaches eight holodeck programmes" claims), the install's asset counts (six vs
  eight holodecks; 147 vs 106 map sources), the `.nav` bake on first load, and the render-target viewscreen
  are all `unverified` here because the material is not in this repository. The game data is read-only on
  `sasquatch`; this is a static host.
- **The engine run.** The falsification below uses the unit tests and the check scripts, not a running
  client; anything whose only witness is the rendered screen stays `unverified` or `partial`.
- **The intention behind a fork.** Where the document and the code disagree and neither is provably the
  later decision — the sector's `% 15` roll against the ledger's stated percentages; the twelve-shot
  adaptation against the continuous ramp; standing distance 24-64 against 24-96 — the register gives the
  fork and does not pick, because the evidence does not.
- **The Track D premise and its mechanical consequences.** `docs/multiplayer-premise.md` and
  `docs/design-north-star.md` §8 are claims about a programme with no code in this tree; their mechanisms
  are recorded `partial` or `absent`, and no rendering judgement is offered.
- **Whether `docs/evidence/` holds a rule-shaped statement this pass did not recognise.** The sweep there was
  keyword-driven and bounded; a rule phrased without those words would have been missed. Said so rather than
  claimed complete.
- **Whether the corpus holds a contradiction nobody has read closely enough to notice.** This pass found
  thirty-three; it does not claim the corpus is clean.

---

## Judgement calls, named as calls

1. **What counts as a design document, and what as history or a proposal.** The list above is a call; a
   reader may move a file. `docs/walkthrough.md` is the sharpest: it is a *route*, but its body states how
   the game behaves, so its route claims were swept as design claims — which is why it appears five times in
   the numbered contradictions with its own gap list correcting it. The ledger was not swept: its rows are the state
   of the work, not claims about the game.
2. **`docs/gates.md` was not touched.** The brief forbids touching the ledger's state. Every contradiction
   here is therefore a **lane or a row not yet written**, recorded in this register and left for the pass
   that repairs.
3. **Contradicted vs superseded, where a later document inside the corpus changed the answer.** Where a
   later ruling is in the corpus (`docs/borg-incursion.md` fixing the dwell thresholds; `docs/endings.md`
   fixing the one loss state; the C2 ruling folding holdings), the earlier claim is `superseded` and the
   ruling is named. Where the code is simply doing something different with no such ruling, it is
   `contradicted`.
4. **Which side is wrong, named only where the evidence names it.** For a stale number or a stale
   "not built" the code is the ruler and the document is corrected; for a mechanism the code lacks, the
   document may be ahead of the build and the code is incomplete. Where neither, the fork is given.
5. **The evidence sweep was bounded.** Only rule-shaped statements were extracted from `docs/evidence/`; its
   measurements are the instrument a claim is checked with and were not themselves classified.
6. **A `contradicted` predicate with no consumer is still counted as a contradiction.** The OOD's authority
   (**C-11**) exists and is tested but no decision uses it; that is a divergence between the document and the
   *game*, and it is the class this pass most wants to surface.
7. **Counts are of extracted claims, not of sentences.** The pass claims coverage of the load-bearing
   normative content of the documents listed, not that every subordinate clause was enumerated.
8. **Nothing was fixed.** Not a document, not a line of code; `BRIEF.md` was deleted before committing, as
   the brief requires, and is not counted among the files above.

---

## The falsification of this instrument

Three claims classified **implemented** were checked by an **independent** witness — a check script that
**fails on a planted error as well as passing on the tree**, so it can fail for the reason we care about.
A classification verified only by reading is a reading. The transcripts and the commands that produced them
are in `docs/evidence/designs-against-code.md`.

1. **`docs/morale.md:141-143` / `docs/character-derivation.md:30-35`** — *"holdings is a fold, not a stored
   value"* → `scripts/holdings-check.sh` (passes; and the guard is shown **catching** a planted
   `s.crew[0].holdings` field, so it fails for the right reason).
2. **`docs/crew-manifest.md:105-116`** — *"battle stations call and stand down with the hands moving, and
   the emergency bills clear a deck"* → `scripts/bills-check.sh` (passes; and the guard is shown **failing on
   a planted break** in the call/stand-down/clearing arithmetic).
3. **`docs/character-derivation.md:264-291`** — *"the crew's state selects a manner, and a save replays
   it"* → `scripts/manner-check.sh` (passes; and the check runs the state **with the wiring removed** and
   requires the broken run to report itself, so the pass is not vacuous).

Each was also reached by the unit tests (`tests/ship/test_ship_core.cpp`:
`TestHoldingsIsAFold`, `TestTheBills`, `TestMannerDeterminismAndSave`), run by `scripts/test.sh`.

