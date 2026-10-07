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
| 14 | Stasis chambers, a holodeck *(contested)* | **no map** | stasis; holodeck 1 | **decided:** Holodeck 1 is here, Holodeck 2 on deck 6 — see `docs/locations/deck14-stasis.brief.md` |
| 15 | Plasma relay room 16, Jefferies tube G-33, landing gear | map ships | **plasma relays; landing systems** | antimatter loading port; forward tractor emitter (not deck 16 -- it does not exist) |

**Ten of fifteen decks ship as maps** (01, 02, 03, 04, 05, 08, 09, 10, 11, 15). The five absent decks --
06, 07, 12, 13, 14 -- are the build order's spine, and `tour/deck15` already exists, so do not build it.

**All five absent decks are built.** Decks 7, 12, 13 and 14 are re-dressed from `tour/deck11`, and
**deck 6 is composed** from three maps -- `tour/deck09` (the quarters frontage), `_brig` (the armory)
and a `_holodeck_*` programme map (the arch) -- which the brief (section 3) requires because deck 6
carries three functions on one deck (below). The **game today** column above describes the retail
game, which has no map for any of them.

## The lift spine

The turbolift is not a menu; it is a graph of `target_level_change` entities carrying a `mapname`, with
**139 edges** across the deck and campaign maps. Every VV deck links to every other VV deck, plus `_brig`,
plus at least one campaign interior (`voy15`), and `deck04` reaches all **eight holodeck programmes**
(Camelot, garden, high noon, temple, warlord, proton, firing range, minigame). `deck04` declares the most
destinations and is the pattern to copy. **A new deck with no incoming edge is unreachable.**

## The systems map

Where each modelled system lives, and therefore which decks the simulation is blocked on:

- **Power**: deck 11 (core, EPS), nacelle feed through the pylons, plasma relays on deck 15.
- **Life support**: environmental control on deck 12, plant on deck 13 -- *both absent, both first*;
  both are now built as re-dresses of `tour/deck11` (see the build order below).
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

1. **Deck 12, environmental control** — **re-dressed from `tour/deck11`** and awaiting the owner's
   walkthrough, per the procedure: the room copied and changed to be this room (compressed, the warp
   core replaced by an atmosphere plant, a Jefferies tube exit, the watch console at the entrance),
   the life-support station marked, reachable and walkable on the merged ship, the emergency lighting
   state authored: `scripts/deck12-check.sh`, `g_shipTest 26`, screenshot `lwh_deck12.tga`. The tool
   is `tools/shipmap/dressdeck12.py`; the parts list `docs/locations/deck12-parts-list.md`; evidence
   `docs/evidence/deck12-redress.md`. What is open is the walkthrough.
2. **Deck 13, life support plant** — **re-dressed from `tour/deck11` (2026-10-07)** and awaiting the
   owner's walkthrough. One plant hall: the warp core replaced by a central machinery island, a
   raised catwalk along the north wall with a stair, a Jefferies tube mouth at the far end, and the
   local plant panel and its post on the catwalk. The brief is
   `docs/locations/deck13-life-support.brief.md`, reachable and walkable: `scripts/deck13-check.sh`,
   `g_shipTest 27`, screenshots `lwh_deck13.tga` / `lwh_deck13_catwalk.tga`. The tool is
   `tools/shipmap/dressdeck13.py` (deck 12's pattern); the parts list
   `docs/locations/deck13-parts-list.md`; evidence `docs/evidence/deck13-redress.md`.
3. **Deck 6, holodeck 2, armory and quarters** — **composed 2026-10-07** and awaiting the owner's
   walkthrough: the fifth and last re-dress, and the only one that composes rather than crops. Three
   functions on one deck, from three maps — a slice of `tour/deck09`'s hall as the quarters frontage,
   `_brig`'s security room as the armory, and the arch of a `_holodeck_*` programme map
   (`Tour/_holodeck_firingrange`) for two holodeck doorframes — carried into one corridor, with the
   copied interiors as detail geometry. `scripts/deck06-check.sh`, `g_shipTest 63`, screenshots
   `lwh_deck06.tga` / `lwh_deck06_holodeck.tga` / `lwh_deck06_armory.tga`; the tool
   `tools/shipmap/dressdeck06.py` (deck 7's mechanism), the parts list
   `docs/locations/deck06-parts-list.md`, evidence `docs/evidence/deck06-composition.md`. The
   `SYS_HOLODECKS` station marker moved here from deck 14 (ship_core's `SystemSpec` puts the system on
   deck 6).
4. **Deck 14, stasis and holodeck 1** — **re-dressed from `tour/deck11` 2026-10-07** and awaiting
   the owner's walkthrough: a row of the game's own stasis pods along the north wall, a holodeck
   doorframe at the far end (Holodeck 1), the raised core deck cleared for one flat chamber, cold
   light over the pods and warm at the holodeck. `scripts/deck14-check.sh`, screenshots
   `lwh_deck14.tga` and `lwh_deck14_holodeck.tga`; the tool `tools/shipmap/dressdeck14.py` (deck 13's
   pattern), the parts list `docs/locations/deck14-parts-list.md`, evidence
   `docs/evidence/deck14-stasis.md`. The contested holodeck placement is decided: Holodeck 1 on deck
   14, Holodeck 2 on deck 6.
5. **Deck 7, auxiliary core, cargo and labs** — **re-dressed from `tour/deck11` 2026-10-07** and
   awaiting the owner's walkthrough: one chamber with an auxiliary core column at its centre, two
   cargo islands of the game's own crates, an overhead escape-pod hatch, and a Jefferies tube to the
   main computer core. The copied interior is detail geometry so the fourth re-dress does not pass
   q3map2's vis-cluster ceiling. `scripts/deck07-check.sh`, `g_shipTest 29`, screenshots
   `lwh_deck07.tga` and `lwh_deck07_core.tga`; the tool `tools/shipmap/dressdeck07.py` (deck 14's
   pattern), the parts list `docs/locations/deck07-parts-list.md`, evidence
   `docs/evidence/deck07-auxcore.md`. The recreational-holodeck default was renamed to say what it
   is (Task B); behaviour unchanged.
6. Decks 10, 8, 4 (published) and the amenity items — **station markers placed at the published maps'
   own fixtures where they exist** (transporters on deck 4, sensors on deck 8; the rest are on the
   deck-4 hub or the unnamed bridge, and need chosen origins — the S5 item). Of the five decks with
   no published source, **all five are built -- 7, 12, 13 and 14 re-dressed and deck 6 composed --
   and all five await the owner's walkthrough.**
