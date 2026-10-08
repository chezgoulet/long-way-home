# Gate ledger

> What is closed, measured and next. **The order of the remainder is not this document** — it is
> `docs/path-to-playtest.md`, which owns sequencing while this owns state.

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

### The way in — the menu line and the launcher ✅ built 2026-10-08

`docs/the-entry-point.md` Part one (the walkthrough's **G1**) is built on `feat/the-way-in`: the
main menu carries a **"Long Way Home"** line beside the retail buttons, and a start-state selector
containing **one entry, the canon default** (the first half of **G2**; the configurator of Part
three is the next pass). `scripts/run-lwh.sh` is the launcher. The run's values live once, in
`configs/lwh-start.cfg`, executed by both the launcher and the menu, so there is no second copy to
drift; `g_shipDeckPitch` is resolved there and is `stitch.py`'s own pitch (`3072 = 3 x 1024`). The
cvars keep their off-by-default values: with the ship map loaded and the config unset, the
simulation does not start. The campaign was re-proven (`+map borg1` loads and the player connects).
Evidence: `docs/evidence/the-way-in.md`, `patches/0020-main-menu-long-way-home.patch`,
`module/ui/ui_lwh_start.cpp`.

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
| **S4** every station's console | 🔶 each station's purpose is in the core and on its console: Tactical fires, the transporter beams and recalls, astrometrics surveys, the Conn lays in a course, Sickbay reads its ward in triage order; the ship's live state is drawn at the panel and painted onto the panel's texture (`patches/0014`); the non-station panels (log, ready room, personnel, replicator) open the ship's own screens; **the 2026-10-07 screen sweep is registered here** — the personal-log screen, the month-report editor, the job-queue board, the workable chart, the tricorder survey, the beacon keys and the phaser-setting ladder (`docs/evidence/every-screen.md`, save version 49). A person at a panel remains | `docs/evidence/s4-station-consoles.md`, `scripts/s4-check.sh`, `scripts/s4-glance-check.sh`, `scripts/s4-panel-check.sh` |
| **S5** crew daily lives | 🔶 the crew embodied on the player's deck are whoever the routine has there, arriving and leaving with it; the **cap is raised to 24 and measured** (all 24 embodied follow the routine, all reach their place within the meal hour). Station markers sit at the generated decks' own fixtures; published-deck markers are blocked (a marker at their interface panels leaks — sky/trigger brushes) and need chosen open-space origins by a person | `docs/evidence/s5-stations.md`, `scripts/s5-check.sh` |
| **S6** damage, repair, resources | 🔶 in the core: crewed system repair and hull sealing that cost parts, airless and burning decks and radiation from a failing core as casualty causes, fighting wounds, sickbay recovery and triage, rations; **a deterministic fourteen-day soak** with the invariants checked daily (`TestSoak`). The crew layer embodies the repair and firefighting parties, and **damage is visible in the world**: a damaged system sparks where it is worked (`SyncDamage`, `scripts/damage-check.sh`) and a burning deck is smoke and flame across it (`SyncFire`, `scripts/fire-check.sh`). **Injury causes** run from air, fire and fighting wounds to radiation, an exploding console and a poisoned site. **The abandonment list is registered here**: `Ship::losses`, `WriteOff`, `ship losses`, the `GIVEN UP` column and the painted panel line (`docs/evidence/abandonment-list.md`, save version 41) | `docs/evidence/s6-damage-and-casualties.md`, `scripts/damage-check.sh`, `scripts/fire-check.sh` |
| **S7** intruders and hacking | 🔶 in the core: boarders, contested control, hijacking, security response, the breach puzzle; boarders are hostile bodies on the player's deck and the ship's security are embodied as bodies too; **boarder kinds and objectives** (raider/Borg/hunter, sent for a deck). The puzzle played by hand needs a session | `docs/evidence/s7-intruders-and-control.md`, `scripts/s7-check.sh` |
| **S8** the Borg | 🔶 rules in the core: drones convert decks and take crew, assimilated systems are lost outright, stripping costs hours and parts; drones are bodies on the player's deck. **Runtime asset replacement** (the first hard problem) is **built and verified on the merged ship's generated decks**: a deck the simulation assimilates turns Borg in the world **part by part** (four sections, turning in order as assimilation rises) and is stripped back (`BorgAssets`, `scripts/borg-deck-check.sh`); the published decks still need the stitcher pass or the engine section tag | `docs/evidence/s8-the-borg.md`, `scripts/s7-check.sh`, `scripts/borg-deck-check.sh` |
| **S9** the outside | 🔶 in the core (2026-10-07): an opponent with targetable weapons, engines and a shield generator, and raider/warship/Borg kinds; choices at a beacon (hail, trade, answer a distress call, run); pursuit that stops the ship sitting still to repair; three sectors and an end to reach; jumps from the Conn and torpedoes/targeting from Tactical; more than one contact at a time (a raider's wingman). Engine-side feedback: the screen shakes on a hit, a live in-world viewscreen draws the contact's image beside the panel, and the alert klaxon plays (the viewscreen is a drawn schematic, not an engine render of the model) | `docs/evidence/s9-the-outside.md`, `scripts/s9-check.sh`, `scripts/viewscreen-check.sh` |
| **S10** play modes, roles, character creation | 🔶 rules in the core: ironman/holodeck, three clocks with wall-clock catch-up (demonstrated in the game), clearance by rank and department, three player roles, character creation, enforced; standing orders from a command console; a personnel screen creates the character; the log/ready-room/personnel **panels open the ship's screens** from a map interface; **the player's character is the body the player walks in** (head, torso and legs set from the crew record and department when a character is chosen), and **the command console carries the career** (shows the character, and `P` confirms a field promotion from the ship). The owner's playthrough remains | `docs/evidence/s10-modes-and-roles.md`, `scripts/s10-check.sh`, `scripts/s4-check.sh`, `scripts/playerbody-check.sh`, `scripts/promote-check.sh`; **the sleep state and the two exits** (the owner's ruling of 2026-10-06) in `docs/evidence/clocks-and-exits.md`, `scripts/clock-check.sh` |

## Adopted from RPG-X prior art (2026-10-04)

Four decisions recorded in `docs/prior-art-rpg-x.md`, each landing somewhere concrete:

- **rank and permission model → Track D** as the starting design for ship authority
- **embedded SQLite persistence → Track D and G3** as the storage pattern
- **emote / interaction vocabulary → capstone and G4**
- **content headroom → Track B, before capstone content work**: `MAX_CONFIGSTRINGS` 1024 against
  their 4096, `MAX_GENTITIES` 1024 against 2048, `MAX_MODELS` 256 against 512 — with the two
  constraints the source states (models and sounds ride the network as 8 bits; configstrings and
  `MAX_GAMESTATE_CHARS` must move together). First task: measure configstring usage on a loaded
  campaign map, so we know how much of the budget retail already spends.
  **Closed 2026-10-07** (`docs/evidence/engine-content-headroom.md`, `patches/0017`). Measured on the
  loaded maps: retail `borg1` spends 104 of 1024 configstrings, 46 of 256 models, 53 of 256 sounds;
  the merged ship spends 213 of 4096 configstrings, 141 of 256 models, 67 of 256 sounds, **3896 of
  4096 gentities (95%)** and **15355 of 16384 vis-clusters (94%)**. The limits are raised:
  `MAX_CONFIGSTRINGS` 4096 and `MAX_GAMESTATE_CHARS` 64000 (moved together, patch `0004`),
  `MAX_GENTITIES` **8192** by `GENTITYNUM_BITS` 10→12→13 (derived, patches `0004`/`0008`/`0018`; the
  last raise, ahead of deck 6, is `docs/evidence/raise-the-gentity-ceiling.md`). **`MAX_MODELS`
  is left at 256**: the model index rides the network as 8 bits (`NETF(modelindex)`, `msg.c`), so
  widening it is a protocol change, and there is no measured pressure. The binding limit was entities,
  not configstrings; the fifth re-dress (deck 6) was projected to cross 4096 gentities, and the raise
  to 8192 was done before it rather than during it. A save round-trips and both `scripts/test.sh` and
  `scripts/check.sh` exit 0.

- **crowded-room snapshot performance → measured 2026-10-07, the answer belongs to Track D**
  (`docs/evidence/performance-survey.md`, `patches/0019`). The design (§8) names "everyone in one
  room" as the case that defeats the engine's culling and needs a designed answer. Measured on the
  loaded ship: the multiplayer cap is `MAX_SNAPSHOT_ENTITIES` **256** (`qcommon.h:142`), silently
  truncated past (`sv_snapshot.c:344`); the single-player bridge's cap is the module's
  `MAX_ENTITIES_IN_SNAPSHOT` **1024** (`efgame/src/cgame/cg_public.h:17`), and it is **already
  spent in the ordinary case** — the ship's PVS passes ~1,166–1,202 entities at every point tried and
  the snapshot writes 1024 on every tick. Distance-based model LOD exists and is on, but is
  render-side and cannot help the wire. The module costs **2.0 ms/frame** idle and **~3.5 ms** with
  32 co-located crew (the layer's ceiling); the snapshot build is **~0.6 ms/tick**. A real 150-body
  room and the two-machine wire were **not measured**. No lever pulled; the menu is in the evidence.

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

**Deck 13 is re-dressed from `tour/deck11`** (2026-10-07, `docs/evidence/deck13-redress.md`): the
second of the absent decks, deck 12's pattern reused rather than re-invented -- `dressdeck13.py` is
the copy of `dressdeck12.py` with deck 13's program, and the build calls both. One plant hall: the
vertical scale reduced (404 to 224), the warp core replaced by a central machinery island of
atmosphere processors, a raised catwalk along the north wall with a stair, a Jefferies tube mouth at
the far end, and the local plant panel and its post on the catwalk. Parts list
`docs/locations/deck13-parts-list.md`; the emergency lighting state in the world, alongside deck
12's, in `scripts/emergency-check.sh` (`g_shipTest 59`) and `scripts/deck13-check.sh`
(`g_shipTest 27`). What closes it is the owner's walkthrough: does it read as the plant that keeps
the ship breathing?

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

Remaining (specified, not built): assimilation taking the personal log **and** the access levels, with the
Collective speaking in the assimilated person's voice. (**The meeting brief built per participant from
marks and log is now built** — see *The meeting, phase one*, below.)

### The two logs — the official record and the private one

`docs/the-record-and-the-log.md`, implemented 2026-10-06 (`docs/evidence/the-two-logs.md`,
`TestTheTwoLogs`, `test_ship_core --personal`). Save format **version 47**. The official log was
already in; this is the **second store**, not a rebuild of it.

- the **personal log** is a store distinct from the official log: per person, `time / who / what`
  with a visibility of its own (`LOG_PERSONAL`) ✅
- **nobody else reads it** — not a post's scope, not command: `PersonalVisibleTo` is the rule in one
  place, `PersonalLog` returns one person's entries, and `ReadOfficialLog` never returns a personal
  entry, whatever the scope ✅
- the **player has one and writes into it** (`ship personal write <text…>`, `ship personal [count]`) ✅
- **the simulation writes both logs and reads neither**: the month report's draft is identical with
  and without a private entry, and no decision path consults either store ✅
- a **purge** takes the published log and leaves the private one, the `MEM_LOG`-sourced marks
  orphaned as before ✅
- both stores survive save and load byte-for-byte ✅
- a unit test, and `scripts/test.sh` and `scripts/check.sh` both exit 0 ✅

Remaining (specified, not built): assimilation taking the personal log **and** the access levels, with the
Collective speaking in the assimilated person's voice. (**The meeting brief built per participant from
marks and log is now built** — see *The meeting, phase one*, below.)

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

