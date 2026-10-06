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

- **Copy**: `tour/deck11` for the plant-room discipline, but this is a recreational deck, so the
  hall-and-alcoves massing is closer to `tour/deck09` (crew quarters). Copy nothing exactly.
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

**Blockout building** — a corridor with two holodeck doorframes and an armory alcove, reachable on the
merged ship (`scripts/deck06-check.sh`).
