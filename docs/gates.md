# Gate ledger

Status of the programme's gates. The charter defines *what* each gate requires; this records *whether*
it passed, on what evidence, and what is still open.

---

## G0 — toolchain ✅ closed 2026-10-04

| item | result | evidence |
|---|---|---|
| single-player game modules build natively | 157/157 units, both modules, four intended exports only | `docs/evidence/g0-native-module-build.md` |
| native ICARUS script compiler | 2,394 of 2,408 corpus files compile clean; 11 are not scripts; 3 rejected with named causes | `docs/evidence/g0-script-compiler.md` |
| round-trip verification | 2,408 of 2,408 compiled scripts read back by the game's own reader | `docs/evidence/g0-roundtrip-and-dictionary.md` |
| scenario validator | six fault classes each caught *and named*; run against all 106 published map sources | `docs/evidence/g0-validator.md` |
| entity dictionary | 262 classes + 2 family templates; 237 of 239 used classes covered | `docs/evidence/g0-roundtrip-and-dictionary.md` |

Reproduce with `scripts/bootstrap-upstream.sh`, `scripts/fetch-gdk.sh` and `scripts/check.sh`.

## G1 — client playable ✅ closed 2026-10-04, signed off by the owner

| milestone | result | evidence |
|---|---|---|
| M1 engine builds and runs | native Linux build; retail data loaded (21,675 files) | `docs/evidence/g1-engine-native-build.md`, `g1-engine-loads-game-data.md` |
| M2 engine loads our modules | `Lilium Voyager SP`, SP UI loaded, SP commands registered, `SP_LoadGame: apiversion=6` | `docs/evidence/g1-sp-engine-starts.md`, `g1-sp-mission-plays.md` |
| M3 playable | campaign map `borg1` spawned through the SP path; player connected; save written mid-mission and reloaded; clean exit | `docs/evidence/g1-sp-mission-plays.md` |

Signed off by Christopher, 2026-10-04, on the mechanics being demonstrated plus his own play session.

## G2 — Virtual Voyager ✅ reported working (2026-10-05)

**Reported working by the owner from his own play session** -- "single player game works great.
virtual voyager works great" -- with the decks, turbolift and station menus exercised by hand.
Awaiting his explicit sign-off, as with G1.

Content confirmed present rather than assumed: the expansion pak carries **32 maps including
`maps/tour/deck01`–`deck04` and beyond** — the VV decks — plus 274 turbolift/virtual-voyager assets.

Acceptance bar (from the charter, plus the owner's addition):

- the decks load and the turbolift menu moves between them
- the station's menus open: Library, Astrometrics, Personal Log, Medical Log, Recipes, Social
  Calendar, Disease Library, Shooting Range, Weapon Library, Cargo, Engineering Library
- holodeck and shooting range enter and exit cleanly
- a save taken inside Virtual Voyager reloads correctly
- **no regression in the base campaign**

Testing needs a logged-in desktop session: under a virtual display the renderer reports
`failed to find graphics queue family` and the load never reaches the SP path.

## The client's contract — three modes (agreed 2026-10-04)

Recorded in full in `docs/client-modes.md`. The client offers:

1. **the original game and all downloadable content** — retail campaign, Expansion Pack, 1.2 voice pack
2. **original LAN multiplayer** — retail Holomatch over a LAN or a VPN (Tailscale, Nebula, ZeroTier)
3. **Long Way Home** — single player (Tracks A/B/C) and multiplayer (Track D)

Two decisions that go with it:

- **Mode 2 is cMod as shipped, not our engine.** Revised 2026-10-05 under the "inherit, don't own"
  principle: our delta from upstream is the rebase cost, so mode 2 runs cMod's own client with **zero**
  delta from us, inheriting twenty years of connect-path fixes rather than re-earning them. The earlier
  "one engine, two game modules" idea is kept as an option for later, not the plan. Modes 1 and 3 are our
  engine; mode 2 is a separate application, and the two share the same game data.
- **The compatibility promise:** "full original" means compatibility of *gameplay and content* — retail
  maps, saves, configuration and demos behaving as they did — not a byte-identical binary. cMod's
  rendering and limit changes are wanted, not violations.

## G6 — retail single player, including the Expansion Pack ✅ reported working (2026-10-05)

Proven in the owner's session: the retail campaign runs, and Virtual Voyager works (G2's bar met). One
item of the bar remains unconfirmed: a save surviving a switch between modes inside the client.