### The ship's budgets squared

`docs/budget-squaring.md`, implemented 2026-10-07 (`docs/evidence/the-budgets.md`,
`TestBudgetShedSequence`, `TestCorelessShip`, `TestCrystalCeilingScalesOutput`, `TestHundredCrew`,
`TestHolodeckTrap`, `TestTorpedoComplement`; `scripts/s2-check.sh`). Save format **version 50**: five
systems are added, so `SYS_COUNT` rises and older saves are invalid, as every bump has invalidated them.

- five new systems — **astrometrics, science labs, gravity plating, non-essential lighting, cargo
  handling** — at the document's demands; **demand totals 1,730** ✅
- the sources rebalanced so the core is the plant: warp core **1,400** (scaled by the crystal's ceiling),
  impulse reactors 250, auxiliary fusion 90, batteries 60; **fresh supply 1,800**, and a fresh ship leaves
  port **seventy EPS in hand** ✅
- `crystalCeiling` scales the warp core's **output**, so recomposition permanently lowers the plant — the
  owner's mechanic, marked `[inv]` in the ledger ✅
- the **shed order** with the **warp drive last of all**, after even the comforts; a test walks the
  sequence at ceilings 1.00, 0.85, 0.70 and 0.55 and matches the document's table ✅
- **coreless:** the critical four, the impulse drive and the deflector (390 of the remaining 400) and
  nothing else — no weapons, no sensors, no comms ✅
- **the batteries alone** hold life support at exactly full for three hours, not a system short of it ✅
- the **engineering console shows the budget twice**, fresh against now (`BUDGET 1800 FRESH  1800 NOW`) ✅
- the **torpedo complement is 38 and depletes**; canon's waypoints (eleven, six) are recorded ✅
- canon's checks: **operable with 100 crew** (22 posts, covered by a watch of 33) ✅ and the **holodeck
  matrix as a trap** (`JumpStartFromHolodeck`, `ship jumpstart`) ✅; **at least one deck holding air
  independently of the main grid** is measured and sized — the largest structural gap — and not built
  (20 `SYS_LIFE_SUPPORT` references, 17 `atmosphere`, 11 `MinutesOfAir`, across four files)
- `scripts/test.sh`, `scripts/check.sh` and the engine checks exit 0 ✅

Remaining: the owner's walkthrough (whether the budget feels tight rather than punishing), and the
deck-by-deck air mechanism — a lane of its own.

### The power assignment — the player decides

`docs/power-assignment.md` (the owner's ruling of 2026-10-07), implemented 2026-10-07 on
`feat/power-assignment` (`docs/evidence/power-assignment.md`, `TestThePlayerDecides`,
`TestLadderOnlyInAutoMode`, `TestOversubscriptionReported`, `TestPartialAllocation`,
`TestRecommendationAndDelegation`; `scripts/power-check.sh`). Save format **51**.

The **correction** this lane is: `feat/the-budgets` implemented a shed order (`SPECS[].prio`) as the
mechanism, which decides before the player arrives. That is the failure the owner named. What replaced it:

- each system now carries a **share** a person sets (a number, not a tier) and a **provenance** (the player,
  an officer under a standing delegation, automatic mode, or unset); operational capacity follows the share
- **nothing sheds itself**: an over-committed plant is reported as a **shortfall** and the commitments are
  honoured; the console refuses an increase that would not fit, until something is freed ✅
- the **ladder is demoted** to the policy of **automatic mode only** (off by default), and is **never applied
  to a system a person has set** — demonstrated by turning automatic mode off after setting an allocation ✅
- **damage is not policy**: a destroyed system, a dead conduit and no fuel override a person's allocation, and
  the log says "damage, not a decision" ✅
- the console shows, per system, the **share set**, the **power getting**, **what it buys** and **who decided**,
  beside the control; and committed against available with the shortfall as a number ✅
- the chief engineer's **recommendation** appears with his reasoning; accepting adopts it, refusing is recorded
  and he takes a `MEM_OVERRULED` mark ✅; a **band delegation** is grantable, held by a name, and revocable
  immediately ✅
- the **meeting seam** is `SetAllocation`, `RecommendAllocation`, `AcceptRecommendation`,
  `RefuseRecommendation`, `GrantBand`, `RevokeBand`, reading `PowerCommitted`, `PowerAvailable` and
  `PowerShortfall` ✅ — and it is now **consumed** by the meeting (see *The meeting, phase one*, below) ✅
- `scripts/test.sh`, `scripts/check.sh`, `scripts/s2-check.sh` and `scripts/power-check.sh` exit 0 ✅

Remaining: the owner's walkthrough (whether choosing feels like command rather than bookkeeping), and the staff
meeting as a **scene** and its overlay, which belong to the meeting lane (phase two/three) — the brief, the
skeleton and the seams are built (see below).

### The meeting, phase one — the brief, the skeleton, and the seams

`docs/staff-meetings.md`, phase one (text and state), implemented 2026-10-08 on `feat/the-meeting-brief`
(`docs/evidence/meeting-brief.md`, `TestMeetingBriefIsARead`, `TestEveryMeetingEmitsABrief`,
`TestSkeletonPlaysWithoutAModel`, `TestMeetingAllocationSeam`, `TestMeetingBriefPerParticipant`,
`TestMeetingSaveRoundTrip`; `scripts/meeting-check.sh`; `test_ship_core --meeting`). Save format **52**.
**No audio and no model call were built** — the seams for them are named and left.

- a **brief** exists for each kind the design names (watch change, departmental, allocation, dilithium,
  casualties, Borg, deferred), carrying the participants, the decision, the enumerated options with their
  costs, and the current state ✅
- **every meeting emits a brief**, in the normal case and not only a dramatic one: the watch change and the
  ordinary departmental meeting fall on the clock, and the emit site is one function (`EmitDueMeetings`,
  called by `Advance`), read by its **emission** and its log line, not a count ✅
- a brief is a **read**: building all seven leaves the ship's blob byte-identical ✅
- a brief is built **per participant from their marks and the log** — a post reads its own scope, command
  reads all — so the room does not all know the same thing ✅
- the **authored skeleton** plays with **no model present**, for every kind: the outcomes, their costs, and
  minimal dialogue for each ✅
- **every line carries a delivery direction** (order / report / confession / condolence / flat), the
  vocabulary is written down with its `exaggeration` values, and the seam to synthesis
  (`LineToSynthesis`) carries the annotation with the text; where the brief cannot know a direction the
  line is **marked**, not guessed, and the seam refuses it ✅
- a **meeting outcome sets an allocation end to end** (`ApplyMeetingOutcome`); a person's set carries that
  person's provenance, the ship's own answer is **automatic mode** (`SetPowerAuto`), and the meeting is
  **refused** when it tries to set an allocation as the ship ✅
- the schedule and the queued briefs round-trip; `scripts/test.sh`, `scripts/check.sh`,
  `scripts/meeting-check.sh`, `scripts/power-check.sh` and `scripts/s2-check.sh` exit 0 ✅

Remaining, and deliberately not begun: the **overlay** (M3), the **async generator** and novelty detection
(M4), **voice out** and the cue track (M5), and **voice in** and the ship's computer (M6). The
**scenario content** is authored against this frame next, per `docs/programme-meetings-and-voice.md`.

### The audio plumbing, phase two — three sources, one owner each

`docs/staff-meetings.md`, phase two (the audio model and the render queue), implemented 2026-10-08 on
`feat/the-audio-plumbing` (`docs/evidence/audio-plumbing.md`, `TestTrackOwnership`,
`TestCueCannotStopALine`, `TestCueSet`, `TestCueEmitSite`, `TestRenderKeyAndCache`, `TestPlanMeetingAudio`,
`TestAudioSaveRoundTrip`, `PrintVoice`; `scripts/audio-check.sh`; `tools/voice/render.py`). **Save format
unchanged (52)** — the audio state is host-local and player-local, so nothing new is saved. **No audio
device, no sound, no model call, no live call**: the overlay, the in-game playback and the live model
call are phase three.

- **three sources, one owner each**: `TRACK_DIALOGUE` ← the renderer, `TRACK_CUE` ← the cue player,
  `TRACK_LIVE` ← the live line (named, not built); `TrackOwner` is the invariant, and a non-owner's
  **write and retirement are refused** ✅
- **one retirement rule per reply**: `VoiceRetire` is the only path, and it refuses any producer that
  does not own the reply's track ✅
- **the cue track cannot stop a dialogue line**: a cue plays and retires, the line plays on ✅
- **the meeting plays with the queue unfinished**: the skeleton carries it; the plan is a read and
  deduplicated ✅
- **the cache key** (`voice, text, delivery`, length-prefixed) makes a second render a **no-op**, and
  `render.py` skips a clip already on disk ✅; the cache is **pruned with the save** ✅ and the
  repository stays **clean of audio** ✅
- **the warm happens in the async window**, and says so in the log ✅
- **the cue emit site fires in the normal case**, read by its emission and not a count ✅
- **the delivery direction reaches `synthesize.py --exaggeration`** end to end (order → 0.80) ✅
- `scripts/test.sh`, `scripts/check.sh`, `scripts/meeting-check.sh` and `scripts/audio-check.sh` exit 0 ✅

Remaining for the meeting lane: the **async generator** and novelty detection (M4), the **overlay**
(M3), and the **meeting that plays the rendered audio** (the player itself, phase three).

### Then, in rough order

**First batch done (2026-10-07)** — `docs/evidence/backlog-materials-and-crew.md`, `scripts/backlog-check.sh`,
`TestMaterialsAndTravel`, `TestCrewJusticeAndBorg`:

- the **tractor beam** (strip a derelict, lock a contact), **salvage** and **fabrication** as the
  materials economy ✅
- **structural integrity** as a gate on hull loss and travel safety ✅
- the **navigational deflector**: a weak one lets dust through on a jump ✅
- **inertial dampers**: an undamped jump shakes the crew ✅; **the EMH** (needs the computer core) ✅
- **grief**: a death is notified and the quarters sealed, and a funeral opens them and lifts the crew ✅
- **discipline and justice**: the brig ✅; the hearing ✅ (below). What remains is the Prime Directive's *institutional* consequence — a live link to Starfleet, which changes what orders can be given — registered as the omissions audit's **O5**, not as a defect here
- **training and qualification** as the source of credentials ✅
- the **Maquis split**: the faction field ✅ (the arc is done below; the earlier parenthetical here was stale)
- the **player's own body**: `PlayerIncapacitated` ✅ (the model in the world remains)
- the **player's career**: promotion by whoever commands ✅
- **Borg strategic awareness**: a Borg vessel adapts to our weapons ✅; a **persistent pursuer** ✅
- **trade** (a trader beacon) ✅
- **Relationships and memory**: the bounded per-character mark set of
  `docs/memory-and-consequence.md` (provenance, valence, salience, decay, eviction), bonds derived
  from remembered valence, and the galley's food fabrication ✅
- **Resource acquisition**: mining a resource belt for material and siphoning fuel ✅ (EVA remains)
- **Population pressure**: survivors and refugees taken aboard eat, breathe and crowd ✅
- **Justice**: a hearing (acquittal or conviction) ✅ (the Prime Directive's *institutional* consequence remains — the live link, **O5**)
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

## The voice mechanism — built and accepted (2026-10-08)

The owner's instruction was to ship **the mechanism and never the output**: an analyzer and a synthesizer, pointed
at the retail voice assets the player owns, generating on the player's own machine (`docs/asset-doctrine.md`,
`docs/staff-meetings.md`).

**It works.** `tools/voice/analyze.py` and `tools/voice/synthesize.py`; four characters proven; the owner's ear
on every clip; two artefacts found (a reverb and an accent drift), both diagnosed, both fixed, both heard gone.

- Six characters rendered across four passes; **Tuvok perfect**, the computer fixed, Janeway fixed twice.
- **The reference rule:** one line, one speaker, one recording condition, as dry as the source allows. A
  concatenation buys duration at the cost of the room.
- **Line length is a rendering constraint.** Condition per line; never render a passage in one call.
- **Delivery direction belongs in the meeting brief, per line** — `exaggeration` is the emotion control and it
  defaults to 0.5 whatever the text says.
- **The prosody proxy screens; the owner's ear decides.**
- The mechanism ships; **no generated audio is in this repository, ever.** The venv and model cache are kept in
  the gitignored scratch on the build host.

Evidence: `docs/evidence/voice-spike.md`, `voice-examples.md`, `janeway-cheeseburger.md`, `voice-review.md`,
`voice-fixes.md`. **The meeting's brief and skeleton now feed this seam** (the delivery direction per line,
`docs/evidence/meeting-brief.md`); what remains is the plumbing that renders the lines to audio and the
meeting that plays them.

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

### From the omissions audit (2026-10-07) — `docs/omissions.md`

A sweep of the whole design corpus against this ledger and `docs/path-to-playtest.md` for work the corpus
commits us to that neither carried. The register is `docs/omissions.md`, with each item's evidence and a
justified order. **O16 and O17 were built-but-unregistered and are now carried in S4 and S6 above; the
`docs/gap-the-log` citation was corrected.** The rest, in the register's order:

**Awaiting the owner's ruling first (they cannot be ordered until he decides):**

*Closed entries are retained here and corrected in place rather than deleted — the finding is the sweep's
evidence and the closing is this ledger's business. The same rule is stated in `docs/omissions.md`.*

- **O1/C1 — the power and supply model — CLOSED 2026-10-07, PR #53.** The sweep recorded it as *"proposed,
  awaiting the owner's ruling. Nothing here is implemented"*, which was true when the sweep ran and false
  within the hour: the owner's ruling was the **headroom curve**, and the implementation landed with its own
  evidence and tests — see *The ship's budgets squared* above. **What remains is the canon torpedo moments at
  eleven and six as content**, not the mechanism. This entry is left in place, corrected, as the register's
  rule requires.
- **C2 — allegiance drift** (`docs/story-and-semantics.md`, conflict 3 and *Open*): whether allegiance
  strength is itself a fold over marks; needed before affinities are authored.
- **C3 — Q-class encounters** (`docs/gap-analysis.md` §4): whether to model them at all.
- **C4 — per-compartment or per-deck atmosphere**: `docs/ship-model.md` says per compartment, `docs/ship-systems.md`
  says per deck, the code and both world evidence documents are per deck. One answer, recorded once.
- **C5 — the player's succession after death** (`docs/evidence/player-in-the-world.md`, judgement call 1):
  whether death ends the evening or hands the player the successor's body.
- **C6 — Track D's four questions** (`docs/design-north-star.md` §8): the station model, the ship clock,
  what persistent failure means, and whether ships interact.

**Committed, not built, not otherwise carried (kind 1 unless marked):**

- **O2 — the three independent power sources and the holodeck-matrix trap** (kind 6): `docs/ship-systems.md`,
  `docs/damage-and-budgets.md`, `docs/budget-squaring.md` Finding two, `docs/handoff-lwh-persistent-ship.md`.
- **O3 — canon named to model and never modelled** (kind 6): `docs/lore-ledger.md`'s closing list — bio-neural
  gel packs, the EMH's dependence on sickbay power and holo-emitters, the variable-geometry nacelles, cargo
  bays, replicator rations as a resource, and holodeck-power incompatibility.
- **O4 — first contact, awareness and reputation**: `docs/exploration-and-science.md` item 5,
  `docs/memory-and-consequence.md`, `docs/scenario-atlas.md`.
- **O5 — discipline from home, the live link to Starfleet**: `docs/gap-analysis.md` §4, `docs/scenario-atlas.md`.
- **O6 — the crew manifest's bills**: watch bill, battle stations, emergency bills, the damage-control
  organisation, and the officer of the deck's authority (`docs/crew-manifest.md`).
- **O7 — the Hazard Team as a standing unit**: two squads, a unit layer above departments
  (`docs/crew-manifest.md`, `docs/crew-roster.md` §6).
- **O8 — the character layer** *(CLOSED 2026-10-08, PR #72)*: the four kinds, the
  species capabilities/needs/susceptibilities, the condition record with source/cure/visibility, and
  the derivation are built (`docs/character-derivation.md`, `docs/evidence/the-character-layer.md`).
  What remains is the wiring of `EffectiveSkill` into the failure roll, named there, not the layer.
- **O9 — the shuttle load-screen decision menu** (`docs/shuttles.md`; `docs/evidence/every-screen.md` row 18).
- **O10 — the in-world control at the panel** (the "glance"), out of console scope and into world work
  (`docs/evidence/every-screen.md` row 12).
- **O11 — the outside's site archetypes and exterior dressing**, then the away-mission loop
  (`docs/outside-the-ship.md`, *Order of work* 2–4).
- **O12 — the phenomenon's two canon story models** (`docs/exploration-and-science.md`).
- **O13 — the Borg security squad embodied as bodies moving deck by deck** (`docs/borg-incursion.md`,
  *Still to come*).
- **O14 — the wall-clock exit wired to the engine's quit path** (`docs/evidence/clocks-and-exits.md`).
- **O15 — the nine-scenario first slate** and the rank-and-role authoring rule (`docs/scenario-atlas.md`).
- **O18 — Track D / the multiplayer premise** (kind 2, deferred): `docs/multiplayer-premise.md`,
  `docs/design-north-star.md` §8.

**Withdrawn promise (kind 4), registered as a capability gap:**

- **W1 — a dead officer's credentials are usable**: `docs/access-and-authority.md` withdrew the promise
  rather than keep one the code cannot keep; `docs/evidence/access-and-authority.md` records it as an open
  item belonging to the memory and consent layer.

**Dangling reference (kind 5), corrected in `docs/the-record-and-the-log.md` and the month-report evidence:**

- **O19 — `docs/gap-the-log.md`** was cited where the file is `docs/evidence/gap-the-log.md` (three sites).
