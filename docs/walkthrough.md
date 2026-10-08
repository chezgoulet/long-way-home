# The walkthrough — one sitting, as a route

**What this is.** The owner's single sitting on *Long Way Home*, written as a route: where to go, what
to press, and what to expect. It is written for a person with no knowledge of the code, and it is
**also the gap-finder** — every place the repository cannot say what to look at, or what should happen,
or where two documents disagree, is recorded here as a finding rather than smoothed over.

**How to read it.** The route is in the order a sitting would happen. Each stop says how you get there,
what to look at, what to do, and **the one question the stop exists to answer**. Where a stop has
nothing in it, that is said plainly. The **gap list** (Section 12) is ordered by what a player would
notice first; the **completeness verdict** is Section 13.

**Sources.** Every claim below is traceable: the design documents named in `AGENTS.md`, the evidence
files in `docs/evidence/`, and the module source for key bindings and console commands. Where a claim
comes from code rather than a rendered artifact it is marked. **A green check is not the answer to your
question** — nearly every stop below is backed by a script that passes and still leaves the human
question open, and the gap list says which.

---

## 1. Before you sit down — how Long Way Home is started, and the first finding

This is the first thing a person meets, so it is the first finding.

**There is no way to start Long Way Home from the game's menus.** `g_ship` defaults to `0` and the
simulation starts only when a cvar turns it on (`module/ship/g_ship.cpp`); the shipped ship map is a
dozen engine cvars away from a normal launch. To run the sitting you build the ship once, then start
the engine by hand on the merged map:

```sh
# once: stitch and compile the whole ship (about three minutes; -vis and -light run)
scripts/build-ship.sh

# the sitting: the merged ship is one map, the simulation on, deck-scoped names on
scripts/run-engine.sh +set com_hunkMegs 768 +set g_ship 1 +set g_shipDeckPitch 3072 +map voyager
```

- `com_hunkMegs 768` is required — the whole-ship BSP is ~54 MB and the default hunk fails
  (`Hunk_Alloc failed`). `docs/evidence/s3-merged-map-measured.md`.
- `g_shipDeckPitch 3072` tells the deck-scoped name lookup which deck the player is on.
- `g_ship 1` starts the ship simulation; the player is placed **on deck 1**. Same evidence.
- `map voyager` is the **single-player** route (the AGENTS.md trap: `map` is SP, `spmap` is Holomatch).

**The rest of the experience is cvars you must remember** (all default `0`, all off unless set;
`module/ship/g_ship.cpp`, `module/crew/g_crew.cpp`):

| cvar | what it turns on |
|---|---|
| `g_crew 1` | the crew direction layer (bodies on your deck) |
| `g_crewFromShip 1` | the deck's crew are whoever the ship's roster puts there |
| `g_env 1` | gravity, breach push/hurt, emergency strips, fire, sparks in the world |
| `g_player 1` | your own body: the console can hurt *you*, and death is not a reload |
| `g_shipMode 1` | holodeck mode — saves allowed. **Default is ironman**, which refuses a hand save |
| `g_shipClock 0/1/2` | accelerated (a ship day in `g_shipDayScale` game seconds, default 60 = a day in 24 min) / real time / wall clock |
| `g_shipRole 0/1/2` | any post / in command / Munro |

**The honest shape of this.** Everything below assumes you typed those. The owner can, because the
documents told him. *A person with no knowledge of the code cannot* — see **G1** in the gap list. This
is the first thing a sitting would hit, before any content.

**The retail modes still work.** Mode 1 (the campaign and Virtual Voyager) is `scripts/run-engine.sh`
and the menu, or `spmap voy1`; mode 2 (retail Holomatch) is a separate cMod client, `scripts/run-cmod.sh`
for the server half. Neither is the route below; both are supported and must stay working
(`docs/client-modes.md`, `docs/gates.md` G6/G7).

---

## 2. Start where the game starts — the opening, character creation, the first watch

**What actually happens when the map loads.** The ship simulates: **141 crew**, one watch on duty,
**day 0, 08:00, alpha watch, condition green**, every system nominal, deuterium and antimatter full,
38 torpedoes (`docs/evidence/s1-ship-core.md`, the `--day` transcript). The player's body is the retail
Munro model by default; `ship::ApplyPlayerBody` swaps head/torso/legs to a chosen character when one is
created (`docs/evidence/s10-modes-and-roles.md`, `scripts/playerbody-check.sh`).

**Character creation exists, and it is the only authored opening beat.** Two ways in:

- In the world, if you can reach a personnel panel, it opens `ui_lwh_character`
  (`docs/evidence/s10-modes-and-roles.md`); or type the command.
- From the console: `ship character <name> <department> <rank>`.

The screen offers a name from a fixed invented list, five departments (COMMAND, ENGINEERING, SECURITY,
SCIENCES, MEDICAL) and five ranks up to **lieutenant commander**; UP/DOWN rank, LEFT/RIGHT department,
ENTER creates (`module/ui/ui_lwh_command.cpp`). The created character replaces a generated crew member
of that department who stands no station, so the complement does not grow and nobody's post is taken
(`docs/evidence/s10-modes-and-roles.md`). Your role is `g_shipRole` (any post / in command / Munro).

**What the opening does not do — and this is a finding, not a detail.** `docs/start-states.md` designs
the opening as *a situation to be handed, not a character to be equipped*: a casualty list, a first log
entry stating the losses and that no rescue is coming, an empty chair or its absence, the Maquis aboard,
and start-state variants from ensign to captain. **None of that is implemented.** The shipped ship is
the *default* start state only — whole, canonical, chain of command intact — and there is no Caretaker
aftermath, no casualty list, no line in the log telling you what you have inherited. See **G2**.

**What you are supposed to do in your first ten minutes** (from `docs/story-and-semantics.md`): read the
inventory before the contacts, find the three people you are worried about, and assign. The route that
supports that begins at the record (Section 11) and the consoles (Section 10). Whether it *reads* as
clear is a human question, and the repository cannot answer it.

**The stop's question:** *does the opening feel like a situation you were handed, and does the game tell
you what you hold?* On the current build the honest first answer is: the situation is not there, and
nothing in the world states your post.

---

## 3. Getting around — the turbolift, the one-map ship, and the Jefferies tubes

**The ship is one map with fifteen decks, and the turbolift travels within it.** The stitcher rewrote
every deck-to-deck `target_level_change` into a teleporter aimed at that deck's arrival point; the
retail turbolift menu reads **our own fifteen-deck list** (`tools/shipmap/data/sp_turbolift.dat`,
packaged into `longway_voyager.pk3`), and the menu opens with `genericmenu turbolift`, the exact command
a turbolift panel fires (`docs/evidence/s3-merged-map-measured.md`).

- **In the world:** walk to a turbolift panel, press use; the menu lists decks 1–8 and 9–15 with Engage
  and Return (`lwh_turbolift.tga`).
- **The menu's own selection UI has not been clicked headlessly** — opening it pauses the simulation, so
  the harness cannot act again without a key-injection seam. What is proven is that the menu reads the
  fifteen-deck list and that the commands it fires (`use tour_turbo_NN`) move the player deck to deck,
  15 of 15, with no level load. **A person must confirm the menu clicks.** *Treated as a call — see
  Section 14.*
- **Not every move is the lift.** Each re-dressed deck has a **Jefferies tube** as the fallback route:
  deck 12 → deck 11, deck 13 → deck 11, deck 14 → deck 13, deck 7 → deck 10, deck 6 → deck 5
  (`docs/evidence/deck*-redress.md`, `deck07-auxcore.md`, `deck06-composition.md`). The tube
  traversals are entities proven present in the map and counted by the stitcher; **nobody has walked
  one and ridden it** — a person must.

**The stop's question:** *does moving around the ship feel like a ship rather than a set of levels?*
The mechanism is measured; the feel is not.

---

## 4. The decks, one at a time