The original game as a *mode*, not a stepping stone. Bar: the campaign runs; the Virtual Voyager decks,
turbolift and station menus work (G2's bar); retail configuration and saves behave; and a save survives
switching modes inside the client. Needs sessions; almost no new code.

## G7 — retail multiplayer, LAN and over a VPN 🔶 server half proven

**Server half proven 2026-10-05**, headless: cMod v1.30's dedicated server locates the retail data
(19,451 files across 5 pk3s, 2,268 shaders), loads `hm_borg1` with AAS bot navigation, and opens 32
client slots. Staged in `build/cmod`, launched by `scripts/run-cmod.sh`; evidence in
`docs/evidence/g7-cmod-staged.md`.

Still needs a human: the client's menus, the server browser, connecting to a local server, and input.
The bar is a match between two machines on a LAN and the same over a VPN address, with bots filling the
population.

**The control fault from the first playtest is no longer ours to close.** It appeared in our engine's
Holomatch path, and mode 2 now runs cMod's client, whose changelog already carries the connect-path
fixes ours lacked. The open question is narrower: whether our engine keeps a Holomatch path at all, or
whether modes 1 and 3 simply do not need one.

Agreed ordering: **G6 and G7 before G4 and G5.** They are cheap, they make the client useful now, and
they are the recruitment path for the multiplayer programme — Track D's hardest constraint is population.
G3 proceeds in parallel, because its code can be written without a session even though its criteria need one.

## G3 — reactive crew 🔶 measured, awaiting the owner's judgement

**Implemented and measured 2026-10-05.** Six crew hold six posts on `tour/deck04` through a ten-minute
headless run: 100% coverage at all 121 samples, every post reached within 6.1 s, no stuck or
out-of-world events, every address acknowledged within 1.5 s, the deck's four scripted characters
undisturbed, a script run on a post-holder honoured and the member returned to duty, crew state
identical across save and reload, 25 bytes of save per crew member against a budget of 256.
Evidence: `docs/evidence/g3-reactive-crew-measured.md`; reproduce with `scripts/g3-measure.sh`.

The bar it was measured against, from the charter: 5-10 NPCs, one deck, no new animations, reused barks,
posts and acknowledgement, with the measurable criteria (post coverage over a sampled run, bounded time to
post, zero navigation failures, save/load restores posts and schedules, an explicit per-NPC save-size
budget, no ICARUS script regressions).

**Foundation added 2026-10-05:** the ship is a **state model**, not the maps -- see `docs/ship-model.md`.
Its first artifact (the ship blob, the crew record, the system table, the clock, and the per-deck
load/unload adapter) lands inside G3, because G3's crew records must be written into that shape from the
first line of code rather than retrofitted. Crossing decks therefore becomes a field on a crew record, and
Borg assimilation becomes a state transition on one. G3's own bar is unchanged.

What is left is the owner's:

- **Does the deck feel inhabited?** `scripts/run-scenario.sh`, in a logged-in session.
- **Is adding crew acceptable for this gate?** None of deck04's own eight NPCs can be given a post —
  four are permanently scripted, four are seated props spawned in solid — so the scenario declares
  six crew of existing character types and the layer spawns them. No new art, voice or animation;
  but the charter says "named crew already placed", and that is not what was measured.
- **Frame time on the target desktop.** Measured here as time inside the game frame (0.34 ms with the
  crew, 0.24 ms without); a frame rate needs a real renderer.

The layer is `module/crew/`, attached by `patches/0005`, authored through the manifest's `crew`
section. It is off by default (`g_crew 0`), and with it off the module behaves and saves exactly as
before. Arbitration precedence — scripted sequence > direct combat/reaction > director override >
duty/routine > idle/social — was written before the code, as required, and is unit-tested.

## G4 and G5 — replaced by the ship programme (2026-10-05)

The owner redirected the programme: the ship must work as a system before a crew can be given lives
aboard it. `docs/ship-programme.md` records the decisions and the gates S1–S10 that replace G4 and
G5. G3 stays open and work proceeds regardless, by his decision.

| gate | state | evidence |
|---|---|---|
| **S1** ship core | ✅ done 2026-10-05 | `docs/evidence/s1-ship-core.md` |
| **S2** Engineering console in game | 🔶 built and verified headless; awaiting the owner at the console | `docs/evidence/s2-engineering-console.md` |
| **S3** whole-ship map | 🔶 one map, fifteen decks (five generated with blockouts), toured by turbolift 15 of 15 with the retail menu reading the ship's fifteen-deck list; game frame **2.6 ms average, 8.7 ms worst** on the latest run (the whole-ship frame time is measured within a 60 fps budget — see the evidence); awaiting a person walking it | `docs/evidence/s3-merged-map-measured.md`, `scripts/s3-check.sh` |
| **S4** every station's console | 🔶 each station's purpose is in the core and on its console: Tactical fires, the transporter beams and recalls, astrometrics surveys, the Conn lays in a course, Sickbay reads its ward in triage order; the ship's live state is drawn at the panel and painted onto the panel's texture (`patches/0014`); the non-station panels (log, ready room, personnel, replicator) open the ship's own screens. A person at a panel remains | `docs/evidence/s4-station-consoles.md`, `scripts/s4-check.sh`, `scripts/s4-glance-check.sh`, `scripts/s4-panel-check.sh` |
| **S5** crew daily lives | 🔶 the crew embodied on the player's deck are whoever the routine has there, arriving and leaving with it; the **cap is raised to 24 and measured** (all 24 embodied follow the routine, all reach their place within the meal hour). Station markers sit at the generated decks' own fixtures; published-deck markers are blocked (a marker at their interface panels leaks — sky/trigger brushes) and need chosen open-space origins by a person | `docs/evidence/s5-stations.md`, `scripts/s5-check.sh` |
| **S6** damage, repair, resources | 🔶 in the core: crewed system repair and hull sealing that cost parts, airless and burning decks and radiation from a failing core as casualty causes, fighting wounds, sickbay recovery and triage, rations; **a deterministic fourteen-day soak** with the invariants checked daily (`TestSoak`). The crew layer embodies the repair and firefighting parties, and **damage is visible in the world**: a damaged system sparks where it is worked (`SyncDamage`, `scripts/damage-check.sh`) and a burning deck is smoke and flame across it (`SyncFire`, `scripts/fire-check.sh`). **Injury causes** run from air, fire and fighting wounds to radiation, an exploding console and a poisoned site | `docs/evidence/s6-damage-and-casualties.md`, `scripts/damage-check.sh`, `scripts/fire-check.sh` |
| **S7** intruders and hacking | 🔶 in the core: boarders, contested control, hijacking, security response, the breach puzzle; boarders are hostile bodies on the player's deck and the ship's security are embodied as bodies too; **boarder kinds and objectives** (raider/Borg/hunter, sent for a deck). The puzzle played by hand needs a session | `docs/evidence/s7-intruders-and-control.md`, `scripts/s7-check.sh` |
| **S8** the Borg | 🔶 rules in the core: drones convert decks and take crew, assimilated systems are lost outright, stripping costs hours and parts; drones are bodies on the player's deck. **Runtime asset replacement** (the first hard problem) is **built and verified on the merged ship's generated decks**: a deck the simulation assimilates turns Borg in the world **part by part** (four sections, turning in order as assimilation rises) and is stripped back (`BorgAssets`, `scripts/borg-deck-check.sh`); the published decks still need the stitcher pass or the engine section tag | `docs/evidence/s8-the-borg.md`, `scripts/s7-check.sh`, `scripts/borg-deck-check.sh` |
| **S9** the outside | 🔶 in the core (2026-10-07): an opponent with targetable weapons, engines and a shield generator, and raider/warship/Borg kinds; choices at a beacon (hail, trade, answer a distress call, run); pursuit that stops the ship sitting still to repair; three sectors and an end to reach; jumps from the Conn and torpedoes/targeting from Tactical; more than one contact at a time (a raider's wingman). Engine-side feedback: the screen shakes on a hit, a live in-world viewscreen draws the contact's image beside the panel, and the alert klaxon plays (the viewscreen is a drawn schematic, not an engine render of the model) | `docs/evidence/s9-the-outside.md`, `scripts/s9-check.sh`, `scripts/viewscreen-check.sh` |
| **S10** play modes, roles, character creation | 🔶 rules in the core: ironman/holodeck, three clocks with wall-clock catch-up (demonstrated in the game), clearance by rank and department, three player roles, character creation, enforced; standing orders from a command console; a personnel screen creates the character; the log/ready-room/personnel **panels open the ship's screens** from a map interface; **the player's character is the body the player walks in** (head, torso and legs set from the crew record and department when a character is chosen), and **the command console carries the career** (shows the character, and `P` confirms a field promotion from the ship). The owner's playthrough remains | `docs/evidence/s10-modes-and-roles.md`, `scripts/s10-check.sh`, `scripts/s4-check.sh`, `scripts/playerbody-check.sh`, `scripts/promote-check.sh`; **the sleep state and the two exits** (the owner's ruling of 2026-10-06) in `docs/evidence/clocks-and-exits.md`, `scripts/clock-check.sh` |

## Adopted from RPG-X prior art (2026-10-04)

Four decisions recorded in `docs/prior-art-rpg-x.md`, each landing somewhere concrete:

- **rank and permission model → Track D** as the starting design for ship authority
- **embedded SQLite persistence → Track D and G3** as the storage pattern
- **emote / interaction vocabulary → capstone and G4**
- **content headroom → Track B, before capstone content work**: `MAX_CONFIGSTRINGS` 1024 against their
  4096, `MAX_GENTITIES` 1024 against 2048, `MAX_MODELS` 256 against 512 — with the two constraints the
  source states (models and sounds ride the network as 8 bits; configstrings and
  `MAX_GAMESTATE_CHARS` must move together). First task: measure configstring usage on a loaded
  campaign map, so we know how much of the budget retail already spends.

## G5 — capstone scenario ⏳ pending

---

## Community harvests (handoff §1b)

**Harvest A — Elite Reinforce fixes and tools: ✅ closed 2026-10-07, no code needed.** The mandate
listed four single-player fixes and two authoring tools as absent. They are not: the pinned upstream
(`VoyagerSP-Android` @ `0d8942e8`) already inherited all of them — the borg1 freeze guard and its
`g_fixFreezeBorg1` cvar (on by default), the forge3 `AimAtTarget` height fix, the menu-softlock
`ingameFlag` reset, the `saveholodeck` command, `cg_highlightDeathScripts`, and `g_showPaths`. The
audit checked our module and patches and missed the base. Evidence: `docs/evidence/harvest-a-elite-reinforce.md`.

**Harvest B — RPG-X's entity classes: 🔶 first class adopted and verified.** `target_shaderremap`
(a placed entity that toggles a shader mapping at run time, riding `gi.RemapShader`) is implemented
in `module/ship/lwh_entities.cpp`, registered by `patches/0015` (as attach points only, off without
`LWH_MODULE_DIR`), documented in the validator dictionary, placed on the generated decks, and proven
by `scripts/shaderremap-check.sh` (`g_shipTest 32`): it spawns and firing it swaps the shader and
swaps it back with no renderer "not found". This is also the mechanism candidate for the first hard
problem. The remaining classes we need are adopted each with a dictionary entry and a fixture.
Licence: RPG-X code is adoptable with credit to UberGames (see `NOTICE`); assets are not. Details:
`docs/evidence/harvest-b-rpgx-classes.md`.

---

## Gaps approved 2026-10-05 — the next work, in order

The owner reviewed `docs/gap-analysis.md` and agreed with the whole of it. This section is the order, so the
work is inherited rather than rediscovered. Each of the first five carries its contract and its acceptance
check, per the five-part contract in `docs/ship-systems.md`.

### First five -- close these before the rest

**1. Morale and fatigue.** *State:* per crew record, morale and fatigue with their drivers -- sleep, food,
losses, hopelessness, warmth. *Control:* the watch roster, the galley, the holodeck, time off, and what the
crew are told. *Visible:* performance at post (acknowledgement latency, error rate) and the crew's manner.
*Failure:* a crew member who stops caring, or one who breaks. *Location:* everywhere, because it is a property
of people. *Acceptance:* a bad month measurably degrades posts, a good one measurably restores them, and the
log can say why.
**First slice 2026-10-07** (`docs/evidence/gap-morale-and-fatigue.md`, `tests/ship/test_ship_core.cpp`):
morale on the crew record (save format **version 8**), eased by rest, food, recreation, alert and losses; a
system's output is staffed by morale/fatigue-weighted hands; the Sickbay readout shows the averages. Manner,
and the log's "why", remain.

**2. Triage and the sickbay queue.** *State:* casualties with severity, beds (three standard, one surgical),
the surgical bay's force field, medical supplies. *Control:* who is treated first, who waits, what is
replicated. *Visible:* who is on a bed, who is on the floor, who is dead. *Failure:* a casualty deteriorating
because no bed freed up. *Location:* sickbay, deck 5. *Acceptance:* casualties exceeding beds produce a
decision rather than a queue that resolves itself.
**First slice 2026-10-07** (`docs/evidence/gap-triage-and-sickbay.md`, `tests/ship/test_ship_core.cpp`):
severity and a derived under-care flag (save format **version 9**), four beds, medical supplies, and a
triage standing order — worst first or rank first — with the untreated deteriorating and able to die; the
Sickbay readout and the command console show it. A triage screen, replication as the restock, and the
surgical force field / EMH remain.

**3. Air and endurance clocks.** *State:* atmosphere per compartment, plus battery and auxiliary endurance for
the ship. *Control:* sealing, force fields, rerouting, power allocation. *Visible:* a countdown wherever the
problem is. *Failure:* a compartment that runs out, a ship that goes dark. *Location:* environmental control
(deck 12), the EPS grid, every sealed compartment. *Acceptance:* a breach produces a number, and the number
moves when the crew act.
**First slice 2026-10-07** (`docs/evidence/gap-air-and-endurance.md`, `tests/ship/test_ship_core.cpp`):
`MinutesOfAir` per deck and `MinutesToDark` for the ship, read from the same rates the sim uses; a force
field over a breach (`SetForceField`, save format **version 10**); the clocks published as `lwh_ship_clocks`
and drawn on the Operations console. Force fields from the panel, endurance per source, and the clocks at
deck 12 itself remain.

**4. The log as a browsable artifact.** *State:* log entries with time, author, subject and fact. *Control:*
each post reads its own scope, command sees all, the player can search. *Visible:* the log itself -- and it
retires the standing assumption in nearly every other document that something "was recorded". *Location:*
any console, the ready room, the captain's log. *Acceptance:* after a run, a player can reconstruct what
happened and when from the log alone.
**First slice 2026-10-07** (`docs/evidence/gap-the-log.md`, `tests/ship/test_ship_core.cpp`): a bounded,
saved log of `{time, who, scope, what}` (save format **version 11**); the events that write (alerts, damage,
breaches and seals, force fields, orders, casualties, boarding, jumps, fire); `ship log [count] [scope]`
reads it newest-first with a scope filter, and `g_shipTest 16` reproduces a transcript. A scrollable screen,
named authors, and the captain's log remain.

**5. Tricorders and away-team kit.** *State:* kit as stores entries (tricorders, phasers, EV suits, medical
kit) with charge and condition. *Control:* loadout at the transporter room or shuttlebay, scanning in the
field. *Visible:* scan results at human scale -- the same writes to the chart, over a smaller radius.
*Failure:* a dead charge, lost kit, or a scan that reads wrong on a low battery. *Location:* transporter
rooms, shuttlebay, every away site. *Acceptance:* an away mission can be solved by scanning rather than by
shooting.
**First slice 2026-10-07** (`docs/evidence/gap-tricorders-and-kit.md`, `tests/ship/test_ship_core.cpp`):
the away kit on Stores (tricorders, phasers, EV suits, a shared charge; save format **version 12**),
`LoadAwayKit`/`Scan` — a scan writes to the chart, misreads below a fifth charge, and dies at zero — the kit
on the Operations console and every loadout/scan in the log. The away site itself, kit condition and loss,
and the acceptance run remain.

**All five gaps now have a first slice** (2026-10-07): morale and fatigue, triage and the sickbay queue,
air and endurance clocks, the log as a browsable artifact, tricorders and away-team kit. Each is
core-plus-console, tested, and honest about what is left; the next work is the deck build order in
`docs/ship-master-map.md` (deck 12 environmental control and deck 13 life support first).

**Deck 12 is re-dressed from `tour/deck11`** (2026-10-06, `docs/evidence/deck12-redress.md`): the first
of the five absent decks built against its brief, as a copy of the reuse target the brief names -- main
engineering -- changed to be this room (reduced vertical scale, the warp core replaced by an atmosphere
plant, a Jefferies tube exit to deck 11, the watch console at the entrance), with the parts list
measured (`docs/locations/deck12-parts-list.md`) and the emergency lighting state in the world.
`tools/shipmap/dressdeck12.py`, `scripts/deck12-check.sh`, `scripts/emergency-check.sh`. What closes it
is the owner's walkthrough; the tool is the procedure the other four decks will follow.

**The five gaps, to their full criteria (2026-10-07, later the same day).** The first slice's
remaining items are now built and evidenced by `scripts/gaps-check.sh` and `TestGapCompletions`:
morale's "the log can say why" (a periodic mood line naming the driver); the triage screen
(`ui_lwh_triage`), replication restocking medical supplies, and the surgical bay's force field;
force fields from the Operations panel and battery/auxiliary endurance as their own numbers; the
browsable log screen (`ui_lwh_log`), named authors (`AuthorFor`), and the captain's log
(`ship captain`); and the tricorder reading the ship's own compartments (`ScanCompartment`) with kit
condition. Save format **version 14**.

What remains is content, not rules: the crew's *manner* (barks wired to morale — needs the crew
layer, session to judge), the EMH (the mobile-emitter backlog item), the environmental-control
countdown at deck 12 itself (the deck build), and a walkable away site (content; the acceptance run
needs it and a session). The model says so in each evidence document.

