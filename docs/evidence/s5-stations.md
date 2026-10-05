# Evidence: S5 — crew stand at stations (first slice, and the research behind it)

Date: 2026-10-07. S5 wants the roster to live its life on the working ship: on duty at the system's
station, off duty at the mess or the holodeck. The blocker from the last session was that a "place"
was an arbitrary navigation node. This is the first slice of making a place a *station*, and the
game/canon research that says where the stations are.

## What the game has (read from the ship's own map, not assumed)

The published decks carry **no named station entities**. Deck 1's bridge has the station **models**
(`models/mapobjects/bridge/station.md3`, `stationsleft.md3`, `helm.md3`, `conflight.md3`) but no
`targetname` or usable entity to stand at, and nothing named tactical/ops/conn/engineering. Sickbay,
the transporter and astrometrics exist only as `target_interface` screens. The system table in
`module/ship/ship_core.cpp` already says where each system is *worked*:

| system | deck | station |
|---|---|---|
| life support | 12 | Environmental Control |
| structural integrity, inertial dampers, warp drive, navigational deflector | 11 | Main Engineering |
| computer core | 9 | Computer Core |
| shields, phasers | 1 | Bridge, Tactical |
| sensors | 8 | Astrometrics |
| impulse drive | 10 | Impulse Engineering |
| torpedo launchers | 9 | Torpedo Bay |
| communications, turbolifts | 1 | Bridge, Operations |
| transporters | 4 | Transporter Room 1 |
| sickbay | 5 | Sickbay |
| tractor beam | 10 | Shuttlebay Control |
| replicators | 2 | Mess Hall |
| holodecks | 6 | Holodeck 1 |

The generated decks (6, 7, 12, 13, 14) are ours, so their stations can be placed exactly. The
published decks need markers authored at their fixtures (the bridge models, the transporter panel,
the warp core, the astrometrics panel). That authoring is the next step; this slice does the
mechanism and the decks we own.

## Canon the placements follow

Intrepid-class deck usage (screen canon, then the technical manuals; each figure here is what the
decks are *for*, the placement of any one console is an invention): deck 1 the bridge; deck 2 the
mess hall; deck 4 the transporter rooms; deck 5 sickbay; deck 6 holodecks; deck 8 astrometrics;
deck 9 the computer core; deck 10 the shuttlebay and impulse engineering; deck 11 Main Engineering
and the warp core; deck 12 environmental control. The invented placements are recorded in the lore
ledger.

## What is built

- **A station is a marker.** `g_crew.cpp` gains `StationFor(system)`: it finds a map entity named
  `lwh_station_<system>` and returns its origin. A crew member on duty at that system stands there;
  with no marker the deck's navigation is used, as before.
- **The generated decks carry their markers** (`tools/shipmap/gendeck.py`): deck 6 has
  `lwh_station_17` (holodecks), deck 12 `lwh_station_0` (life support). The stitcher keeps them.

Run on the merged ship with the player on deck 12 (`g_crewDeck 12`), the life-support post-holder
walks to the marker, not to a random node:

```
CREW: lwh_crew_015 at post 'place_lwh_crew_015' after 1000 ms
```

## What is left

- **Markers on the published decks.** The bridge (shields/phasers, communications/turbolifts),
  Main Engineering (warp drive and the rest), Sickbay, the transporter room, astrometrics, the mess
  hall. The stitcher can place them at chosen fixture origins once the bridge's station models are
  identified by eye; do that with a person, or record the chosen origins as invention.
- Then raise the embodied cap (10 now) toward 20-30 and measure, as the hand-off plans.