Ten decks ship as retail Virtual Voyager maps; five were absent and were built (four re-dressed from
`tour/deck11`, one composed). A re-dressed deck is real geometry copied from a shipped interior and
changed to be this room; the brief for each names its reuse target and every judgement call
(`docs/ship-master-map.md` lists all fifteen). This section says, per deck, **how you reach it, what
there is to look at, what there is to do, and the question the stop answers.**

> **Where the deck's own content is retail.** On the ten published decks, the corridors, props and
> room dressing are the retail Virtual Voyager content, unchanged. The Long Way Home additions on them
> are: the station panels open our consoles, three real console textures are remapped to the live
> status panel, the environment layer's effects appear on deck 12, and the crew layer can embody the
> roster. Everything else you see on those decks is the retail game's.

### Deck 1 — the bridge (published)
- **Reach:** turbolift, deck 1. This is where the player spawns.
- **Look at:** the bridge; Tactical holds shields and phasers (deck 1 in `SPECS[]`), Operations holds
  communications and turbolifts, the Conn holds helm systems. The bridge models ship
  (`models/mapobjects/bridge/*`), but the published deck carries **no named station entity to stand at**
  (`docs/evidence/s5-stations.md`).
- **Do:** call the alert (`1`/`2`/`3` on a console), fire from Tactical, work the Conn, read the contact
  and the comms picture (Tactical reads sensors and comms; Operations operates them).
- **Question:** *can you find the bridge's own decision surfaces, or do you need the console command?*
  The panels' command mapping is proven by command, not by a person walking to one (**G8**).

### Deck 2 — mess hall, replicators (published)
- **Reach:** turbolift, deck 2.
- **Look at:** the mess; the **galley** is worked from the Operations console (the replicator/mess panel
  opens Operations), rationing is `Stores.rations`.