### The month report, the promise and the lie, and the toll

`docs/memory-and-consequence.md` and `docs/the-record-and-the-log.md`, implemented 2026-10-06
(`docs/evidence/the-month-report-and-the-toll.md`, `TestMonthReportAndToll`,
`test_ship_core --month`). Save format **version 44**. The memory layer was already in the record
since version 18; this is its missing wiring, not a rebuild.

- the promise writes `MEM_PROMISE` with the claim **held** so it can mature — kept, broken, or lapsed
  by deadline — moving the mark's valence and the bond, and logging the reason in the crew member's
  voice ✅
- a signed report that contradicts a `MEM_SAW` mark writes `MEM_LIE` **in the witness**, naming the
  signer ✅
- the toll is **paid downward**: only a witness under the signer, reading a report published to the
  crew, loses trust; the same falsehood filed upward costs nothing from below. Divergent allegiance
  amplifies, alignment suppresses ✅
- the **month report** is drafted honestly from the record, edited by the player (strike, soften,
  add), signed per scope; the record keeps the diff, the crew see the published version, and the
  headline is the navigation counter's change since the last entry ✅
- **purge** empties the published logs and **orphans** the `MEM_LOG`-sourced marks rather than erasing
  them ✅
- save and load carry the report, its diff, the promises and the orphaned mark byte-for-byte ✅

