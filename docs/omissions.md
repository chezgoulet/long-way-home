# The omissions register

An audit, on `testing`, of what the design corpus commits us to that the roadmap never registered. The
roadmap for this purpose is `docs/gates.md` (the ledger) plus `docs/path-to-playtest.md` (the sequencing it
defers to). A commitment recorded in either is **registered** and is not a finding here. Everything else the
corpus states as design, or defers, or promises and withdraws, that no gate carries is below.

Method: a sweep of all 63 prose documents in `docs/` and all 52 in `docs/evidence/`, the location briefs and
the research briefs, read against `docs/confidence-and-verification.md`'s rule that a claim is worth its
witnesses. Every entry cites the document and the words that commit us to it. The **considered and rejected**
section at the end records what was checked and dismissed, so the sweep is auditable.

*Nothing here was built or changed; this session touched no code and ran no engine.*

---

## Kinds

The six kinds the brief names:

1. **Designed but not built** — a mechanism stated as the design that no gate carries.
2. **Deferred by a recorded decision** — deferred, and never reached the roadmap.
3. **Contested or unresolved** — an open question, or two documents disagreeing, with no ruling.
4. **Promised and withdrawn** — a capability removed because it could not be kept; still a gap.
5. **Dangling references** — a cited file that does not exist (verified).
6. **Canon we said we would model and have not.**

---

## Findings

One entry per finding. **Order** is against the existing work: the ledger corrections and the owner's rulings
first because everything else is ordered after them; then the items that close against the current ship and
crew work; then content; then the separate programme. The reason is given per item.

**This register is a snapshot of the sweep; `docs/gates.md` owns the status.** A row whose work has since
landed is **corrected in place** rather than deleted — the finding is the audit's evidence, the closing is the
roadmap's business. **A row that still reads as unbuilt after the work has shipped is the defect this section
exists to prevent**, and O1 acquired exactly that defect within the hour of the sweep.

