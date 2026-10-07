# Location brief: Deck 6 — Holodeck 2, Armory, Crew Quarters

## 1. Function, in one sentence

The recreational and security deck: Holodeck 2 with its arch, the armory where the security post
draws its weapons, and the crew quarters and labs that open off the same corridor.

## 2. Canon anchors

- **Deck 6** carries Holodeck 2, the armory and crew quarters — source: the sourced deck list in
  `docs/research/voyager-interior-systems.md` (Memory Alpha's Intrepid-class list).
- **Neighbours**: the brig and security office (master map), so security is here; the labs and
  quarters open off the corridor.
- **Episode evidence**: the holodeck programmes are canon; the armory is where a boarding is answered.
- **Reference image**: the game's own holodeck maps (`holodeck` maps in the GDK) show the holodeck
  look, but deck 6 has no published source. Say so.
- **Contested**: which holodeck is which (1 or 2) is a lore decision; recorded as invention.

## 3. Reuse target

**This deck is a composition, not a crop.** Decks 12, 13 and 14 each re-dressed *one room from one source*.
Deck 6 carries **three functions on one deck** -- a holodeck, an armory with a security post, and crew
quarters -- so its parts come from three places, and **the commit message names which map each zone came
from.**

- **The massing and the corridor spine -- `tour/deck09`** (crew quarters): the hall-and-alcoves read the
  spatial program asks for, and the reason the corridor stays the *route* rather than the room.
- **The armory and the security side -- `_brig`**: the only security interior in the game, and the map that
  carries the brig the security office sits beside, so the alcove borrows its lockers, grating and lighting.
- **The holodeck doorframes -- the `_holodeck_*` programme maps** (six ship): the arch read and the warm
  light, not a working programme, which is this brief's own non-goal.
- **Copy nothing exactly**: three sources, composited. **The failure to beat, in deck 14's own words:** a
  copied interior can dominate the room it was copied into -- and this deck has three inside one corridor. The
  lighting brief is the instrument; the corridor is the route, not the room.
- **Change**: a corridor with two holodeck doorframes and an armory alcove.
- **Keep**: the corridor-with-doors read.

## 4. Parts list

To be completed from the pak census (holodeck door frames, armory lockers). Invented geometry only
until then.

## 5. Spatial program

- **Footprint**: a long corridor, wider at the holodeck end.
- **Entries**: turbolift lobby at one end; a **Jefferies tube** at the other.
- **Circulation**: corridor spine; holodeck doorframes on one side, armory alcove off it.
- **Verticality**: flat; the holodeck doors read as portals.
- **Posts**: the holodeck station (SYS_HOLODECKS) and a security post by the armory.

## 6. Lighting brief

Warm at the quarters and holodeck, working light at the armory; the holodeck arch glows.

## 7. The hook

- **System node**: `SYS_HOLODECKS`.
- **Job sites**: holodeck maintenance; armory inventory.
- **Control surfaces**: the holodeck arch (recreation/training/therapy).

## 8. Acceptance checks

Reachable on foot; no dead ends; `.nav` bakes; the holodeck station is marked. Owner walks it.

## 9. Non-goals

No new art. Do not build a holodeck programme here.

## 10. Status (2026-10-07)

**Composed** — three zones from three maps (`tour/deck09`, `_brig`, `Tour/_holodeck_firingrange`)
carried into one corridor, reachable on the merged ship. Tool `tools/shipmap/dressdeck06.py`;
`scripts/deck06-check.sh`; `g_shipTest 63`; the parts list `docs/locations/deck06-parts-list.md`;
evidence `docs/evidence/deck06-composition.md`. The `SYS_HOLODECKS` station (section 5) is marked as
`lwh_station_17` at the holodeck arch (section 8), moved here from deck 14. Awaiting the owner's
walkthrough.