Remaining (specified, not built): the official and personal logs as separate stores; the meeting brief
built per participant from marks and log; assimilation taking the personal log **and** the access
levels, with the Collective speaking in the assimilated person's voice.

### The navigation counter

`docs/navigation-counter.md`, implemented 2026-10-06 (`docs/evidence/the-navigation-counter.md`,
`TestNavigation`, `test_ship_core --nav`, `scripts/nav-check.sh`). Save format **version 45**. It
replaces the misnamed stand-in (`NavigationCounter` returned the fuel range) with the projection the
document asks for, over state already in the save.

- distance to Earth from the position model (`sectorNumber`/`SECTORS_TO_CROSS`,
  `beacon`/`SECTOR_BEACONS`), in the goal's own unit ✅
- the estimated time projected from the capability the ship can actually sustain — crystal, drive,
  crew, charted resupply — not an arithmetic quotient ✅
- the **nominal** and **current-capability** figures, both shown, with the gap visible ✅
- the **change since the last entry**, the derivative, and the month report's headline ✅
- recorded in the log about weekly, and the read survives save and load ✅
- on the panel and the HUD glance without a special mode, on every station console, and queryable as
  `ship nav` ✅
- **command sees the forecasts** — the estimate under each available course ✅
- a unit test, and `scripts/test.sh` and `scripts/check.sh` both exit 0 ✅

