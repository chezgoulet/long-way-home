# The ship programme — an operational Voyager

Decided with the owner on 2026-10-05. This replaces gates G4 and G5 of `program-charter-v3.md`; the
rest of that charter (evidence rules, the editor ladder, the track contracts) stands.

## Why the plan changed

The owner's reasoning: a model of where the crew spend their day cannot be built before the ship
they spend it on works. So the ship comes first, as a real system, and the crew live on it.

> "we must build a fully lore accurate operatable Voyager where every control panel has its
> canonical purpose and presents the player with a UI that allows for controlling its systems.
> voyager needs a fully dynamic lore accurate set of systems, to be played similarly to the video
> game Faster than Light. in this way, we can actually have the correct working environment for our
> NPCs to exist within, live their lives in, and play out the events and emergencies that occur in
> the course of the Year of Hell style playthrough we're building."

## Decisions

| question | decision |
|---|---|
| G3 | **Stays open.** Measured and passing, not signed off. Work proceeds regardless. |
| The ship beyond one deck | **One merged map of the whole ship.** The ten published deck sources stitched at canonical heights, joined by working turbolifts and Jefferies tubes; the five decks that exist nowhere (6, 7, 12, 13, 14) generated from lore deck plans. |
| Engine | **We own the engine fork for modes 1 and 3, kept layered.** Game logic stays in `module/`; engine changes are capabilities, each a reviewable patch with tests. This supersedes "inherit, do not own" for our engine. Retail multiplayer remains unmodified cMod. |
| Crew | **All ~141 are individuals** — name, department, watch, station, quarters, routine. Only those where the player is are embodied. |
| Clock | **Accelerated game time** by default, with play modes for 1:1 real time and for a ship that lives on while the game is closed. |
| The outside | **The full FTL loop**: a sector map, ship-to-ship combat resolved by the systems simulation and felt from inside, boarding parties as real intruders. |
| Consoles | **Both**: live status on the panel in the world, and a full-screen LCARS screen for that station when used. |
| The player | **Every style**: any crew member at any post they are cleared for; in command, issuing orders the crew carry out; or Munro as in the retail game. This needs a rank hierarchy and character creation. |
| Failure | **Ironman is the game**: one autosaved ship, permanent deaths and assimilations, damage that stays until repaired. A "holodeck" mode allows saves, for learning and testing. |
| Lore | **Screen canon, then the technical manuals, then invention** — every figure recorded in `docs/lore-ledger.md` with its source, or marked invented, so it can be overruled. |
| Hosting | **Single player first; the simulation is an engine-independent library** a multiplayer server can host unchanged. |
| Intruders | **Contested control per subsystem.** Intruders at a panel push a control value; crew push back through a breach-protocol puzzle, by cutting power, or by retaking the panel. NPCs contest by skill. |
| The Borg | Hold a section long enough and they assimilate it: its assets become Borg, and the crew must strip and repair it afterwards. Crew can be assimilated. |
| Effort | Not a constraint. "just gate milestones and keep going forward." |

## Gates

Hard, as before: each has exit evidence that exists or does not, and nothing is called done on a
green build.

| gate | contents | exit evidence |
|---|---|---|
| **S1** | **Ship core**: power generation and EPS distribution, the major systems, decks and atmosphere, consumables, the clock, the 141 roster with watches and a daily routine, persistence. Headless. | Unit tests for every property (`tests/ship`); a simulated day printed hour by hour. |
| **S2** | **Engineering console in game**: the core hosted in the game module, ticking and saved with the game; Main Engineering's LCARS screen reading and driving it on an existing deck. | Headless run: console actions change ship state; state survives save and reload. Owner operates it. |
| **S3** | **The whole ship as one map**: stitcher, generated decks, turbolifts and tubes, engine limits raised as measured. | The map compiles, passes the structural check, loads; every deck reachable on foot; navigation bakes; frame time measured. |
| **S4** | **Every station's console**, each with its canonical purpose, plus live in-world panel status. | A table of stations → systems → controls, each exercised by a headless test; panel surfaces show live state. |
| **S5** | **Crew daily lives on the working ship**: the roster embodied where the player is, walking the routine the core schedules, handing over watches. Subsumes G4. | G3's criteria at ship scale: station coverage across watch changes over a simulated day, 20–30 embodied at the frame budget. |
| **S6** | **Damage, repair and resources**: compartment damage with effects, repair as crewed work that consumes parts, casualties and sickbay, the economy. | Scripted casualty and damage scenarios with asserted outcomes; a multi-day soak with no stuck state. |
| **S7** | **Intruders and hacking**: boarders who fight and take panels; contested subsystem control; the breach-protocol minigame. | A boarding scenario run headless both ways (repelled, and lost) with asserted system ownership throughout. |
| **S8** | **The Borg**: progressive assimilation of sections and crew, asset replacement, strip-and-repair. | A section assimilated and recovered in a headless run, assets restored, state identical across save and load. |
| **S9** | **The outside**: sector map, encounters, ship-to-ship combat. | A full encounter resolved headless from both sides' systems; damage lands in the right compartments. |
| **S10** | **Play modes, roles and character creation**: ironman and holodeck; the three clocks; rank and clearance; the three player roles. | Each mode's rules enforced by test; the owner's playthrough. |

Order is by dependency. S3 and S4 may overlap once S2 has fixed how a console talks to the core.

## What exists

- **S1 — done.** `module/ship/ship_core.*`, `tests/ship`. See `docs/evidence/s1-ship-core.md`.
- **S2 — built, verified headless, awaiting the owner.** The core is hosted in the game module
  (`module/ship/g_ship.*`) and Main Engineering's console is a screen in the UI module
  (`module/ui/ui_lwh_engineering.cpp`). See `docs/evidence/s2-engineering-console.md`.
- **G3's direction layer** is the embodiment mechanism S5 builds on: posts, arbitration against
  scripts, the save chunk, the measurement harness.
- **Track B's pipeline** — map generation, headless compile, the validator, the scenario manifest —
  is what S3 is built with.

## Known hard problems, named now

- **The merged map against engine limits.** Ten decks of entities and brushes in one BSP will pass
  the entity, brush and visibility ceilings somewhere. S3 starts by measuring where.
- **Crew cannot open doors** (found in G3). A crew that walks the whole ship needs door handling
  before S5.
- **Missing decks have no canonical interior plans.** What is generated is invention, logged as such.
- **Borg asset replacement** means runtime shader and model swaps per section — an engine capability
  that does not exist yet.
- **Balance.** An FTL-like loop is tuned by playing it. Every invented number is in the ledger so it
  can be changed in one place.