- **Do:** fabricate rations (`ship rations`), feed the crew (morale's driver), take a meal.
- **Question:** *does rationing read as a cost you feel?* The rule exists (rations, fabrication,
  airdponics — `docs/evidence/backlog-materials-and-crew.md`); the airponics bay is not a mapped place
  (**G21**).

### Deck 3 — quarters (published)
- **Reach:** turbolift, deck 3.
- **Look at:** crew quarters; living conditions colour morale (`quartersQuality`).
- **Do:** nothing authored. There is no per-door quarters interaction; a sealed casualty's quarters are
  visible only through `ship wall` / the console, not by walking to the door
  (`docs/evidence/the-two-missing-decks.md`, judgement call 5).
- **Question:** *does a sealed door mean anything when you walk past it?* On the current build, only if
  you open a console — the world does not tell you (**G20**).

### Deck 4 — transporters, stores, cargo bay 2 (published)
- **Reach:** turbolift, deck 4. This is also the **crew measurement deck** (`tour/deck04`).
- **Look at:** transporter rooms 1 and 2. The transporter panel opens the Operations console, focused on
  the transporters (`docs/evidence/s4-station-consoles.md`).
- **Do:** beam an away team (`T` on the Ops console, or `ship transport 3`) and recall it (`R`). Note the
  rule: a beam cannot reach through your own shields, so beaming mid-fight means dropping them. The
  pattern buffer is the transport system's condition, and it is stated before the act —
  *"the pattern buffer is at 100%, nominal: I would send anyone"* — and a degraded one can mangle, copy
  or merge a traveller (`docs/evidence/condition-sets-the-odds.md`).
- **Question:** *is the transporter a decision or a button?* The risk statement is the design's answer;
  whether it lands is yours. The away *site* does not exist, so the team arrives as a chart entry
  (**G18**).

### Deck 5 — sickbay (published)
- **Reach:** turbolift, deck 5.
- **Look at:** the ward; four beds (three standard, one surgical). The Sickbay console reads
  `INJURED / BEDS / WAITING / LOST / ASSIMILATED / MORALE / FATIGUE / TRIAGE`.
- **Do:** set the triage order (`T` on the triage board, command's), raise the surgical field (`B`),
  call the Doctor (`E`), recover a captive in the Borg window (`R`). Four beds against more casualties is
  a decision, not a queue that resolves itself (`docs/evidence/gap-triage-and-sickbay.md`).
- **Question:** *who gets the bed, and does the board make that a decision?* The board now acts, but it
  is command's to order the triage and this is proven mechanically, not experienced.

### Deck 6 — Holodeck 2, armory and crew quarters (composed, re-dressed)
- **Reach:** turbolift, deck 6.
- **Look at:** **a composition of three sources in one corridor** — a slice of `tour/deck09`'s quarters
  hall on the north side, `_brig`'s security room in a south-west alcove, and the arch of
  `Tour/_holodeck_firingrange` set twice on the south-east wall. Warm at the holodeck and the quarters,
  working neutral down the corridor, cool at the armory (`docs/evidence/deck06-composition.md`).
- **Do:** the holodeck station marker (`lwh_station_17`, `SYS_HOLODECKS`) is here; holodeck uses
  (recreation / training / therapy / forensic) are the console's (`ship holo <use> <crew>`). There is a
  holodeck maintenance post and a security post. The holodeck is a **closed doorframe**, not a working
  programme — the brief's non-goal.
- **Question (owed):** *does deck six read as three places, or as one corridor with three props?*
  **Only you can answer it, and it is the deck's whole risk.** The evidence is explicit that a *slice is
  not a room* and that the cuts may read as architecture or as a diorama. The screenshots
  (`lwh_deck06.tga`, `..._holodeck.tga`, `..._armory.tga`) show the corridor and two ends. **Crew-post
  coverage on deck 6 was never measured** — no deck-6 scenario exists (**G15**).

### Deck 7 — auxiliary computer core, cargo and labs (re-dressed)
- **Reach:** turbolift, deck 7.
- **Look at:** one chamber with an **auxiliary core column** at its centre, two cargo islands of the
  game's own crates, an overhead escape-pod hatch, a Jefferies tube to the main core. Deck 11's interior
  is compressed 404 → 240 and is **detail geometry** (invisible at runtime, but it means the room's
  *structure* is the shell only) (`docs/evidence/deck07-auxcore.md`).
- **Do:** the computer-core station marker (`lwh_station_3`) and a core-watch post; the core panel shows
  live state.
- **Question (owed):** *does the depth deck read as the core the ship falls back to?* The evidence names
  the aesthetic risk outright: the copied engineering interior plus a cylinder may or may not read as a
  place. Nobody has walked it.

### Deck 8 — astrometrics, science lab, deuterium (published)
- **Reach:** turbolift, deck 8.
- **Look at:** astrometrics; sensors are stationed here. The astrometrics panel opens Operations on the
  sensors (`docs/evidence/s4-station-consoles.md`).
- **Do:** the survey board (`U` on Ops) is the tricorder/away-kit decision; the chart is command's.
  Sensors survey the beacons; probes are launched from the launcher (`ship probe <beacon>`).
- **Question:** *is exploration a mechanic here?* The rules exist (survey, probe, phenomenon,
  dilithium); the **away site does not**, so a survey is a chart row rather than a place (**G18**).

### Deck 9 — crew quarters (published)
- **Reach:** turbolift, deck 9.
- **Look at:** crew quarters. The model's daily routine sends the off-duty crew to quarters here
  (`docs/evidence/s1-ship-core.md`).
- **Do:** nothing authored. **Find out whether the roster is embodied here** — see the crew stop
  (Section 9).

### Deck 10 — shuttlebay, main computer core, impulse (published)
- **Reach:** turbolift, deck 10.
- **Look at:** the shuttlebay; the **main computer core** and the **torpedo launchers** are on this deck
  (`SPECS[]`, `module/ship/ship_core.cpp`; `docs/evidence/access-and-authority.md` corrected the two
  entries from deck 9). The tractor beam is at Shuttlebay Control.
- **Do:** shuttles are supported, not pilotable — the bay's complement and where each is; launch and
  dock through the console (`ship shuttle`, `ship launch`, `ship shuttledock`). A hit on the bay wrecks
  what is parked (`docs/gates.md`).
- **Question:** *is the shuttlebay a place with a decision?* The **shuttle load-screen is not built** —
  it is a launch-time decision *menu*, deliberately not forced into the console pattern
  (`docs/evidence/every-screen.md`, row 18) (**G19**).

### Deck 11 — main engineering, warp core, deflector (published)
- **Reach:** turbolift, deck 11; or the Jefferies tube from decks 12/13.
- **Look at:** the warp core, EPS, power distribution, structural integrity, inertial dampers, the
  navigational deflector. **This is the densest systems room** and the deck every re-dress copies.
- **Do:** the **Engineering console** (Section 10) — allocate power, switch systems, reorder the
  priority list, set the alert, raise the emergency override, run the warp-core cascade through coolant
  → eject. The core is worked here.
- **Question (owed):** *does the ship feel like a science vessel spending her science budget on a war —
  the labs dark so the phasers can charge?* **The honest answer from the repository is that the question
  cannot be answered as posed**: there are **no labs, no astrometrics, no lighting and no cargo loads
  in the power model** — the model is a warship's budget on a science vessel's hull, and life support
  and the holodecks are *not* independent sources as canon and `docs/ship-systems.md` require
  (`docs/budget-squaring.md`, findings one to three; **G12**). What you can judge is the tactical fit;
  the science half does not exist to go dark.

### Deck 12 — environmental control (re-dressed)
- **Reach:** turbolift, deck 12; or the tube from deck 11.
- **Look at:** one low control room (404 → 192) with the warp core replaced by an **atmosphere plant**,
  a watch console at the entrance, a Jefferies tube exit opposite, section 42 sealed next door. The
  life-support station marker (`lwh_station_0`) sits in front of the watch console
  (`docs/evidence/deck12-redress.md`).
- **Do:** this is where **the environment is felt**: with `g_env 1` the authored breach pushes and hurts,
  a force field holds the air (`O` on Ops), gravity fails and magnetic boots (`ship boots`) give it
  back, and the emergency strips switch. The breach check stands here and reports the push, the hurt,
  the minutes of air and the field (`docs/evidence/environment-in-the-world.md`).
- **Question (owed):** *does it read as somewhere a person watches over fifteen decks of air, or like a
  box with a console in it?* And, the specific world question: **does the red emergency strip read as
  red?** The rendered evidence says **no** — the toggle is correct, the *signal* is not perceptible from
  the camera, on all five re-dressed decks (`docs/evidence/access-and-authority.md`, "The world's
  emergency strips") (**G9**). Only a person can overrule that, and the current evidence is that it
  fails.

### Deck 13 — life-support plant (re-dressed)
- **Reach:** turbolift, deck 13; or the tube from deck 14.
- **Look at:** one plant hall (404 → 224) with a central machinery island of atmosphere processors, a
  raised catwalk with a stair, a Jefferies tube mouth, and the local plant panel and post on the
  catwalk (`docs/evidence/deck13-redress.md`).
- **Do:** the plant panel is a live status surface; the system is operated from the Engineering console.
- **Question (owed):** *does it read as the plant that keeps the ship breathing?* The evidence names the
  risk: the room reads dark under the copied lighting, with the machinery as unlit masses.

### Deck 14 — stasis chambers and Holodeck 1 (re-dressed)
- **Reach:** turbolift, deck 14; or the tube from deck 13.
- **Look at:** one flat chamber (404 → 208, the raised core deck cleared) with a row of the game's own
  **stasis pods** along the north wall, a **Holodeck 1 doorframe** at the far end, and function lighting
  — cold over the pods, warm at the holodeck, neutral between (`docs/evidence/deck14-stasis.md`).
- **Do:** the stasis post and the holodeck panel.
- **Question (owed):** *does the walk from the cold pods to the warm holodeck read as the contrast the
  brief intends?* The evidence: the cold and warm zones are present, but the copied engineering
  interior still dominates and the final *read* is unjudged.
- **Note on the flag:** `HOLODECK_RECREATION_DECK` still sends the crew to deck 6 for recreation even
  though Holodeck 1 is here on deck 14 — behaviour deliberately unchanged, flagged for you
  (`docs/evidence/deck07-auxcore.md`, Task B; **G13**).

### Deck 15 — plasma relays, landing gear (published)
- **Reach:** turbolift, deck 15.
- **Look at:** plasma relay room 16, Jefferies tube G-33, landing gear (retail content). This deck is
  the smallest published map and is **not to be rebuilt** (`docs/vv-gap-list-verified.md`, Correction 1).
- **Do:** nothing LWH-authored.

**The fifteen-deck stop's question:** *does the ship feel like one continuous place with fifteen floors,
rather than fifteen levels that happen to be linked?* The mechanism (one map, lifts, tubes, no load
screen) is measured; the feel is yours.

---

## 5. The two decks that have no authored hazard, and the ones that do

The environment layer (`g_env 1`) is authored **where the hazard is**: the breach push/hurt/field, the
gravity failure and the emergency strips live on **deck 12** (and the strips on decks 13, 14 and 7); the
damage sparks and the deck fire follow **the player's own deck**; the console-injury follows the
console. Everything else is state in a console.

- **Damage is visible in the world**: a damaged system sparks at its station marker on the deck you are
  on (`SyncDamage`, `docs/evidence/s6-damage-and-casualties.md`).
- **A burning deck is smoke and flame** across the deck you are on (`SyncFire`, same evidence).
- **A breach** pushes, hurts and can be fielded (deck 12 only) — `docs/evidence/environment-in-the-world.md`.
- **The environment is per deck, not per compartment** — the model has no per-compartment atmosphere, so
  one authored breach stands for the whole deck (`docs/evidence/environment-in-the-world.md`,
  judgement call; `docs/evidence/player-in-the-world.md`, judgement call 3; **G11**).

**The stop's question:** *does the ship's damage read as an inventory of what still works, seen in the
world, rather than a number?* On the player's deck, yes for sparks and fire; elsewhere, you are reading a
console. Whether that is enough is yours.

---

## 6. The crew — does the deck feel inhabited?

This is G3's own open question, unchanged since it was measured, and it is one of the loudest findings.

**How to see the crew:**

- **The merged ship, the roster embodied:** add `+set g_crew 1 +set g_crewFromShip 1` to the launch.
  The crew on your deck are whoever the ship's routine puts there; they arrive and leave with the watch.
  The evidence measured **24 embodied** on one deck, all reaching their place within the meal hour
  (`docs/evidence/s5-stations.md`). Station markers exist on the generated decks (holodecks on 6, life
  support on 12, the core on 7, the plant on 13, stasis on 14), so a post-holder walks to a real place.
- **The dedicated scenario:** `scripts/run-scenario.sh` builds the crew file for
  `scenarios/deck04-watch` and starts the engine on Virtual Voyager's deck 4 with the layer on; six crew
  walk to six posts, look at you, and answer. `docs/playtest.md`, *Walking a crewed deck*.
- **Address them:** the game's own use path; a crew member answers from their strongest memory
  (*"You came back for us. I have not forgotten."*) — `docs/evidence/backlog-materials-and-crew.md`.

**What is measured:** 100% post coverage over 121 samples, every post reached within 6.1 s, every
address acknowledged within 1.5 s, save and reload identical, 25 bytes of save per crew member against a
256 budget (`docs/evidence/g3-reactive-crew-measured.md`).

**What is not:** **whether the deck feels inhabited.** The six crew in the measured scenario are
**added** to deck 4, not found on it — none of deck 4's own eight NPCs can hold a post. That is a
charter question the owner has never ruled on (`docs/evidence/g3-reactive-crew-measured.md`;
`docs/HANDOFF.md`). And the crew's **manner** — barks and idles wired to morale — is the one item of the
morale contract still open (`docs/evidence/gap-morale-and-fatigue.md`; **G14**).

**The stop's question (owed):** *does the deck read as inhabited?* And the second half: *is adding crew
acceptable for G3?*

---

## 7. The stations and consoles — where to stand, what to press, what you decide

Every station shares **one screen** (`module/ui/ui_lwh_engineering.cpp`), showing only that station's
own systems, plus reads where the post needs a second content type. The retail panels open these
consoles when `g_ship 1`; with the simulation off they open the retail screens as before
(`docs/evidence/s4-station-consoles.md`).

**Reaching a console.** In the world, walk to the station's panel and use it (the panel's own command is
intercepted). On the generated decks the live panel surface is present but no session has stood at one
and operated a system through it (`docs/evidence/deck*-redress.md`; **G8**). Reliably, from the console:
`ship console` opens Main Engineering, `ui_lwh_engineering` / `ui_lwh_command` etc. open screens directly.

### Main Engineering (station 0)
- **Where:** deck 11 (or `ship console`).
- **What:** all eighteen systems in power order, with allocation, output, condition and crew at station;
  four power sources with output bars.
- **Press:** UP/DOWN select · ENTER on/off · LEFT/RIGHT move a system in the priority order · `1`/`2`/`3`
  condition green/yellow/red · `Y` emergency override · `H` countermeasures (breach puzzle) · ESC leave.
- **The decision:** **what do you turn off, and what does it cost?** Shedding follows the priority list;
  the damage-control party, the repair jobs and the crew-hours all draw on the same people.
- **The human question (owed):** *is the console pleasant to operate — not whether it works; whether it
  is good to use?* This is the first place to answer it, and it is one of the questions owed.

### Tactical (station 1)
- **Where:** deck 1 (bridge) or `ui_tactical`.
- **What:** shields, phasers, torpedo launchers, tractor beam; the contact's hull, shields, weapons and
  engines and the target; a **read** of the sensor picture and the comms traffic (Tactical reads what
  Operations operates).
- **Press:** UP/DOWN · ENTER on/off · `V` cycle the **phaser setting** (STUN / HEAVY STUN / KILL /
  VAPORIZE) · `F` fire a torpedo · `H` countermeasures · `1`/`2`/`3` condition · `Y` override.
- **The decision:** with a contact, *what do you shoot and what do you spare*; with **no** contact, the
  phaser setting is the standing decision with a price — the console shows
  `PHASER BANK VAPORIZE  POWER 174%`, and a vaporize setting asks 175% of the bank's nominal demand so
  it competes with shields for EPS (`docs/evidence/the-missing-screens.md`).
- **The human question (owed):** *does the phaser-yield ladder feel like a decision with a price on it,
  or only a number?* The unit test proves the effect (`hull after a minute: vaporize 0.858, kill 0.917,
  stun 1.000`); the *feel* is yours.

### Operations (station 2)
- **Where:** deck 1 (bridge) or a transporter/astrometrics/environmental panel, or `ui_ops`.
- **What:** sensors, communications, transporters, life support, structural integrity, computer core,
  turbolifts, replicators, holodecks. Carries **one content type**, by the owner's ruling.
- **Press:** UP/DOWN · ENTER on/off · `T` beam a party · `R` recall · `U` survey board · `O` force field
  over the deck losing air · `L` hail · `M` trade · `A` distress · `H` countermeasures · `Y` override.
- **The decision:** the transporter is the decision (drop the shields to beam, or wait); the survey is
  the decision (what to spend a scan on, out of one shared charge); the field is the decision (hold the
  air where it is going).
- **The human question:** *is one content type enough for the ship's services, or does Operations feel
  starved?*

### Conn (station 3)
- **Where:** deck 1 (bridge) or `ui_navigation`.
- **What:** warp drive, impulse drive, navigational deflector, inertial dampers. **No read layer** — the
  owner's ruling is that the helm's job is flying, one content type.
- **Press:** UP/DOWN · ENTER on/off · `J`/`K`/`L` jump along a link · `C` lay in a course · `X` run from
  a fight · `H` countermeasures · `Y` override.
- **The decision:** *where to make for, and whether to run.* Command sees the per-course forecasts; the
  Conn's own act is the jump.
- **The human question (owed):** *does the Conn console feel right carrying one content type while a
  captain can still reach everything their clearance opens?* The single-type shape is proven
  (`lwh_conn_single.tga`, no `RD` rows) and the remote call-up is proven mechanically (a commander at
  the Conn reaches sensors; an uncleared ensign is refused); the *feel* is yours.

### Sickbay (station 4)
- **Where:** deck 5 (sickbay panel) or `ui_lwh_triage`.
- **What:** the ward, a read of life support and the computer the Doctor runs on; the triage board one
  row per casualty in triage order.
- **Press:** on the console, ENTER on/off and `B` surgical field; on the triage board, `B` surgical
  field · `E` call the Doctor (EMH) · `T` triage order (command's) · `R` recover a captive in the Borg
  window.
- **The decision:** who gets the bed — worst-first or rank-first — and whether to hold the gravest case
  with the surgical field when there are no supplies.
- **The human question:** *is the ward legible and the decision worth making?* The board acts now; it
  was "thin" before the thickening.

### The standing orders and the command console
- **Where:** a ready-room/command panel, or `ui_lwh_command`.
- **Press:** UP/DOWN system · LEFT/RIGHT deck · `R` see to that system first · `G` guard that deck ·
  `V` evacuate that deck · `W` **write it off** (the abandonment list) · `T` triage order ·
  `C` clear orders · `O` month report · `J` job queue · `A` chart · `P` promote · ESC leave.
- **The decision:** this is where the player's hand shows: orders, the destination course, the report,
  the write-off. Only whoever commands may do them, and the screen names the refusal
  (*"the report is the commanding officer's to write"*) — the two-lock model, proven by
  `docs/evidence/every-screen.md`.
- **The human question:** *does command feel like deciding who goes, rather than operating the ship?*

### The panels that are not stations
A **log / padd** terminal opens the ship's log; a **ready room / command** panel the command console; a
**personnel / crew** panel the personnel screen; a **replicator / mess** panel the Operations console
(`docs/evidence/s10-modes-and-roles.md`). These are the commands the panels fire; no session has walked
up to one and opened it (**G8**).

---

## 8. The screens that only show themselves under stress

The brief asks, for each of these, **whether it can be reached deliberately or only by luck**. The
honest answer is the headline finding: **the stress systems have no scenario content behind them, so
they are reachable deliberately only from the developer console, or by luck inside a fight that itself
has to be invoked.** `ship ...` typed bare at the game's own console is a developer's and unrestricted
(`docs/evidence/s10-modes-and-roles.md`). The commands below are how **you** can provoke each one for
the sitting.

| what to see | how to reach it deliberately | only by luck? | what to look at |
|---|---|---|---|
| **damage visible in the world** | `ship damage <system> <amount>` — then stand on the deck that hosts it | no | sparks at the system's station marker (`SyncDamage`) |
| **a fire** | `ship ignite <deck> <amount>` | a fight starts one every third penetrating hit | smoke and flame across the deck (`SyncFire`) |
| **a hull breach + field** | `ship breach <deck> <amount>`; field with `ship field <deck> on` | no | the push, the hurt, the minutes of air, the containment display (deck 12) |
| **an intruder** | `ship board <deck> <n> [raider\|borg\|hunter] [objective]` | an enemy with our shields down sends a party | hostile bodies on your deck, the ship's security embodied |
| **an opposing ship** | `ship jump <beacon>` until a hostile beacon; `ship fire`; `ship target …`; `ship wingman` | yes, by sector seed | the target readout, the viewscreen schematic, the screen shake |
| **the viewscreen** | stand facing a panel; it draws the contact beside you (`scripts/viewscreen-check.sh`) | with a contact | a **drawn schematic**, not the engine rendering the model |
| **the alert** | `1`/`2`/`3` on any console, or `ship alert red`; `ship klaxon red` | no | the header word, the alarm band, the klaxon sound |
| **assimilation turning a deck Borg** | `ship borg <deck> <n>` | a Borg boarding party | a **generated** deck turns in four sections as assimilation rises, and strips back; the published decks cannot |
| **a warp-core cascade** | `ship damage warp drive …`, then `ship core …` through coolant → eject | a fight can start the chain | the falling containment, the countdown, the eject |
| **the environment on you** | `ship boots`, the airless compartment on deck 12 (`g_env 1`, `g_player 1`) | no | your own gravity, injury, being carried to sickbay |

**The findings this table exposes:**

- **There is no scenario system.** `docs/scenario-atlas.md` designs nine scenarios with *breeding
  conditions*; no breeding-condition director exists in the module (the only "director" is the crew
  layering's restaffing rule). There are **two** scenarios in `scenarios/`, both measurement
  harnesses. So nothing in the game recruits a scenario from the ship's state, and the player cannot
  meet one by playing (**G6**).
- **The combat and Borg slices are rules in the core, reachable from the console, with the visible half
  partial**: boarders and drones are bodies on your deck (Klingons as the raider stand-in); the Borg
  turn is only on the five generated decks (published decks share shader names, so a per-deck remap is
  one shader address short); the viewscreen is a module-drawn schematic because the renderer has no
  render-to-texture path (`docs/evidence/s8-the-borg.md`, `docs/evidence/s9-the-outside.md`).
- **The breach puzzle** (`H` on a console) is the one stress screen that is genuinely playable by hand:
  a 5×5 grid, three target sequences, a seven-pick buffer, a 30 s timer. Nobody has solved one at a
  console by hand — the harness drives it (`g_shipTest`) — so this is a deliberate stop for the sitting.

**The stop's question:** *can you provoke each thing and judge it?* For the console commands above, yes.
For "meet one by playing", no — that is the content gap.

---

## 9. The record — the log, the report, the personal log, the counter, the budget

`docs/the-record-and-the-log.md` calls the record the game's scoreboard and its last frame. These stops
are where an attrition game either lands or does not.

### The ship's log (the official record)
- **Reach:** a log/padd terminal (`ui_lwh_log`), or `ship log [count] [scope]`; scroll with UP/DOWN.
- **What:** `{time, who, scope, what}` newest-first, filterable by scope (bridge, engineering, hull,
  command, sickbay, outside). Every event writes: alerts, damage, breaches, seals, fields, orders,
  casualties, boarding, jumps, fire, the mood line (`docs/evidence/gap-the-log.md`).
- **Do:** after a run, *reconstruct what happened and when from the log alone* — that is the acceptance,
  and it is built.

### The personal log (the private record)
- **Reach:** press `P` on the log screen (`ui_lwh_personal`), `L` to go back; or
  `ship personal [count]` / `ship personal write <text…>`.
- **What:** one owner's entries and nobody else's — the model never returns another person's words
  (`docs/evidence/the-two-logs.md`, `docs/evidence/the-missing-screens.md`).
- **The question:** *does writing in it mean anything?* No transcript can answer that; it is yours.

### The month report (the period beat, and the lie)
- **Reach:** `O` on the command console (`ui_lwh_report`).
- **What:** the report drafted honestly from the record; **strike** a line (`S`), **soften** a number
  (`F`), **sign it to the crew** (ENTER) or **file it upward** (`U`); the record keeps the diff.
- **The decision:** the lie, and its direction. The toll is paid **downward** — only a witness under the
  signer reading a report published to the crew loses trust; the same falsehood filed upward costs
  nothing from below. The headline is the navigation counter's change since the last entry.
- **What it writes:** a signed falsehood contradicting a `MEM_SAW` mark writes `MEM_LIE` in the witness,
  by name (`docs/evidence/the-month-report-and-the-toll.md`).

### The navigation counter (the emotional centre)
- **Reach:** on every station console (`NAV 75000 LY OUT 75 YR NOMINAL 77 YR NOW +0.0 SINCE LAST`), the
  in-world panel, the HUD glance, and `ship nav`; command alone sees the forecasts.
- **What:** distance to Earth, the nominal years and the years at current capability, and the change
  since the last entry. A wrecked crystal sends current capability from 77 to 82 years; a jump toward
  home takes the distance down (`docs/evidence/the-navigation-counter.md`).
- **The human check (owed):** *after a hard month, does the arrow move?* The number moves in the
  transcripts; whether it *feels* like a hard month is yours.

### The budget display
- **Reach:** the Engineering console's power sources and allocation; `ship status`.
- **What:** four sources against eighteen systems. **There is no nominal-against-current power display**
  — `docs/budget-squaring.md` Part five asks for exactly that ("what the plant can deliver fresh,
  against what it can deliver now") and it is **not implemented** (**G12**). The nearest thing is the
  navigation counter's nominal/current pair for *time*.

### The written-off list (the abandonment north star)
- **Reach:** the command console's `GIVEN UP` column and `ship losses [kind]`; the live panel paints
  `GIVEN UP <n>` / `NOTHING GIVEN UP` without opening a screen; `W` on the command console writes off
  the deck under the cursor.
- **What:** `{time, kind, what, who}` — sealed, stripped, uninhabitable, written off — with the author
  `CommandingOfficer`. It is a deliberate act, not an automatic hook
  (`docs/evidence/abandonment-list.md`).
- **Note:** `docs/story-and-semantics.md` still lists this as "the headline claim and it does not exist"
  (conflict 1). **That is stale** — the evidence shows it built. See **G13**.

### The wall and the sealed quarters
- `ship wall`, `ship status` and the personnel screen carry the wall of names and the sealed quarters; a
  funeral (`ship funeral`) opens them and lifts the crew (`docs/evidence/the-two-missing-decks.md`).

---

## 10. The exploration and outside loop, for completeness

- **The chart** (`A` on the command console, `ui_lwh_chart`): the sector, where the ship is, the forecast
  per beacon, a course set by key. **What to press:** UP/DOWN, ENTER to set a course.
- **The survey** (`U` on Operations, `ui_lwh_survey`): the site the ship is at, or one of the ship's own
  compartments, out of one shared tricorder charge; a weak charge reads wrong
  (`docs/evidence/every-screen.md`).
- **Dilithium** (`ship dilithium|recomposite|acquire|finddilithium`): the crystal's life falls with warp
  use, Engineering recomposites until the ceiling will not rise, and a new crystal comes from mining,
  trade, salvage or research (`docs/evidence/backlog-materials-and-crew.md`).
- **Probes and phenomena** (`ship probe`, `ship scan`, `ship study`): a probe charts a target without
  going there; a phenomenon's attributes are revealed one scan at a time and the correct response is a
  function of all three (`docs/exploration-and-science.md`).
- **The away kit** (`ship kit`, `ship scancomp`): tricorders, phasers, EV suits, one shared charge. **The
  site itself does not exist** — the transporter delivers to the beacon the ship is at, so a scan is of
  a chart entry, not of a place a person stands (**G18**).

**The stop's question:** *is science a mechanic or a menu?* The rules are in; the places are not.

---

## 11. The endgame, for completeness

`docs/endings.md` designs arrival as a reading: three seconds of our own engine's warp-in, then the log
as the last frame; destinations are readers (Starfleet reads the paperwork, the Klingons read the
deaths), no destination is better than another, and the core breach is the only unwinnable end. **None
of it is implemented.** Three sectors and a `Won` flag exist in the outside loop; the ending, the
arrival, the readers and the log-as-last-frame do not (**G17**).

**The stop's question:** *what did you choose to lose, and who was entitled to read it?* The build
cannot yet ask it.

---

## 12. The gap list, ordered by what a player would notice first

Each entry: **what is missing or contradictory · which document or evidence file says so · what it
would take to close.** Leniency is the failure mode; this is what is not there.

### G1 — There is no way to start Long Way Home from the game's menus
- **Missing:** `g_ship` defaults to `0`; the ship map, deck pitch, crew, environment, player body,
  mode, clock and role are all cvars. The only launch is
  `scripts/run-engine.sh +set com_hunkMegs 768 +set g_ship 1 +set g_shipDeckPitch 3072 +map voyager`,
  plus the optional cvars. A person with no knowledge of the code cannot find it.
- **Says so:** `module/ship/g_ship.cpp` (default `0`); `docs/HANDOFF.md`; `docs/gates.md` S2–S10
  ("reachable only from the console").
- **To close:** one menu entry or launcher that sets the cvars and loads `maps/voyager`, and a "Long
  Way Home" line on the main menu. This is the cheapest, highest-value change on the board.

### G2 — The opening situation does not exist
- **Missing:** `docs/start-states.md` designs the opening as a situation — a casualty list, vacancies
  derived from it, the Caretaker aftermath, a first log entry, start-state variants from ensign to
  captain, and "the empty chair". The shipped ship is the whole, canonical default only: 141 crew, day
  0 08:00, condition green, intact chain of command.
- **Says so:** `docs/start-states.md` (design, no implementation evidence); `docs/evidence/s1-ship-core.md`
  (the whole ship); `docs/evidence/s10-modes-and-roles.md` (only `ship character` and `PlayerRole` are
  built).
- **To close:** the casualty list and derived vacancies as data; a `log_seed`; the start-state selector;
  and at least the "default canon" beat checked against the four "what the opening must do" items.

### G3 — Nothing tells you what you hold or how to use it
- **Missing:** no in-world prompt or manual names your post, your authority, your systems, or the
  console keys. The key hints live only on each console's footer, which you must find first. The world
  gives no opening statement of your position (`docs/start-states.md`, "The player's own position
  stated plainly" — not built).
- **Says so:** the absence of any tutorial content in `module/`; `docs/start-states.md`.
- **To close:** a first-person opening statement (a log entry or a brief), and a way to find the
  consoles without the `ui_lwh_*` command names.

### G4 — There is no scenario content at all, and nothing recruits one
- **Missing:** `docs/scenario-atlas.md` designs nine scenarios with breeding conditions; no breeding
  director exists. `scenarios/` holds two measurement harnesses (`deck04-watch`, `testroom-watch`).
  Nothing spawns a contact, a distress call, an anomaly or a boarding during normal play.
- **Says so:** `docs/scenario-atlas.md`; `scenarios/`; the absence of any director beyond the crew
  layer's restaffing rule (`module/crew/g_crew.cpp`).
- **To close:** the scenario manifest and a breeding-condition evaluator, plus at least one scenario end
  to end. This is the largest content gap and the reason most of Section 8 is console-invoked.

### G5 — The systems under stress are reachable only from the developer console
- **Missing:** combat, boarding, Borg, fire, breach and the cascade are rules with console commands
  (`ship board`, `ship borg`, `ship ignite`, `ship breach`, `ship jump`, `ship fire`, `ship core`) and
  no in-game way to meet them. A player cannot provoke them without docs.
- **Says so:** `docs/HANDOFF.md` ("reachable only from the console"); `docs/evidence/s7-intruders-and-control.md`,
  `s8-the-borg.md`, `s9-the-outside.md`.
- **To close:** G4. Until a scenario or a director exists, the sitting must use the commands in the
  Section 8 table.

### G6 — The published decks carry no station markers, so duty crew go to arbitrary nodes
- **Missing:** placing a station marker at a published deck's own interface panel leaks (the panels are
  sky/trigger brushes) and was reverted. The generated decks have markers; the published decks do not.
  The crew therefore walk to the deck's navigation, not to a workstation.
- **Says so:** `docs/evidence/s5-stations.md` ("Markers on the published decks: a blocker, measured");
  `docs/HANDOFF.md`.
- **To close:** chosen open-space origins per system per published deck, recorded as invention, authored
  with a person who can see the map.

### G7 — The red emergency strip does not read as red
- **Missing:** the toggle is correct; the signal is not perceptible from the camera on all five
  re-dressed decks. It is an accessibility failure wearing a rendering finding's clothes, and it is the
  world counterpart of the console palette work.
- **Says so:** `docs/evidence/access-and-authority.md`, "The world's emergency strips";
  `docs/evidence/deck*-redress.md` (all five).
- **To close:** an emissive red strip material in the deck maps (authoring; the no-new-art rule points
  at re-using `hall/hall_light_red` with an emissive stage, or an authored glow). This is a stop the
  owner should judge in the sitting — the current evidence says it fails.

### G8 — A person at a panel is unproven
- **Missing:** every panel→console mapping is proven by firing the panel's command
  (`genericmenu tactical`, `ui_tactical`, …). No session has walked up to a panel and opened a console,
  and the generated decks' panels have no proven station mapping. The glance and the painted panel are
  proven to render headless, not by a person.
- **Says so:** `docs/evidence/s4-station-consoles.md` ("A person at a panel" remains);
  `docs/evidence/access-and-authority.md`; the deck redresses ("authored, not exercised").
- **To close:** a logged-in session walking to each station and opening it. This is one of S4's exit
  criteria.

### G9 — The generated decks' aesthetics are unjudged and named as the risk
- **Missing:** decks 6, 7, 12, 13 and 14 are re-dresses composed from slices; the evidence repeatedly
  says a *slice is not a room* and that the copied interior may dominate. Deck 13 and 14 read dark
  under the copied lighting; deck 14's cold/warm contrast may be too weak; deck 7's structure is shell
  only. All five await the walkthrough; deck 6's three-zones question is the sharpest.
- **Says so:** `docs/evidence/deck12-redress.md`, `deck13-redress.md`, `deck14-stasis.md`,
  `deck07-auxcore.md`, `deck06-composition.md` (each "What could not be verified").
- **To close:** the walkthrough itself; and for any deck that fails, the next pass the evidence names
  (light, scale, a neutral frame).

### G10 — Crew post coverage on the merged ship's generated decks was never measured
- **Missing:** the station markers on decks 6, 7, 12, 13 and 14 are present and the crew layer can use
  them, but the G3 harness runs on deck 4 and no deck-6/7/12/13/14 scenario exists, so coverage was not
  measured there. The posts are authored, not exercised.
- **Says so:** every deck evidence file ("Crew post coverage … was not measured");
  `docs/evidence/s5-stations.md`.
- **To close:** a crew scenario per generated deck (the scenario tooling exists; the content does not).

### G11 — The environment is per deck, not per compartment, and authored on one deck
- **Missing:** the engine has no pressure concept; the model is per-deck, so one breach stands for a
  whole deck, and the authored breach effect is on deck 12 only. Most decks have no hazard to walk into.
- **Says so:** `docs/evidence/environment-in-the-world.md` (judgement calls 3, 5);
  `docs/evidence/player-in-the-world.md` (judgement call 3).
- **To close:** per-compartment atmosphere (an engine/model change, parked); and authored hazards on
  more decks, or acceptance that the hazard is a deck-12 demonstration.

### G12 — The power budget is a warship's budget on a science vessel's hull, and the owner's headroom ruling is not implemented
- **Missing / contradictory:** `docs/ship-systems.md` says life support and the holodecks are
  *independent sources* and gives a five-rung brownout ladder; the code has neither — life support and
  the holodecks are ordinary loads, and only the holodecks and replicators exist to shed. There are no
  labs, no astrometrics, no non-essential lighting and no cargo handling in the model. `docs/budget-squaring.md`
  (status: *proposed, awaiting the owner's ruling; nothing implemented*) says the tactical fit is 28.6%
  of the grid and the science fit one sensors row (3.9%). The owner's headroom ruling (Part five) is
  not built; the constant 1,500-vs-1,540 shortfall remains.
- **Says so:** `docs/budget-squaring.md` (Parts three and five); `docs/ship-systems.md` (the ladder and
  the three independent sources); `module/ship/ship_core.cpp` (`SPECS[]`, `SOURCES[]`).
- **To close:** the owner's ruling; then the five added loads, the source rebalance, and the
  nominal-vs-current power display. This is also why **Stop 11's science-vessel question cannot be
  answered as posed**.

### G13 — Documentation contradictions a careful reader will trip on
These are not all player-visible, but each is a false claim if quoted, and several sit at the top of a
document an agent reads first.
- **The computer core's deck.** `docs/evidence/s1-ship-core.md` prints deck 7; `docs/evidence/s3-merged-map-measured.md`
  says "the computer core is on deck 9"; `docs/evidence/s5-stations.md` says 9; the code says **10**
  (`module/ship/ship_core.cpp`, with a comment citing `docs/ship-master-map.md`), and
  `docs/evidence/access-and-authority.md` says the correction was made. The torpedo launchers are the
  same story (9 vs 10). **A line number from the wrong document is a false claim.**
- **The deck build order.** `docs/gates.md` (the gaps section) says the next work is "the deck build
  order … Deck 12 and deck 13 first", while `docs/ship-master-map.md` records all five absent decks
  built. The gates text is stale.
- **`docs/story-and-semantics.md` conflicts 1 and 7 are stale** — the abandonment list and the
  navigation counter both exist and are evidenced.
- **`docs/evidence/the-month-report-and-the-toll.md`** judgement call 1 says the report's headline is
  `DilithiumRange`; `docs/evidence/the-navigation-counter.md` supersedes it (the headline is the
  counter's change). The stale file is not annotated as superseded at the call.
- **`docs/access-and-authority.md` contradicts itself** on the dead officer's credentials: the "Three
  edge cases" section still says they "still work", while the acceptance says the item is **reported,
  not built**, and the older phrasing is withdrawn.
- **`docs/path-to-playtest.md`** still calls Stage A "in flight" and Stage B/C pending, though
  `docs/evidence/environment-in-the-world.md` and `docs/evidence/player-in-the-world.md` record both
  built.
- **`docs/HANDOFF.md`** is dated 2026-10-06 and predates the five decks, the five gaps, the record and
  most of S6–S10; it is stale throughout (crew cap 10 vs 24, "core only", etc.).
- **The Maquis contradiction** is self-reported in `docs/path-to-playtest.md` ("Defects found in the
  corpus") and still present in `docs/gates.md`.
- **The EMH keyboard slip:** `docs/evidence/backlog-materials-and-crew.md` calls the EMH "Sickbay's `B`
  companion"; `B` is the surgical field and the EMH is `E` on the triage board
  (`module/ui/ui_lwh_engineering.cpp`).
- **`docs/budget-squaring.md` says "nothing here is implemented" in its header** yet its Part five
  says the dilithium mechanic "is already built"; both are true of different parts, but the header
  reads as covering the whole document.

### G14 — Morale is a scalar, not the three components, and the crew have no manner
**CLOSED 2026-10-08 (`feat/the-character-layer`), first half built and second half demonstrated.**
The single `float morale` is **gone**: morale is now `Morale(c)`, read from **deficit, outlook and
holdings**, each eased by its own drivers, and `MoraleReason(c)` names the component most
responsible so the log can say which moved and why. The manner is built as `MannerLine(c)` and
**demonstrated at three morale levels** (`scripts/character-check.sh`) rather than declared — the
judgement whether it reads is the owner's. One-place account: `docs/character-derivation.md`;
evidence: `docs/evidence/the-character-layer.md`.
- **Was missing:** `docs/morale.md` defines morale per person as derived from **deficit, outlook and
  holdings**, never a hidden counter. The code had a single `float morale` plus drivers, and the log
  could say why — but the three components did not exist, and the crew's **manner** (barks wired to
  morale) was not built. `docs/story-and-semantics.md` conflict 2 named this.
- **Says so:** `docs/story-and-semantics.md` (conflict 2); `docs/morale.md`;
  `docs/evidence/gap-morale-and-fatigue.md` ("What is left for this gap").
- **To close:** the three components as derived reads, and the barks wired to morale — both landed;
  what remains is the owner's read of the manner at a playtest.

### G15 — No crew scenario exists for the generated decks (see G10) and the fleet's own characters cannot hold posts
- **Missing:** G3's measured six are *added* to deck 4; none of deck 4's own eight NPCs can hold a post.
  The charter says "named crew already placed". This is an open owner judgement, not an oversight.
- **Says so:** `docs/evidence/g3-reactive-crew-measured.md`; `docs/gates.md` G3; `docs/HANDOFF.md`.
- **To close:** the owner's ruling, and crew scenarios on the decks where they belong.

### G16 — The record's endgame pieces are not built
- **Missing:** the meeting brief built per participant from marks and the log; assimilation taking the
  **personal log and the access levels**, with the Collective speaking in the assimilated person's
  voice; the purge's full consequence; the destination set and the arrival; the log as the last frame.
- **Says so:** `docs/the-record-and-the-log.md` ("Remaining (specified, not built)");
  `docs/evidence/the-two-logs.md` ("What could not be verified"); `docs/endings.md`.
- **To close:** the meeting brief and the assimilation transfer are the two the record calls out twice;
  the endings are a later stage.

### G17 — There is no away site, so the away mission's acceptance run is impossible
- **Missing:** the transporter delivers the party to the sector beacon the ship is at; the tricorder
  scans a chart entry, not a place. The acceptance — "an away mission can be solved by scanning rather
  than by shooting" — needs a walkable site and a session.
- **Says so:** `docs/evidence/gap-tricorders-and-kit.md` ("What is left for this gap").
- **To close:** an away-site map re-dressed from the archetypes in `docs/outside-the-ship.md`; no new
  art needed.

### G18 — The air/endurance countdown is not drawn where the problem is
- **Missing:** the clocks are on the Operations console, and the force-field key is there; deck 12
  itself does not display its own countdown. `docs/evidence/gap-air-and-endurance.md` names this as the deck-12
  build item.
- **Says so:** `docs/evidence/gap-air-and-endurance.md`; `docs/ship-master-map.md`.
- **To close:** an environmental-control readout at the deck-12 watch console; the panel surface path
  already exists.

### G19 — The shuttle load-screen is deliberately a menu and is not built
- **Missing:** `docs/shuttles.md` asks the load-screen to make you decide (manifest, loadout,
  destination confidence, return condition). It is a launch-time decision menu, not a station console,
  and it is left unbuilt with the reason recorded.
- **Says so:** `docs/evidence/every-screen.md` (row 18, Task D); `docs/evidence/the-missing-screens.md`.
- **To close:** build the launch menu (the `LaunchShuttle` model exists). Treated as a call, not a
  defect — see Section 14.

### G20 — The glance has no in-world control, and sealed quarters have no world prompt
- **Missing:** the in-world readout is a read by design; the control it lacks is a world interaction
  (an interactive panel surface), parked as out-of-console-scope. Sealed quarters are visible only
  through the console/HUD, not by a per-door prompt.
- **Says so:** `docs/evidence/every-screen.md` (row 12); `docs/evidence/the-two-missing-decks.md`
  (judgement call 5).
- **To close:** engine/world affordances on panel and door brushes.

### G21 — The airponics bay and living quarters are not mapped places
- **Missing:** their *rules* are built (airponics `SetAirponics`; quarters quality and `ImproveQuarters`);
  their rooms are not. `docs/ship-master-map.md` puts airponics on deck 11 (a published deck, so it
  shares the S8 published-deck blocker's shape).
- **Says so:** `docs/gates.md` ("Still to do"); `docs/evidence/backlog-materials-and-crew.md`.
- **To close:** author the rooms (deck-build path), or accept them as numbers.

### G22 — The viewscreen and the opponent are schematic, not rendered
- **Missing:** the viewscreen is a module-drawn image of hull/shields bars, not the engine rendering the
  contact's model; there is no exterior space, no flight, and boarding arrives as a party rather than a
  rendered assault. `docs/outside-the-ship.md` recommends taking the cheap exteriors (a starfield, a
  debris field) and skipping playable exteriors.
- **Says so:** `docs/evidence/s9-the-outside.md`; `docs/outside-the-ship.md`.
- **To close:** an engine render-target extension if you want a photographic viewscreen; otherwise take
  the cheap windows the evidence names.

### G23 — Borg assimilation's visible half is generated-decks-only, and assimilated crew have no faces
- **Missing:** `RemapShader` is global by shader name, so the published decks (which share names) cannot
  turn Borg; only the five generated decks can, in four sections. Assimilated crew become drones in the
  simulation but not as drones with their own faces.
- **Says so:** `docs/evidence/s8-the-borg.md` ("What remains, and it is the honest boundary").
- **To close:** the stitcher pass giving each deck unique shader names, or an engine section tag.

### G24 — The retail-multiplayer human half, and cutscenes, are unconfirmed
- **Missing:** G7's server half is proven headless; the client menus, the server browser, connecting to
  a local server and input need a person and two machines. Cutscene playback is linked but whether one
  *plays* wants a session.
- **Says so:** `docs/gates.md` G6/G7; `docs/evidence/deps-durable-and-cutscenes.md`.
- **To close:** a two-machine LAN and VPN session; one cutscene watched.

### G25 — Ironman's edges
- **Missing:** the save and load menus still offer saves ironman will refuse; a save on quitting is not
  written; loading the ironman save twice is not prevented (copying the file aside defeats it, as in any
  ironman mode). `docs/evidence/s10-modes-and-roles.md`.
- **To close:** hide or grey the refused menu items; a save on quit.

---

## 13. Completeness verdict

| area | verdict |
|---|---|
| **The ship as state (S1, the save, the systems table, the clock, the adapter)** | **whole for what it is** — the core is the part to trust; 24 tests; saves replay identically. It is rules, not content. |
| **The whole-ship map (S3)** | **whole mechanically, thin in judgement** — one map, fifteen decks, lifts and tubes, frame time within budget; nobody has walked it, and one deck's real content on the published maps is retail's. |
| **The deck re-dresses (decks 6, 7, 12, 13, 14)** | **placeholder-to-thin, awaiting judgement** — real geometry, no new art, structural checks pass; the evidence itself calls a slice "not a room", and the aesthetic risk on each is unjudged. |
| **The station consoles (S4)** | **whole in mechanism, thin in the world** — every station's purpose and its control exist and are tested; a person at a panel, mouse, LCARS artwork and the panel-surface placement remain. |
| **The crew (G3/S5)** | **thin** — measured reactive crew and a roster that follows the routine; the deck's inhabitation unjudged, the crew's manner unwired, and the generated decks' posts unexercised. |
| **Damage, repair, resources (S6)** | **whole in the core, partial in the world** — deterministic soak, visible sparks and fire on the player's deck; per-compartment placement and most decks' hazards absent. |
| **Intruders and hacking (S7)** | **thin** — rules and hostile bodies on the player's deck; the puzzle is the one stress screen playable by hand, and it is untried by hand. |
| **The Borg (S8)** | **thin, honest boundary** — rules whole; the visible turn only on generated decks; assimilated crew without faces; no cube outside. |
| **The outside (S9)** | **thin** — opponent with systems, choices, pursuit, sectors and an end; the viewscreen is a schematic and there is no exterior. |
| **Play modes, roles, character creation (S10)** | **thin** — ironman/holodeck, three clocks, clearance, orders, character creation and the player's body exist and are tested; the owner's playthrough, the opening and the career's world side remain. |
| **The record (log, personal log, report, counter, write-off list)** | **whole for the built pieces** — the log, the two logs, the report editor with a diff, the navigation counter and the written-off list all exist and are tested; the meeting brief and the assimilation transfer remain. |
| **Authored content (scenarios, openings, endings, away sites, rooms)** | **no content at all** — the atlas, the start states, the endings and the site archetypes are design; none ships. This is the single largest absence. |
| **Retail modes 1 and 2** | **mode 1 works and is signed off; mode 2's client half is unproven** — G6 reported working; G7 server half only. |

**The one-sentence verdict.** Every gate S1–S10 is open on the engineering side and closed on the human
side exactly as `docs/gates.md` says — but *below* them there is a layer the ledger does not name: the
game has a ship and no situations to put it in. The sitting will prove the mechanics and expose the
content.

---

## 14. What I could not determine from the repository alone

- **Whether any console is pleasant to operate**, and whether the LCARS layout reads at a glance.
  Screenshots prove colour and fit; they do not prove *fun* — the standing caveat in every screen
  evidence file.
- **Whether the five re-dressed decks read as places.** The screenshots are on the playtest build, not
  in this repository, and the evidence itself withholds judgement.
- **Whether the red emergency strip reads as red.** The rendered evidence says it does not; only a
  person can overrule it.
- **Whether the crew read as inhabited**, and whether adding six crew satisfies G3.
- **Whether the turbolift menu's selection UI works by hand** (opening it pauses the sim, so the harness
  cannot click it).
- **Whether a person can open a console by walking to a panel** — every mapping is command-proven, not
  session-proven.
- **A real frame rate.** The harness measures the game frame (2.6 ms average), not the GPU frame; and
  the G3 worst-frame outlier (3.4 ms) was never explained.
- **Whether a cutscene plays**, and whether the two-machine LAN and VPN match actually works.
- **Whether the saved ship's deck change via the carry file works in a multi-map session** — the
  pack/unpack is unit-tested; the session is not.
- **The `num_entities` discrepancy** between two loads of the same map (3,323 vs 2,543) recorded in S3.
- **Whether the generated decks' painted panels are wired to open a console** — the panel renders live
  state; the station mapping is not proven.
- **Whether the owner's own prior saves load** — every version bump invalidates older saves; the current
  save version is past 49.

---

## 15. Judgement calls, named as calls

1. **I treated "no scenario content" as the top content gap, above any single system.** The brief lists
   systems to walk; a player notices first that nothing happens. `docs/scenario-atlas.md` designs nine
   scenarios and ships none, and the only "director" in the module is the crew layer's restaffing rule.
2. **I treated the missing menu entry (G1) as the first player-visible finding**, because it is met
   before anything else.
3. **I read stale evidence as contradicting current code where the code cites a newer source of truth.**
   The computer core is deck 10 because `SPECS[]` says so and cites `docs/ship-master-map.md`; I did not
   average the documents. This is a call, and the right fix is to correct the stale evidence, not to
   weaken the current claim.
4. **I kept the design's deliberate refusals out of the gap list.** The shuttle load-screen (a launch
   *menu* by design), the glance (a *read* by design), and no time travel / no point defence are
   decisions recorded in `docs/design-creed.md` and `docs/endings.md`; I list them as stops, not as
   defects.
5. **I did not re-open contested canon.** Where canon is silent (the brownout ladder, the second
   holodeck, the torpedo total), I recorded the design's decision and its source rather than deciding
   again.
6. **I treated the retail Virtual Voyager content as out of scope for the Long Way Home route** except
   where LWH adds to it (the panels, the environment on deck 12, the crew layer). A full retail content
   tour belongs to mode 1, not this document.
7. **I did not resolve the contradictions in G13.** The brief asks me to find them, not to fix them, and
   fixing a document requires knowing which one is right — which in two cases needs a person or a code
   read, named per entry.
8. **I did not run or build anything.** Every claim is from the documents, the evidence, and the module
   source for bindings and console commands; where a rendered artifact would be needed I said so.

---

*One document. No code changed. No build run. The route above is what I would hand the owner; the gap
list is what I would hand the programme.*