Remaining: the human check (whether the arrow *feels* like a hard month) is the owner's; and the
per-route risk split in the document's own example is projected as the estimate per course, not two
numbers per course.

### Then, in rough order

**First batch done (2026-10-07)** — `docs/evidence/backlog-materials-and-crew.md`, `scripts/backlog-check.sh`,
`TestMaterialsAndTravel`, `TestCrewJusticeAndBorg`:

- the **tractor beam** (strip a derelict, lock a contact), **salvage** and **fabrication** as the
  materials economy ✅
- **structural integrity** as a gate on hull loss and travel safety ✅
- the **navigational deflector**: a weak one lets dust through on a jump ✅
- **inertial dampers**: an undamped jump shakes the crew ✅; **the EMH** (needs the computer core) ✅
- **grief**: a death is notified and the quarters sealed, and a funeral opens them and lifts the crew ✅
- **discipline and justice**: the brig ✅ (the hearing and Prime Directive consequences remain)
- **training and qualification** as the source of credentials ✅
- the **Maquis split**: the faction field ✅ (resentment and integration as an arc remain)
- the **player's own body**: `PlayerIncapacitated` ✅ (the model in the world remains)
- the **player's career**: promotion by whoever commands ✅
- **Borg strategic awareness**: a Borg vessel adapts to our weapons ✅; a **persistent pursuer** ✅
- **trade** (a trader beacon) ✅
- **Relationships and memory**: the bounded per-character mark set of
  `docs/memory-and-consequence.md` (provenance, valence, salience, decay, eviction), bonds derived
  from remembered valence, and the galley's food fabrication ✅