| # | finding | commits us | kind | to close | order, and why |
|---|---|---|---|---|---|
| **O1** | **CLOSED 2026-10-07, PR #53** — the power and supply model of `docs/budget-squaring.md` **was** unimplemented and unregistered. Five systems the document adds to the model (astrometrics 60, science labs 60, gravity plating 30, non-essential lighting 20, cargo handling 20), the source rebalance (warp core `1,400 × crystalCeiling`, impulse 250, auxiliary 90, batteries 60), the crystal ceiling bearing on **output**, the drive shed last, the torpedo count of 38 with the canon moments at 11 and 6, and the two display numbers (fresh against now) | `docs/budget-squaring.md` header: *"Status: proposed, awaiting the owner's ruling. Nothing here is implemented."* and Part three *"there is no mechanism for it. This is the largest structural gap in the ship's model."* | 3 (and 6) | the owner's ruling (Part four/Part five); then `SYS_COUNT`, `SPECS[]`, the allocation order, the console rows, `SAVE_VERSION` | **first of the contested items**: it changes the system table, the save version and every console row, so nothing else that touches power ("O2", the science load, the deck-12 countdown) can be ordered ahead of it. It landed with demand 1,730, a fresh supply of 1,800, `crystalCeiling` scaling the
core's output, the drive shed last, and the console's fresh-against-now numbers; save format 50. **What remains
is the canon torpedo moments at eleven and six, which is content rather than mechanism.** |
| **O2** | **The three independent power sources, and the holodeck-matrix trap, are not modelled.** Canon names exactly three things that run when the main grid does not (life support, holodecks, shuttlecraft), and that the holodeck matrix is incompatible — jumping from it destroys relays | `docs/ship-systems.md` *("Life support and the holodecks keep running when the main grid dies… That is dramatic, and it is free"*; *"a designed failure mode, not a solution"*); `docs/damage-and-budgets.md`; `docs/budget-squaring.md` Finding two; `docs/handoff-lwh-persistent-ship.md` (*"Use both"*) | 6 | separate source/sink per the budget ruling; the holodeck reactor as a damaging control, not a free one | after **O1**: it is the same power table, and the brief names it as the canonical example of this kind |
| **O3** | **Canon named as ours to model and never modelled**, in `docs/lore-ledger.md`'s own closing list: bio-neural gel packs, the EMH's dependence on sickbay power and holo-emitters, the variable-geometry nacelles, cargo bays, replicator rations **as a resource**, and holodeck power being incompatible with other systems | `docs/lore-ledger.md`, the final section (a list of canon the ledger says would change rows above it) | 6 | one system at a time, each with the five-part contract in `docs/ship-systems.md` | after **O1**/**O2**: gel packs and replicator rations are power/stores rows; the others are individual systems |
| **O4** | **First contact: the awareness value and travelling reputation are designed and unbuilt.** A detected civilisation carries a technology level and an awareness; hailing, approaching, beaming and scanning move it; reputation travels ahead of the ship | `docs/exploration-and-science.md` item 5 (*"Writes: reputation per faction, awareness, crew memory"*); `docs/memory-and-consequence.md` (*"factions carry reputation and awareness, and the record moves them"*); `docs/scenario-atlas.md` (scenario 3, 7; *"the three unattributed factions… investigation — a first contact that can go either way"*) | 1 | a faction record and an awareness value on the chart row, and what reads them | with the exploration content, after the systems it reads; the chart row already exists (`docs/evidence/the-navigation-counter.md`) |
| **O5** | **Discipline from home — the live link to Starfleet — is named and not modelled.** A live link for eleven minutes a day changes morale *and* command: the decisions made alone can now be questioned | `docs/gap-analysis.md` §4 (*"a live link to Starfleet changes what orders you can give. Named, not modelled."*); `docs/scenario-atlas.md` seventh vein and scenario 6 (*"Eleven minutes"*) | 1 | a link state, a window, and the order/authority consequence | with S10/authority work, because it is the authority model with a reachable audience added |
| **O6** | **The crew manifest's bills are designed and unbuilt.** The watch bill, **battle stations** ("code-blue stations", almost nobody at their normal post), the emergency bills, the damage-control organisation, and the officer of the deck's authority over senior officers | `docs/crew-manifest.md`, *"The bills: the answer to 'who does what'"* and *"Who reports to whom"* | 1 | a bill assignment per crew record and the reconfiguration the situation applies | with the crew layer (S5); it is data over the records S5 already embodies |
| **O7** | **The Hazard Team as a standing unit is designed and unbuilt.** Two squads, A and B, with authored roles; a unit layer above departments; its casualties gut engineering, medical and security at once; its training is the holodeck firing range | `docs/crew-manifest.md`, *"The Hazard Team: the unit the manifest is missing"*; `docs/crew-roster.md` §6 | 1 | the unit as data over the roster, and the two squads as a relief resource | with the crew roster depth (**O8**), which it is the authored core of |
| **O8** | **CLOSED 2026-10-08, PR #72** — the four kinds of modifier (skills, **traits**, **conditions/buffs-debuffs**, state) and the derivation, plus species **capabilities, needs and susceptibilities** (never bonuses) **are built**. The condition record carries source, cure and visibility; the derivation is a function of the record and the seed and is written down in **`docs/character-derivation.md`**; morale is three derived reads (deficit, outlook, holdings), not the old `float morale`; the manner is demonstrated at three levels. Evidence: `docs/evidence/the-character-layer.md`. **What remains** is not this layer: wiring `EffectiveSkill` into the failure roll's odds, and the clone/merge cases, which belong to the memory layer. | `docs/character-attributes.md` (*"Conditions — the missing layer"*; *"The derivation, which was also missing"*; the species section); `docs/crew-roster.md` (traits, drives) | 1 | a condition record with source, cure and visibility, and the derivation written down | with S5/crew; `docs/path-to-playtest.md` Stage D registers only the taste and allegiance priors, not this layer |
| **O9** | **The shuttle load-screen decision menu is not built.** The manifest (which posts empty), the loadout out of stores, the destination's chart confidence, the return condition and the risk, at the moment of launch | `docs/shuttles.md` (*"Not built: the **load-screen** menu"*); `docs/evidence/every-screen.md` row 18 (*"missing, left — a launch decision menu, not a station screen"*) | 1 | a launch-time menu over the existing `LaunchShuttle` model | with the away-mission content; the model is in place (`scripts/shuttle-check.sh`) |
| **O10** | **The in-world control at the panel (the "glance") is not built.** The panel carries the ship's live state as a *read*; the missing half is an interactive surface on the panel brush | `docs/evidence/every-screen.md` row 12 (*"thin, left — in-world control, out of console scope"*); `docs/scenario-atlas.md` ("what you can see") | 1 | an engine/world affordance on the panel brush | with the world work (`docs/path-to-playtest.md` Stage A), because it is world, not console |
| **O11** | **The outside's site archetypes and exterior dressing are designed and unbuilt.** Derelict interior, alien ship, surface facility, station, cave/mine, colony — re-dressed from named shipped maps — plus the starfield/debris-sky pass, then the away-mission loop on top | `docs/outside-the-ship.md` *"Order of work"* items 2–4 | 1 | one map per archetype by `docs/authoring-a-location.md`; the skyboxes | after the chart (built) and with the deck-build path; the away *site* is named in `docs/gates.md`, the archetype set is not |
| **O12** | **The phenomenon's two canon story models are not built:** a nebula the crew enter for fuel that is alive, and a phenomenon that has been experimenting on the crew | `docs/exploration-and-science.md` (*"Not built: the two canon story models"*) | 1 | authored phenomenon templates over the built phenomenon model | with exploration content; low, because the mechanism exists and these are content |
| **O13** | **The Borg security squad is not embodied as bodies moving deck by deck.** The design's "room by room" advance; only the model-level `OrderAdvance` exists | `docs/borg-incursion.md` *"Still to come: the crew layer embodying the squad as bodies moving deck by deck"* | 1 | the crew layer walking the squad | with S7 (security as bodies, built) and a session; `docs/gates.md` S7 does not carry the squad-as-bodies |
| **O14** | **The wall-clock exit is not wired to the engine's quit path.** The model has `Suspend`, `leftStanding` and the catch-up; the engine does not call it on quit, so the third clock has no real exit yet | `docs/evidence/clocks-and-exits.md` (*"The engine's quit path calling `Suspend` is not wired here and is named as the remaining work"*) | 1 | wire the engine quit path, cvar-gated per the extension policy | with S10's clocks; small, and it completes a ruling already implemented at the model level |
| **O15** | **The scenario set is designed and unregistered.** Nine authored scenarios, each with breeding conditions, systems stressed, staff needed, cost to answer/ignore, residue, visibility and warmth; and the rank-and-role "author the event once, instantiate it through the post" rule | `docs/scenario-atlas.md`, *"A first slate"* and *"What a scenario must therefore declare"* | 1 | author the slate against the built systems | G5 is a bare "⏳ pending" in the ledger; this is the content it has no plan for. Last of the content |
| **O16** | **The abandonment list — the ship's written-off list — is built but the ledger never registered it.** `Ship::losses`, `WriteOff`, `ship losses`, the `GIVEN UP` column and the painted panel line; the design calls it "the attrition north star expressed as a user interface" | `docs/damage-and-budgets.md` (*"the ship carries a written list of what she has given up"*); `docs/story-and-semantics.md` conflict 1 (*"the headline claim"*); built in `docs/evidence/abandonment-list.md` (save version 41) | 1 (ledger) | a gate entry carrying it (done here) | **first**: it is a ledger correction, not code, and it is a headline mechanism the ledger silently lacked |
| **O17** | **The 2026-10-07 screen sweep is built but the ledger never registered it.** The personal-log screen (`ui_lwh_personal`), the month-report editor (`ui_lwh_report`), the job-queue board (`ui_lwh_jobs`), the workable chart (`ui_lwh_chart`), the tricorder survey (`ui_lwh_survey`), the beacon keys, and the phaser-setting ladder | `docs/evidence/the-missing-screens.md`, `docs/evidence/every-screen.md` (save version 49); the phaser ladder in `docs/lore-ledger.md` | 1 (ledger) | a gate entry carrying it (done here) | **first**: a ledger correction; `docs/gates.md` S4 names only the station panels |
| **O18** | **Track D — one ship, one server — and the multiplayer premise are designed and unregistered.** The premise (the command crew died at the Caretaker; the chair is empty; a new branch), 64 played characters with NPCs filling the complement, the post as the unit with hand-over, authority human-anchored, NPC attrition counts, and command changing hands mid-run with rules | `docs/multiplayer-premise.md`; `docs/design-north-star.md` §8 | 2 | its own gated programme | last: it is explicitly a separate programme built on these systems, and the ledger names Track D only as a destination |
| **O19** | **Dangling reference: `docs/gap-the-log` / `docs/gap-the-log.md`.** The file is `docs/evidence/gap-the-log.md`; the path cited lacks `evidence/` | `docs/the-record-and-the-log.md` lines 20 and 64; `docs/evidence/the-month-report-and-the-toll.md` line 143 | 5 | correct the three citations (done here) | **first**: a one-line fix |

---

## Withdrawn promises (kind 4)

A promise removed because it could not be kept. The withdrawal is correct; losing the fact that the
capability is missing is not, so each is a capability gap and is registered as such.

| # | withdrawn capability | commits us | to close | order, and why |
|---|---|---|---|---|
| **W1** | **A dead officer's credentials are usable.** The record's credentials are retained after death (`MayOperate` refuses only a record that is not `CREW_FIT`), but *using* them is not offered. The older phrasing ("still usable") was withdrawn rather than kept as a promise the code cannot keep. | `docs/access-and-authority.md` (*"This is reported, not built, and the older phrasing ('still usable') is withdrawn"*); `docs/evidence/access-and-authority.md` (*"withdrawn from the acceptance and left to the memory/consent layer"*) | a deliberate act at the console that grants a dead holder's authority, with its cost and its log line | with the memory and consent layer, which the document says owns it (`docs/memory-and-consequence.md`); it is a scene, not a clearance-table entry, so it waits for that layer rather than S10's clearance work |

---

## Contested items (kind 3) — need the owner's ruling before they can be ordered

These are recorded as open, or two documents name different answers, and no ruling exists. They cannot be
scheduled until the owner decides; each says what the decision is.

| # | the question | where it is open | what the decision is |
|---|---|---|---|
| **C1** | **The power budget and the system table.** `docs/budget-squaring.md` is "proposed, awaiting the owner's ruling. Nothing here is implemented"; it supersedes its own Part four shortfall with the Part five headroom curve and adds five systems. | `docs/budget-squaring.md` Parts four–six; the last section names the open questions as the owner's | rule for or against the number set; specifically the five additions, the source rebalance, and whether `crystalCeiling` bears on **output** (ours) as well as life |
| **C2** | **Allegiance drift.** Allegiance strength is fixed but nothing changes it, so a Maquis-weighted crew member reacts the same in year seven as year one. | `docs/story-and-semantics.md` conflict 3 and *"Open"* (*"needs a decision before affinities are authored"*) | whether allegiance strength is itself a fold over marks (the document's recommendation) |
| **C3** | **Q-class encounters.** Canon-adjacent and nearly free, but the risk is a licence to break the rules. | `docs/gap-analysis.md` §4 (*"Low priority, deliberate decision needed"*) | whether to model them at all |
| **C4** | **Per-compartment or per-deck atmosphere and hazards.** `docs/ship-model.md` lists atmosphere and temperature **per compartment**; `docs/ship-systems.md` says per-**deck**; the model and both world evidence documents are per deck. No ruling. | `docs/ship-model.md` ("Compartments and damage"); `docs/ship-systems.md` ("per-deck atmosphere, gravity and temperature"); `docs/evidence/environment-in-the-world.md` and `player-in-the-world.md` ("no per-compartment atmosphere") | one answer, recorded once; a per-compartment model is a different data shape and affects breaches, force fields and the S1/S6 soak |
| **C5** | **The player's succession after death.** The record closes, the body falls and respawn is refused; whether the evening ends or the player plays on as the next officer was deliberately not guessed. | `docs/evidence/player-in-the-world.md` judgement call 1 (*"that … is the owner's call, and I have not guessed it"*) | whether death ends the session or hands the player the successor's body |
| **C6** | **Track D's four questions:** the station model (free bodies or consoles), the ship clock (real time or sessions), what failure means when it persists, and whether ships interact. | `docs/design-north-star.md` §8 (*"New questions this forces (unresolved, for the owner)"*) | four rulings, before the multiplayer programme is scoped |

---

## Considered and rejected (the audit trail)

Checked and **not** registered, with the reason — so the sweep's coverage is auditable.

- **The ten S-gates, and everything already in `docs/gates.md` or `docs/path-to-playtest.md`**, including
  Stage A–E (the environment, the player, places, the crew, an ending) and the named examples the brief
  excludes (`docs/staff-meetings.md` M1–M6, the voice work). Registered; not findings.
- **`docs/evidence/*.md` "what this still needs" items** — worked through: S4's person at a panel, S6's
  further injury causes, S7's hand-played puzzle, S9's engine render-target viewscreen, S10's playthrough,
  the air countdown at deck 12, the EMH, the away site, the manner, the mobile emitter. All are named in
  `docs/gates.md`; none is a finding.
- **The "Borg strategic awareness beyond a single vessel" note** in `docs/lore-ledger.md` — stale; the
  mechanism is built and carried in `docs/gates.md`.
- **The abandonment list's absence as a *build* gap** (`docs/story-and-semantics.md` conflict 1) — stale;
  `docs/evidence/abandonment-list.md` proves it built. Registered as a **ledger** gap instead (**O16**).
- **`docs/community-inheritance-audit.md`'s 51 RPG-X entity classes, `ui_msd`, and the Elite Reinforce
  fixes** — registered in `docs/gates.md` Harvest A/B; `target_shaderremap` is the first adopted. Not
  findings. (`docs/movement_physics/` is Elite Reinforce's own documentation directory, an external
  repository, not a file of ours; rejected as a dangling reference.)
- **`voyager-master-map.png`** — named in `docs/ship-master-map.md` as *regenerable from
  `tools/mastermap.py`*; a generated artifact, deliberately not committed, in the same class as extracted
  data. Not a dangling reference.
- **`docs/evidence/the-navigation-counter.md`'s "§1"** — a loose citation of a judgement-call number in
  `docs/evidence/the-month-report-and-the-toll.md` ("Judgement calls, named" item 1), not a missing section.
  Nothing is absent; not a finding.
- **The ReachLock-facing documents** (`docs/workshop-suite.md`, `docs/ship-workshop-proposal.md`,
  `docs/proving-ground-for-reachlock.md`, `docs/prior-art-rpg-x.md`) — proposals for a different project;
  their deferred items (the audio crate, lifting the transferable rules) are ReachLock's, not this
  programme's. Not findings.
- **`docs/sp-and-rpgx-plan.md`, `docs/ef-community-catalog.md`, `docs/elite-force-content-options.md`,
  `docs/native-linux-client-proposal.md`, `docs/program-proposal-v2.md`** — research and superseded
  proposals, not live commitments.
- **`docs/HANDOFF.md`'s S3 items** (the turbolift menu's selection click, restoring each level change's
  original `target`, the entity-count discrepancy) — the file is a dated session snapshot that says of
  itself "keep this file current"; the click seam is implied by S3's own exit criterion (a person walks the
  ship), and S3 is registered. Not registered as separate findings.
- **The gates-internal contradictions** (the Maquis parenthetical, the Prime Directive parenthetical) —
  corrected in the ledger rather than registered as work; they are wrong lines, not missing work.
- **`docs/client-modes.md`'s two mode-2 questions** (the control fault, the game module) — the control fault
  is superseded by cMod-as-shipped and is carried in `docs/gates.md` G7; the module choice is documented as
  decided. Not findings.

---

## Ledger corrections made in the same pass

`docs/gates.md` was corrected where it was internally inconsistent, and the findings above were added to its
**Residuals** section (and O16/O17 to S4/S6) in the form that section uses:

- the **Maquis** parenthetical "resentment and integration as an arc remain" was stale next to the later
  entry recording the arc done — removed (a defect already named in `docs/path-to-playtest.md`);
- the **Prime Directive** parentheticals were reconciled: the mark is built, the *institutional consequence*
  is not, and that remaining item is now **O5** (discipline from home) rather than two contradictory lines.

## Judgement calls, named as calls

1. **The roadmap is `docs/gates.md` plus `docs/path-to-playtest.md`.** The brief names only `gates.md`, but
   `gates.md` itself defers sequencing to `path-to-playtest.md`, and the brief's own examples (staff meetings,
   the voice work) are registered there rather than in the ledger. Treating both as the roadmap is why those
   examples are not re-reported.
2. **"Built but unregistered" is registered as kind 1.** The brief's kind 1 is labelled "designed but not
   built" but its test is "no gate carries it". A mechanism that is built and evidenced but absent from the
   ledger fails that test, so **O16** and **O17** are in the register; closing them is a ledger edit, not
   code, and they are ordered first for that reason.
3. **O15 (the scenario slate) is registered even though G5 is "pending".** G5 is a bare title with no
   content and no order; the slate has never been carried. If the owner reads G5's pending state as already
   carrying the scenarios, O15 is the one entry that would move to considered-and-rejected.
4. **O18 (Track D) is registered as kind 2, deferral.** `docs/gates.md` names Track D as a destination but
   the premise and its mechanical consequences are in no gate. It is registered so the corpus is complete,
   not because it is next.
5. **`docs/lore-ledger.md`'s closing list is treated as a commitment.** It is a bare list with no lead-in
   in the file as committed. Its items are unambiguously canon-as-yet-unmodelled, and the brief names this
   kind explicitly, so they are registered (**O3**) rather than dismissed for the missing lead-in — which is
   itself noted here so the list can be given a heading.
6. **Atmosphere is contested, not simply a gap (C4).** The two design documents disagree; deciding "per
   compartment" would change the save shape, so it is an owner's ruling before it is work.
