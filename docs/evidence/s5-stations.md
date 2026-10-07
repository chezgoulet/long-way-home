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
| holodecks | 6 | Holodeck 2 |

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

## Markers on the published decks: a blocker, measured (2026-10-07)

Placing a station marker at a published deck's **own interface panel** was tried and **reverted**. The
panels are brush entities whose surfaces use sky/trigger shaders (`common/junk_sky`, `common/trigger`),
so a marker at the panel's centre is inside the void — q3map2 reports:

```
******* leaked *******
Entity 1708, Brush 0: Entity leaked
```

— writes no portal file (`.prt`), and `-vis` then fails (`Error opening voyager.prt: No such file`), so
the whole-ship build is lost. That is the measurement: the fixture positions the maps carry are not
usable as marker origins.

The markers therefore need a **chosen open-space origin on the right deck**, recorded as invention —
the S5 brief's "do that with a person, or record the chosen origins as invention". It needs a person
who can see the merged map. The **mechanism** is in: `g_crew.cpp`'s `StationFor` reads
`lwh_station_<system>`, and the generated decks already carry their own (holodecks on 6, life support
on 12), so the crew go to a real place there.

## The embodied cap raised to 24 and measured (2026-10-07)

The layer's default cap is raised from 10 to 24 (`module/crew/crew_core.h`, toward the hand-off's
20–30). `scripts/s5-check.sh` on deck 4 told it is the mess deck, with the ship's own roster:

```
ship time 15:04, deck 2: the ship has 40 here (showing up to 24), 24 embodied
...
PASS  through 4 meals and 4 spells between them, the crew embodied were the crew the ship had aboard
INFO  of 24 embodied, 24 reached their place within the meal hour and 0 gave up
PASS  most of the crew embodied walked to their place on the deck
```

Twenty-four are embodied and follow the routine — arriving with the watch, leaving with it, all 24
reaching their place within the meal hour. That is S5's "20–30 embodied" figure for one deck; the
frame budget with the crew was measured in G3 (0.34 ms with crew, 0.24 without) and on the merged
ship in S3.

**Station coverage across watch changes** is `TestADayAboard` in `tests/ship`: over two full days,
with every watch change included, no station is ever short-handed — `s.systems[i].manned >=
Spec(i).crewNeeded` for every system at every hour, while the crew also sleep, eat and live.