- **Resource acquisition**: mining a resource belt for material and siphoning fuel ✅ (EVA remains)
- **Population pressure**: survivors and refugees taken aboard eat, breathe and crowd ✅
- **Justice**: a hearing (acquittal or conviction) ✅ (Prime Directive consequences remain)
- **The holodeck's uses**: recreation, training, therapy, forensic reconstruction ✅; **memory read by
  the simulation** (trauma drags on morale) ✅
- **Living conditions**: quarters quality colours the mood, and can be improved ✅ (bunking as a
  mapped place remains)
- **The holodeck programme that will not end**: time in the program accumulates and command can pull
  a crew member out ✅
- **Nacelle pylons**: damageable, and a ship without them cannot go to warp ✅
- **The mobile emitter** as an artifact, strengthening the EMH ✅
- **EVA** as a way to acquire what mining cannot reach ✅
- **The Prime Directive**: observing a pre-warp civilisation, or interfering and owning the mark ✅
- **The Maquis split as an arc**: resentment seeds from the roster, drags on morale, and command can
  reconcile the two factions into one crew ✅
- **Airponics and the galley**: the galley fabricates food, and the airponics bay grows it without the
  replicators ✅ (the bay as a mapped place remains)
- **Borg strategic awareness**: awareness rises with every Borg contact, makes the next sector more
  theirs, and speeds their adaptation ✅
