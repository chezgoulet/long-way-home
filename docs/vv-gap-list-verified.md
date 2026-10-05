# Virtual Voyager: the gap list, verified

A consolidated "missing content" list for the Virtual Voyager DLC was produced outside this repo. Before
it becomes a build order it has been checked against three witnesses: the deck maps that actually ship,
the lift graph those maps declare, and the canon deck list in `docs/research/voyager-interior-systems.md`.
Most of it survives. Two items do not, and one omission matters more than anything on the list.

## What VV actually ships

Ten deck maps: **deck01, 02, 03, 04, 05, 08, 09, 10, 11, 15**. Plus `_brig`, and **eight holodeck
programme maps** reachable from deck04's lift and from deck15: `_holodeck_camelot`, `_garden`,
`_highnoon`, `_temple`, `_warlord`, `_proton`, `_firingrange`, `_minigame`.

So the list is right that **decks 06, 07, 12, 13 and 14 have no maps** -- those five are the genuinely
unbuilt decks. Everything else is a gap *within* a deck that exists.

## Correction 1 -- Deck 15 is not missing

`tour/deck15` ships. It is the smallest deck map (186 entities), it links to ten other decks and to
`_holodeck_minigame`, and its own entities already carry the Jefferies tube and landing-gear references
the list asks us to add. Struck from the build order.

## Correction 2 -- Deck 16 does not exist

Voyager is a **fifteen-deck** ship, and the game's own deck maps stop at fifteen. There is no Deck 16, so
neither the ground hover footpads nor a second forward tractor emitter belongs on one. The tell is in the
list itself: *"if not already placed on Deck 15; check your deck plans to avoid duplication"* is a guess
being hedged, not a finding. Canon places the forward tractor emitter under the main deflector and a
second emitter aft, on Deck 14. Drop both Deck 16 lines, and place the landing systems -- which are real,
Voyager lands in "The 37's" -- on the decks that actually carry them.

## The omission -- the holodeck rooms are missing, but the programmes are not

The list asks for "Holodeck 1" and "Holodeck 2" as missing areas. The **experiences already exist** as
eight programme maps. What is missing is the *room*: the corridor outside, the arch, the controls, the
safety interlocks, and the reactor that canonically cannot feed the main grid. That reframes the work
from "build two holodecks" to "build two doors and a console onto content we already have" -- a far
smaller job, and the safety-interlock failure becomes a scenario rather than a missing feature.

## Also missing from the list -- the lift graph

If decks 06, 07, 12, 13 and 14 are built, they must be **wired into the turbolift**, which in this game is
not a menu but a set of `target_level_change` entities carrying a `mapname`. The graph is data, and
deck04 is the model to copy: it currently declares the most destinations, including all eight holodeck
programmes and deck15. A new deck without a lift edge is unreachable.

## The build order, by what the ship model needs -- not by deck number

The decks that matter first are the ones that **host a modelled system**, because the systems design in
`docs/ship-systems.md` is already waiting for them:

1. **Deck 12 -- environmental control.** The node life support is implemented against; without it,
   atmosphere and gravity are unlocated.
2. **Deck 13 -- life support machinery.** The other half of the same system, and canon places it just
   below engineering, which puts it on the path between the core and the crew.
3. **Deck 10 -- the main computer core.** The gel-pack grid, the ship's memory, and the single most
   valuable thing for a boarding party to reach.
4. **Deck 8 -- deuterium processing.** Where the fuel economy becomes a place instead of a number.
5. **Deck 4 -- Cargo Bay 2.** Stores, materials, the adjustable-environment bay, and the canon precedent
   for explosive decompression as a tactic.
6. **Deck 6 -- security office and Tuvok's office by the brig.** The security post, and the room where an
   intrusion is interrogated rather than shot.
7. **Deck 14 -- stasis chambers and Holodeck 1.** Note the placement of the second holodeck is
   *contested* in canon (see the interior brief); decide it once and record it.
8. **Deck 7 -- auxiliary computer core, cargo, labs, escape pods, RCS access.** Depth rather than
   dependency.
9. **Deck 1, 2, 3, 5, 9 amenity and quarters items.** Worth having, but the campaign maps already contain
   versions of most of them (quarters, sickbay, mess), so they are the cheapest to defer and the least
   load-bearing.

Everything below the line is real work with real value; everything above it is work the systems are
already blocked on.
