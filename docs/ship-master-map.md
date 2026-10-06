# The master map: fifteen decks, what exists, what is missing

The single sheet of the ship. Sources: the canon deck list in
`docs/research/voyager-interior-systems.md` (every line sourced), the shipped map sources, the lift graph
those maps declare, and the verified gap list `docs/vv-gap-list-verified.md`.

Diagram, regenerable from `tools/mastermap.py`: `voyager-master-map.png`.

## The decks

| deck | canon contents (sourced) | game today | what it hosts in the model | planned |
|---|---|---|---|---|
| 1 | Bridge, ready room, briefing room, aft cargo hold | map ships | command; weapons locker | spacesuit lockers, upper sensor platform |
| 2 | Mess hall, captain's dining room, officers' quarters | map ships | galley and replicator rationing | officers' mess, VIP quarters, labs |
| 3 | Captain's and officers' quarters | map ships | crew life | Janeway's quarters, torpedo trackers |
| 4 | Transporter rooms 1 & 2, cargo bay 2, gel pack grid | map ships | **transporters; stores** | aft torpedo launchers, phaser maintenance |
| 5 | Sickbay, CMO's office, sections 10-53 | map ships | **medical** | doctor's office, escape pod access |
| 6 | Holodeck 2, armory, crew quarters | **no map** | holodeck; security post | security office by the brig, Tuvok's and Kim's quarters, aux deflector |
| 7 | Auxiliary computer core, cargo, labs, escape pods | **no map** | computer depth; escape | RCS thruster access |
| 8 | Astrometrics, cargo bay 2 sec 4, science lab, quarters | map ships | **sensors; deuterium processing** | docking ports, lower cargo bays |
| 9 | Crew quarters | map ships | crew life | Aerowing dock, cargo loading doors |
| 10 | Shuttlebay, junction 32 Alpha | map ships | **shuttlecraft; main computer core** | forward torpedo launchers |
| 11 | Main engineering, airponics, deflector control | map ships | **warp core; EPS; power distribution** | -- |
| 12 | Environmental control, navigational control B7, section 42 | **no map** | **LIFE SUPPORT (atmosphere, gravity)** | all of it; section 42 stays off-limits for a scenario |
| 13 | Life support plant, ~10 m below engineering | **no map** | **life support machinery** | all of it |
| 14 | Stasis chambers, a holodeck *(contested)* | **no map** | stasis; holodeck 1 | decide the contested placement once and record it |
| 15 | Plasma relay room 16, Jefferies tube G-33, landing gear | map ships | **plasma relays; landing systems** | antimatter loading port; forward tractor emitter (not deck 16 -- it does not exist) |

**Ten of fifteen decks ship as maps** (01, 02, 03, 04, 05, 08, 09, 10, 11, 15). The five absent decks --
06, 07, 12, 13, 14 -- are the build order's spine, and `tour/deck15` already exists, so do not build it.

## The lift spine

The turbolift is not a menu; it is a graph of `target_level_change` entities carrying a `mapname`, with
**139 edges** across the deck and campaign maps. Every VV deck links to every other VV deck, plus `_brig`,
plus at least one campaign interior (`voy15`), and `deck04` reaches all **eight holodeck programmes**
(Camelot, garden, high noon, temple, warlord, proton, firing range, minigame). `deck04` declares the most
destinations and is the pattern to copy. **A new deck with no incoming edge is unreachable.**

## The systems map

Where each modelled system lives, and therefore which decks the simulation is blocked on:

- **Power**: deck 11 (core, EPS), nacelle feed through the pylons, plasma relays on deck 15.
- **Life support**: environmental control on deck 12, plant on deck 13 -- *both absent, both first*.
- **Computer**: deck 10 (main core), deck 7 (auxiliary), gel packs distributed.
- **Sensors**: astrometrics and science lab on deck 8; navigational control on deck 12.
- **Medical**: deck 5. **Transporters**: deck 4. **Stores**: cargo bay 2 (deck 4), lower bays (deck 8).
- **Weapons**: armory on deck 6, launchers fore (deck 10) and aft (deck 4).
- **Craft**: shuttlebay deck 10, Aerowing dock deck 9, landing gear deck 15.

## Where the scenarios happen

Environmental control with **section 42 next door** (the haunting); cargo bay 2 (explosive decompression);
the transporter rooms (boarding arrives here); sickbay (the wounded become drones); the computer core (they
inherit the crew roster); deuterium processing (the price of fuel); the plasma relays (nineteen relays,
no turbolifts); the holodecks (warmth, and a safety interlock failure).

## The content inventory, in one place

- **Enemies we can field with no new art**: Borg, Species 8472, Hirogen, Malon, Klingons, and Raven's own --
  Etherians, Vohrsrith, Scavengers, Reavers, Harvesters, the machine family. Detail and counts:
  `docs/research/game-alien-roster.md`.
- **Not in the game at all**: no Kazon, Vidiian, Jem'Hadar, Ferengi; no Romulan or Cardassian enemies.
- **Spaces already built that are not decks**: the brig, and the eight holodeck programmes.
- **Procedure for anything new**: `docs/authoring-a-location.md`, with a brief per bespoke room from
  `docs/location-brief-template.md` (see `docs/locations/deck12-environmental-control.brief.md`).

## The deck build, in order (status)

1. **Deck 12, environmental control** — **blockout built 2026-10-07** and awaiting the owner's
   approval before detail, per the procedure. The generated deck is a working half-deck in two
   chambers (a dividing wall with a doorway) with the plant on a raised deck and a lower walkway, the
   life-support station marked, reachable and walkable on the merged ship: `scripts/deck12-check.sh`,
   `g_shipTest 26`, screenshot `lwh_deck12.tga`. The detail items the brief names — the plant itself,
   the watch console furniture, the lighting states, the parts list — wait on that approval.
2. **Deck 13, life support plant** — **blockout built 2026-10-07**, awaiting approval. A plant hall
   with a central machinery island and a raised catwalk, the brief in
   `docs/locations/deck13-life-support.brief.md`, reachable and walkable: `scripts/deck13-check.sh`,
   `g_shipTest 27`, screenshot `lwh_deck13.tga`.
3. **Deck 6, holodeck 2, armory and quarters** — **blockout built 2026-10-07**: a corridor with two
   holodeck doorframes and an armory locker; `scripts/deck-check.sh 6`, screenshot `lwh_deck06.tga`.
4. **Deck 14, stasis and holodeck 1** — **blockout built 2026-10-07**: stasis pods along a wall and a
   holodeck doorframe; `scripts/deck-check.sh 14`, screenshot `lwh_deck14.tga`. The contested holodeck
   placement is decided: Holodeck 1 on deck 14, Holodeck 2 on deck 6.
5. **Deck 7, auxiliary core, cargo and labs** — **blockout built 2026-10-07**: a central auxiliary
   core column and two cargo islands; `scripts/deck-check.sh 7`, screenshot `lwh_deck07.tga`.
6. Decks 10, 8, 4 (published) and the amenity items — **station markers placed at the published maps'
   own fixtures where they exist** (transporters on deck 4, sensors on deck 8; the rest are on the
   deck-4 hub or the unnamed bridge, and need chosen origins — the S5 item). All five generated decks
   have blockouts awaiting the owner's approval before detail.