- **Memory read by the simulation**: a grudge toward whoever commands measurably reduces a post's
  output, alongside trauma (the marks move the sim, not only answer a query) ✅
- **The job queue** of `docs/crew-work.md` as a visible, saved, ordered work list (repair/seal/reclaim/
  **build** with kind, target, progress and priority; console `ship jobs`; a build job fabrics parts
  from material) and **deferred maintenance** (an undermanned station fails, traceably) ✅
- **The dilithium constraint** that forces exploration (`docs/exploration-and-science.md`): the
  crystal's life falls with warp use, Engineering recomposites it until the ceiling will not rise
  again, and a new crystal is found by mining a belt, trading, salvaging or researching a better one;
  no crystal, no warp (`ship dilithium|recomposite|acquire`; `TestDilithium`,
  `scripts/dilithium-check.sh`) ✅
- **The warp core cascade** (`docs/failure-is-content.md`, `docs/damage-and-budgets.md`): coolant loss
  -> overheat -> falling containment -> a breach countdown, visible on the console and interruptible
  (coolant, shutdown, restart, eject, repair); ignored, the breach is the one unwinnable end
  (`ship core`, `TestCoreCascade`, `scripts/core-check.sh`); and **every system's three named failure
  states** (degraded, offline, destroyed), each change written to the log (`TestSystemStates`) ✅
- **Condition sets the odds, and stress sets the severity** (`docs/failure-is-content.md`, the owner's
  ruling of 2026-10-06): the general mechanism on every system -- condition (health capped by power)
  sets the chance of an anomaly, zero in the top tenth, and the load at the moment of use sets the
  severity (degraded, acute, catastrophic); the console states the system's condition before the act,
  the log carries the chain, and at the severe end the console lets go at the operator. The
  transporter is worked end to end: a nominal beam mangles no one, and a degraded one yields a
  misaligned beam, a mangled arrival, a copy or a merge, each written to the records
  (`TestConditionOdds`, `TestTransporterAnomaly`, `scripts/condition-check.sh`,
  `docs/evidence/condition-sets-the-odds.md`; save format 42) ✅
- **Probes** (`docs/exploration-and-science.md`): the safe way to look at something hostile -- a
  probe charts a target without the ship going there, spending one of the complement; sometimes lost
  (`ship probe`, `TestProbes`, `scripts/probe-check.sh`) ✅; and **phenomena** -- one anomaly per
  sector, hidden attributes revealed one scan at a time, a correct response rewarded and a wrong one
  damaging (`ship study`, `TestPhenomenon`, `scripts/phenomenon-check.sh`) ✅; probe classes and
  response-demanding telemetry, and a response depending on the revealed set, remain
- **Shuttles** (`docs/shuttles.md`): supported, not pilotable -- the bay's complement and where each
  is, an away shuttle with its manifest and cargo, a loss (stranded or lost with its crew) becoming a
  build job in the second bay, and a hit on the bay wrecks what is parked (`TestShuttles`,
  `scripts/shuttle-check.sh`) ✅
- **The Borg incursion's keystone** (`docs/borg-incursion.md`): a per-deck `controller`
  (crew/borg/contested/sealed/uninhabitable), `compromised` and `dwell`; the **hard "no dwell, no
  write" gate in three thresholds** (120 s compromise, 240 s contested, 360 s seized); and the
  **clean intercept recorded as a win** when intruders are cleared before they touch a system; and
  the **counter-play kit** (rotate the phaser modulation to break adaptation; a vinculum raid that
  severs coordination at a cost; **force fields rated 1-10**, a level-10 field cutting a drone from
  the Collective); and **de-assimilation** in a narrow window, never whole (a lasting scar and a
  mark); and a **security squad** command sends to retake a deck, advancing one at a time
  (`TestIncursion`/`TestSquad`/`TestForceFieldKit`/`TestCounterPlay`/`TestDeassimilation`,
  `scripts/controller-check.sh`, `scripts/squad-check.sh`, `scripts/forcefield-check.sh`,
  `scripts/counterplay-check.sh`, `scripts/recovery-check.sh`) ✅

Still to do, in order:

- the **airponics bay and the galley** as mapped places and job sites (their food production is in)
- **living quarters and bunking** as a mapped place (the quality that colours morale is in)
- ~~the **player's body and career in the world**~~ **Done 2026-10-07** — the model the player walks
  in (`ApplyPlayerBody`, `scripts/playerbody-check.sh`) and the career in a UI (the console's field
  promotion, `scripts/promote-check.sh`)
- ~~**memory driving spoken dialogue**~~ **Done 2026-10-07** — asked to account, a crew member says a
  line drawn from their strongest mark (`scripts/speech-check.sh`)
- **time travel explicitly refused**

## Residuals (named, queued, not forgotten)

- ~~**The Game Development Kit was no longer on the playtest host**~~ **Closed 2026-10-05** — it had
  been extracted under `/tmp` and was lost. `scripts/fetch-gdk.sh` now restores it into `build/gdk`
  from the Internet Archive, checksummed, and G0 was re-run against it: validator negative tests all
  pass; dictionary 262 classes + 2 templates, 237 of 239 used classes covered across 106 map sources
  (unchanged); corpus 2,016 of 2,024 scripts compile and read back, the 8 rejections being the three
  named in `docs/evidence/g0-script-compiler.md` plus five files that are not scripts. This archive's
  script set is smaller than the 2,408 files counted originally, which included the GDK's own copies.
- **The entity dictionary's flags and keys were wrong until 2026-10-05** — a header pattern ran on
  into each description. Class names, and so G0's coverage figures, were unaffected. Regenerated
  from the restored GDK: 211 classes with documented keys, 232 with spawnflag sets.
- **Crew are stopped by some doors — probably the ones meant to be shut.** Two suggested starting
  positions on deck04 were one waypoint from their posts and unreachable. Reading the source and
  the map afterwards: ordinary doors open for any client, NPCs included; 26 of the ship's doors are
  security doors and 34 open only by script (the turbolift's among them). Not yet confirmed in the
  engine; it is S5's first task.

- **~30 `Cmd_AddCommand: … already defined`** on each level transition. Harmless, but it means the SP
  path's console registrations are not idempotent.
- **`MAX_PACKET_USERCMDS`** printed twice at shutdown. Cosmetic.
- **Dependencies no longer live in `/tmp`** — the tree is in `build/deps`, on the binary's RPATH, and
  `ldd` with no environment set reports zero missing libraries. Closed.
- ~~**No cutscene playback.**~~ **Closed** — FFmpeg is available from the plain archive with the version
  pinned (the ESM build was merely the preferred candidate), and the engine now links Bink playback.
  See `docs/evidence/deps-durable-and-cutscenes.md`. Whether a cutscene *plays* still wants a session.
- **126 script references the GDK corpus lacks** — the validator found them; most likely the
  expansion's scripts. Worth resolving before G2 content work.
- **Retail PC saves are incompatible** with the port (upstream documents this). Start new campaigns.
